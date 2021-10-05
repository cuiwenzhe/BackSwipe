// Copyright 2006 Google Inc. All rights reserved
//
// Routines to do Google manipulation of Unicode characters or text
//
// The StructurallyValid routines accept buffers of arbitrary bytes.
// For CoerceToStructurallyValid(), the input buffer and output buffers may
// point to exactly the same memory.
//
// In all other cases, the UTF-8 string must be structurally valid and
// have all codepoints in the range  U+0000 to U+D7FF or U+E000 to U+10FFFF.
// Debug builds take a fatal error for invalid UTF-8 input.
// The input and output buffers may not overlap at all.
//
// Each property is described by a text file giving the complete set
// of Unicode characters that have that property. These text files are
// derived automatically (via scripts) from the Unicode Consortium's
// machine-readable tables, with Google-specific edits. The text files
// are in turn used to build UTF-8 state tables that are used by the
// implementation. The state tables are indexed by individual bytes of
// UTF-8, so lookups are fast and do not involve conversions.
//
// The char32 routines are here only for convenience; they convert to UTF-8
// internally and use the UTF-8 routines.
//
//
// The exact text-file definitions of the sets of characters, along
// with scripts to extract them from Unicode.org files and turn them
// into state tables are all in: util/utf8/unilib
//
// See the design document in
// https://g3doc.corp.google.com/util/utf8/public/g3doc/utf8_state_tables.md
// For more information on UTF-8 validity please see
// https://g3doc.corp.google.com/util/utf8/public/g3doc/valid_utf8.md

#ifndef UTIL_UTF8_PUBLIC_UNILIB_H_
#define UTIL_UTF8_PUBLIC_UNILIB_H_

#include <string>                      // for string

#include "integral_types.h"        // for char32, uint8, uint32
#include "macros.h"                // for GOOGLE_DEPRECATED
#include "port.h"
#include "stringpiece.h"        // for StringPiece

// We export OneCharLen, IsValidCodepoint, and IsTrailByte from here,
// but they are defined in unilib_utf8_utils.h.
//#include "strings/cord.h"
#include "unilib_utf8_utils.h"  // IWYU pragma: export

class OffsetMap;

namespace UniLib {

    // Returns true if the source is all valid ISO-8859-1 bytes
    // with no C0 or C1 control codes (other than CR LF HT)
    //
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
    bool IsValidLatin1(const char* src, int byte_length);
    bool IsValidLatin1(const string& src);


    // Returns true if the source is all structurally valid UTF-8 and
    // includes no surrogate codepoints.
    bool IsStructurallyValid(const char* src, int byte_length);
    bool IsStructurallyValid(StringPiece src);
    // TODO: Import Cord
    //    bool CordIsStructurallyValid(const Cord& cord);

    // Returns true if 'str' is the start of a structurally valid UTF-8
    // sequence and is not a surrogate codepoint. Returns false if str.empty()
    // or if str.length() < UniLib::OneCharLen(str[0]). Otherwise, this function
    // will access 1-4 bytes of src, where n is UniLib::OneCharLen(src[0]).
    bool IsUTF8ValidCodepoint(StringPiece str);

    // Returns true if 'src' is the start of a structurally valid UTF-8
    // sequence and is not a surrogate codepoint. Returns false if src == nullptr.
    // Otherwise, this function will access 1-4 bytes of src, where n is
    // UniLib::OneCharLen(src[0]). Note that src is not treated as a
    // null-terminated string.
    GOOGLE_DEPRECATED("Use IsUTF8ValidCodepoint(StringPiece(str, ...))")
    inline bool IsUTF8ValidCodepoint(const char* src) {
        return src != nullptr
               && IsUTF8ValidCodepoint(StringPiece(src, OneCharLen(src)));
    }
    // See also IsValidCodepoint(char32 c) in util/utf8/public/unilib_utf8_utils.h.

    // Returns the length in bytes of the prefix of src that is all
    // structurally valid UTF-8
    int SpanStructurallyValid(const char* src, int byte_length);
    int SpanStructurallyValid(StringPiece src);

