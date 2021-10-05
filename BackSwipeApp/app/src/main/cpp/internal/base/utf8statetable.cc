// Copyright 2005, 2006 Google Inc. All rights reserved
//
// State Table follower for scanning UTF-8 strings without converting to
// 32- or 16-bit Unicode values.
//
// See design document:
// https://g3doc.corp.google.com/util/utf8/public/g3doc/utf8_state_tables.md

#include <cstdint>                     // for uintptr_t
#include <cstring>                     // for NULL, memcpy, memmove

#include "macros.h"

#ifdef COMPILER_MSVC
// MSVC warns: warning C4309: 'initializing' : truncation of constant value
// But the value is in fact not truncated.  0xFF still comes out 0xFF at
// runtime.
#pragma warning ( disable : 4309 )
#endif

#include "utf8statetable.h"

#include "integral_types.h"        // for uint8, uint32, int8
#include "logging.h"               // for COMPACT_GOOGLE_LOG_FATAL, etc
#include "port.h"                  // for UNALIGNED_{LOAD,STORE}32
#include "stringpiece.h"        // for StringPiece
#include "utf8statetable_internal.h"
#include "offsetmap.h"  // for OffsetMap
#include "unilib_utf8_utils.h"  // for OneCharLen, IsTrailByte
#include "utf/utf8acceptnonsurrogates.h"  // Pure table bytes
#include "utf/utf8cvtfolded.h"  // for utf8cvtfolded_obj
#include "utf/utf8cvtlower.h"  // for utf8cvtlower_obj
#include "utf/utf8cvtlowerorig.h"         // Pure table bytes
#include "utf/utf8cvtlowerorigja.h"       // Pure table bytes
#include "utf/utf8cvttitle.h"  // for utf8cvttitle_obj
#include "utf/utf8cvtupper.h"  // for utf8cvtupper_obj
#include "fixedarray.h"        // for FixedArray

using UniLib::IsTrailByte;

static const int kReplaceAndResumeFlag = 0x80;
// Bit in del byte to distinguish optional next-state field after replacement
// text

static const int kHtmlPlaintextFlag = 0x80;
// Bit in add byte to distinguish HTML replacement vs. plaintext


/**
 * This code implements a little interpreter for UTF8 state
 * tables. There are three kinds of quite-similar state tables,
 * property, scanning, and replacement. Each state in one of
 * these tables consists of an array of 256 or 64 one-byte
 * entries. The state is subscripted by an incoming source byte,
 * and the entry either specifies the next state or specifies an
 * action. Space-optimized tables have full 256-entry states for
 * the first byte of a UTF-8 character, but only 64-entry states
 * for continuation bytes. Space-optimized tables may only be
 * used with source input that has been checked to be
 * structurally- (or stronger interchange-) valid.
 *
 * A property state table has an unsigned one-byte property for
 * each possible UTF-8 character. One-byte character properties
 * are in the state[0] array, while for other lengths the
 * state[0] array gives the next state, which contains the
 * property value for two-byte characters or yet another state
 * for longer ones. The code simply loads the right number of
 * next-state values, then returns the final byte as property
 * value. There are no actions specified in property tables.
 * States are typically shared for multi-byte UTF-8 characters
 * that all have the same property value.
 *
 * A scanning state table has entries that are either a
 * next-state specifier for bytes that are accepted by the
 * scanner, or an exit action for the last byte of each
 * character that is rejected by the scanner.
 *
 * Scanning long strings involves a tight loop that picks up one
 * byte at a time and follows next-state value back to state[0]
 * for each accepted UTF-8 character. Scanning stops at the end
 * of the string or at the first character encountered that has
 * an exit action such as "reject". Timing information is given
 * below.
 *
 * Since so much of Google's text is 7-bit-ASCII values
 * (approximately 94% of the bytes of web documents), the
 * scanning interpreter has two speed optimizations. One checks
 * 8 bytes at a time to see if they are all in the range lo..hi,
 * as specified in constants in the overall statetable object.
 * The check involves ORing together four 4-byte values that
 * overflow into the high bit of some byte when a byte is out of
 * range. For seven-bit-ASCII, lo is 0x20 and hi is 0x7E. This
 * loop is about 8x faster than the one-byte-at-a-time loop.
 *
 * If checking for exit bytes in the 0x00-0x1F and 7F range is
 * unneeded, an even faster loop just looks at the high bits of
 * 8 bytes at once, and is about 1.33x faster than the lo..hi
 * loop.
 *
 * Exit from the scanning routines backs up to the first byte of
 * the rejected character, so the text spanned is always a
 * complete number of UTF-8 characters. The normal scanning exit
 * is at the first rejected character, or at the end of the
 * input text. Scanning also exits on any detected ill-formed
 * character or at a special do-again action built into some
 * exit-optimized tables. The do-again action gets back to the
 * top of the scanning loop to retry eight-byte ASCII scans. It
 * is typically put into state tables after four seven-bit-ASCII
 * characters in a row are seen, to allow restarting the fast
 * scan after some slower processing of multi-byte characters.
 *
 * A replacement state table is similar to a scanning state
 * table but has more extensive actions. The default
 * byte-at-a-time loop copies one byte from source to
 * destination and goes to the next state. The replacement
 * actions overwrite 1-3 bytes of the destination with different
 * bytes, possibly shortening the output by 1 or 2 bytes. The
 * replacement bytes come from within the state table, from
 * dummy states inserted just after any state that contains a
 * replacement action. This gives a quick address calculation for
 * the replacement byte(s) and gives some cache locality.
 *
 * Additional replacement actions use one or two bytes from
 * within dummy states to index a side table of more-extensive
 * replacements. The side table specifies a length of 0..15
 * destination bytes to overwrite and a length of 0..127 bytes
 * to overwrite them with, plus the actual replacement bytes.
 *
 * This side table uses one extra bit to specify a pair of
 * replacements, the first to be used in an HTML context and the
 * second to be used in a plaintext context. This allows
 * replacements that are spelled with "&lt;" in the former
 * context and "<" in the latter.
 *
 * The side table also uses an extra bit to specify a non-zero
 * next state after a replacement. This allows a combination
 * replacement and state change, used to implement a limited
 * version of the Boyer-Moore algorithm for multi-character
 * replacement without backtracking. This is useful when there
 * are overlapping replacements, such as ch => x and also c =>
 * y, the latter to be used only if the character after c is not
 * h. in this case, the state[0] table's entry for c would
 * change c to y and also have a next-state of say n, and the
 * state[n] entry for h would specify a replacement of the two
 * bytes yh by x. No backtracking is needed.
 *
 * A replacement table may also include the exit actions of a
 * scanning state table, so some character sequences can
 * terminate early.
 *
 * During replacement, an optional data structure called an
 * offset map can be updated to reflect each change in length
 * between source and destination. This offset map can later be
 * used to map destination-string offsets to corresponding
 * source-string offsets or vice versa.
 *
 * The routines below also have variants in which state-table
 * entries are all two bytes instead of one byte. This allows
 * tables with more than 240 total states, but takes up twice as
 * much space per state.
 *
**/

// Return true if current table pointer is within state0 range
// Note that unsigned compare checks both ends of range simultaneously
static inline bool InStateZero(const UTF8ScanObj* st, const uint8* table) {
    const uint8* table_0 = &st->state_table[st->state0];
    return (static_cast<uint32>(table - table_0) < st->state0_size);
}

static inline bool InStateZero_2(const UTF8ReplaceObj_2* st,
                                 const uint16* table) {
    const uint16* table_0 =  &st->state_table[st->state0];
    // Word difference, not byte difference
    return (static_cast<uint32>(table - table_0) < st->state0_size);
}

static inline bool Is7BitAscii(uint8 c) {
    return static_cast<signed char>(c) >= 0;  // faster than c < 0x80
}

// UTF8PropObj, UTF8ScanObj, UTF8ReplaceObj are all typedefs of
// UTF8MachineObj. It would be good if they were actually subclasses
// of UTF8MachineObj, so that we could avoid problems where a
// UTF8ScanObj, for example, is passed to UTF8GenericReplace. Changing
// the class definitions is not difficult to do, but all the types
// need constructors, and the "static const" initializers have to call
// the constructors explicitly -- they can't use just { ... }. So
// every header file that defines a state table has to be
// regenerated. The main clients are in util/utf8, i18n/encodings,
// quality/diacriticals (and its replacement,
// quality/characterconverter), and one file in gws/base. Since that
// produces a changelist of 352 files, *not* including gws, we'll put
// that off for some later time. In the meantime, we can exploit
// knowledge of how the table-builder works to check the types at
// runtime, but only in debug mode.
// TODO(jrm) After characterconverter is released and the unaccenter is
// decommissioned, change the class definitions.

