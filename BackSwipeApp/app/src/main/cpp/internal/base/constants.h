// Description:
//   Declares constants used by the keyboard decoder.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_CONSTANTS_H_
#define INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_CONSTANTS_H_

#include <limits>

using std::numeric_limits;

namespace keyboard {
namespace decoder {

    static constexpr float NEG_INF = -numeric_limits<float>::infinity();

    static constexpr float INF = numeric_limits<float>::infinity();

    static constexpr float EPSILON = numeric_limits<float>::epsilon();

    // The default word-separator character.
    static const char kSpaceUtf8Str[] = " ";

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_CONSTANTS_H_
