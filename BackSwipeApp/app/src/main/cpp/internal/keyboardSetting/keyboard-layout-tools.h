// Description:
//   Tools for creating test QWERTY keyboard layouts.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_KEYBOARD_LAYOUT_TOOLS_H_
#define INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_KEYBOARD_LAYOUT_TOOLS_H_

#include <string>
#include <vector>

#include "../base/integral_types.h"
#include "KeyboardParam.h"

namespace keyboard {
namespace decoder {
namespace keyboard_layout_tools {

    using namespace std;

    static constexpr float kDefaultKeyWidth = 100.0f;
    static constexpr float kDefaultKeyHeight = 150.0f;

    // Creates a generic QWERTY KeyboardLayout for testing.
    //
    // Args:
    //   key_width       - The standard width of the keys on the keyboard.
    //   key_height      - The standard height of the keys on the keyboard.
    //   keyboard_layout - Populated with the generated KeyboardLayout.
    void CreateQwertyKeyboardLayout(const float key_width, const float key_height,
                                    KeyboardLayout* keyboard_layout);

    // Retrieve the center_x and center_y coordinates for the key matching the
    // given codepoint. Returns true if there was a key on the keyboard_layout
    // that matched the codepoint.
    bool GetKeyCenterForCode(const KeyboardLayout& keyboard_layout, int codepoint,
                             float* center_x, float* center_y);

    // Calls GetKeyCenterForCode for variants of codepoint.  First tries the
    // downcased codepoint, then the normalized downcased codepoint.  This finds a
    // reasonable character if the codepoint has a diacritic, but still finds ñ if
    // there is a dedicated key for it.
    // TODO(kep): Perform locale-specific case-insensitive searches.  Consider
    // taking CapitalizationUtils as a parameter, or making a KeyboardLayoutTools
    // class that has a pointer to CapitalizationUtils.
    bool MatchKeyCenterForCode(const KeyboardLayout& keyboard_layout,
                               int codepoint, float* center_x, float* center_y);

    // Same as MatchKeyCenterForCode, but also returns the key's height and width
    bool MatchKeyCenterAndSizeForCode(const KeyboardLayout& keyboard_layout,
                                      int codepoint, float* center_x,
                                      float* center_y, float* height, float* width);

    // Adds a row of keys to the keyboard layout. Note that the x, y coordinate
    // represents the upper-left corner of the leftmost key in the row.
    void AddTestRowToKeyboardLayout(const string& keys, const float x,
                                    const float y, const float key_width,
                                    const float key_height,
                                    KeyboardLayout* keyboard);

    // Creates a keyboard layout according to the given parameters.
    //
    // Args:
    //   most_common_key_width   - The width of a typical letter key. This is used
    //                             to normalize the spatial features so that they
    //                             are independent of the absolute keyboard scale.
    //   most_common_key_height  - The height of a typical letter key.
    //   keyboard_width          - The width of the entire keyboard.
    //   keyboard_height         - The height of the entire keyboard.
    //   codes                   - The vector of codes for each key (in unicode).
    //   xs                      - The vector of x-coordinates for each key
    //                             (relative to the top-left corner).
    //   ys                      - The vector of y-coordinates for each key
    //                             (relative to the top-left corner).
    //   widths                  - The vector of widths for each key.
    //   heights                 - The vector of heights for each key.
    KeyboardLayout CreateKeyboardLayoutFromParams(
            const int most_common_key_width, const int most_common_key_height,
            const int keyboard_width, const int keyboard_height,
            const vector<int32>& codes, const vector<int>& xs, const vector<int>& ys,
            const vector<int>& widths, const vector<int>& heights);

    // Finds the key closest to the point (x, y) in keyboard_layout.  The layout
    // must contain at least one key.
    Key GetNearestKey(const KeyboardLayout& keyboard_layout, float x, float y);

}  // namespace keyboard_layout_tools
}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_KEYBOARD_LAYOUT_TOOLS_H_