static bool IsPropObj(const UTF8StateMachineObj& obj) {
    return obj.fast_state == nullptr
           && obj.max_expand == 0;
}

static bool IsPropObj_2(const UTF8StateMachineObj_2& obj) {
    return obj.fast_state == nullptr
           && obj.max_expand == 0;
}

static bool IsScanObj(const UTF8StateMachineObj& obj) {
    return obj.fast_state != nullptr
           && obj.max_expand == 0;
}

static bool IsScanObj_2(const UTF8StateMachineObj_2& obj) {
    return obj.fast_state != nullptr
           && obj.max_expand == 0;
}

static bool IsReplaceObj(const UTF8StateMachineObj& obj) {
    // Normally, obj.fast_state != NULL, but the handwritten tables
    // in utf8statetable_unittest don't handle fast_states.
    return obj.max_expand > 0;
}

static bool IsReplaceObj_2(const UTF8StateMachineObj_2& obj) {
    return obj.max_expand > 0;
}

// Look up property of one UTF-8 character and advance over it
// Return 0 if input length is zero
// Return 0 and advance one byte if input is ill-formed
uint8 UTF8GenericProperty(const UTF8PropObj* st,
                          const uint8** src,
                          int* src_len) {
            DCHECK(IsPropObj(*st));
    if (*src_len <= 0) {
        return 0;
    }

    const uint8* lsrc = *src;
    const uint8* table_0 = &st->state_table[st->state0];
    const uint8* table = table_0;
    int e;
    int eshift = st->entry_shift;

    // Short series of tests faster than switch, optimizes 7-bit ASCII
    uint8 c = lsrc[0];
    if (Is7BitAscii(c)) {           // one byte
        e = table[c];
        *src += 1;
        *src_len -= 1;
    } else if (((c & 0xe0) == 0xc0) && (*src_len >= 2)) {     // two bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        *src += 2;
        *src_len -= 2;
    } else if (((c & 0xf0) == 0xe0) && (*src_len >= 3)) {     // three bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << eshift];
        e = table[lsrc[2]];
        *src += 3;
        *src_len -= 3;
    } else if (((c & 0xf8) == 0xf0) && (*src_len >= 4)) {     // four bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << eshift];
        e = table[lsrc[2]];
        table = &table_0[e << eshift];
        e = table[lsrc[3]];
        *src += 4;
        *src_len -= 4;
    } else {                                                // Ill-formed
        e = 0;
        *src += 1;
        *src_len -= 1;
    }
    return static_cast<uint8>(e);
}

bool UTF8HasGenericProperty(const UTF8PropObj& st, const char* src) {
            DCHECK(IsPropObj(st));
    const uint8* lsrc = reinterpret_cast<const uint8*>(src);
    const uint8* table_0 = &st.state_table[st.state0];
    const uint8* table = table_0;
    int e;
    int eshift = st.entry_shift;

    // Short series of tests faster than switch, optimizes 7-bit ASCII
    uint8 c = lsrc[0];
    if (Is7BitAscii(c)) {           // one byte
        e = table[c];
    } else if ((c & 0xe0) == 0xc0) {     // two bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
    } else if ((c & 0xf0) == 0xe0) {     // three bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << eshift];
        e = table[lsrc[2]];
    } else {                             // four bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << eshift];
        e = table[lsrc[2]];
        table = &table_0[e << eshift];
        e = table[lsrc[3]];
    }
    return e != 0;
}


// BigOneByte versions are needed for tables > 240 states, but most
// won't need the TwoByte versions.
// Internally, to next-to-last offset is multiplied by 16 and the last
// offset is relative instead of absolute.
// Look up property of one UTF-8 character and advance over it
// Return 0 if input length is zero
// Return 0 and advance one byte if input is ill-formed
uint8 UTF8GenericPropertyBigOneByte(const UTF8PropObj* st,
                                    const uint8** src,
                                    int* src_len) {
            DCHECK(IsPropObj(*st));
    if (*src_len <= 0) {
        return 0;
    }

    const uint8* lsrc = *src;
    const uint8* table_0 = &st->state_table[st->state0];
    const uint8* table = table_0;
    int e;
    int eshift = st->entry_shift;

    // Short series of tests faster than switch, optimizes 7-bit ASCII
    uint8 c = lsrc[0];
    if (Is7BitAscii(c)) {           // one byte
        e = table[c];
        *src += 1;
        *src_len -= 1;
    } else if (((c & 0xe0) == 0xc0) && (*src_len >= 2)) {     // two bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        *src += 2;
        *src_len -= 2;
    } else if (((c & 0xf0) == 0xe0) && (*src_len >= 3)) {     // three bytes
        e = table[c];
        table = &table_0[e << (eshift + 4)];  // 16x the range
        e = (reinterpret_cast<const int8*>(table))[lsrc[1]];
        table = &table[e << eshift];          // Relative +/-
        e = table[lsrc[2]];
        *src += 3;
        *src_len -= 3;
    } else if (((c & 0xf8) == 0xf0) && (*src_len >= 4)) {     // four bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << (eshift + 4)];  // 16x the range
        e = (reinterpret_cast<const int8*>(table))[lsrc[2]];
        table = &table[e << eshift];          // Relative +/-
        e = table[lsrc[3]];
        *src += 4;
        *src_len -= 4;
    } else {                                                // Ill-formed
        e = 0;
        *src += 1;
        *src_len -= 1;
    }
    return static_cast<uint8>(e);
}

// BigOneByte versions are needed for tables > 240 states, but most
// won't need the TwoByte versions.
bool UTF8HasGenericPropertyBigOneByte(const UTF8PropObj& st, const char* src) {
    const uint8* lsrc = reinterpret_cast<const uint8*>(src);
    const uint8* table_0 = &st.state_table[st.state0];
    const uint8* table = table_0;
    int e;
    int eshift = st.entry_shift;

    // Short series of tests faster than switch, optimizes 7-bit ASCII
    uint8 c = lsrc[0];
    if (Is7BitAscii(c)) {           // one byte
        e = table[c];
    } else if ((c & 0xe0) == 0xc0) {    // two bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
    } else if ((c & 0xf0) == 0xe0) {    // three bytes
        e = table[c];
        table = &table_0[e << (eshift + 4)];  // 16x the range
        e = (reinterpret_cast<const int8*>(table))[lsrc[1]];
        table = &table[e << eshift];          // Relative +/-
        e = table[lsrc[2]];
    } else {                            // four bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << (eshift + 4)];  // 16x the range
        e = (reinterpret_cast<const int8*>(table))[lsrc[2]];
        table = &table[e << eshift];          // Relative +/-
        e = table[lsrc[3]];
    }
    return e != 0;
}


// TwoByte versions are needed for tables > 240 states
// Look up property of one UTF-8 character and advance over it
// Return 0 if input length is zero
// Return 0 and advance one byte if input is ill-formed
uint8 UTF8GenericPropertyTwoByte(const UTF8PropObj_2* st,
                                 const uint8** src,
                                 int* src_len) {
            DCHECK(IsPropObj_2(*st));
    if (*src_len <= 0) {
        return 0;
    }

    const uint8* lsrc = *src;
    const uint16* table_0 = &st->state_table[st->state0];
    const uint16* table = table_0;
    int e;
    int eshift = st->entry_shift;

    // Short series of tests faster than switch, optimizes 7-bit ASCII
    uint8 c = lsrc[0];
    if (Is7BitAscii(c)) {           // one byte
        e = table[c];
        *src += 1;
        *src_len -= 1;
    } else if (((c & 0xe0) == 0xc0) && (*src_len >= 2)) {     // two bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        *src += 2;
        *src_len -= 2;
    } else if (((c & 0xf0) == 0xe0) && (*src_len >= 3)) {     // three bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << eshift];
        e = table[lsrc[2]];
        *src += 3;
        *src_len -= 3;
    } else if (((c & 0xf8) == 0xf0) && (*src_len >= 4)) {     // four bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << eshift];
        e = table[lsrc[2]];
        table = &table_0[e << eshift];
        e = table[lsrc[3]];
        *src += 4;
        *src_len -= 4;
    } else {                                                // Ill-formed
        e = 0;
        *src += 1;
        *src_len -= 1;
    }
    return static_cast<uint8>(e);
}