    // Coerces a source buffer to be structurally valid UTF-8 by copying
    // it to a destination buffer, and replacing illegal bytes in the
    // destination with replace_char (typically ' ' or '?').  replace_char
    // must be printable 7-bit Ascii in the range 0x20..0x7e inclusive.
    // Caller owns the destination buffer, dst_bytes must be >= src_bytes.
    // Return value is the number bytes filled in the destination buffer
    // (and is always equal to src_bytes).
    //
    // src and dst pointers may be equal, for doing the operation in
    // place, but they must not otherwise overlap.
    //
    // In the second form, a string is returned that is a copy of the src
    // in which the bytes have been coerced to be structurally valid.  In
    // the third form, the string has been coerced in place.
    int CoerceToStructurallyValid(const char* src, int src_bytes,
                                  const char replace_char,
                                  char* dst, int dst_bytes);
    string CoerceToStructurallyValid(const char* src, int src_bytes,
                                     const char replace_char);
    void CoerceToStructurallyValid(string* s, char replace_char);


    // Returns true if the source is structurally valid and contains only
    // interchange-valid characters, which are the characters that are
    // valid in XML and HTML. This means no C0 or C1 control codes (other
    // than CR LF HT FF) and no non-characters.
    bool IsInterchangeValid(const char* src, int byte_length);
    bool IsInterchangeValid(StringPiece src);
    bool IsInterchangeValid(char32 codepoint);

    // Returns the length in bytes of the prefix of src that is all
    // interchange valid UTF-8
    int SpanInterchangeValid(const char* src, int byte_length);
    int SpanInterchangeValid(StringPiece src);

    // Coerce source buffer to interchange valid UTF-8 in destination buffer.
    // See block of CoerceToStructurallyValid comments above
    int CoerceToInterchangeValid(const char* src, int src_bytes,
                                 const char replace_char,
                                 char* dst, int dst_bytes);
    string CoerceToInterchangeValid(const char* src, int src_bytes,
                                    const char replace_char);
    string CoerceToInterchangeValid(StringPiece src, const char replace_char);
    void CoerceToInterchangeValid(string* s, char replace_char);


    // Returns true if the source is all interchange valid 7-bit ASCII
    bool IsInterchangeValid7BitAscii(const char* src, int byte_length);
    bool IsInterchangeValid7BitAscii(StringPiece src);

    // Returns the length in bytes of the prefix of src that is all
    //  interchange valid 7-bit ASCII
    int SpanInterchangeValid7BitAscii(const char* src, int byte_length);
    int SpanInterchangeValid7BitAscii(StringPiece src);


    // Case-fold source buffer and write to destination buffer
    // This does full Unicode.org case-folding to put all strings that differ only
    // by case into a common form. The output can be longer than the input.
    // Folding maps U+00DF Latin small letter sharp s (es-zed) to "ss", while
    // lower-casing leaves it unchanged. Folding allows "MASSE" "masse" and
    // "Maße" [third character is es-zed] to match; lowercasing does not.
    //
    // Caller allocates the destination buffer.
    // The output may contain up to 3 times as many bytes as the input.
    // Note that the output may also contain more *characters* than the
    // input, even when it contains fewer *bytes* than the input.
    // Set 'consumed' to the number of bytes read from the source buffer.
    // Set 'filled' to the number of bytes written to the destination buffer.
    // Optimized to do seven-bit ASCII quickly.
    //
    // NOTE ON OUTPUT SIZE
    // The conversion routines assume that each byte of input will
    // generate at least one byte of output, and they do not promise to
    // make "partial progress," so dst_bytes must be at least as large as
    // src_bytes. If *consumed < src_bytes after the function returns, the
    // caller should handle the bytes that were written to the output
    // buffer (if any), allocate a larger output buffer, and call the
    // function again to convert the remaining input.
    //
    void ToFolded(const char* src, int src_bytes,
                  char* dst, int dst_bytes,
                  int* consumed, int* filled);

    // Case-fold source buffer and write to returned string.
    // Optimized to do seven-bit ASCII ToLower quickly
    string ToFolded(const char* src, int src_bytes);
    string ToFolded(const string& src);

    // *** DEPRECATION WARNING ***
    //
    // UniLib's ToLower and ToUpper routines are deprecated and broken by design.
    // Case mapping is generally language-sensitive. The text's language is not
    // even considered here.
    //  - For proper case conversion, please see
    //    //depot/google3/i18n/utf8/case_converter.h.
    //  - Please note that case conversion is a process that discards information
    //    and should generally only be used for display purposes. For processing
    //    purposes, Unicode normalized string matching generally solves the same
    //    problems in a better way than case
    //    conversion. See //depot/google3/util/utf8/public/normalize_utf8.h.

