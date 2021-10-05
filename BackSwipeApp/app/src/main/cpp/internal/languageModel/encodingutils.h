// Copyright 2001 Google Inc. All Rights Reserved.
// Author: Jenny Zhou
//
// This file contains the utilities related to
// encodings. Specifically, we can test whether a string is in some
// common encodings.
//

#ifndef I18N_UTF8_ENCODINGUTILS_H__
#define I18N_UTF8_ENCODINGUTILS_H__

#include <string>                       // for string
#include <vector>                       // for vector

#include "../base/integral_types.h"        // for char32, uint16
#include "../base/unilib.h"    // for IsValidCodepoint
#include "../base/utf.h"        // for ::UTFmax

static const int kMaxUTF8CharBytes = 4;
// Maximum number of bytes required to encode a Unicode value in UTF8.
// For Unicode 2.0 and above this is 4.

// Note: For UTF-16 string handling consider using icu::UnicodeString
// from //third_party/icu. It is a full-featured UTF-16 string class.
// See http://go/icu and http://g3doc/third_party/icu/g3doc/how-to-use-icu.md.
// For conversion between UTF-8 and UTF-16 use the functions in
// i18n/utils/public/unicodestring.h .
// There are also functions there for checking and coercing to
// well-formed (structurally valid) UTF-16.
// For other UTF-16 string functionality take a look at
// third_party/icu/include/unicode/unistr.h or ask icu-users@google.com .

namespace EncodingUtils {

    // Unicode range - [0, 10FFFF], translates to [0, 1114111], so the max length
    // would be len(&#1114111;) = 10
    const int kMaxHtmlUnicodeCharEntityLength = 10;

    // ----------------------------------------------------------------------
    // IsASCII7BIT()
    //
    // Check if the input text contains only 7bit ascii characters. If text_len
    // is zero, this function returns true.
    //
    // See Unilib::IsInterchangeValid7BitAscii for testing strings that are
    // intended for inclusion in Web pages (not an exact replacement).
    //
    // ----------------------------------------------------------------------
    bool IsASCII7BIT(const char* text, int text_len);

    // ---------------------------------------------------------------------
    // HasBothASCIIAndNonASCIIChars()
    //
    // Returns true if the content has both 7bit ascii and non 7bit ascii chars.
    // False otherwise. Returns false if len == 0
    //
    //-----------------------------------------------------------------------
    bool HasBothASCIIAndNonASCIIChars(const char* text, int len);


    // ----------------------------------------------------------------------
    // IsBeginWithUTF8Char()
    //
    //    This function takes a string marked by a start char* and end
    //    char*. It tests whether the the string begins with a UTF8
    //    char. If yes, the function also returns how many bytes this UTF8
    //    char contains.
    //
    //    end should point to the byte *after* the last byte in the string.
    // ----------------------------------------------------------------------
/* Deprecated.
   Use UniLib::SpanStructurallyValid [jrm]
   For length, use UniLib:OneCharLen [hta] */
    bool IsBeginWithUTF8Char(const char *start, const char *end,
                             int *length);

    // ----------------------------------------------------------------------
    // GetUnicodeForUTF8Char()
    //
    //    This function takes a string marked by a start char* and end
    //    char*. It tests whether the the string begins with a UTF8
    //    char. If yes, the function return the unicode of the character.
    //    It also returns how many bytes this UTF8 char contains.
    //
    //    end should point to the byte *after* the last byte in the string.
    //
    // ----------------------------------------------------------------------
/* Deprecated. Use DecodeUTF8Char [jrm] */
    bool GetUnicodeForUTF8Char(const char *start, const char *end,
                               int *length, int *unicode);

    // ----------------------------------------------------------------------
    // IsUTF8String()
    //
    //    This function takes a string and tests whether it conforms to
    //    the syntax for UTF-8.
    //
    //    Note that since 7-bit ASCII characters do not change when they
    //    are encoded in UTF-8, a string that contains only 7-bit ASCII
    //    is also a valid "UTF8 string".
    //    ----------------------------------------------------------------------
/* Deprecated. Use UniLib::IsInterchangeValid [jrm] */
    bool IsUTF8String(const char *text, int text_len);

