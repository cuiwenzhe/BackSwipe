// Copyright 2006 Google Inc. All rights reserved
//
// Routines to do Google manipulation of Unicode characters or text
//
//
// The exact text-file definitions of the sets of characters, along with scripts
// to extract them from Unicode.org files and turn them into state tables are
// all in:
//   util/utf8/unilib
//
// See the design document in
// https://g3doc.corp.google.com/util/utf8/public/g3doc/utf8_state_tables.md

#include "unilib.h"

#include <string.h>   // for memcpy, strncmp
#include <algorithm>  // for max, min

#include "logging.h"         // for operator<<, etc
#include "macros.h"          // for NULL, ARRAYSIZE
//#include "strings/cord.h"         // for CordReader, Cord (ptr only)
#include "escaping.h"     // for CHexEscape
#include "utf.h"  // for chartorune, ::UTFmax, Rune, etc
#include "fixedarray.h"  // for FixedArray
#include "stl_util.h"    // for STLStringResizeUninitialized
#include "utf8statetable_internal.h"
#include "offsetmap.h"
#include "utf8statetable.h"  // for kExitDstSpaceFull
#include "utf/utf8acceptinterchange.h"      // Pure table bytes
#include "utf/utf8acceptinterchange7bit.h"  // Pure table bytes
#include "utf/utf8cvtfw.h"         // for utf8cvtfw_obj
#include "utf/utf8cvtfw_an.h"      // for utf8cvtfw_an_obj
#include "utf/utf8cvthwk.h"        // for utf8cvthwk_obj
#include "utf/utf8scannotfw.h"     // for utf8scannotfw_obj
#include "utf/utf8scannotfw_an.h"  // for utf8scannotfw_an_obj
#include "utf/utf8scannothwk.h"    // for utf8scannothwk_obj

bool UniLib::IsUTF8ValidCodepoint(StringPiece str) {
    char32 c;
    int consumed;
    // It's OK if str.length() > consumed.
    return !str.empty()
           && isvalidcharntorune(str.data(), str.length(), &c, &consumed)
           && IsValidCodepoint(c);
}

// C0 control codes CR LF HT valid, others not, 7F not valid
// C1 control codes not valid
static const unsigned char kValidLatin1Tbl[256] = {
        0,0,0,0,0,0,0,0, 0,1,1,0,1,1,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,0,

        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
};


// Returns true if the source is all valid ISO-8859-1 bytes
// with no C0 or C1 control codes (other than CR LF HT)
// NOTE: NOT Microsoft code page 1252; bytes 0x80-0x9f return false.
// A source buffer that is not structurally valid UTF-8 but is valid
// Latin1 needs to be input-converted to proper UTF-8.
// It cannot be used as-is!
// This is a common enough bug in existing Google
// code and data that this test function is included for the convenience
// of callers who will then do the input conversion:
// if (!IsStructurallyValid()) {
//   if (IsValidLatin1()) {
//     ConvertEncodingBufferToUTF8(..., ISO_8859_1, ...);
//   } else {
//     CoerceToStructurallyValid(..., ' ');
//   }
// }
bool UniLib::IsValidLatin1(const char* src, int byte_length) {
    for (int i = 0; i < byte_length; i++) {
        if (kValidLatin1Tbl[static_cast<uint8>(src[i])] == 0) {
            return false;
        }
    }
    return true;
}

bool UniLib::IsValidLatin1(const string& src) {
    return UniLib::IsValidLatin1(src.data(), src.length());
}

// Returns true if the source is all structurally valid UTF-8
bool UniLib::IsStructurallyValid(const char* src, int byte_length) {
    return UTF8IsStructurallyValid(StringPiece(src, byte_length));
}

bool UniLib::IsStructurallyValid(StringPiece src) {
    return UTF8IsStructurallyValid(src);
}

// TODO: Import Cord
/*bool UniLib::CordIsStructurallyValid(const Cord& cord) {
    CordReader reader(cord);
    StringPiece fragment;
    string s;  // Leftover bytes (< UTFmax) from previous fragment.
    while (reader.ReadFragment(&fragment)) {
        if (!s.empty()) {
            s.append(fragment.data(), fragment.size());
            fragment = StringPiece(s);
        }
        const int valid = SpanStructurallyValid(fragment);
        fragment.remove_prefix(valid);
        // If the last Unicode char crosses to next fragment, length must be
        // smaller than UTFmax.
        if (UTFmax <= fragment.size()) {
            return false;
        }
        s = fragment.ToString();
    }
    return s.empty();
}*/