    // Convert source buffer to lowercase and write to destination buffer
    // Caller allocates the destination buffer
    // Output can be 1.5x input, worst case
    // Set 'consumed' to the number of bytes read from the source buffer.
    // Set 'filled' to the number of bytes written to the destination buffer.
    // Optimized to do seven-bit ASCII ToLower quickly.
    // (See NOTE ON OUTPUT SIZE above.)
    // If offset_map is not NULL, it is populated with a mapping from the input
    // buffer to the output buffer in bytes.
    //
    /* Deprecated. See WARNING above. */
    void ToLower(const char* src, int src_bytes,
                 char* dst, int dst_bytes,
                 int* consumed, int* filled,
                 OffsetMap* offset_Map = nullptr);

    // Convert source buffer to a lowercase string.
    // Optimized to do seven-bit ASCII ToLower quickly
    /* Deprecated. See WARNING above. */
    string ToLower(const char* src, int src_bytes);
    string ToLower(const string& src);


    // HACKS for backwards compatibility with FLAGS_ja_normalize behavior
    // Normally, the caller should pass FLAGS_ja_normalize as the third argument,
    // to mimic the behavior of our old i18n/utf8/letter.cc FastUTF8CharToLower()
    // If the flag is false,
    //    (1) change fullwidth 0-9 A-Z a-z to ASCII
    //        (but not other fullwidth ASCII)
    // If it is true, do that plus
    //    (2) change halfwidth Katakana to fullwidth
    //        (but not all halfwidth Katakana)
    //    (3) halfwidth-letter, voiced mark sequence => normal combined Katakana
    //        halfwidth-letter, semi-voiced mark sequence => normal combined
    //        Katakana
    //
    // (See NOTE ON OUTPUT SIZE above.)
    //
    /* Deprecated. See WARNING above. */
    void ToLowerHack(const char* src, int src_bytes,
                     bool ja_normalize,
                     char* dst, int dst_bytes,
                     int* consumed, int* filled);

    string ToLowerHack(const char* src, int src_bytes,
                       bool ja_normalize);

    // Convert source buffer to uppercase and write to destination buffer
    // Caller allocates the destination buffer
    // Set 'consumed' to the number of bytes read from the source buffer.
    // Set 'filled' to the number of bytes written to the destination buffer.
    // (See NOTE ON OUTPUT SIZE above.)
    /* Deprecated. See WARNING above. */
    void ToUpper(const char* src, int src_bytes,
                 char* dst, int dst_bytes,
                 int* consumed, int* filled);

    // Convert source buffer to an uppercase string.
    /* Deprecated. See WARNING above. */
    string ToUpper(const char* src, int src_bytes);
    string ToUpper(const string& src);

    // Convert the source buffer to title case. Note that THIS CONVERTS
    // EVERY CHARACTER TO TITLECASE. You probably only want to call it
    // with a single character, at the front of a "word", then convert the
    // rest of the word to lowercase. You have to figure out where the
    // words begin and end, and what to do with e.g. l'Hospital
    /* Deprecated. See WARNING above. */
    string ToTitle(const char* src, int src_bytes);
    inline string ToTitle(StringPiece s) {
        return ToTitle(s.data(), s.size());
    }

    // Return the byte length <= src_bytes of
    // the longest prefix of src that is complete UTF-8 characters.
    // This is useful especially when a UTF-8 string must be put into a fixed-
    // maximum-size buffer cleanly, such as a MySQL buffer.
    int CompleteCharLength(const char* src, int src_bytes);

    // Convert a full-width UTF-8 string to half-width.
    // Specifically, apply the wide (or zenkaku) compatibility mapping, as
    // defined in the Unicode Character Database.
    //
    // Examples:
    // U+FF21 -> U+0041 (Latin capital letter A)
    // U+FF05 -> U+0025 (percent sign)
    //
    // NOTE on is_plain_text:
    //
    // There are full-width versions of the five ASCII "syntax" characters
    // in HTML: <, >, ', ", and &, namely, U+FF1C, U+FF1E, U+FF07, U+FF02,
    // and U+F06. If the conversion from full-width to half-width is being
    // performed on an HTML string (i.e., a string that has been read from
    // an HTML file or that will be written to an HTML file), then it is
    // important NOT to generate the ASCII ("half-width") version of those
    // characters, since that could affect the subsequent parsing of the
    // output string. Therefore the conversion functions take a boolean
    // parameter, is_plain_text, which should be false for HTML strings
    // and true for non-HTML ("plain") strings. If is_plain_text is false,
    // then the full-width versions of the syntax characters will be
    // converted to the corresponding HTML entities: &lt;, &gt;, &#x27;
    // [which is a more portable version of &apos;], &quot;, and &amp;.
    //
    // NOTE on input and output length:
    //
    // If is_plain_text is false and the input string contains any of the
    // five HTML syntax characters, then the output may be longer than the
    // input.  For example, the full-width quotation mark, U+FF02, will be
    // represented by three bytes of UTF-8, "\xEF\xBD\x82", but its
    // halfwidth counterpart will be the six-byte string
    // "&quot;". Therefore it is not safe to assume that this operation
    // can be done in place.
    //
    // As with all such routines, callers should compare 'consumed' with
    // 'src_bytes' to test whether the output buffer was large enough to
    // hold the conversion of the entire input buffer. It is guaranteed
    // that only complete UTF-8 byte-sequences will be consumed, so it is
    // safe to resume converting at 'src + consumed'.
    //
    // NOTE on output values:
    //
    // Not all the output characters will be 7-bit ASCII.
    // Example:
    // U+FF36 -> U+20A9 (won sign)
    //
    void FullwidthToHalfwidth(const char* src, int src_bytes,
                              char* dst, int dst_bytes,
                              bool is_plain_text,
                              int* consumed, int* filled);