    //
    // IsXMLUnsafe7BitASCIIChar
    //
    // This function takes a 7Bit ASCII char and determines if it is one of the
    // control characters that are invalid in XML text.
    //
    // According to http://www.w3.org/TR/REC-xml#charsets, all the control chars
    // between 0x00 to 0x1f are unsafe except tab(0x09), line feed(0x0a) and
    // carriage return(0x0d).
    //
    // This function treat 0x00 (null) a safe character since the input may have
    // multiple null terminated strings.
    //
    bool IsXMLUnsafe7BitASCIIChar(const char *text);


    // ----------------------------------------------------------------------
    // CHARACTER SETS
    //
    // Each of these functions comes in two forms.
    // The first takes a char32 representing a Unicode code-point value.
    // The second takes a UTF-8 buffer and a length; the buffer must be
    // well-formed UTF-8, and must contain exactly one code-point value.
    //
    // ----------------------------------------------------------------------

    // ----------------------------------------------------------------------
    // IsQuote, IsUTF8Quote
    //    U+0022 ("), U+02BA, U+030E, U+2018 (‘), U+2019 (’), U+201A,
    //    U+201B, U+201C, U+201D, U+201E, U+2-1F, U+2033, U+2035,
    //    U+275D, U+275E, U+301D, U+301E, U+301F, U+FF02.
    //    See also IsApostrophe.
    //
    // ----------------------------------------------------------------------
    bool IsQuote(char32 c);
    bool IsUTF8Quote(const char* utf8char, int len);

    // ----------------------------------------------------------------------
    // IsApostrophe, IsUTF8Apostrophe
    //    U+0027 ('), U+2019 (’)
    //    U+2019 is also known as a "printer's quote" and is produced
    //    by many word-processing programs.
    //
    // ----------------------------------------------------------------------
    bool IsApostrophe(char32 c);
    bool IsUTF8Apostrophe(const char* utf8char, int len);

    // ----------------------------------------------------------------------
    // IsHyphen, IsUTF8Hyphen
    //    U+002D (-), U+2010, U+2011, U+2012, U+2013, U_2014
    //    This code only checks for explicit "hard" hyphens which are
    //    those that are always displayed. For soft hyphens, which are
    //    optional hints, use IsSoftHyphen.
    //
    // ----------------------------------------------------------------------
    bool IsHyphen(char32 r);
    bool IsUTF8Hyphen(const char* utf8char, int len);

    // ----------------------------------------------------------------------
    // IsSoftHyphen, IsUTF8SoftHyphen
    //    U+00AD (soft hyphen), U+1806 (Mongolian soft hyphen)
    //    Soft hyphens are normally not displayed at all and should
    //    not separate words, but are hints to the line-wrapping algorithm.
    //
    // ----------------------------------------------------------------------
    bool IsSoftHyphen(char32 c);
    bool IsUTF8SoftHyphen(const char* utf8char, int len);

    // ----------------------------------------------------------------------
    // IsUTF8Whitespace
    //    This test is true for all characters that have the White_Space
    //    property in the Unicode database. For the complete list, see
    //    util/utf8/rawprops/has_whitespace_category.txt
    //
    //    Note: a previous version of this function returned false for
    //    all ASCII characters, and clients typically wrote
    //       isspace(*buf) || IsUTF8Whitespace(buf, len)
    //    That is no longer necessary (and not recommended).
    // ----------------------------------------------------------------------
/* Deprecated. Use UnicodeProps::IsWhitespace [jrm] */
    // TODO: Temporarily substitute UnicodeProps::IsUTF8Whitespace()
    bool IsWhitespace(char32 c);
    bool IsUTF8Whitespace(const char *utf8char, int len);

    // ----------------------------------------------------------------------
    // IsIgnore, IsUTF8Ignore
    //    true only for U+200D (zero width joiner)
    //    and U+200C (zero width non-joiner) and
    //    soft hyphens (see IsSoftHyphen).
    //
    // ----------------------------------------------------------------------
    bool IsIgnore(char32 c);
    bool IsUTF8Ignore(const char *utf8char, int len);




