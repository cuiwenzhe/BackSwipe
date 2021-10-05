// Tools for converting unicode codepoints (e.g., diacritics) into their lower
// and base case versions.
//
// Imported from Android:
// packages/inputmethods/LatinIME/native/jni/src/utils/char_utils.h
//
// TODO(ouyang): Check if either of the following:
// /quality/characterconverter/internal/characterconverter.cc
// /i18n/utf8/case_converter.h
// can provide the same functionality and does not affect performance.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_LATINIME_CHARCONVERTER_H_
#define INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_LATINIME_CHARCONVERTER_H_

#include <cctype>
#include <cstring>
#include <vector>

namespace keyboard {
namespace decoder {

    #define NELEMS(x) (sizeof(ArraySizeHelper(x)))

    using std::vector;

    template <typename T, int N>
    char (&ArraySizeHelper(T (&array)[N]))[N];

    class LatinImeCharConverter {
    public:
        static inline bool isAsciiUpper(int c) {
            // Note: isupper(...) reports false positives for some Cyrillic characters,
            // causing them to be incorrectly lower-cased using toAsciiLower(...) rather
            // than latin_tolower(...).
            return (c >= 'A' && c <= 'Z');
        }

        static inline bool isAsciiLower(int c) {
            return (c >= 'a' && c <= 'z');
        }

        static inline int toAsciiLower(int c) { return c - 'A' + 'a'; }

        static inline int toAsciiUpper(int c) { return c - 'a' + 'A'; }

        static inline bool isAscii(int c) { return isascii(c) != 0; }

        static inline int toLowerCase(const int c) {
            if (isAsciiUpper(c)) {
                return toAsciiLower(c);
            }
            if (isAscii(c)) {
                return c;
            }
            return static_cast<int>(latin_tolower(static_cast<unsigned short>(c)));
        }

        static inline int toUpperCase(const int c) {
            if (isAsciiLower(c)) {
                return toAsciiUpper(c);
            }
            if (isAscii(c)) {
                return c;
            }
            return static_cast<int>(latin_toupper(static_cast<unsigned short>(c)));
        }

        static inline int toBaseLowerCase(const int c) {
            return toLowerCase(toBaseCodePoint(c));
        }

        static inline int toBaseCodePoint(int c) {
            if (c < BASE_CHARS_SIZE) {
                return static_cast<int>(BASE_CHARS[c]);
            } else if (c >= BASE_CHARS_LATIN_ADDITIONAL_START &&
                       c < BASE_CHARS_LATIN_ADDITIONAL_START +
                           BASE_CHARS_LATIN_ADDITIONAL_SIZE) {
                return static_cast<int>(BASE_CHARS_LATIN_ADDITIONAL[
                        c - BASE_CHARS_LATIN_ADDITIONAL_START]);
            }
            return c;
        }

        static inline bool isLatin(int c) {
            const int lower_c = toLowerCase(c);
            if (lower_c >= 'a' && lower_c <= 'z') {
                return true;
            }
            return is_latin_lower(lower_c);
        }

        // Returns the digraph codes associated with the input code (expected to be in
        // lowercase).
        //
        // Return the empty vector if the input code is not associated with a digraph.
        static const vector<int>& GetDigraphForCode(const int code);

        static unsigned short latin_tolower(const unsigned short c);
        static unsigned short latin_toupper(const unsigned short c);
        static bool is_latin_lower(const unsigned short c);
        static const std::vector<int> EMPTY_STRING;

    private:
        static const int MIN_UNICODE_CODE_POINT;
        static const int MAX_UNICODE_CODE_POINT;

        /**
         * Table mapping most combined Latin, Greek, and Cyrillic characters
         * to their base characters.  If c is in range, BASE_CHARS[c] == c
         * if c is not a combined character, or the base character if it
         * is combined.
         */
        static const int BASE_CHARS_SIZE = 0x0500;
        static const unsigned short BASE_CHARS[BASE_CHARS_SIZE];

        /**
         * Table mapping characters in Latin Extended Additional range (0x1E00-0x1EFF)
         * to their base characters.
         */
        static const int BASE_CHARS_LATIN_ADDITIONAL_START = 0x1E00;
        static const int BASE_CHARS_LATIN_ADDITIONAL_SIZE = 0x0100;
        static const unsigned short BASE_CHARS_LATIN_ADDITIONAL[
                BASE_CHARS_LATIN_ADDITIONAL_SIZE];
    };

}  // namespace decoder
}  // namespace keyboard
#endif  // INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_LATINIME_CHARCONVERTER_H_