    // Convert full-width source buffer to a half-width string.
    string FullwidthToHalfwidth(const char* src, int src_bytes, bool is_plain_text);

    // As above but takes a StringPiece.
    inline string FullwidthToHalfwidth(StringPiece src, bool is_plain_text) {
        return FullwidthToHalfwidth(src.data(), src.size(), is_plain_text);
    }

    // Convert full-width letters (A-Z and a-z) and digits (0-9) to
    // their half-width (7-bit ASCII) equivalents. This is a subset of
    // the complete mapping provided by FullwidthToHalfwidth.
    //
    // Note: The output will not be longer than the input, so dst may be
    // the same as src.
    void FullwidthAlphanumToHalfwidth(const char* src, int src_bytes,
                                      char* dst, int dst_bytes,
                                      int* consumed, int* filled);

    // Convert full-width letters and digits in the source buffer to a
    // half-width string.
    string FullwidthAlphanumToHalfwidth(const char* src, int src_bytes);

    // Convert full-width Katakana characters (U+30A0...U+30FF, with some
    // exceptions) to half-width. In general, we apply the canonical
    // decomposition mapping, if any, and then the inverse of the hankaku
    // ("<narrow>") mapping. For the complete list, see
    // util/utf8/rawprops/google_halfwidth_katakana.txt.
    //
    // NOTE: The output may be bigger than the input, so dst must not be
    // the same as src.
    void FullwidthKatakanaToHalfwidth(const char* src, int src_bytes,
                                      char* dst, int dst_bytes,
                                      int* consumed, int* filled);

    // Convert full-width Katakana characters to a half-width string.
    string FullwidthKatakanaToHalfwidth(const char* src, int src_bytes);

    // Given two UTF8 chars, check if they are equal. We assume both are
    // well-formed UTF8 chars.
    bool UTF8CharEqual(const char* a, const char* b);

    // Copy 'src' to 'dest'. 'src' must be a valid UTF-8 string. (Debug
    // builds take a fatal error for invalid UTF-8 input; optimized builds
    // use CoerceToStructurallyValid on a copy of 'src'.)
    //
    // The characters \n, \r, \t, \", \', and \\ are copied as 2-byte
    // escape sequences, as in C.
    //
    // Printable characters (in the range from 0x20 to 0x7E) are copied as is.
    //
    // All other characters are decoded from UTF-8 and written to dest as
    // either \u followed by four hex digits (for characters that require
    // at most 16 bits) or \U followed by eight hex digits (for everything
    // else). For example, if 'src' contains the three-byte sequence 0xE2,
    // 0x80, and 0x99, then 'dest' will contain the six ASCII characters
    // \u2019, representing the Unicode code point U+2019.
    //
    // See UnescapeCEscapeString in strings/escaping.h for a function that
    // reverses this transformation.
    //
    // In the first version of this function, only complete UTF-8
    // characters will be consumed from src, and only as many as whose
    // escaped versions will fit in dest. Since the output may require
    // more bytes than the input, src and dest must not be the same
    // pointer.
    //
    // In the second version, src is entirely consumed.
    // ----------------------------------------------------------------------
    void UTF8EscapeString(const char* src, int src_len,
                          char* dest, int dest_len,
                          int* consumed, int* filled);
    string UTF8EscapeString(const char* src, int src_len);
    string UTF8EscapeString(StringPiece str);


}     // namespace UniLib

#endif  // UTIL_UTF8_PUBLIC_UNILIB_H_