    // A simple function that converts UTF8 chars to Html entities,
    // while keeping the ASCIIs untouched. This function assumes valid
    // UTF8 input.
    void ConvertToHtmlEntityDetectCopyASCII(const char* text, int len,
                                            string* output);
    //
    // CleanupUTF8Chars
    //
    // Description: Sometime users lie about their encodings. They claim the
    // input is UTF8 but it is actually not. This lie is bad since it screws up
    // our output sometimes. Therefore, we need to cleanup the input by
    // replacing the illegal characters with ?s. The output string will have the
    // same length as the input.
    //
    // As usual, for performance reasons, we do a lazy copy: the function
    // returns the original string if the input is indeed a valid UTF8
    // string. Otherwise,  it returns the result string with newly allocated
    // memory. THE CALLER IS RESPONSIBLE FOR THE SPACE OF THE RESULT STRING.
    //
/* Deprecated. Use InputConverter::ConvertString [jrm] */
    const char* CleanupUTF8Chars(const char* maybeutf8input,
                                 int maybeutf8input_bytes);

    //
    // IsTruncatedUTF8
    //
    // This function tests whether the specified string ends with a truncated
    // multibyte UTF8 character. Note that it assumes, and does NOT assert, that
    // the entire string, except for the possible truncation, is valid UTF8.
    //
    // The rationale for this function is as follows:
    //
    // MySQL silently truncates string values on insertion to fit them into
    // columns. In Ads we store UTF8 byte streams in columns that MySQL thinks
    // are Latin1. Therefore MySQL will sometimes truncate the UTF8 bytestream
    // between character boundaries and insert malformed UTF8 into the column.
    // This function allows us to detect such an occurence.
    //
    // This function tests the bytes near the end pointer.
    // The start pointer is used only to verify that we don't go past the
    // beginning of the string as we scan backwards from end. Therefore the
    // start pointer can just be the beginning of the string, it doesn't need to
    // be the beginning of the suspicious character.
    //
    // If the string does end with a truncated UTF8 character,
    // *character_fragment_length is set to the number of remaining bytes in that
    // character. This is the minimal number of bytes that need to be
    // removed to make the string valid UTF8.
    //
    // end should point to the byte *after* the last byte in the string.
/* Deprecated. Use UniLib::CompleteCharLength [jrm] */
    bool IsTruncatedUTF8(const char *start, const char *end,
                         int *character_fragment_length);

    // ---------------------------------------------------------------------
    // HasCJKChars()
    //
    // Returns true if the content contains at least one CJK character. The
    // input is assumed to be a valid UTF-8 string.
    // To check if a string begins or ends with a CJK character, use
    // IsFirstLetterCJK and IsLastLetterCJK.
    //
    //-----------------------------------------------------------------------
    //bool HasCJKChars(const char* const text, const int len);

    // ---------------------------------------------------------------------
    // HasThaiChars()
    //
    // Returns true if the content contains at least one Thai character. The
    // input is assumed to be a valid UTF-8 string.
    //-----------------------------------------------------------------------
    //bool HasThaiChars(const char* const text, const int len);


    // /////////////////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////
    // The remaining functions were moved here from strings/strutil.h.
    // /////////////////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////

    // ----------------------------------------------------------------------
    // Latin1CharToUTF8Char()
    // Converts a Latin-1 char to UTF8.
    // If 'c' is not a valid Latin1 byte, the output will contain the
    // 3-byte sequence 0xEF 0xBF 0xBD, which is the UTF-8 encoding of
    // the Replacement character, 0xFFFD.
    // Returns the length in bytes of the UTF8 char.
    // ----------------------------------------------------------------------
    int Latin1CharToUTF8Char(char c, char utf8_char[4]);

    // This is sooooo annoying.  Microsoft has its own non-standard
    // extensions to iso-8859-1 (iso latin-1) and Microsoft tools
    // generate documents using this encoding that claim to be
    // iso-8859-1 even though they really aren't.  So we have to
    // check for these characters and map them to the right
    // values.
    const uint16  kMicrosoft1252ToUcs2[0x20] = {
            0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
            0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
            0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
            0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178};