// TwoByte versions are needed for tables > 240 states
bool UTF8HasGenericPropertyTwoByte(const UTF8PropObj_2& st, const char* src) {
    const uint8* lsrc = reinterpret_cast<const uint8*>(src);
    const uint16* table_0 = &st.state_table[st.state0];
    const uint16* table = table_0;
    int e;
    int eshift = st.entry_shift;

    // Short series of tests faster than switch, optimizes 7-bit ASCII
    uint8 c = lsrc[0];
    if (Is7BitAscii(c)) {           // one byte
        e = table[c];
    } else if ((c & 0xe0) == 0xc0) {     // two bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
    } else if ((c & 0xf0) == 0xe0) {     // three bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << eshift];
        e = table[lsrc[2]];
    } else {                             // four bytes
        e = table[c];
        table = &table_0[e << eshift];
        e = table[lsrc[1]];
        table = &table_0[e << eshift];
        e = table[lsrc[2]];
        table = &table_0[e << eshift];
        e = table[lsrc[3]];
    }
    return e != 0;
}

// Approximate speeds on 2.8 GHz Pentium 4:
//   GenericScan 1-byte loop           300 MB/sec *
//   GenericScan 4-byte loop          1200 MB/sec
//   GenericScan 8-byte loop          2400 MB/sec *
//   GenericScanFastAscii 4-byte loop 3000 MB/sec
//   GenericScanFastAscii 8-byte loop 3200 MB/sec *
//
// * Implemented below. FastAscii loop is memory-bandwidth constrained.

// Scan a UTF-8 stringpiece based on state table.
// Always scan complete UTF-8 characters
// Set number of bytes scanned. Return reason for exiting
int UTF8GenericScan(const UTF8ScanObj* const st,
                    StringPiece str,
                    int* bytes_consumed) {
            DCHECK(IsScanObj(*st));
    *bytes_consumed = 0;
    if (str.empty()) {
        return kExitOK;
    }
    const int eshift = st->entry_shift;       // 6 (space optimized) or 8

    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    const uint8* const src_limit = src + str.length();
    const uint8* const initial_src = src;

    const uint8* const table_0 = &st->state_table[st->state0];
    const uint8* const table_2 = &st->fast_state[0];
    const uint32 losub = st->losub;
    const uint32 hiadd = st->hiadd;

    DoAgain:
    // Do state-table scan
    int e = 0;

    // Do fast for groups of 8 identity bytes.
    // This covers a lot of 7-bit ASCII ~8x faster than the 1-byte loop,
    // including slowing slightly on cr/lf/ht
    //----------------------------
    while (src_limit - src >= 8) {
        uint32 s0123 = UNALIGNED_LOAD32(src + 0);
        uint32 s4567 = UNALIGNED_LOAD32(src + 4);
        src += 8;
        // This is a fast range check for all bytes in [lowsub..0x80-hiadd)
        uint32 temp = (s0123 - losub) | (s0123 + hiadd) |
                      (s4567 - losub) | (s4567 + hiadd);
        if ((temp & 0x80808080) != 0) {
            // We typically end up here on cr/lf/ht; src was incremented
            int e0123 = (table_2[src[-8]] | table_2[src[-7]]) |
                        (table_2[src[-6]] | table_2[src[-5]]);
            if (e0123 != 0) {src -= 8; break;}    // Exit on Non-interchange
            e0123 = (table_2[src[-4]] | table_2[src[-3]]) |
                    (table_2[src[-2]] | table_2[src[-1]]);
            if (e0123 != 0) {src -= 4; break;}    // Exit on Non-interchange
            // Else OK, go around again
        }
    }
    //----------------------------

    // Byte-at-a-time scan
    //----------------------------
    const uint8* table = table_0;
    while (src < src_limit) {
        uint8 c = *src;
        e = table[c];
        src++;
        if (e >= kExitIllegalStructure) {break;}
        table = &table_0[e << eshift];
    }
    //----------------------------


    // Exit possibilities:
    //  Some exit code, !state0, back up over last char
    //  Some exit code, state0, back up one byte exactly
    //  source consumed, !state0, back up over partial char
    //  source consumed, state0, exit OK
    // For illegal byte in state0, avoid backup up over PREVIOUS char
    // For truncated last char, back up to beginning of it

    if (e >= kExitIllegalStructure) {
        // Back up over exactly one byte of rejected/illegal UTF-8 character
        src--;
        // Back up more if needed
        if (!InStateZero(st, table)) {
            do {src--;} while ((src > initial_src) && IsTrailByte(src[0]));
        }
    } else if (!InStateZero(st, table)) {
        // Back up over truncated UTF-8 character
        e = kExitIllegalStructure;
        do {src--;} while ((src > initial_src) && IsTrailByte(src[0]));
    } else {
        // Normal termination, source fully consumed
        e = kExitOK;
    }

    if (e == kExitDoAgain) {
        // Loop back up to the fast scan
        goto DoAgain;
    }

    *bytes_consumed = src - initial_src;
    return e;
}

int UTF8GenericScanTwoByte(const UTF8ScanObj_2* const st,
                           StringPiece str,
                           int* bytes_consumed) {
    // TODO(jrm) Do fast loop for groups of 8 identity bytes.
            DCHECK(IsScanObj_2(*st));
    const int eshift = st->entry_shift;
    const uint16* const table_0 = &st->state_table[st->state0];

    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    const uint8* const initial_src = src;
    int src_len = str.length();

    while (true) {
        // Invariant: We are at a character boundary, and all the
        // characters seen so far (if any) have the property.
        if (src_len == 0) {
            *bytes_consumed = src - initial_src;
            return kExitOK;
        }
        uint8 c = src[0];
        const uint16* table = table_0;
        int n;  // length in bytes of the current character
        uint16 e;
        if (c < 0x80) {  // one byte
            e = table[c];
            n = 1;
        } else if (((c & 0xe0) == 0xc0) && (src_len >= 2)) {     // two bytes
            e = table[c];
            table = &table_0[e << eshift];
            e = table[src[1]];
            n = 2;
        } else if (((c & 0xf0) == 0xe0) && (src_len >= 3)) {     // three bytes
            e = table[c];
            table = &table_0[e << eshift];
            e = table[src[1]];
            table = &table_0[e << eshift];
            e = table[src[2]];
            n = 3;
        } else if (((c & 0xf8) == 0xf0) && (src_len >= 4)) {     // four bytes
            e = table[c];
            table = &table_0[e << eshift];
            e = table[src[1]];
            table = &table_0[e << eshift];
            e = table[src[2]];
            table = &table_0[e << eshift];
            e = table[src[3]];
            n = 4;
        } else {                                             // Ill-formed
            *bytes_consumed = src - initial_src;
            return kExitIllegalStructure;
        }
        // e should be either 0 (the character has the property)
        // or kExitReject_2 (the character does not have the property).
        if (e != 0) {
                    DCHECK_EQ(e, kExitReject_2);
            *bytes_consumed = src - initial_src;
            return kExitOK;
        }
        src += n;
        src_len -= n;
    }
}

// Scan a UTF-8 stringpiece based on state table.
// Always scan complete UTF-8 characters
// Set number of bytes scanned. Return reason for exiting
// OPTIMIZED for case of 7-bit ASCII 0000..007f all valid
int UTF8GenericScanFastAscii(const UTF8ScanObj* st,
                             StringPiece str,
                             int* bytes_consumed) {
            DCHECK(IsScanObj(*st));
    *bytes_consumed = 0;
    if (str.empty()) return kExitOK;
    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    const uint8* const src_limit = src + str.length();
    const uint8* const initial_src = src;

    int exit_reason;
    do {
        //  Skip 8 bytes of ASCII at a whack; no endianness issue
        while ((src_limit - src >= 8) &&
               (((UNALIGNED_LOAD32(src + 0) | UNALIGNED_LOAD32(src + 4)) &
                 0x80808080) == 0)) {
            src += 8;
        }
        //  Run state table on the rest
        int rest_consumed;
        exit_reason = UTF8GenericScan(st, str.substr(src - initial_src),
                                      &rest_consumed);
        src += rest_consumed;
    } while (exit_reason == kExitDoAgain);

    *bytes_consumed = src - initial_src;
    return exit_reason;
}

int UTF8SpnStructurallyValid(StringPiece str) {
    int bytes_consumed;
    UTF8GenericScanFastAscii(&utf8acceptnonsurrogates_obj, str, &bytes_consumed);
    return bytes_consumed;
}

