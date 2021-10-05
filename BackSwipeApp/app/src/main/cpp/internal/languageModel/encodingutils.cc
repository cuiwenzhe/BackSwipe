//
// Copyright 2001 Google, Inc. All Rights Reserved.
// Author: Jenny Zhou
//

#include "encodingutils.h"

#include <string.h>                     // for memcpy, strlen, strcpy
#include <string>                       // for string, __versa_string

#include "../base/logging.h"               // for COMPACT_GOOGLE_LOG_FATAL, etc
#include "../base/macros.h"                // for NULL, arraysize
#include "../base/stringprintf.h"          // for StringAppendF
#include "../base/ascii_ctype.h"        // for ascii_isspace
//#include "util/utf8/public/unicodeproperty.h"  // for UnicodeProperty
//#include "util/utf8/public/unicodeprops.h"  // for CJKLetter, etc
#include "../base/unilib.h"    // for IsStructurallyValid, etc
//#include "util/utf8/public/utf8whitespace.h"  // for IsUTF8Whitespace
using std::string;
// Invalid UTF-8 bytes are decoded as the Replacement Character, U+FFFD
// (which is also Runeerror). Invalid code points are encoded in UTF-8
// with the UTF-8 representation of the Replacement Character.
static const char ReplacementCharacterUTF8[3] = {'\xEF', '\xBF', '\xBD'};

static inline bool InvalidUTF8(Rune r, int length) {
    // If chartorune detects invalid UTF-8, it sets the Rune to
    // Runeerror (0xFFFD) and returns 0 or 1 for the number of bytes that
    // were consumed. "\xEF\xBF\xBD", however, is the valid UTF-8 encoding
    // for 0xFFFD, but in this case, chartorune will return 3, since 3
    // bytes were consumed. So to check for invalid UTF-8, we need to
    // test both the Rune value and the length.
    return r == Runeerror && length != 3;
}

// ----------------------------------------------------------------------
// IsASCII7BIT()
// ----------------------------------------------------------------------

static inline bool IsASCIIchar(char c) {
    // return c < 0x80;
    return static_cast<signed char>(c) >= 0;
}

bool EncodingUtils::IsASCII7BIT(const char* text, int text_len) {
    CHECK( text_len >= 0 );

    if (text_len == 0) return true;

    CHECK( text != NULL );

    for ( int i = 0; i < text_len; i++ ) {
        if (!IsASCIIchar(text[i])) {
            return false;
        }
    }

    return true;
}

// ----------------------------------------------------------------------
// HasBothASCIIAndNonASCIIChars()
// ----------------------------------------------------------------------
bool EncodingUtils::HasBothASCIIAndNonASCIIChars(const char* text,
                                                 int len) {
    CHECK_GE(len, 0);
    if (len == 0) return false;

    CHECK(text != NULL);
    bool ascii_found = false;
    bool nonascii_found = false;
    for (int i = 0; i < len; ++i) {
        if (IsASCIIchar(text[i])) {
            // we have an ascii char
            ascii_found = true;
            // a nonascii char was found earlier
            if (nonascii_found) return true;
        } else {
            // we have a nonascii char
            nonascii_found = true;
            // an ascii char was found earlier
            if (ascii_found) return true;
        }
    }

    // no conflict found during the scan
    return false;
}


// ----------------------------------------------------------------------
// IsBeginWithUTF8Char()
//  ---------------------------------------------------------------------

bool EncodingUtils::IsBeginWithUTF8Char(const char *start, const char *end,
                                        int *length) {
    DCHECK(start && end && length);
    if (start >= end) {
        *length = 0;
        return false;
    }
    Rune r;
    int n = charntorune(&r, start, end - start);
    if (InvalidUTF8(r, n)) {
        *length = 1;
        return false;
    } else {
        *length = n;
        return true;
    }
}



// ----------------------------------------------------------------------
// GetUnicodeForUTF8Char()
// ----------------------------------------------------------------------

bool EncodingUtils::GetUnicodeForUTF8Char(const char *start, const char *end,
                                          int *length, int *unicode) {
    *unicode = 0;
    Rune r;
    *length = charntorune(&r, start, end - start);
    if (InvalidUTF8(r, *length)) {
        // Many callers assume that length will never be 0, and they will
        // get into infinite loops if it is. Since some of them will
        // simply skip over the bad byte, it's best to set length to 1.
        *length = 1;
        return false;
    }
    *unicode = r;
    return true;
}