// Returns the length in bytes of the prefix of src that is all
// structurally valid UTF-8.
int UniLib::SpanStructurallyValid(const char* src, int byte_length) {
    return UTF8SpnStructurallyValidCharStar(src, byte_length);
}

int UniLib::SpanStructurallyValid(const StringPiece src) {
    return UTF8SpnStructurallyValid(src);
}



// Coerce source buffer to structurally valid UTF-8 in destination buffer,
// overwriting illegal bytes with replace_char (typically ' ' or '?').
// replace_char must be legal printable 7-bit Ascii in the range 0x20..0x7e.
// Caller allocates the destination buffer
// Return value is the number bytes filled in the destination buffer
// (and is always equal to src_bytes)
// dst_bytes must be >= src_bytes
// src and dst pointers may be exactly equal.
int UniLib::CoerceToStructurallyValid(const char* src, int src_bytes,
                                      const char replace_char,
                                      char* dst, int dst_bytes) {
    CHECK_GE(dst_bytes, src_bytes);
    CHECK(replace_char >= 0x20 && replace_char <= 0x7E);

    StringPiece sp(src, src_bytes);
    UTF8CoerceToStructurallyValid(sp, dst, replace_char);
    return src_bytes;
}


// Coerce source buffer to structurally valid UTF-8 in destination string.
string UniLib::CoerceToStructurallyValid(const char* src, int src_bytes,
                                         const char replace_char) {
    string dst_str;
    // Don't copy anything. CoerceToStructurallyValid will do it if needed.
    STLStringResizeUninitialized(&dst_str, src_bytes);
    CoerceToStructurallyValid(src, src_bytes, replace_char, &dst_str[0],
                              src_bytes);
    return dst_str;
}

void UniLib::CoerceToStructurallyValid(string* s, char replace_char) {
    if (s != nullptr && !s->empty()) {
        int length = s->length();
        CoerceToStructurallyValid(&(*s)[0], length, replace_char, &(*s)[0], length);
    }
}


// Returns true if the source is all interchange valid UTF-8
// "Interchange valid" is a stronger than structurally valid --
// no C0 or C1 control codes (other than CR LF HT FF) and no non-characters.
bool UniLib::IsInterchangeValid(StringPiece src) {
    int bytes_consumed;
    const UTF8ReplaceObj* st = &utf8acceptinterchange_obj;
    UTF8GenericScan(st, src, &bytes_consumed);
    return (bytes_consumed == src.length());
}

bool UniLib::IsInterchangeValid(const char* src, int byte_length) {
    return IsInterchangeValid(StringPiece(src, byte_length));
}

bool UniLib::IsInterchangeValid(char32 c) {
    if (!IsValidCodepoint(c)) return false;
    char buf[UTFmax];
    int len = runetochar(buf, &c);
    return IsInterchangeValid(StringPiece(buf, len));
}

// Returns the length in bytes of the prefix of src that is all
//  interchange valid UTF-8
int UniLib::SpanInterchangeValid(const StringPiece src) {
    int bytes_consumed;
    const UTF8ReplaceObj* st = &utf8acceptinterchange_obj;
    UTF8GenericScan(st, src, &bytes_consumed);
    return bytes_consumed;
}

int UniLib::SpanInterchangeValid(const char* src, int byte_length) {
    return SpanInterchangeValid(StringPiece(src, byte_length));
}

// Coerce source buffer to interchange valid UTF-8 in destination buffer,
// Return value is the number bytes filled in the destination buffer
int UniLib::CoerceToInterchangeValid(const char* src, int src_bytes,
                                     const char replace_char,
                                     char* dst, int dst_bytes) {
    CHECK_GE(dst_bytes, src_bytes);
    CHECK(replace_char >= 0x20 && replace_char <= 0x7E);

    if (dst != src) {
        // Copy the whole string; overwrite bytes as needed.
        memcpy(dst, src, src_bytes);
    }
    StringPiece str(src, src_bytes);
    while (!str.empty()) {
        int n_valid_bytes = SpanInterchangeValid(str);
        if (n_valid_bytes == str.size()) {
            break;
        }
        dst += n_valid_bytes;
        *dst++ = replace_char;                    // replace one bad byte
        str.remove_prefix(n_valid_bytes + 1);
    }
    return src_bytes;
}

// Coerce source buffer to an interchange-valid UTF-8 string.
string UniLib::CoerceToInterchangeValid(const char* src, int src_bytes,
                                        const char replace_char) {
    string dst_str;
    STLStringResizeUninitialized(&dst_str, src_bytes);
    CoerceToInterchangeValid(src, src_bytes, replace_char, &dst_str[0],
                             src_bytes);
    return dst_str;
}