char* UTF8CoerceToStructurallyValid(StringPiece str, char* dst,
                                    const char replace_char) {
    // Copy the whole string (unless src == dst).
    if (str.data() != dst) {
        memmove(dst, str.data(), str.size());
    }
    char* const initial_dst = dst;
    while (!str.empty()) {
        int n_valid_bytes = UTF8SpnStructurallyValid(str);
        if (n_valid_bytes == str.size()) {
            break;
        }
        dst += n_valid_bytes;
        *dst++ = replace_char;  // replace one bad byte
        str.remove_prefix(n_valid_bytes + 1);
    }
    return initial_dst;
}

// C0 control codes cr lf ht valid, others not, 7f not valid
// C1 control codes not valid
static const uint8 kValidLatin1Tbl[256] = {
        0,0,0,0,0,0,0,0, 0,1,1,0,0,1,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,0,

        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
};

// Wrapper that gives true if all bytes are valid ISO-8859-1
// NOTE: NOT Microsoft code page 1252; bytes 0x80-0x9f return false.
bool IsValidLatin1(StringPiece str) {
    return IsValidLatin1CharStar(str.data(), str.length());
}

// Alternate wrapper without StringPiece
bool IsValidLatin1CharStar(const char* ch, const int len) {
    for (int i = 0; i < len; i++) {
        if (kValidLatin1Tbl[ch[i]] == 0) {
            return false;
        }
    }
    return true;
}

// Hack to change halfwidth katakana to match current UTF8CharToLower()
// TODO(jrm) remove when Unicode compatibility mapping is done.
//
// dsites 2006.06.15 update to track change 2563285 in letter.cc, getting
// legal result for *bare* sound marks

/*****
Initial non-hack transformation has just been applied to bytes before src, which
points to possible FF9E/FF9F, HALFWIDTH KATAKANA VOICED/SEMI-VOICED SOUND MARK
src[0] is the byte just after "]" below. c is the byte just before
dst points just after the transformed pre-src character, e.g.
  FF73;HALFWIDTH KATAKANA LETTER U; ==> 30A6;KATAKANA LETTER U
from google_lowercase_edits_ja.txt
"h" indicates a halfwidth character, "*" voiced s.m., "+" semi-voiced s.m.


DoSpecialFixup does these additional transformations:

HALFWIDTH KATAKANA VOICED SOUND MARK
  (ef bd b3] ef be 9e) => (e3 82 a6) => (e3 83 b4)
  FF73 FF9E => 30A6 => 30F4, Uh* => U* => VU    // Single mapping

  (ef bd b6] ef be 9e) ==> (e3 82 ac)
  FF76 FF9E ==> 30AC, KAh* ==> GA               // Spaced by 2's range
  FF77 FF9E ==> 30AE, KIh* ==> GI
   ...
  FF7F FF9E ==> 30BE, SOh* ==> ZO

  (ef be 80] ef be 9e) => (e3 82 bf) => (e3 83 80) Result 0xef8380: Expected 0xe38380
  FF80 FF9E => 30BF => 30C0, TAh* => TA* => DA  // Spaced by 3's range
   ...
  FF8D FF9E ==> 30D9, HEh* ==> BE
  FF8E FF9E ==> 30DC, HOh* ==> BO
  ---- FF9E ==> 3099 (] ef be 9e ==> e3 82 99)

HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK
  (ef be 8a] ef be 9f) => (e3 83 8f) => (e3 83 91)
  FF8A FF9F ==> 30D1, HAh+ => HA+ => PA         // Spaced by 3's range
   ...
  FF8D FF9F ==> 30DA, HEh+ ==> PE
  FF8E FF9F ==> 30DD, HOh+ ==> PO
  ---- FF9F ==> 309A (] ef be 9f ==> e3 82 9a)

*****/

// Return number of src bytes skipped
static int DoSpecialFixup(const uint8 c,
                          const uint8* const src,
                          const uint8* const src_limit,
                          uint8* dst) {
    if (src_limit - src >= 3) {
        if ((src[0] == 0xef) && (src[1] == 0xbe)) {
            if (src[2] == 0x9e) {
                if (src[-1] == 0xb3) {
                    // This is the single case U* => U => VU
                    // U+FF73 U+FF9E (ef bd b3] ef be 9e) => U+30F4 (e3 83 b4] ), U* => VU
                    dst[-2] = 0x83;           // make e3 83 b4 to get VU at dst
                    dst[-1] = 0xb4;           // make e3 83 b4 to get VU at dst
                } else if (src[-1] == 0x80) {
                    // This is the single case TA* => TA => DU, which carries in dst[-2]
                    // U+FF80 U+FF9E (ef be 80] ef be 9e) => U+30C0 (e3 83 80] ), TA*=> DA
                    // U+FF80 => U+30C0 TA => DA, have to carry into previous dst byte
                    dst[-2] = 0x83;           // make e3 83 b4 to get DA at dst
                    dst[-1] = 0x80;           // make e3 83 b4 to get DA at dst
                } else {
                    // This is all other voiced cases XXh* => XX* => YY
                    dst[-1] += 1;             // add 1 to get voiced character at dst
                }
                return 3;
            } else if ((src[2] == 0x9f) && (0x8a <= c) && (c <= 0x8e)) {
                // This is all semi-voiced cases XXh* => XX* => YY
                dst[-1] += 2;             // add 2 to get semi-voiced character at dst
                return 3;
            }
        }
    }
    return 0;
}