// ----------------------------------------------------------------------
// IsUTF8String()
// ----------------------------------------------------------------------
bool EncodingUtils::IsUTF8String(const char *text, int text_len)  {
    return UniLib::IsStructurallyValid(text, text_len);
}


// Returns true if min <= x <= max, copied from lang_enc.cc
static inline bool is_between(int x, int min, int max) {
    return ((x >= min) & (x <= max));
}

// ----------------------------------------------------------------------
// IsXMLUnsafe7BitASCIIChar
// ----------------------------------------------------------------------
bool EncodingUtils::IsXMLUnsafe7BitASCIIChar(const char *text) {
    int c = text[0];
    return (is_between(c, 0x01, 0x08) ||
            is_between(c, 0x0b, 0x0c) ||
            is_between(c, 0x0e, 0x1f));
}

void EncodingUtils::ConvertToHtmlEntityDetectCopyASCII(const char* text,
                                                       int len,
                                                       string* output) {
    output->clear();

    char* clean = 0;
    if (!UniLib::IsStructurallyValid(text, len)) {
        clean = new char[len];
        UniLib::CoerceToStructurallyValid(text, len, ' ', clean, len);
        text = clean;
    }

    const char* end = text + len;
    while (text < end) {
        if (*text < 0x80) {
            output->push_back(*text++);
        } else {
            Rune r;
            text += chartorune(&r, text);
            base::StringAppendF(output, "&#%d;", r);
        }
    }

    delete[] clean;
}


// ----------------------------------------------------------------------
// CleanupUTF8Chars
// ----------------------------------------------------------------------
const char* EncodingUtils::CleanupUTF8Chars(const char* s, int n) {
    if (UniLib::IsStructurallyValid(s, n))
        return s;

    char* output = new char[n];
    UniLib::CoerceToStructurallyValid(s, n, '?', output, n);
    return output;
}


// ----------------------------------------------------------------------
// IsTruncatedUTF8
//
// ----------------------------------------------------------------------
bool EncodingUtils::IsTruncatedUTF8(const char *start, const char *end,
                                    int *character_fragment_length) {
    int len = end - start;
    int good = UniLib::CompleteCharLength(start, len);
    *character_fragment_length = len - good;
    return (len > good);
}


// ---------------------------------------------------------------------
// HasCJKChars()
//
// Returns true if the content contains at least one CJK character. The
// input is assumed to be a UTF-8 encoded string.
//
//-----------------------------------------------------------------------
//bool EncodingUtils::HasCJKChars(const char* const text, const int len) {
//    DCHECK(UniLib::IsStructurallyValid(text, len));
//    return !UnicodeProps::CJKLetter().None(text, len);  // some == not none
//}

// ---------------------------------------------------------------------
// HasThaiChars()
//
// Returns true if the content contains at least one Thai character. The
// input is assumed to be a UTF-8 encoded string.
//
//-----------------------------------------------------------------------
//bool EncodingUtils::HasThaiChars(const char* const text, const int len) {
//    DCHECK(UniLib::IsStructurallyValid(text, len));
//    return !UnicodeProps::ThaiLetter().None(text, len);  // some == not none
//}


// /////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////
// The following functions were moved here from strings/strutil.cc.
// /////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------
// UnicodeFromUTF8()
// This function accepts a UTF8-encoded string and returns an integer vector,
// containing for each character its Unicode.
// ----------------------------------------------------------------------

std::vector<int>* EncodingUtils::UnicodeFromUTF8(const char *utf8_str) {
    std::vector<int> *v= new std::vector<int>;
    if (utf8_str == NULL)
        return v;
    while (*utf8_str != '\0') {
        Rune r;
        utf8_str += chartorune(&r, utf8_str);
        v->push_back(r);
    }
    return v;
}

// ----------------------------------------------------------------------
// UnicodeFromUTF8Character()
// This function accepts a single UTF8-encoded character and returns the
// unicode integer value for that character.
// ----------------------------------------------------------------------

int EncodingUtils::UnicodeFromUTF8Character(const char* utf8_str) {
    Rune r;
    chartorune(&r, utf8_str);
    return r;
}



// /////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////
//    The following functions were moved here from encodingutils.h
//    (originally from strings/strutil.h), where they were defined inline.
// /////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////