string UniLib::CoerceToInterchangeValid(StringPiece src,
                                        const char replace_char) {
    return CoerceToInterchangeValid(src.data(), src.size(), replace_char);
}

void UniLib::CoerceToInterchangeValid(string* s, char replace_char) {
    if (s != nullptr && !s->empty()) {
        int length = s->length();
        CoerceToInterchangeValid(&(*s)[0], length, replace_char, &(*s)[0], length);
    }
}

// Returns true if the source is all interchange valid
// 7-bit ASCII
bool UniLib::IsInterchangeValid7BitAscii(const char* src, int byte_length) {
    // This is like an ASCII-specialized call to
    // UTF8GenericScan(utf8acceptinterchange7bit_obj...),
    // but about 21% faster.
    const unsigned char *p = reinterpret_cast<const unsigned char*>(src);
    const unsigned char *srclimit = p + byte_length;
    const unsigned char *srclimit8 = p + (byte_length & ~7);
    const unsigned char *table = utf8acceptinterchange7bit_fast;
    // TODO(gpike): This loop is great on 32-bit architectures, but
    // not as fast as it could be on 64-bit architectures.  In late 2008,
    // I'd like to spend some time optimizing it.  By that time we won't
    // care much about 32-bit performance, so I won't feel bad replacing
    // the loop with a clean and fast-for-64-bit version.
    while (p < srclimit8) {
        uint32 s0123 = UNALIGNED_LOAD32(p + 0);
        uint32 s4567 = UNALIGNED_LOAD32(p + 4);
        uint32 temp = (s0123 - 0x20202020) | (s0123 + 0x01010101) |
                      (s4567 - 0x20202020) | (s4567 + 0x01010101);
        p += 8;
        // True if not all characters in s0123 and s4567 are in [0x20..0x7e]
        if ((temp & 0x80808080) != 0) {
            // Now we check for characters 0x09, 0x0a and 0x0d
            if (table[p[-8]] | table[p[-7]] | table[p[-6]] | table[p[-5]] |
                table[p[-4]] | table[p[-3]] | table[p[-2]] | table[p[-1]] ) {
                return false;
            }
        }
    }
    while (p < srclimit) {
        if (table[*p]) return false;
        ++p;
    }
    return true;
}

bool UniLib::IsInterchangeValid7BitAscii(const StringPiece src) {
    return IsInterchangeValid7BitAscii(src.data(), src.size());
}

// Returns the length in bytes of the prefix of src that is all
//  interchange valid 7-bit ASCII
int UniLib::SpanInterchangeValid7BitAscii(const char* src, int byte_length) {
    // This is like an ASCII-specialized call to
    // UTF8GenericScan(utf8acceptinterchange7bit_obj...),
    // but about 21% faster.
    const unsigned char *p = reinterpret_cast<const unsigned char*>(src);
    const unsigned char *p0 = p;
    const unsigned char *srclimit = p + byte_length;
    const unsigned char *srclimit8 = p + (byte_length & ~7);
    const unsigned char *table = utf8acceptinterchange7bit_fast;
    // TODO(gpike): This loop is great on 32-bit architectures, but
    // not as fast as it could be on 64-bit architectures.  In late 2008,
    // I'd like to spend some time optimizing it.  By that time we won't
    // care much about 32-bit performance, so I won't feel bad replacing
    // the loop with a clean and fast-for-64-bit version.
    while (p < srclimit8) {
        uint32 s0123 = UNALIGNED_LOAD32(p + 0);
        uint32 s4567 = UNALIGNED_LOAD32(p + 4);
        uint32 temp = (s0123 - 0x20202020) | (s0123 + 0x01010101) |
                      (s4567 - 0x20202020) | (s4567 + 0x01010101);
        p += 8;
        // True if not all characters in s0123 and s4567 are in [0x20..0x7e]
        if ((temp & 0x80808080) != 0) {
            // Now we check for characters 0x09, 0x0a and 0x0d
            if (table[p[-8]] | table[p[-7]] | table[p[-6]] | table[p[-5]]) {
                p -= 8;
                break;
            }
            if (table[p[-4]] | table[p[-3]] | table[p[-2]] | table[p[-1]]) {
                p -= 4;
                break;
            }
        }
    }
    while (p < srclimit && !table[*p]) ++p;
    return p - p0;
}

int UniLib::SpanInterchangeValid7BitAscii(const StringPiece src) {
    return SpanInterchangeValid7BitAscii(src.data(), src.size());
}