// Scan a UTF-8 buffer based on state table, copying to output buffer
//   and doing text replacements.
// Caller needs to loop if the returned value is kExitDoAgain.
static int UTF8GenericReplaceInternal(
        const UTF8ReplaceObj* st,
        const uint8* src,
        int src_len,
        uint8* dst,
        int dst_len,
        bool is_plain_text,
        int* const bytes_consumed,
        int* const bytes_filled,
        int* const chars_changed,
        OffsetMap* offsetmap) {
    int eshift = st->entry_shift;
    int n_entries = (1 << eshift);       // 64 or 256 entries per state

    // Keep track of the initial src and dst pointers, so we can easily compute
    // the number of bytes read (consumed) and written (filled).

    const uint8* const src_limit = src + src_len;
    const uint8* const initial_src = src;

    uint8* const dst_limit = dst + dst_len;
    uint8* const initial_dst = dst;

    *bytes_consumed = 0;
    *bytes_filled = 0;
    *chars_changed = 0;
    int total_changed = 0;

    // A starting point for each segment in the offset map.
    const uint8* copystart = src;

    // Invariant condition during replacements:
    //  remaining dst size >= remaining src size
    if ((dst_limit - dst) < (src_limit - src)) {
        if (offsetmap != nullptr) {
            offsetmap->Copy(src - copystart);
        }
        return kExitDstSpaceFull;
    }
    const uint8* const table_0 = &st->state_table[st->state0];

    Do_state_table:
    // Do state-table scan, copying as we go
    const uint8* table = table_0;
    int e = 0;
    uint8 c = 0;

    Do_state_table_newe:

    //----------------------------
    while (src < src_limit) {
        c = *src;
        e = table[c];
        *dst = c;
        src++;
        dst++;
        if (e >= kExitIllegalStructure) {break;}
        table = &table_0[e << eshift];
    }
    //----------------------------

    // Exit possibilities:
    //  Replacement code, do the replacement and loop
    //  Some other exit code, state0, back up one byte exactly
    //  Some other exit code, !state0, back up over last char
    //  source consumed, state0, exit OK
    //  source consumed, !state0, back up over partial char
    // For illegal byte in state0, avoid backup up over PREVIOUS char
    // For truncated last char, back up to beginning of it

    if (e >= kExitIllegalStructure) {
        // Switch on exit code; most loop back to top
        int offset = 0;
        switch (e) {
            // These all make the output string the same size or shorter
            // No checking needed
            case kExitReplace31:    // del 2, add 1 bytes to change
                dst -= 2;
                if (offsetmap != nullptr) {
                    offsetmap->Copy(src - copystart - 2);
                    offsetmap->Delete(2);
                    copystart = src;
                }
                dst[-1] = table[c + (n_entries * 1)];
                total_changed++;
                goto Do_state_table;
            case kExitReplace32:    // del 3, add 2 bytes to change
                dst--;
                if (offsetmap != nullptr) {
                    offsetmap->Copy(src - copystart - 1);
                    offsetmap->Delete(1);
                    copystart = src;
                }
                dst[-2] = table[c + (n_entries * 2)];
                dst[-1] = table[c + (n_entries * 1)];
                total_changed++;
                goto Do_state_table;
            case kExitReplace21:    // del 2, add 1 bytes to change
                dst--;
                if (offsetmap != nullptr) {
                    offsetmap->Copy(src - copystart - 1);
                    offsetmap->Delete(1);
                    copystart = src;
                }
                dst[-1] = table[c + (n_entries * 1)];
                total_changed++;
                goto Do_state_table;
            case kExitReplace3:    // update 3 bytes to change
                dst[-3] = table[c + (n_entries * 3)];
                FALLTHROUGH_INTENDED;
            case kExitReplace2:    // update 2 bytes to change
                dst[-2] = table[c + (n_entries * 2)];
                FALLTHROUGH_INTENDED;
            case kExitReplace1:    // update 1 byte to change
                dst[-1] = table[c + (n_entries * 1)];
                total_changed++;
                goto Do_state_table;
            case kExitReplace1S0:     // update 1 byte to change, 256-entry state
                dst[-1] = table[c + (256 * 1)];
                total_changed++;
                goto Do_state_table;
                // These can make the output string longer than the input
            case kExitReplaceOffset2:
                if ((n_entries != 256) && InStateZero(st, table)) {
                    // For space-optimized table, we need multiples of 256 bytes
                    // in state0 and multiples of n_entries in other states
                    offset += (table[c + (256 * 2)] << 8);
                } else {
                    offset += (table[c + (n_entries * 2)] << 8);
                }
                FALLTHROUGH_INTENDED;
            case kExitSpecial:      // Apply special fixups [read: hacks]
            case kExitReplaceOffset1:
                if ((n_entries != 256) && InStateZero(st, table)) {
                    // For space-optimized table, we need multiples of 256 bytes
                    // in state0 and multiples of n_entries in other states
                    offset += table[c + (256 * 1)];
                } else {
                    offset += table[c + (n_entries * 1)];
                }
                {
                    const RemapEntry* re = &st->remap_base[offset];
                    int del_len = re->delete_bytes & ~kReplaceAndResumeFlag;
                    int add_len = re->add_bytes & ~kHtmlPlaintextFlag;

                    // Special-case non-HTML replacement of five sensitive entities
                    //   &quot; &amp; &apos; &lt; &gt;
                    //   0022   0026  0027   003c 003e
                    // A replacement creating one of these is expressed as a pair of
                    // entries, one for HTML output and one for plaintext output.
                    // The first of the pair has the high bit of add_bytes set.
                    if ((re->add_bytes & kHtmlPlaintextFlag) != 0) {
                        // Use this entry for plain text
                        if (!is_plain_text) {
                            // Use very next entry for HTML text (same back/delete length)
                            re = &st->remap_base[offset + 1];
                            add_len = re->add_bytes & ~kHtmlPlaintextFlag;
                        }
                    }

                    int string_offset = re->bytes_offset;
                    // After the replacement, need (dst_limit - newdst) >= (src_limit - src)
                    uint8* newdst = dst - del_len + add_len;
                    if ((dst_limit - newdst) < (src_limit - src)) {
                        // Won't fit; don't do the replacement. Caller may realloc and retry
                        e = kExitDstSpaceFull;
                        break;    // exit, backing up over this char for later retry
                    }
                    dst -= del_len;
                    memcpy(dst, &st->remap_string[string_offset], add_len);
                    dst += add_len;
                    total_changed++;
                    if (offsetmap != nullptr) {
                        if (add_len > del_len) {
                            offsetmap->Copy(src - copystart);
                            offsetmap->Insert(add_len - del_len);
                            copystart = src;
                        } else if (add_len < del_len) {
                            offsetmap->Copy(src - copystart + add_len - del_len);
                            offsetmap->Delete(del_len - add_len);
                            copystart = src;
                        }
                    }
                    if ((re->delete_bytes & kReplaceAndResumeFlag) != 0) {
                        // There is a non-zero  target state at the end of the
                        // replacement string
                        e = st->remap_string[string_offset + add_len];
                        table = &table_0[e << eshift];
                        goto Do_state_table_newe;
                    }
                }
                if (e == kExitRejectAlt) {break;}
                if (e != kExitSpecial) {goto Do_state_table;}

                // case kExitSpecial:      // Apply special fixups [read: hacks]
                // In this routine, do either UTF8CharToLower()
                //   fullwidth/halfwidth mapping or
                //   voiced mapping or
                //   semi-voiced mapping
                // TODO(jrm) Remove all this crap when we do true
                // compatibility mapping

                // First, do EXIT_REPLACE_OFFSET1 action (above)
                // Second: do additional code fixup
                {
                    int srcdel = DoSpecialFixup(c, src, src_limit, dst);
                    src += srcdel;
                    if (offsetmap != nullptr) {
                        if (srcdel != 0) {
                            offsetmap->Copy(src - copystart - srcdel);
                            offsetmap->Delete(srcdel);
                            copystart = src;
                        }
                    }
                }
                goto Do_state_table;

            case kExitIllegalStructure:   // structurally illegal byte; quit
            case kExitReject:             // NUL or illegal code encountered; quit
            case kExitRejectAlt:          // Apply replacement, then exit
            default:                      // and all other exits
                break;
        }   // End switch (e)

        // Exit possibilities:
        //  Some other exit code, state0, back up one byte exactly
        //  Some other exit code, !state0, back up over last char

        // Back up over exactly one byte of rejected/illegal UTF-8 character
        src--;
        dst--;
        // Back up more if needed
        if (!InStateZero(st, table)) {
            do {
                src--;
                dst--;
            } while ((src > initial_src) && IsTrailByte(src[0]));
        }
    } else if (!InStateZero(st, table)) {
        // src >= src_limit, !state0
        // Back up over truncated UTF-8 character
        e = kExitIllegalStructure;
        do {
            src--;
            dst--;
        } while ((src > initial_src) && IsTrailByte(src[0]));
    } else {
        // src >= src_limit, state0
        // Normal termination, source fully consumed
        e = kExitOK;
    }

    if (offsetmap != nullptr) {
        if (src > copystart) {
            offsetmap->Copy(src - copystart);
        }
    }

    // Possible return values here:
    //  kExitDstSpaceFull         caller may realloc and retry from middle
    //  kExitIllegalStructure     caller my overwrite/truncate
    //  kExitOK                   all done and happy
    //  kExitReject               caller may overwrite/truncate
    //  kExitDoAgain              LOOP NOT DONE; caller must retry from middle
    //                            (may do fast ASCII loop first)
    //  kExitPlaceholder          -unused-
    //  kExitNone                 -unused-
    *bytes_consumed = src - initial_src;
    *bytes_filled = dst - initial_dst;
    *chars_changed = total_changed;

    // Sanity check that src advanced by at least one byte
    CHECK((e != kExitDoAgain) || (*bytes_consumed > 0))
                    << "Internal error: GenericReplace source pointer did not advance.";

    return e;
}