int EncodingUtils::Latin1CharToUTF8Char(char c, char utf8_char[4]) {
    if (UniLib::IsValidLatin1(&c, 1)) {
        return EncodeAsUTF8Char(c, utf8_char);
    } else {
        memcpy(utf8_char, ReplacementCharacterUTF8, 3);
        return 3;
    }
}

int EncodingUtils::UCS2CharToUTF8Char(uint16 ucs2_char, char *utf8_buf) {
    if ((0x80 <= ucs2_char) && (ucs2_char < 0xa0)) {
        // This is sooooo annoying.  Microsoft has its own non-standard
        // extensions to iso-8859-1 (iso latin-1) and Microsoft tools
        // generate documents using this encoding that claim to be
        // iso-8859-1 even though they really aren't.  So we have to
        // check for these characters and map them to the right
        // values.
        ucs2_char = kMicrosoft1252ToUcs2[ucs2_char - 0x80];
    }

    return EncodeAsUTF8Char(ucs2_char, utf8_buf);
}

int EncodingUtils::UTF8CharToUCS2Char(const char *utf8_char,
                                      const int utf8_len,
                                      uint16 *ucs2_char) {
    Rune r;
    int len = charntorune(&r, utf8_char, utf8_len);
    if (InvalidUTF8(r, len))
        return 0;
    if (r > 0xFFFF)  // out of range
        return 0;
    *ucs2_char = static_cast<uint16>(r);
    return len;
}

// ----------------------------------------------------------------------
// Character sets
// ----------------------------------------------------------------------

static bool rbsearch(char32 c, const char32 *array, int len) {
    while (len > 1) {
        int mid = len >> 1;
        const char32* p = &array[mid];
        if (c >= *p) {
            array = p;
            len = len - mid;
        } else {
            len = mid;
        }
    }
    return c == array[0];
}

bool EncodingUtils::IsUTF8Quote(const char* utf8char, int len) {
    if (len == 1 && *utf8char == '\"') return true;  // common case
    Rune r;
    int n = charntorune(&r, utf8char, len);
    if (InvalidUTF8(r, n) || n != len)
        return false;
    return IsQuote(r);
}

bool EncodingUtils::IsQuote(char32 r) {
    static const char32 Quotes[] = {
            0x0022,  // QUOTATION MARK
            0x02BA,  // MODIFIER LETTER DOUBLE PRIME
            0x030E,  // COMBINING DOUBLE VERTICAL LINE ABOVE
            0x2018,  // LEFT SINGLE QUOTATION MARK
            0x2019,  // RIGHT SINGLE QUOTATION MARK
            0x201A,  // SINGLE LOW-9 QUOTATION MARK
            0x201B,  // SINGLE HIGH-REVERSED-9 QUOTATION MARK
            0x201C,  // LEFT DOUBLE QUOTATION MARK
            0x201D,  // RIGHT DOUBLE QUOTATION MARK
            0x201E,  // DOUBLE LOW-9 QUOTATION MARK
            0x201F,  // DOUBLE HIGH-REVERSED-9 QUOTATION MARK
            0x2033,  // DOUBLE PRIME
            0x2035,  // REVERSED PRIME
            0x275D,  // HEAVY DOUBLE TURNED COMMA QUOTATION MARK ORNAMENT
            0x275E,  // HEAVY DOUBLE COMMA QUOTATION MARK ORNAMENT
            0x301D,  // REVERSED DOUBLE PRIME QUOTATION MARK
            0x301E,  // DOUBLE PRIME QUOTATION MARK
            0x301F,  // LOW DOUBLE PRIME QUOTATION MARK
            0xFF02,  // FULLWIDTH QUOTATION MARK
    };
    if (r == '\"') return true;  // common case
    return rbsearch(r, Quotes, arraysize(Quotes));
}

bool EncodingUtils::IsUTF8Hyphen(const char* utf8char, int len) {
    Rune r;
    int n = charntorune(&r, utf8char, len);
    if (InvalidUTF8(r, n) || n != len)
        return false;
    return IsHyphen(r);
}

bool EncodingUtils::IsHyphen(char32 r) {
    static const char32 Hyphens[] = {
            0x002D,  // HYPHEN-MINUS ('-')
            0x2010,  // HYPHEN
            0x2011,  // NON-BREAKING HYPHEN
            0x2012,  // FIGURE DASH
            0x2013,  // EN DASH
            0x2014,  // EM DASH
    };
    return rbsearch(r, Hyphens, arraysize(Hyphens));
}