    // ----------------------------------------------------------------------
    // UCS2CharToUTF8Char()
    //    converts a UCS2 char to UTF8.
    //
    // NOTE: This function is essentially a wrapper for
    // EncodeAsUTF8Char. However, it first checks whether the character
    // is in the range 0x80 to 0x9F, and if so, assumes that it is in
    // Microsoft's non-standard extension to ISO-8859-1 (see above) and
    // converts the mapped value.  If your input is not using this
    // encoding, then you should call EncodeAsUTF8Char directly.
    //
    // ----------------------------------------------------------------------
    int UCS2CharToUTF8Char(uint16 ucs2_char, char *utf8_buf);


    // ----------------------------------------------------------------------
    // UTF8CharToUCS2Char()
    //    converts a UTF8 char to UCS2.
    //
    // The conversion rule:
    // Sequence of                Four-octet
    // octets in UTF-8            sequences in UCS4
    // z = 00 .. 7F;              z;
    // z = C0 .. DF; y;           (z-C0)*2**6 + (y-80);
    // z = E0 .. EF; y; x;        (z-E0)*2**12 + (y-80)*2**6 + (x-80);
    //
    // ----------------------------------------------------------------------
/* Deprecated. Use DecodeUTF8Char [jrm] */
    int UTF8CharToUCS2Char(const char *utf8_char, const int utf8_len,
                           uint16 *ucs2_char);


    // ----------------------------------------------------------------------
    // IsFirstLetterCJK()
    //    checks whether the first letter is CJK.
    // ----------------------------------------------------------------------
    //bool IsFirstLetterCJK(const char *buf_utf8, int len);

    // ----------------------------------------------------------------------
    // IsLastLetterCJK()
    //    checks whether the last letter is CJK.
    //
    // ----------------------------------------------------------------------
    //bool IsLastLetterCJK(const char *buf_utf8, int len);


    // On entry, buf_utf8 is a buffer containing len bytes UTF8 data,
    // and latin1_char points to a single char, buf_utf8 must contain a single
    // code-point.
    // If the UTF8 character at buf_utf8 is Latin1, the Latin1-encoded
    // character is stored into *latin1_char and true is returned;
    // otherwise *latin1_char is unchanged and false is returned.
    //
    // To convert complete strings, use the OutputConverter class defined in
    // i18n/encodings/outputconverter/public/outputconverter.h
    bool UTF8ToLatin1(const char *buf_utf8, int len, char *latin1_char);

    // Returns a pointer to the byte following the first character of buf_utf8.
    // This function assumes that buf_utf8 is a valid, non-NULL UTF-8 string.
    // There is no special handling if the first byte is null ('\0'). It is
    // guaranteed that the resulting pointer is greater than the input pointer.
    const char* AdvanceOneUTF8Character(const char *buf_utf8);

    // returns a pointer advanced count UTF8 characters, or a pointer to
    // the end of the string if it isn't that long
    const char* AdvanceUTF8Characters(const char* buf_utf8, int count);

    // Returns a pointer to the UTF8 character *before* the one at 'end'.
    // This will not back up beyond 'start'. (I.e., start <= end.)
    // The byte *at* 'end' is not accessed. 'start' must be a valid UTF8
    // string.
    const char* BackUpOneUTF8Character(const char* start, const char* end);

    // Truncate a UTF-8 string, if necessary, so that it contains at most max_len
    // complete Unicode characters (Unicode code points).
    // If the string doesn't contain more than max_len characters, then
    // don't change it.
    // Returns true if the string was changed (truncated).
    bool TruncateUTF8String(string* str, int max_len);

    // ----------------------------------------------------------------------
    // ClipUTF8String
    //   Same as ClipString, but we also respect UTF8 boundaries.
    //   And also, if max_len is too small to hold ..., we will just clip max_len
    //   and we assume UTF8 chars.  If we think the chars look too bad, we'll
    //   quit being smart.  We also return the number of chars remaining.
    //   We also provide a string friendly interface.
    //
    //   ***NOTE***
    //   ClipUTF8String counts in terms of *bytes*.  If you have non-ascii
    //   strings, and you are trying to clip at a certain number of characters
    //   or a visual width, this is the wrong function.  If you are displaying
    //   the clipped strings to users in a frontend, consider using
    //   ClipStringOnWordBoundary in webserver/util/snippets/rewriteboldtags,
    //   which considers the width of the string instead of the number of bytes.
    // ----------------------------------------------------------------------