// TwoByte versions are needed for tables > 240 states, such
// as the table for full Unicode 4.1 canonical + compatibility mapping
// TODO(jrm) It would be nice to write this as a template, rather than
// duplicate all the source code.
// Scan a UTF-8 buffer based on state table with two-byte entries,
//   copying to output buffer
//   and doing text replacements.
// The caller needs to loop while the returned value is kExitDoAgain
static int UTF8GenericReplaceInternalTwoByte(
        const UTF8ReplaceObj_2* st,
        const uint8* src,
        int src_len,
        uint8* dst,
        int dst_len,
        bool is_plain_text,
        int* bytes_consumed,
        int* bytes_filled,
        int* chars_changed,
        OffsetMap* offsetmap) {
    int eshift = st->entry_shift;
    int n_entries = (1 << eshift);       // 64 or 256 entries per state

    const uint8* const src_limit = src + src_len;
    const uint8* const initial_src = src;

    uint8* const dst_limit = dst + dst_len;
    uint8* const initial_dst = dst;

    *bytes_consumed = 0;
    *bytes_filled = 0;
    *chars_changed = 0;
    int total_changed = 0;

    // A starting point for each segment in the offset map.
    const uint8* copystart = src;

    // Invariant condition during replacements:
    //  remaining dst size >= remaining src size
    if ((dst_limit - dst) < (src_limit - src)) {
        if (offsetmap != nullptr) {
            offsetmap->Copy(src - copystart);
            copystart = src;
        }
        return kExitDstSpaceFull_2;
    }
    const uint16* table_0 = &st->state_table[st->state0];

    Do_state_table_2:
    // Do state-table scan, copying as we go
    const uint16* table = table_0;
    int e = 0;
    uint8 c = 0;

    Do_state_table_newe_2:

    //----------------------------
    while (src < src_limit) {
        c = *src;
        e = table[c];
        *dst = c;
        src++;
        dst++;
        if (e >= kExitIllegalStructure_2) {break;}
        table = &table_0[e << eshift];
    }
    //----------------------------

    // Exit possibilities:
    //  Replacement code, do the replacement and loop
    //  Some other exit code, state0, back up one byte exactly
    //  Some other exit code, !state0, back up over last char
    //  source consumed, state0, exit OK
    //  source consumed, !state0, back up over partial char
    // For illegal byte in state0, avoid backup up over PREVIOUS char
    // For truncated last char, back up to beginning of it

    if (e >= kExitIllegalStructure_2) {
        // Switch on exit code; most loop back to top
        int offset = 0;
        switch (e) {
            // These all make the output string the same size or shorter
            // No checking needed
            case kExitReplace31_2:    // del 2, add 1 bytes to change
                dst -= 2;
                if (offsetmap != nullptr) {
                    offsetmap->Copy(src - copystart - 2);
                    offsetmap->Delete(2);
                    copystart = src;
                }
                dst[-1] = static_cast<uint8>(table[c + (n_entries * 1)] & 0xff);
                // TODO(jrm) Determine whether these static_casts are necessary. Since
                // every right-hand side is of the form "x & 0xff", they're probably not
                // needed. Perhaps the "& 0xff" isn't needed, either, since implicit
                // conversion from uint16 to uint8 always truncates the high-order bits.
                total_changed++;
                goto Do_state_table_2;
            case kExitReplace32_2:    // del 3, add 2 bytes to change
                dst--;
                if (offsetmap != nullptr) {
                    offsetmap->Copy(src - copystart - 1);
                    offsetmap->Delete(1);
                    copystart = src;
                }
                dst[-2] = static_cast<uint8>(table[c + (n_entries * 1)] >> 8 & 0xff);
                dst[-1] = static_cast<uint8>(table[c + (n_entries * 1)] & 0xff);
                total_changed++;
                goto Do_state_table_2;
            case kExitReplace21_2:    // del 2, add 1 bytes to change
                dst--;
                if (offsetmap != nullptr) {
                    offsetmap->Copy(src - copystart - 1);
                    offsetmap->Delete(1);
                    copystart = src;
                }
                dst[-1] = static_cast<uint8>(table[c + (n_entries * 1)] & 0xff);
                total_changed++;
                goto Do_state_table_2;
            case kExitReplace3_2:    // update 3 bytes to change
                dst[-3] = static_cast<uint8>(table[c + (n_entries * 2)] & 0xff);
                FALLTHROUGH_INTENDED;
            case kExitReplace2_2:    // update 2 bytes to change
                dst[-2] = static_cast<uint8>(table[c + (n_entries * 1)] >> 8 & 0xff);
                FALLTHROUGH_INTENDED;
            case kExitReplace1_2:    // update 1 byte to change
                dst[-1] = static_cast<uint8>(table[c + (n_entries * 1)] & 0xff);
                total_changed++;
                goto Do_state_table_2;
            case kExitReplace1S0_2:     // update 1 byte to change, 256-entry state
                dst[-1] = static_cast<uint8>(table[c + (256 * 1)] & 0xff);
                total_changed++;
                goto Do_state_table_2;
                // These can make the output string longer than the input
            case kExitReplaceOffset2_2:
                if ((n_entries != 256) && InStateZero_2(st, table)) {
                    // For space-optimized table, we need multiples of 256 bytes
                    // in state0 and multiples of n_entries in other states
                    offset +=
                            (static_cast<uint8>(table[c + (256 * 1)] >> 8 & 0xff)
                                    << 8);
                } else {
                    offset +=
                            (static_cast<uint8>(table[c + (n_entries * 1)] >> 8 & 0xff)
                                    << 8);
                }
                FALLTHROUGH_INTENDED;
            case kExitReplaceOffset1_2:
                if ((n_entries != 256) && InStateZero_2(st, table)) {
                    // For space-optimized table, we need multiples of 256 bytes
                    // in state0 and multiples of n_entries in other states
                    offset += static_cast<uint8>(table[c + (256 * 1)] & 0xff);
                } else {
                    offset += static_cast<uint8>(table[c + (n_entries * 1)] & 0xff);
                }
                {
                    const RemapEntry* re = &st->remap_base[offset];
                    int del_len = re->delete_bytes & ~kReplaceAndResumeFlag;
                    int add_len = re->add_bytes & ~kHtmlPlaintextFlag;
                    // Special-case non-HTML replacement of five sensitive entities
                    //   &quot; &amp; &apos; &lt; &gt;
                    //   0022   0026  0027   003c 003e
                    // A replacement creating one of these is expressed as a pair of
                    // entries, one for HTML output and one for plaintext output.
                    // The first of the pair has the high bit of add_bytes set.
                    if ((re->add_bytes & kHtmlPlaintextFlag) != 0) {
                        // Use this entry for plain text
                        if (!is_plain_text) {
                            // Use very next entry for HTML text (same back/delete length)
                            re = &st->remap_base[offset + 1];
                            add_len = re->add_bytes & ~kHtmlPlaintextFlag;
                        }
                    }

                    // After the replacement, need (dst_limit - dst) >= (src_limit - src)
                    int string_offset = re->bytes_offset;
                    // After the replacement, need (dst_limit - newdst) >= (src_limit - src)
                    uint8* newdst = dst - del_len + add_len;
                    if ((dst_limit - newdst) < (src_limit - src)) {
                        // Won't fit; don't do the replacement. Caller may realloc and retry
                        e = kExitDstSpaceFull_2;
                        break;    // exit, backing up over this char for later retry
                    }
                    dst -= del_len;
                    memcpy(dst, &st->remap_string[string_offset], add_len);
                    dst += add_len;
                    if (offsetmap != nullptr) {
                        if (add_len > del_len) {
                            offsetmap->Copy(src - copystart);
                            offsetmap->Insert(add_len - del_len);
                            copystart = src;
                        } else if (add_len < del_len) {
                            offsetmap->Copy(src - copystart + add_len - del_len);
                            offsetmap->Delete(del_len - add_len);
                            copystart = src;
                        }
                    }
                    if ((re->delete_bytes & kReplaceAndResumeFlag) != 0) {
                        // There is a two-byte non-zero target state at the end of the
                        // replacement string
                        uint8 c1 = st->remap_string[string_offset + add_len];
                        uint8 c2 = st->remap_string[string_offset + add_len + 1];
                        e = (c1 << 8) | c2;
                        table = &table_0[e << eshift];
                        total_changed++;
                        goto Do_state_table_newe_2;
                    }
                }
                total_changed++;
                if (e == kExitRejectAlt_2) {break;}
                goto Do_state_table_2;

            case kExitSpecial_2:           // NO special fixups [read: hacks]
            case kExitIllegalStructure_2:  // structurally illegal byte; quit
            case kExitReject_2:            // NUL or illegal code encountered; quit
                // and all other exits
            default:
                break;
        }   // End switch (e)

        // Exit possibilities:
        //  Some other exit code, state0, back up one byte exactly
        //  Some other exit code, !state0, back up over last char

        // Back up over exactly one byte of rejected/illegal UTF-8 character
        src--;
        dst--;
        // Back up more if needed
        if (!InStateZero_2(st, table)) {
            do {src--; dst--;} while (src > initial_src && IsTrailByte(src[0]));
        }
    } else if (!InStateZero_2(st, table)) {
        // src >= src_limit, !state0
        // Back up over truncated UTF-8 character
        e = kExitIllegalStructure_2;

        do {src--; dst--;} while (src > initial_src && IsTrailByte(src[0]));
    } else {
        // src >= src_limit, state0
        // Normal termination, source fully consumed
        e = kExitOK_2;
    }

    if (offsetmap != nullptr) {
        if (src > copystart) {
            offsetmap->Copy(src - copystart);
            copystart = src;
        }
    }


    // Possible return values here:
    //  kExitDstSpaceFull_2         caller may realloc and retry from middle
    //  kExitIllegalStructure_2     caller my overwrite/truncate
    //  kExitOK_2                   all done and happy
    //  kExitReject_2               caller may overwrite/truncate
    //  kExitDoAgain_2              LOOP NOT DONE; caller must retry from middle
    //                            (may do fast ASCII loop first)
    //  kExitPlaceholder_2          -unused-
    //  kExitNone_2                 -unused-
    *bytes_consumed = src - initial_src;
    *bytes_filled = dst - initial_dst;
    *chars_changed = total_changed;
    // Sanity check that src advanced by at least one byte
    CHECK((e != kExitDoAgain_2) || (*bytes_consumed > 0))
                    << "Internal error: GenericReplace2 source pointer did not advance.";
    return e;
}