bool EncodingUtils::IsUTF8SoftHyphen(const char* utf8, int len) {
    // SOFT HYPHEN (U+00AD) or MONGOLIAN SOFT HYPHEN (U+1806)
    return (len == 2 && utf8[0] == 0xC2 && utf8[1] == 0xAD)
           || (len == 3
               && utf8[0] == 0xE1
               && utf8[1] == 0xA0
               && utf8[2] == 0x86);
}

bool EncodingUtils::IsSoftHyphen(char32 r) {
    return r == 0x00AD || r == 0x1806;
}

bool EncodingUtils::IsUTF8Apostrophe(const char* utf8, int len) {
    return (len == 1 && utf8[0] == 0x27) // APOSTROPHE (U+0027)
           || (len == 3
               && utf8[0] == 0xE2
               && utf8[1] == 0x80
               && utf8[2] == 0x99);  // RIGHT SINGLE QUOTATION MARK (U+2019)
}

bool EncodingUtils::IsApostrophe(char32 c) {
    return c == 0x0027 || c == 0x2019;
}

bool EncodingUtils::IsUTF8Whitespace(const char *buf_utf8, int len) {
    if (len == 1 && *buf_utf8 == ' ') return true;  // optimize most common case
    Rune r;
    int n = charntorune(&r, buf_utf8, len);
    if (InvalidUTF8(r, n) || n != len)
        return false;
    //return UnicodeProps::IsUTF8Whitespace(buf_utf8);
    return IsWhitespace(r);
}

bool EncodingUtils::IsWhitespace(char32 r){
    static const char32 WhiteSpaces[] = {
            0x0020,  // SPACE
            0x00a0,  // NO-BREAK SPACE
            0x1680,  // OGHAM SPACE MARK
            0x2000,  // EN QUAD
            0x2001,  // EM QUAD
            0x2002,  // EN SPACE
            0x2003,  // EM SPACE
            0x2004,  // THREE-PER-EM SPACE
            0x2005,  // FOUR-PER-EM SPACE
            0x2006,  // SIX-PER-EM SPACE
            0x2007,  // FIGURE SPACE
            0x2008,  // PUNCTUATION SPACE
            0x2009,  // THIN SPACE
            0x200a,  // HAIR SPACE
            0x200b,  // ZERO WIDTH SPACE
            0x202f,  // NARROW NO-BREAK SPACE
            0x205f,  // MEDIUM MATHEMATICAL SPACE
            0x3000,  // IDEOGRAPHIC SPACE
            0xfeff,  // ZERO WIDTH NO-BREAK SPACE
    };
    return rbsearch(r, WhiteSpaces, arraysize(WhiteSpaces));
}

bool EncodingUtils::IsUTF8Ignore(const char *buf_utf8, int len) {
    // Test for U+200C, ZERO WIDTH NON-JOINER, and U+200D, ZERO WIDTH JOINER,
    // and soft hyphens.
    return (len == 3
            && buf_utf8[0] == 0xE2
            && buf_utf8[1] == 0x80
            && (buf_utf8[2] == 0x8C || buf_utf8[2] == 0x8D))
           || IsUTF8SoftHyphen(buf_utf8, len);
}

bool EncodingUtils::IsIgnore(char32 c) {
    return c == 0x200C || c == 0x200D || IsSoftHyphen(c);
}

//bool EncodingUtils::IsFirstLetterCJK(const char *buf_utf8, int len) {
//    if (len > 0 && IsASCIIchar(buf_utf8[0])) return false;
//    Rune r;
//    int n = charntorune(&r, buf_utf8, len);
//    if (InvalidUTF8(r, n))
//        return false;
//    return UnicodeProps::IsUTF8CJKLetter(buf_utf8);
//}


//bool EncodingUtils::IsLastLetterCJK(const char *buf_utf8, int len) {
//    const char* end = buf_utf8 + len;
//    const char* prev = BackUpOneUTF8Character(buf_utf8, end);
//    int char_len = end - prev;
//    return char_len > 0 && IsFirstLetterCJK(prev, char_len);
//}

bool EncodingUtils::UTF8ToLatin1(const char* buf_utf8, int len,
                                 char * latin1_char) {
    Rune r;
    int consumed = charntorune(&r, buf_utf8, len);
    if (r > 0xFF || consumed != len || InvalidUTF8(r, consumed))
        return false;
    char c = static_cast<char>(r);
    if (UniLib::IsValidLatin1(&c, 1)) {
        *latin1_char = c;
        return true;
    }
    return false;
}

