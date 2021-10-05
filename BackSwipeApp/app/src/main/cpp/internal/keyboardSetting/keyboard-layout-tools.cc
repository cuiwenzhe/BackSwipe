// Description:
//   Tools for creating a test QWERTY keyboard.

#include "keyboard-layout-tools.h"

#include "../base/latinime-charconverter.h"
#include "../math-utils.h"
#
#include "../base/constants.h"
#include "../base/unicodetext.h"

namespace keyboard {
namespace decoder {
namespace keyboard_layout_tools {

    namespace {

        bool GetKeyCenterAndSizeForCode(const KeyboardLayout& keyboard_layout,
                                        int codepoint, float* center_x, float* center_y,
                                        float* height, float* width) {
            for (const auto& key : keyboard_layout.keys) {
                if (key.codepoint == codepoint) {
                    *center_x = key.x;
                    *center_y = key.y;
                    *width = key.width;
                    *height = key.height;
                    return true;
                }
            }
            return false;
        }

    }  // namespace

    void CreateQwertyKeyboardLayout(const float key_width, const float key_height,
                                    KeyboardLayout* keyboard_layout) {
        static const string kRowKeys[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
        static const float kRowXOffsets[] = {0.0f, 0.5f, 1.5f};
        keyboard_layout->most_common_key_width = key_width;
        keyboard_layout->most_common_key_height = key_height;
        keyboard_layout->keyboard_width = key_width * kRowKeys[0].size();
        keyboard_layout->keyboard_height = key_height * 3;
        keyboard_layout->clear_keys();
        for (int i = 0; i < 3; ++i) {
            AddTestRowToKeyboardLayout(kRowKeys[i], key_width * kRowXOffsets[i],
                                       key_height * i, key_width, key_height,
                                       keyboard_layout);
        }
        // Add a space key at the bottom of the keyboard that is 5 keys wide.
        Key* space_key = keyboard_layout->add_keys();
        space_key->set_codepoint(' ');
        space_key->set_x(key_width * 5.0);
        space_key->set_y(key_height * 3.5);
        space_key->set_width(key_width * 5);
        space_key->set_height(key_height);
    }

    bool GetKeyCenterForCode(const KeyboardLayout& keyboard_layout, int codepoint,
                             float* center_x, float* center_y) {
        float dummy;
        return GetKeyCenterAndSizeForCode(keyboard_layout, codepoint, center_x,
                                          center_y, &dummy, &dummy);
    }

    bool MatchKeyCenterAndSizeForCode(const KeyboardLayout& keyboard_layout,
                                      int codepoint, float* center_x,
                                      float* center_y, float* height,
                                      float* width) {
        return GetKeyCenterAndSizeForCode(
                keyboard_layout, LatinImeCharConverter::toLowerCase(codepoint),
                center_x, center_y, height, width) ||
               GetKeyCenterAndSizeForCode(
                       keyboard_layout, LatinImeCharConverter::toBaseLowerCase(codepoint),
                       center_x, center_y, height, width);
        return false;
    }

    bool MatchKeyCenterForCode(const KeyboardLayout& keyboard_layout, int codepoint,
                               float* center_x, float* center_y) {
        float dummy;
        return MatchKeyCenterAndSizeForCode(keyboard_layout, codepoint, center_x,
                                            center_y, &dummy, &dummy);
    }

    void AddTestRowToKeyboardLayout(const string& keys, const float x,
                                    const float y, const float key_width,
                                    const float key_height,
                                    KeyboardLayout* keyboard) {
        UnicodeText keys_uni = UTF8ToUnicodeText(keys, false /* copy string */);
        int i = 0;
        for (char32 codepoint : keys_uni) {
            Key* key = keyboard->add_keys();
            key->set_codepoint(codepoint);
            // Convert from the key's upper left coordinate to its center coordinate.
            key->set_x(x + key_width * i + key_width * 0.5);
            key->set_y(y + key_height * 0.5);
            key->set_width(key_width);
            key->set_height(key_height);
            ++i;
        }
    }

    KeyboardLayout CreateKeyboardLayoutFromParams(
            const int most_common_key_width, const int most_common_key_height,
            const int keyboard_width, const int keyboard_height,
            const vector<char32>& codes, const vector<int>& xs, const vector<int>& ys,
            const vector<int>& widths, const vector<int>& heights) {
        const int key_count = xs.size();
        KeyboardLayout keyboard_layout;
        keyboard_layout.set_most_common_key_width(most_common_key_width);
        keyboard_layout.set_most_common_key_height(most_common_key_height);
        keyboard_layout.set_keyboard_width(keyboard_width);
        keyboard_layout.set_keyboard_height(keyboard_height);
        keyboard_layout.clear_keys();
        for (int i = 0; i < key_count; ++i) {
            Key* key = keyboard_layout.add_keys();
            key->set_codepoint(codes[i]);
            // Convert from the key's upper left coordinate to its center coordinate.
            key->set_x(xs[i] + widths[i] / 2.0f);
            key->set_y(ys[i] + heights[i] / 2.0f);
            key->set_width(widths[i]);
            key->set_height(heights[i]);
        }
        return keyboard_layout;
    }

    Key GetNearestKey(const KeyboardLayout& keyboard_layout, float x, float y) {
        float min_distance = INF;
        Key nearest_key;
        for (const Key& key : keyboard_layout.keys) {
            const float distance = MathUtils::Distance(x, y, key.x, key.y);
            if (min_distance == INF || distance < min_distance) {
                nearest_key = key;
                min_distance = distance;
            }
        }
        return nearest_key;
    }

}  // namespace keyboard_layout_tools
}  // namespace decoder
}  // namespace keyboard