// ***************** CASE-FOLDING ROUTINES *********************
//
// Each of the case-folding routines comes in two forms.
//
// In the first, the caller provides the input and output buffers and
// gets back information about how many bytes were read from the input
// and written to the output. The caller is responsible for handling
// the case where the output buffer was not big enough.  Note that the
// output buffer must always be at least as big as the input buffer;
// if it isn't, the number of bytes read will be less than the length
// of the input buffer. In this case, the caller should allocate a
// bigger output buffer and try again.
//
// In the second form, the result is a string. In this case, which we
// expect most callers to use, the allocation and re-allocation of the
// output buffer is handled by this routine. The entire input buffer
// will be read unless it contains invalid UTF-8 data.
//
// REALLOCATION NOTE:
//
// All of the "string" versions of these routines have the same logic
// for handling the output buffer. They all allocate an output buffer
// that is slightly bigger than the input buffer, to accommodate the
// occasional code point whose upper (or lower or folded) version
// requires more bytes than the original code point. For example,
// U+023A or U+023E require 2 bytes in UTF-8, but their lowercase
// versions require 3 bytes.
//
// If the exit-code from the underlying utf8statetable routine
// (UTF8ToUpperReplace, UTF8ToLowerReplace, UTF8ToFoldedReplace) is
// kExitDstSpaceFull, then we must have encountered more of these
// "expanding" code points than would fit in the output buffer. In
// this case, if consumed == 0, then they were at the beginning of the
// input buffer, in which case we need to expand the output buffer. If
// consumed > 0, then they were somewhere past the beginning, in which
// case we need to advance the input pointer but may not need to
// expand the output buffer. To keep things simple in this very rare
// case, we do both.
//
// If the exit-code is anything other than kExitDstSpaceFull, then
// either we converted the entire input buffer, or we encountered
// invalid UTF-8 in the input buffer. In either case, we're done.
//
// **************************************************************


// Case-fold source buffer and write to destination buffer
// This does full Unicode.org case-folding to put all strings that differ only
// by case into a common form. The output can be longer than the input.
// Folding maps U+00DF Latin small letter sharp s (es-zed) to "ss", while
// lower-casing leaves it unchanged. Folding allows "MASSE" "masse" and
// "Maße" [third character is es-zed] to match; lowercasing does not.
// //
// Caller allocates the destination buffer
// Set 'consumed' to the number of bytes read from the source buffer.
// Set 'filled' to the number of bytes written to the destination buffer.
// Optimized to do seven-bit ASCII quickly.
//
// Output can be 3x input, worst case
void UniLib::ToFolded(const char* src, int src_bytes,
                      char* dst, int dst_bytes,
                      int* consumed, int* filled) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        FixedArray<char> clean(src_bytes);
        CoerceToStructurallyValid(src, src_bytes, ' ', clean.get(), src_bytes);
        ToFolded(clean.get(), src_bytes, dst, dst_bytes, consumed, filled);
        return;
    }
    int changed;
    UTF8ToFoldedReplace(in, dst, dst_bytes, consumed, filled, &changed);
}

// Convert source buffer to a case-folded string.
// Optimized to do seven-bit ASCII ToLower quickly
//
// Output can be 3x input, worst case
// (See CASE-FOLDING ROUTINES comment above.)
string UniLib::ToFolded(const char* src, int src_bytes) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        string s(CoerceToStructurallyValid(src, src_bytes, ' '));
        return ToFolded(s.data(), s.size());
    }
    string result;
    int dst_bytes = src_bytes + (src_bytes >> 4) + 8;
    while (true) {
        FixedArray<char> buf(dst_bytes);
        int consumed, filled, changed;
        int exit_code = UTF8ToFoldedReplace(in, buf.get(), dst_bytes, &consumed, &filled, &changed);
        result.append(buf.get(), filled);
        if (exit_code == kExitDstSpaceFull) {
            in.remove_prefix(consumed);
            dst_bytes += dst_bytes >> 1;
        } else {
            break;
        }
    }
    return result;
}

string UniLib::ToFolded(const string& src) {
    return UniLib::ToFolded(src.data(), src.size());
}

// Convert source buffer to lowercase and write to destination buffer
// Caller allocates the destination buffer
// Return value is the number bytes filled in the destination buffer.
// Optimized to do seven-bit ASCII ToLower quickly
//
// Output can be 1.5x input, worst case
void UniLib::ToLower(const char* src, int src_bytes,
                     char* dst, int dst_bytes,
                     int* consumed, int* filled,
                     OffsetMap* offsetmap) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        FixedArray<char> clean(src_bytes);
        CoerceToStructurallyValid(src, src_bytes, ' ', clean.get(), src_bytes);
        ToLower(clean.get(), src_bytes, dst, dst_bytes, consumed, filled, offsetmap);
        return;
    }
    int changed;
    UTF8ToLowerReplace(in, dst, dst_bytes,
                       consumed, filled, &changed,
                       true,    // *DO NOT* map fullwidth to halfwidth
                       false,   // *DO NOT* normalize Japanese
                       offsetmap);
}