const char* EncodingUtils::AdvanceOneUTF8Character(const char *buf_utf8) {
    return buf_utf8 + UniLib::OneCharLen(buf_utf8);
}

const char* EncodingUtils::AdvanceUTF8Characters(const char* buf_utf8,
                                                 int count) {
    while (count > 0 && *buf_utf8) {
        --count;
        buf_utf8 += UniLib::OneCharLen(buf_utf8);
    }
    return buf_utf8;
}

const char* EncodingUtils::BackUpOneUTF8Character(const char* start,
                                                  const char* end) {
    // 10xx xxxx is a continuation byte. Back up over those.
    while (start < end && UniLib::IsTrailByte(*--end));
    return end;
}


bool EncodingUtils::TruncateUTF8String(string* str, int max_len) {
    // NB: max_len is measured in codepoints (see UTF8StrLen), not bytes.
            DCHECK(UniLib::IsStructurallyValid(*str));
    if (str->length() < max_len) {
        // There are not even max_len _bytes_ in the string.
        return false;
    }
    const char* utf8 = str->c_str();
    const char* utf8_end = utf8 + str->length();
    // AdvanceUTF8Characters() will go no further than the end of str.
    const char* new_end = AdvanceUTF8Characters(utf8, max_len);
    if (new_end < utf8_end) {
        str->erase(new_end - utf8, utf8_end - new_end);
        return true;
    }
    return false;
}


// ----------------------------------------------------------------------
// ClipUTF8String
//   Same as ClipString, but we also respect UTF8 boundaries.
//   And also, if max_len is too small to hold ..., we will just clip max_len
//   and we assume UTF8 chars.  If we think the chars look too bad, we'll
//   quit being smart.  We also return the number of chars remaining.
//   We also provide a string friendly interface.
// ----------------------------------------------------------------------

int EncodingUtils::ClipUTF8String(char* str, int max_len) {
    const int length = strlen(str);
    if (length <= max_len) {
        return length;
    }

    if (max_len <= 0) {
        return 0;
    }

    const int kMaxOverCut = 12;
    const char* const kCutStr = "...";
    const int kCutStrLen = strlen(kCutStr);

    if (max_len <= kCutStrLen) {
        // In this case, we at best return the ..., corner case,
        // do something simple, i.e., return an empty string.
        str[0] = '\0';
        return 0;
    }

    UniLib::CoerceToInterchangeValid(str, length, '?', str, length);

    char* cut_at = str + max_len - kMaxOverCut;
    if (max_len < kMaxOverCut) {
        cut_at = str;
    }
    // Now go backwards to make sure we are at the boundary of a UTF8 char.
    while (UniLib::IsTrailByte(*cut_at) &&  // not the start of a multi-byte char
           cut_at > str) {
        cut_at--;
    }
    const char* cut_by = str + max_len - kCutStrLen;  // leave space for NULL
    // Now go forward.
    while (cut_at < cut_by) {
        if (IsASCIIchar(*cut_at)) {  // one char ASCII
            if (ascii_isspace(*cut_at)) {
                break;
            }
            ++cut_at;
        } else {  // multi byte
            char* iter =  cut_at + 1;
            while (iter < cut_by && UniLib::IsTrailByte(*iter)) {
                ++iter;
            }
            // Ok, we only advance if we have a valid, non-space UTF8 char
            if (UniLib::IsTrailByte(*iter) ||  // not first char of UTF8, invalid
                IsWhitespace((char32)(size_t) cut_at)) {  // whitespace
                //UnicodeProps::IsUTF8Whitespace(cut_at)) {  // whitespace
                break;
            } else {
                cut_at = iter;  // advance
            }
        }
    }
    if (cut_at == str) {  // corner case, return an empty string
        str[0] = '\0';
        return 0;
    }
    strcpy(cut_at, kCutStr);
    return (cut_at + kCutStrLen - str);
}

int EncodingUtils::ClipUTF8String(string* str, int len) {
    str->append(1, '\0');
    str->resize(ClipUTF8String(&((*str)[0]), len));
    return str->size();
}