    int ClipUTF8String(char* str, int max_len);

    int ClipUTF8String(string* str, int len);

    // Counts the number of Unicode characters in a UTF-8 string.
    // UTF8StrLen() is significantly faster than the utflen() function
    // defined in the third_party/utf. Run the benchmark in the unit-test for
    // performance numbers.
    // Note: This requires a valid, complete UTF-8 string.
    int UTF8StrLen(StringPiece input);
    //int UTF8StrLen(const Cord& cord);
    int UTF8StrLen(const char* buf, int len);


    // ----------------------------------------------------------------------
    // UnicodeFromUTF8()
    // This function accepts a UTF8-encoded string and returns an integer vector,
    // containing for each character its Unicode.
    //
    // ----------------------------------------------------------------------
/* Deprecated. Use DecodeUTF8 [jrm] */
    std::vector<int> *UnicodeFromUTF8(const char *utf8_str);

    // ----------------------------------------------------------------------
    // UnicodeFromUTF8Character()
    // This function accepts a single UTF8-encoded character and returns the
    // unicode integer value for that character.
    //
    // ----------------------------------------------------------------------
/* Deprecated. Use DecodeUTF8 [jrm] */
    int UnicodeFromUTF8Character(const char *utf8_str);




    // ----------------------------------------------------------------------
    // Encoding/decoding functions
    //
    // To test arrays of UTF-8 and Latin1 for validity, use
    //   UTF8IsStructurallyValid
    //   IsValidLatin1
    // ----------------------------------------------------------------------

    // Unicode code-point values are in [0, 0x10FFFF], excluding values
    // in [0xD800, 0xDFFF], which are used for surrogate pairs in
    // UTF-16.
    inline bool IsValidCodepoint(char32 c) {
        return UniLib::IsValidCodepoint(c);
    }

    // ----------------------------------------------------------------------
    // DecodeUTF8
    // ----------------------------------------------------------------------

    // Decode a UTF-8 array into a vector of Unicode code-point values.
    // If the array contains bytes of invalid UTF-8, the output vector
    // will contain 0xFFFD for each such byte. (Note that the presence
    // of 0xFFFD in the output vector does not imply that the input was
    // not well-formed. 0xFFFD is the decoded version of the well-formed
    // 3-byte sequence "\xBF\xEF\xED".)
    void DecodeUTF8(const char* in, int size, std::vector<char32>* out);

    // As above but takes a StringPiece as input.
    void DecodeUTF8(StringPiece in, std::vector<char32>* out);

    // Decode one Unicode code-point value from a UTF-8 array. Return
    // the number of bytes read from the array. If the array does not
    // contain valid UTF-8, store 0xFFFD in the output variable
    // and return 1.
    int DecodeUTF8Char(const char* in, char32* out);

    // Decode one Unicode code-point value from a UTF-8 array. Return
    // the number of bytes read from the input array. If the array does
    // not contain valid UTF-8 within the first 'size' bytes, store
    // 0xFFFD in the output variable, and return 0 if the byte-sequence
    // was too short or 1 if there was some other error.
    int DecodeUTF8Char(const char* in, int size, char32* out);

    // ----------------------------------------------------------------------
    // EncodeAsUTF8
    // ----------------------------------------------------------------------

    // Encode an array of Unicode code-point values as UTF-8, and return
    // a string containing the encoded sequence.
    string EncodeAsUTF8(const char32* in, int size);

    // Encode an array of 8-bit Latin1 values as UTF-8, and return a
    // string containing the encoded sequence. See Latin1CharToUTF8Char
    // for a description of how invalid Latin1 bytes are handled.
    string EncodeLatin1AsUTF8(const char* in, int size);
    // As above, but takes in a StringPiece.
    string EncodeLatin1AsUTF8(StringPiece input);

    // Encode one Unicode code-point value as UTF-8. Return the number
    // of bytes written into the output array (which is assumed to be
    // large enough).
    int EncodeAsUTF8Char(char32 in, char* out);
}  // namespace EncodingUtils

#endif  // I18N_UTF8_ENCODINGUTILS_H__