// Convert source buffer to lowercase and write to destination string
// Optimized to do seven-bit ASCII ToLower quickly
//
// Output can be 1.5x input, worst case
// (See CASE-FOLDING ROUTINES comment above.)
string UniLib::ToLower(const char* src, int src_bytes) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        string s(CoerceToStructurallyValid(src, src_bytes, ' '));
        return ToLower(s.data(), s.size());
    }
    string result;
    int dst_bytes = src_bytes + 32;
    while (true) {
        FixedArray<char> buf(dst_bytes);
        int consumed, filled, changed;
        int exit_code =
                UTF8ToLowerReplace(in, buf.get(), dst_bytes,
                                   &consumed, &filled, &changed,
                                   true,    // *DO NOT* map fullwidth to halfwidth
                                   false);  // *DO NOT* normalize Japanese
        result.append(buf.get(), filled);
        if (exit_code == kExitDstSpaceFull) {
            in.remove_prefix(consumed);
            dst_bytes += dst_bytes >> 1;
        } else {
            break;
        }
    }
    return result;
}

string UniLib::ToLower(const string& src) {
    return UniLib::ToLower(src.data(), src.size());
}

// HACKS for backwards compatibility with FLAGS_ja_normalize behavior
// Output can be 1.5x input, worst case
void UniLib::ToLowerHack(const char* src, int src_bytes,
                         bool ja_normalize,
                         char* dst, int dst_bytes,
                         int* consumed, int* filled) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        FixedArray<char> clean(src_bytes);
        CoerceToStructurallyValid(src, src_bytes, ' ', clean.get(), src_bytes);
        ToLowerHack(clean.get(), src_bytes, ja_normalize, dst, dst_bytes, consumed,
                    filled);
        return;
    }
    int changed;
    UTF8ToLowerReplace(in, dst, dst_bytes,
                       consumed, filled, &changed,
                       false,    // *DO* map fullwidth to halfwidth
                       ja_normalize);
}

// Output can be 1.5x input, worst case
// (See CASE-FOLDING ROUTINES comment above.)
string UniLib::ToLowerHack(const char* src, int src_bytes,
                           bool ja_normalize) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        string s(CoerceToStructurallyValid(src, src_bytes, ' '));
        return ToLowerHack(s.data(), s.size(), ja_normalize);
    }
    string result;
    int dst_bytes = src_bytes + 8;
    while (true) {
        FixedArray<char> buf(dst_bytes);
        int consumed, filled, changed;
        int exit_code =
                UTF8ToLowerReplace(in, buf.get(), dst_bytes,
                                   &consumed, &filled, &changed,
                                   false,          // *DO* map fullwidth to halfwidth
                                   ja_normalize);  // *MAYBE* normalize Japanese
        result.append(buf.get(), filled);
        if (exit_code == kExitDstSpaceFull) {
            in.remove_prefix(consumed);
            dst_bytes += dst_bytes >> 1;
        } else {
            break;
        }
    }
    return result;
}

// Convert source buffer to uppercase and write to destination buffer
// Caller allocates the destination buffer
// Return value is the number bytes filled in the destination buffer.
//
// Output can be 1.5x input, worst case
void UniLib::ToUpper(const char* src, int src_bytes,
                     char* dst, int dst_bytes,
                     int* consumed, int* filled) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        FixedArray<char> clean(src_bytes);
        CoerceToStructurallyValid(src, src_bytes, ' ', clean.get(), src_bytes);
        ToUpper(clean.get(), src_bytes, dst, dst_bytes, consumed, filled);
        return;
    }
    int changed;
    UTF8ToUpperReplace(in, dst, dst_bytes, consumed, filled, &changed);
}

// Convert source buffer to uppercase and write to destination string
//
// Output can be 1.5x input, worst case
// (See CASE-FOLDING ROUTINES comment above.)
string UniLib::ToUpper(const char* src, int src_bytes) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        string s = CoerceToStructurallyValid(src, src_bytes, ' ');
        return ToUpper(s.data(), s.size());
    }
    string result;
    int dst_bytes = src_bytes + 8;
    while (true) {
        FixedArray<char> buf(dst_bytes);
        int consumed, filled, changed;
        int exit_code = UTF8ToUpperReplace(in, buf.get(), dst_bytes, &consumed,
                                           &filled, &changed);
        result.append(buf.get(), filled);
        if (exit_code == kExitDstSpaceFull) {
            in.remove_prefix(consumed);
            dst_bytes += dst_bytes >> 1;
        } else {
            break;
        }
    }
    return result;
}