// Counts the number of UTF8 characters in a string.
// Takes advantage of the fact that the length of the buffer is known and
// computes the number of bytes that are skipped, this avoid using any branch
// in the loop.
inline int UTF8StrLen_Unverified(const StringPiece buf_utf8) {
    const auto byte_size = buf_utf8.size();
    const char* const end = buf_utf8.data() + byte_size;
    const char* ptr = buf_utf8.data();
    int skipped_count = 0;
    while (ptr < end) {
        skipped_count += UniLib::IsTrailByte(*ptr++) ? 1 : 0;
    }
    const int result = byte_size - skipped_count;
    return result;
}

int EncodingUtils::UTF8StrLen(const StringPiece buf_utf8) {
    const int result = UTF8StrLen_Unverified(buf_utf8);
            DCHECK_GE(result, buf_utf8.size() / kMaxUTF8CharBytes)
                    << "Input is not valid utf8.";
    return result;
}

//int EncodingUtils::UTF8StrLen(const Cord& cord) {
//    StringPiece fragment;
//    // Shortcut for flat cords.
//    if (cord.GetFlat(&fragment)) return UTF8StrLen(fragment);
//
//    int num_chars = 0;
//    CordReader reader(cord);
//    while (reader.ReadFragment(&fragment)) {
//        // We're merely counting the number of trailing bytes and removing them from
//        // the total length, so checking invalid separate fragments is OK as long as
//        // we examine the entire Cord.
//        num_chars += UTF8StrLen_Unverified(fragment);
//    }
//            DCHECK_GE(num_chars, cord.size() / kMaxUTF8CharBytes)
//                    << "Input '" << cord << "' is not valid utf8.";
//    return num_chars;
//}

// Same as above, but we don't count the UTF8 chars all the way towards
// the end of buf, instead, we stop after min(len, len(buf)) bytes.
// Same as above, we don't care for incomplete chars.
// TODO: take care of the incomplete chars
int EncodingUtils::UTF8StrLen(const char* buf, int len) {
    int count = 0;
    const char* const end = buf + len;
    while (buf < end && *buf) {
        Rune r;
        int n = charntorune(&r, buf, len);
        if (InvalidUTF8(r, n)) break;  // incomplete char
        ++count;
        buf += n;
        len -= n;
    }
    return count;
}

// ----------------------------------------------------------------------

// /////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////
// The following functions are new.
// /////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------
// DecodeUTF8
// ----------------------------------------------------------------------

void EncodingUtils::DecodeUTF8(const StringPiece input,
                               std::vector<char32>* out) {
    DecodeUTF8(input.data(), input.size(), out);
}

void EncodingUtils::DecodeUTF8(
        const char* in, int size, std::vector<char32>* out) {
    out->clear();
    out->reserve(size);  // there will be at most <size> code points (pure ASCII).
    while (size > 0) {
        Rune r;
        int len = charntorune(&r, in, size);
        out->push_back(r);  // Note that r might be 0xFFFD.
        if (InvalidUTF8(r, len)) {
            len = 1;  // skip the (first) bad byte and go on
        }
        in += len;
        size -= len;
    }
}

int EncodingUtils::DecodeUTF8Char(const char* in, char32* out) {
    Rune r;
    int len = chartorune(&r, in);
    *out = r;
    return len;
}

int EncodingUtils::DecodeUTF8Char(const char* in, int size, char32* out) {
    Rune r;
    int len = charntorune(&r, in, size);
    *out = r;
    return len;
}



// ----------------------------------------------------------------------
// EncodeAsUTF8
// ----------------------------------------------------------------------

string EncodingUtils::EncodeAsUTF8(const char32* in, int size) {
    string out;
    out.reserve(size);
    for (int i = 0; i < size; ++i) {
        char buf[UTFmax];
        int len = EncodeAsUTF8Char(*in++, buf);
        out.append(buf, len);
    }
    return out;
}

string EncodingUtils::EncodeLatin1AsUTF8(const char* in, int size) {
    string out;
    out.reserve(size);
    for (int i = 0; i < size; ++i) {
        char buf[UTFmax];
        int n = Latin1CharToUTF8Char(in[i], buf);
        out.append(buf, n);
    }
    return out;
}

string EncodingUtils::EncodeLatin1AsUTF8(StringPiece input) {
    return EncodeLatin1AsUTF8(input.data(), input.size());
}

int EncodingUtils::EncodeAsUTF8Char(char32 in, char* out) {
    if (UniLib::IsValidCodepoint(in)) {
        return runetochar(out, &in);
    } else {
        memcpy(out, ReplacementCharacterUTF8, 3);
        return 3;
    }
}