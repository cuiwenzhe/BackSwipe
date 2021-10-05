// Character utilities used by the keyboard decoder.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_CHAR_UTILS_H_
#define INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_CHAR_UTILS_H_

#include <string>
#include <vector>

#include "base/integral_types.h"
#include "base/macros.h"
#include "languageModel/encodingutils.h"
#include "base/latinime-charconverter.h"

namespace keyboard {
namespace decoder {

    class CharUtils {
    public:
        // Extracts the sequence of base lowercase unicode codepoints for the given
        // UTF8 string. E.g., It converts the word "Über" to the codes [u,b,e,r].
        static inline vector<char32> GetBaseLowerCodeSequence(const string& word) {
            vector<char32> unicode_chars;
            vector<char32> lower_codes;
            EncodingUtils::DecodeUTF8(word.c_str(), word.length(), &unicode_chars);
            for (int i = 0; i < unicode_chars.size(); ++i) {
                lower_codes.push_back(
                        LatinImeCharConverter::toBaseLowerCase(unicode_chars[i]));
            }
            return lower_codes;
        }

        // Returns whether or not the given code is skippable, meaning that it may
        // be omitted in gesture or touch typing. For example users may omit the
        // apostrophe, typing the letters [hes] for the word [he's].
        static inline bool IsSkippableCharCode(const int code) {
            return (code == '\'' || code == '-');
        }

        // Returns whether or not the term represented by result_codes matches the
        // term represented by literal_codes. This method will allow skippable codes
        // in the result_codes to remain unmatched (e.g., the result "i'll" is
        // considered a match for the literal code sequence "ill"). The match is
        // sensitive to case and diacritics, so the input codes should be converted to
        // base/lower case beforehand if needed.
        static inline bool ResultMatchesLiteral(const vector<char32>& result_codes,
                                                const vector<char32>& literal_codes) {
            if (result_codes.size() == 0 || literal_codes.size() == 0) {
                return false;
            }
            int j = 0;
            for (int i = 0; i < result_codes.size(); ++i) {
                const bool codes_match =
                        (j < literal_codes.size() && result_codes[i] == literal_codes[j]);
                if (IsSkippableCharCode(result_codes[i])) {
                    if (codes_match) {
                        ++j;
                    }
                    continue;
                }
                if (!codes_match) {
                    return false;
                }
                ++j;
            }
            return j == literal_codes.size();
        }

    private:
        DISALLOW_IMPLICIT_CONSTRUCTORS(CharUtils);
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_CHAR_UTILS_H_