// Scan a UTF-8 stringpiece based on state table, copying to output stringpiece
//   and doing text replacements.
// Also writes an optional OffsetMap. Pass NULL to skip writing one.
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Return reason for exiting
int UTF8GenericReplace(const UTF8ReplaceObj* st,
                       StringPiece str,
                       char* dst_buf,
                       int dst_len,
                       bool is_plain_text,
                       int* bytes_consumed,
                       int* bytes_filled,
                       int* chars_changed,
                       OffsetMap* offsetmap) {
            DCHECK(IsReplaceObj(*st));
    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    int src_len = str.size();
    uint8* dst = reinterpret_cast<uint8*>(dst_buf);
    int total_consumed = 0;
    int total_filled = 0;
    int total_changed = 0;
    int local_bytes_consumed, local_bytes_filled, local_chars_changed;
    int e;
    do {
        e = UTF8GenericReplaceInternal(st,
                                       src, src_len, dst, dst_len, is_plain_text,
                                       &local_bytes_consumed, &local_bytes_filled,
                                       &local_chars_changed,
                                       offsetmap);
        src += local_bytes_consumed;
        src_len -= local_bytes_consumed;
        dst += local_bytes_filled;
        dst_len -= local_bytes_filled;
        total_consumed += local_bytes_consumed;
        total_filled += local_bytes_filled;
        total_changed += local_chars_changed;
    } while ( e == kExitDoAgain );
    *bytes_consumed = total_consumed;
    *bytes_filled = total_filled;
    *chars_changed = total_changed;
    return e;
}

// Scan a UTF-8 stringpiece based on state table with two-byte entries,
//   copying to output stringpiece
//   and doing text replacements.
// Also writes an optional OffsetMap. Pass NULL to skip writing one.
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Return reason for exiting
int UTF8GenericReplaceTwoByte(const UTF8ReplaceObj_2* st,
                              StringPiece str,
                              char* dst_buf,
                              int dst_len,
                              bool is_plain_text,
                              int* bytes_consumed,
                              int* bytes_filled,
                              int* chars_changed,
                              OffsetMap* offsetmap) {
            DCHECK(IsReplaceObj_2(*st));
    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    int src_len = str.size();
    uint8* dst = reinterpret_cast<uint8*>(dst_buf);
    int total_consumed = 0;
    int total_filled = 0;
    int total_changed = 0;
    int local_bytes_consumed, local_bytes_filled, local_chars_changed;
    int e;
    do {
        e = UTF8GenericReplaceInternalTwoByte(st,
                                              src, src_len, dst, dst_len, is_plain_text,
                                              &local_bytes_consumed,
                                              &local_bytes_filled,
                                              &local_chars_changed,
                                              offsetmap);
        src += local_bytes_consumed;
        src_len -= local_bytes_consumed;
        dst += local_bytes_filled;
        dst_len -= local_bytes_filled;
        total_consumed += local_bytes_consumed;
        total_filled += local_bytes_filled;
        total_changed += local_chars_changed;
    } while ( e == kExitDoAgain_2 );
    *bytes_consumed = total_consumed;
    *bytes_filled = total_filled;
    *chars_changed = total_changed;

    return e - kExitOK_2 + kExitOK;
}

static const uint8 AsciiToLowerTbl[128] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17, 0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27, 0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37, 0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,

        0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67, 0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77, 0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
        0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67, 0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77, 0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
};

static const uint8 AsciiChangedTbl[128] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

        0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


// Scan a UTF-8 stringpiece quickly, copying to output stringpiece
//   and doing Ascii lowercase replacements. Stops on non-Ascii.
// Return reason for exiting
// DO NOT CALL DIRECTLY. Use UTF8ToLowerReplace() below
static int UTF8AsciiToLowerInternal(const uint8* src, int src_len,
                                    uint8* dst, int dst_len,
                                    int* bytes_consumed, int* bytes_filled,
                                    int* chars_changed) {
    const uint8* const src_limit = src + src_len;
    const uint8* const initial_src = src;
    uint8* const dst_limit = dst + dst_len;
    *bytes_consumed = 0;
    *bytes_filled = 0;
    *chars_changed = 0;

    int total_changed = 0;

    // Invariant condition during replacements:
    //  remaining dst size >= remaining src size
    if ((dst_limit - dst) < (src_limit - src)) {
        return kExitDstSpaceFull;
    }

    // Align ASCII to a 4-byte destination boundary if possible
    while (src < src_limit
           && (reinterpret_cast<uintptr_t>(dst) & 3) != 0
           && Is7BitAscii(*src)) {
        *dst = AsciiToLowerTbl[*src];
        total_changed += AsciiChangedTbl[*src];
        src++;
        dst++;
    }

    // Do 4 bytes of ASCII at a whack; no endianness issue
    //----------------------------
    while ((src_limit - src >= 4) &&
           ((UNALIGNED_LOAD32(src) & 0x80808080) == 0)) {
        if ((UNALIGNED_LOAD32(src) & 0xa0a0a0a0) == 0x20202020) {
            // Four bytes of 2x, 3x, 6x, 7x, punct, digits, lowercase;
            // just copy 4 bytes to aligned dst, no endianness issue
            uint32* dst4 = reinterpret_cast<uint32*>(dst);
            dst4[0] = UNALIGNED_LOAD32(src);
        } else {
            // Some 0x, 1x, 4x, 5x possible uppercase
            dst[0] = AsciiToLowerTbl[src[0]];
            total_changed += AsciiChangedTbl[src[0]];
            dst[1] = AsciiToLowerTbl[src[1]];
            total_changed += AsciiChangedTbl[src[1]];
            dst[2] = AsciiToLowerTbl[src[2]];
            total_changed += AsciiChangedTbl[src[2]];
            dst[3] = AsciiToLowerTbl[src[3]];
            total_changed += AsciiChangedTbl[src[3]];
        }
        src += 4;
        dst += 4;
    }
    //----------------------------

    *bytes_filled = *bytes_consumed = src - initial_src;
    *chars_changed = total_changed;
    if (src_limit - src < 4) {
        return kExitOK;
    }
    return kExitDoAgain;
}

// Scan a UTF-8 stringpiece based on a state table,
//  doing full Unicode.org case folding.
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Return reason for exiting
//
// Optimized to do ASCII ToLower quickly
// Does generic scan with a specific state table
//
// The state table is built with --exit_optimize, which means it
// can exit after 4 consecutive 7-bit Ascii characters. Thus the caller (here)
// must loop while e == kExitDoAgain
int UTF8ToFoldedReplace(StringPiece str,
                        char* dst_buf,
                        int dst_len,
                        int* bytes_consumed,
                        int* bytes_filled,
                        int* chars_changed) {
    // Pick the table to use
    const UTF8ReplaceObj* st = &utf8cvtfolded_obj;

    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    int src_len = str.size();
    uint8* dst = reinterpret_cast<uint8*>(dst_buf);
    int total_consumed = 0;
    int total_filled = 0;
    int total_changed = 0;
    int local_bytes_consumed, local_bytes_filled, local_chars_changed;
    int e = kExitOK;
    do {
        // About 90% of all our text is all-7bit-ASCII, so we special-case it
        if (src_len > 0) {
            e = UTF8AsciiToLowerInternal(src, src_len, dst, dst_len,
                                         &local_bytes_consumed, &local_bytes_filled,
                                         &local_chars_changed);
            src += local_bytes_consumed;
            src_len -= local_bytes_consumed;
            dst += local_bytes_filled;
            dst_len -= local_bytes_filled;
            total_consumed += local_bytes_consumed;
            total_filled += local_bytes_filled;
            total_changed += local_chars_changed;
        }

        if (src_len > 0) {
            bool is_plain_text = false;
            e = UTF8GenericReplaceInternal(st,
                                           src, src_len, dst, dst_len, is_plain_text,
                                           &local_bytes_consumed, &local_bytes_filled,
                                           &local_chars_changed,
                                           nullptr);
            src += local_bytes_consumed;
            src_len -= local_bytes_consumed;
            dst += local_bytes_filled;
            dst_len -= local_bytes_filled;
            total_consumed += local_bytes_consumed;
            total_filled += local_bytes_filled;
            total_changed += local_chars_changed;
        }
    } while ( e == kExitDoAgain );
    *bytes_consumed = total_consumed;
    *bytes_filled = total_filled;
    *chars_changed = total_changed;
    return e;
}