string UniLib::ToUpper(const string& src) {
    return UniLib::ToUpper(src.data(), src.size());
}

string UniLib::ToTitle(const char* src, int src_bytes) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        string s = CoerceToStructurallyValid(src, src_bytes, ' ');
        return ToTitle(s.data(), s.size());
    }
    string result;
    int dst_bytes = src_bytes + 8;
    while (true) {
        FixedArray<char> buf(dst_bytes);
        int consumed, filled, changed;
        int exit_code = UTF8ToTitleReplace(in, buf.get(), dst_bytes, &consumed,
                                           &filled, &changed);
        result.append(buf.get(), filled);
        if (exit_code == kExitDstSpaceFull) {
            in.remove_prefix(consumed);
            dst_bytes += dst_bytes >> 1;
        } else {
            break;
        }
    }
    return result;
}


// Return the byte length <= src_bytes of
// the longest prefix of src that is complete UTF-8 characters.
// This is useful especially when a UTF-8 string must be put into a fixed-
// maximum-size buffer cleanly, such as a MySQL buffer.
int UniLib::CompleteCharLength(const char* src, int src_bytes) {
    // Exit if empty string
    if (src_bytes <= 0) {
        return 0;
    }

    // Exit on simple, common case
    if (static_cast<signed char>(src[src_bytes - 1]) >= 0) {
        // last byte is 7-bit ASCII -- done
        return src_bytes;
    }

    // Adjust the back end, src_bytes > 0
    // Backscan over any ending continuation bytes to find last char start
    int k = src_bytes - 1;         // Last byte of the string
    while ((0 <= k) && ((src[k] & 0xc0) == 0x80)) {
        k--;
    }
    // Include entire last char if it fits
    if (0 <= k) {
        int last_char_len = UniLib::OneCharLen(&src[k]);
        if (k + last_char_len <= src_bytes) {
            // Last char fits, so include it, else exclude it
            k += last_char_len;
        }
        return k;
    }
    return 0;   // Input buffer was all continuation characters
}

static void ApplyWideCompatibility(const char* src, int src_bytes,
                                   char* dst, int dst_bytes,
                                   bool is_plain_text,
                                   int* consumed, int* filled,
                                   const UTF8ScanObj* const scan_not,
                                   const UTF8ReplaceObj* const replace) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        FixedArray<char> clean(src_bytes);
        UniLib::CoerceToStructurallyValid(src, src_bytes, ' ',
                                          clean.get(), src_bytes);
        ApplyWideCompatibility(clean.get(), src_bytes,
                               dst, dst_bytes,
                               is_plain_text,
                               consumed, filled,
                               scan_not, replace);
        return;
    }

    *filled = *consumed = 0;
    if (dst_bytes < src_bytes) {  // Don't bother with partial conversions
        return;
    }

    int not_fw;
    // Scan through the string to see how much is NOT full-width.
    UTF8GenericScan(scan_not, in, &not_fw);
    // Copy those bytes (if necessary).
    if (src != dst && not_fw > 0) {
        memcpy(dst, src, not_fw);
    }
    *filled = *consumed = not_fw;
    if (not_fw == src_bytes) {
        // The entire string is NOT full-width, i.e., it contains NO
        // full-width characters. So there's nothing to convert.
        return;
    }

    StringPiece out(dst, dst_bytes);
    // Advance the pointers.
    out.remove_prefix(not_fw);
    in.remove_prefix(not_fw);

    // Convert the remaining input.
    int consumed_repl, filled_repl, changed_repl;
    UTF8GenericReplace(replace, in, out, is_plain_text,
                       &consumed_repl, &filled_repl, &changed_repl);
    *consumed += consumed_repl;
    *filled += filled_repl;
}



void UniLib::FullwidthToHalfwidth(const char* src, int src_bytes,
                                  char* dst, int dst_bytes,
                                  bool is_plain_text,
                                  int* consumed, int* filled) {
    ApplyWideCompatibility(src, src_bytes,
                           dst, dst_bytes,
                           is_plain_text,
                           consumed, filled,
                           &utf8scannotfw_obj, &utf8cvtfw_obj);
}


string UniLib::FullwidthToHalfwidth(const char* src,
                                    int src_bytes,
                                    bool is_plain_text) {
    int dst_bytes = src_bytes;
    string dst_str;
    // Assume the output will not be longer than the input
    dst_str.resize(dst_bytes);
    char* dst = &dst_str[0];
    int consumed, filled;
    ApplyWideCompatibility(src, src_bytes,
                           dst, dst_bytes,
                           is_plain_text,
                           &consumed, &filled,
                           &utf8scannotfw_obj, &utf8cvtfw_obj);
    dst_str.resize(filled);
    int overflow_length = std::max(32, 2 * src_bytes);
    while (consumed < src_bytes) {
        FixedArray<char> overflow(overflow_length);
        int new_consumed;
        ApplyWideCompatibility(src + consumed, src_bytes - consumed,
                               overflow.get(), overflow_length,
                               is_plain_text,
                               &new_consumed, &filled,
                               &utf8scannotfw_obj, &utf8cvtfw_obj);
        dst_str.append(overflow.get(), filled);
        consumed += new_consumed;
        overflow_length <<= 1;  // If we need more, double the buffer length.
    }

    return dst_str;
}

void UniLib::FullwidthAlphanumToHalfwidth(const char* src, int src_bytes,
                                          char* dst, int dst_bytes,
                                          int* consumed, int* filled) {
    // We're not converting any fullwidth HTML syntax characters here.
    const bool is_plain_text = true;

    ApplyWideCompatibility(src, src_bytes,
                           dst, dst_bytes,
                           is_plain_text,
                           consumed, filled,
                           &utf8scannotfw_an_obj, &utf8cvtfw_an_obj);
}

string UniLib::FullwidthAlphanumToHalfwidth(const char* src, int src_bytes) {
    int dst_bytes = src_bytes;
    string dst_str;
    dst_str.resize(dst_bytes);
    char* dst = &dst_str[0];
    int consumed, filled;
    // We're not converting any fullwidth HTML syntax characters here.
    const bool is_plain_text = true;

    ApplyWideCompatibility(src, src_bytes,
                           dst, dst_bytes,
                           is_plain_text,
                           &consumed, &filled,
                           &utf8scannotfw_an_obj, &utf8cvtfw_an_obj);
    dst_str.resize(filled);
    return dst_str;
}

void UniLib::FullwidthKatakanaToHalfwidth(const char* src, int src_bytes,
                                          char* dst, int dst_bytes,
                                          int* consumed, int* filled) {
    // We're not converting any fullwidth HTML syntax characters here.
    const bool is_plain_text = true;

    ApplyWideCompatibility(src, src_bytes,
                           dst, dst_bytes,
                           is_plain_text,
                           consumed, filled,
                           &utf8scannothwk_obj, &utf8cvthwk_obj);
}

string UniLib::FullwidthKatakanaToHalfwidth(const char* src, int src_bytes) {
    int dst_bytes = src_bytes;
    string dst_str;
    dst_str.resize(dst_bytes);
    char* dst = &dst_str[0];
    int consumed, filled;
    // We're not converting any fullwidth HTML syntax characters here.
    const bool is_plain_text = true;

    ApplyWideCompatibility(src, src_bytes,
                           dst, dst_bytes,
                           is_plain_text,
                           &consumed, &filled,
                           &utf8scannothwk_obj, &utf8cvthwk_obj);
    dst_str.resize(filled);
    while (consumed < src_bytes) {
        // We need more space.
        // Output can be as much as twice as big as the input,
        // so we should never have to do this more than once, but
        // play it safe.
        src += consumed;
        src_bytes -= consumed;
        StringPiece input(src, src_bytes);
        dst_bytes = src_bytes * 2;
        FixedArray<char> outbuf(dst_bytes);
        StringPiece output(outbuf.get(), dst_bytes);
        int changed;
        UTF8GenericReplace(&utf8cvthwk_obj, input, output,
                           &consumed, &filled, &changed);
        dst_str.append(outbuf.get(), filled);
    }
    return dst_str;
}

bool UniLib::UTF8CharEqual(const char* a, const char* b) {
    return strncmp(a, b, UniLib::OneCharLen(a)) == 0;
    // You may wonder why it is not necessary to first compare the
    // lengths of a and b to make sure they are equal.  The expression
    // above exploits an important property of UTF-8: UTF-8 was designed
    // so that no byte sequence of one character is contained within a
    // longer byte sequence of another character.  So let's consider the
    // possibilities:
    //
    // UniLib::OneCharLen(a) == UniLib::OneCharLen(b)
    //   ==> In this case the strncmp call above is clearly correct.
    // UniLib::OneCharLen(a) < UniLib::OneCharLen(b)
    //   ==> If strncmp returns 0, then b must start with a.  But because of
    //       the UTF-8 property above, b must therefore be identical to a.
    // UniLib::OneCharLen(a) > UniLib::OneCharLen(b)
    //   ==> This is the trickiest case.  Naively one might expect that a
    //       segmentation fault is possible; but remember that strncmp
    //       stops reading memory as soon as the first differing byte is found.
    //       If no differing byte is found in the first UniLib::OneCharLen(b)
    //       then b must be a prefix of a.  But because of the UTF-8
    //       property above, b must therefore be identical to a.
}