// Scan a UTF-8 stringpiece based on one of three state tables,
//  converting to lowercase characters
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Return reason for exiting
//
// Optimized to do ASCII ToLower quickly
// Does generic scan with a specific state table
//
// The state tables are built with --exit_optimize, which means they
// can exit after 4 consecutive 7-bit Ascii characters. Thus the caller (here)
// must loop while e == kExitDoAgain
//
// DO NOT call directly. Use UniLib::ToLower() or UniLib::ToLowerHack()
int UTF8ToLowerReplace(StringPiece str,
                       char* dst_buf,
                       int dst_len,
                       int* bytes_consumed,
                       int* bytes_filled,
                       int* chars_changed,
                       bool no_tolower_normalize,
                       bool ja_normalize,
                       OffsetMap* offsetmap) {
    // Pick the table to use
    const UTF8ReplaceObj* st;
    if (no_tolower_normalize) {
        st = &utf8cvtlower_obj;                 // Standard Unicode
    } else {
        if (ja_normalize) {
            st = &utf8cvtlowerorigja_obj;         // Google fullwidth &
            // halfwidth Japanese hacks
        } else {
            st = &utf8cvtlowerorig_obj;           // Google fullwidth hacks
        }
    }

    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    int src_len = str.size();
    uint8* dst = reinterpret_cast<uint8*>(dst_buf);
    int total_consumed = 0;
    int total_filled = 0;
    int total_changed = 0;
    int local_bytes_consumed, local_bytes_filled, local_chars_changed;
    int e = kExitOK;
    do {
        // About 90% of all our text is all-7bit-ASCII, so we special-case it
        if (src_len > 0) {
            e = UTF8AsciiToLowerInternal(src, src_len, dst, dst_len,
                                         &local_bytes_consumed, &local_bytes_filled,
                                         &local_chars_changed);
            src += local_bytes_consumed;
            src_len -= local_bytes_consumed;
            dst += local_bytes_filled;
            dst_len -= local_bytes_filled;
            total_consumed += local_bytes_consumed;
            total_filled += local_bytes_filled;
            total_changed += local_chars_changed;
            if (offsetmap != nullptr) {
                offsetmap->Copy(local_bytes_filled);
            }
        }

        if (src_len > 0) {
            bool is_plain_text = false;
            e = UTF8GenericReplaceInternal(st,
                                           src, src_len, dst, dst_len, is_plain_text,
                                           &local_bytes_consumed, &local_bytes_filled,
                                           &local_chars_changed,
                                           offsetmap);
            src += local_bytes_consumed;
            src_len -= local_bytes_consumed;
            dst += local_bytes_filled;
            dst_len -= local_bytes_filled;
            total_consumed += local_bytes_consumed;
            total_filled += local_bytes_filled;
            total_changed += local_chars_changed;
        }
    } while ( e == kExitDoAgain );
    *bytes_consumed = total_consumed;
    *bytes_filled = total_filled;
    *chars_changed = total_changed;
    return e;
}

// Scan a UTF-8 stringpiece based on state table, copying to output stringpiece
//   and doing titlecase text replacements.
// THIS CONVERTS EVERY CHARACTER TO TITLECASE. You probably only want to call
//  it with a single character, at the front of a "word", then convert the rest
//  of the word to lowercase. You have to figure out where the words begin and
//  end, and what to do with e.g. l'Hospital
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Set number of input characters changed (0 if none).
// Return reason for exiting
int UTF8ToTitleReplace(StringPiece str,
                       char* dst_buf,
                       int dst_len,
                       int* bytes_consumed,
                       int* bytes_filled,
                       int* chars_changed) {
    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    int src_len = str.size();
    uint8* dst = reinterpret_cast<uint8*>(dst_buf);
    return UTF8GenericReplaceInternal(
            &utf8cvttitle_obj,  // Pick the table to use
            src, src_len, dst, dst_len,
            /*is_plain_text=*/false,
            bytes_consumed,
            bytes_filled,
            chars_changed,
            /*offsetmap=*/nullptr);
}

// Scan a UTF-8 stringpiece based on state table, copying to output stringpiece
//   and doing uppercase text replacements.
// Always scan complete UTF-8 characters
// Set number of bytes consumed from input, number filled to output.
// Set number of input characters changed (0 if none).
// Return reason for exiting
int UTF8ToUpperReplace(StringPiece str,
                       char* dst_buf,
                       int dst_len,
                       int* bytes_consumed,
                       int* bytes_filled,
                       int* chars_changed) {
    const uint8* src = reinterpret_cast<const uint8*>(str.data());
    int src_len = str.size();
    uint8* dst = reinterpret_cast<uint8*>(dst_buf);
    return UTF8GenericReplaceInternal(
            &utf8cvtupper_obj,   // Pick the table to use
            src, src_len, dst, dst_len,
            /*is_plain_text=*/false,
            bytes_consumed,
            bytes_filled,
            chars_changed,
            /*offsetmap=*/nullptr);
}

//#if 1
////----------------------------------------------------------------------
//// EXPERIMENT
////
//// Design center is 10-40 bytes of input
//// Hardwired utf8cvtlowerorigja_obj conversion table
//// Returns number of changes
//int UTF8ToLowerShort(const char* src_buf,
//                     int src_len,
//                     string* output_buffer) {
//    const uint8* src = reinterpret_cast<const uint8*>(src_buf);
//    // Initial estimate for dst_len is src_len.
//    int dst_len = src_len;
//    output_buffer->clear();
//    int total_changed = 0;
//    FixedArray<char> dst_buf(dst_len);
//    uint8* dst = reinterpret_cast<uint8*>(dst_buf.get());
//    bool is_plain_text = false;
//    while (true) {
//        // No special case of all-7bit-ASCII
//        int local_bytes_consumed, local_bytes_filled, local_chars_changed;
//        int e = UTF8GenericReplaceInternal(
//                &utf8cvtlowerorigja_obj, src, src_len, dst, dst_len, is_plain_text,
//                &local_bytes_consumed, &local_bytes_filled, &local_chars_changed,
//                nullptr);
//        output_buffer->append(dst_buf.get(), local_bytes_filled);
//        total_changed += local_chars_changed;
//        if (e == kExitDoAgain || e == kExitDstSpaceFull) {
//            src += local_bytes_consumed;
//            src_len -= local_bytes_consumed;
//        } else {
//            break;
//        }
//    }
//    return total_changed;
//}
//
////----------------------------------------------------------------------
//#endif


// Adjust a stringpiece to encompass complete UTF-8 characters.
// The data pointer will be increased by 0..3 bytes to get to a character
// boundary, and the length will then be decreased by 0..3 bytes
// to encompass the last complete character.
void UTF8TrimToChars(StringPiece* str) {
    const char* const src = str->data();
    int src_len = str->length();
    // Exit if empty string
    if (src_len == 0) {
        return;
    }

    // Exit on simple, common case
    if (!IsTrailByte(src[0]) && Is7BitAscii(src[src_len - 1])) {
        // First byte is not a continuation and last byte is 7-bit ASCII -- done
        return;
    }

    // Adjust the back end, src_len > 0
    const char* src_limit = src + src_len;
    // Backscan over any ending continuation bytes to find last char start
    const char* s = src_limit - 1;         // Last byte of the string
    while ((src <= s) && IsTrailByte(*s)) {
        s--;
    }
    // Include entire last char if it fits
    if (src <= s) {
        int last_char_len = UniLib::OneCharLen(s);
        if (src_limit - s >= last_char_len) {
            // Last char fits, so include it, else exclude it
            s += last_char_len;
        }
    }
    if (s != src_limit) {
        // s is one byte beyond the last full character, if any
        str->remove_suffix(src_limit - s);
        // Exit if now empty string
        if (str->length() == 0) {
            return;
        }
    }

    // Adjust the front end, src_len > 0
    src_len = str->length();
    src_limit = src + src_len;
    s = src;                            // First byte of the string
    // Scan over any beginning continuation bytes to find first char start
    while ((s < src_limit) && IsTrailByte(*s)) {
        s++;
    }
    if (s != src) {
        // s is at the first full character, if any
        str->remove_prefix(s - src);
    }
}