namespace{

    static const char hex_digits[] = "0123456789ABCDEF";

    bool FindEscapeChar(Rune rune, char* esc) {
        static const struct {Rune r; char c;} escape_pair [] = {
                {0xA, 'n'},
                {0xD, 'r'},
                {0x9, 't'},
                {0x22, '\"'},
                {0x27, '\''},
                {0x5C, '\\'}};
        for (int i = 0; i < ARRAYSIZE(escape_pair); ++i) {
            if (escape_pair[i].r == rune) {
                *esc = escape_pair[i].c;
                return true;
            }
        }
        return false;
    }

}  // namespace

void UniLib::UTF8EscapeString(const char* src, int src_bytes,
                              char* dst, int dst_bytes,
                              int* consumed, int* filled) {
    StringPiece in(src, src_bytes);
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        FixedArray<char> clean(src_bytes);
        CoerceToStructurallyValid(src, src_bytes, ' ', clean.get(), src_bytes);
        UTF8EscapeString(clean.get(), src_bytes, dst, dst_bytes, consumed, filled);
        return;
    }
    const char* src_start = src;
    const char* dst_start = dst;
    const char* src_end = src_start + src_bytes;
    const char* dst_end = dst_start + dst_bytes;
    while (src < src_end && dst < dst_end) {
        char32 rune;
        src += chartorune(&rune, src);
        char esc;
        if (FindEscapeChar(rune, &esc)) {
            if (dst_end - dst < 2)
                break;
            *dst++ = '\\';
            *dst++ = esc;
        } else if (0x20 <= rune && rune <= 0x7E) {  // printable ASCII
            *dst++ = rune;
        } else if (rune <= 0xFFFF) {
            if (dst_end - dst < 2 + 4) // \uhhhh
                break;
            *dst++ = '\\';
            *dst++ = 'u';
            for (int shift = 16 - 4; shift >= 0; shift -= 4 ) {
                *dst++ = hex_digits[(rune >> shift) & 0xF];
            }
        } else {
            if (dst_end - dst < 2 + 8)  // \Uhhhhhhhh
                break;
            *dst++ = '\\';
            *dst++ = 'U';
            for (int shift = 32 - 4; shift >= 0; shift -= 4 ) {
                *dst++ = hex_digits[(rune >> shift) & 0xF];
            }
        }
    }
    *consumed = src - src_start;
    *filled = dst - dst_start;
}

string UniLib::UTF8EscapeString(StringPiece in) {
    const char* src = in.data();
    int src_bytes = in.size();
    if (!UTF8IsStructurallyValid(in)) {
        LOG(DFATAL) << "Invalid UTF-8: " << strings::CHexEscape(in);
        FixedArray<char> clean(src_bytes);
        CoerceToStructurallyValid(src, src_bytes, ' ', clean.get(), src_bytes);
        return UTF8EscapeString(clean.get(), src_bytes);
    }
    const char* src_end = src + src_bytes;
    string out;
    out.reserve(src_bytes);
    while (src < src_end) {
        char32 rune;
        src += chartorune(&rune, src);
        char esc;
        if (FindEscapeChar(rune, &esc)) {
            out.push_back('\\');
            out.push_back(esc);
        } else if (0x20 <= rune && rune <= 0x7E) {  // printable ASCII
            out.push_back(static_cast<char>(rune));
        } else if (rune <= 0xFFFF) {
            out.push_back('\\');
            out.push_back('u');
            for (int shift = 16 - 4; shift >= 0; shift -= 4 ) {
                out.push_back(hex_digits[(rune >> shift) & 0xF]);
            }
        } else {
            out.push_back('\\');
            out.push_back('U');
            for (int shift = 32 - 4; shift >= 0; shift -= 4 ) {
                out.push_back(hex_digits[(rune >> shift) & 0xF]);
            }
        }
    }
    return out;
}

string UniLib::UTF8EscapeString(const char* src, int src_bytes) {
    return UTF8EscapeString(StringPiece(src, src_bytes));
}