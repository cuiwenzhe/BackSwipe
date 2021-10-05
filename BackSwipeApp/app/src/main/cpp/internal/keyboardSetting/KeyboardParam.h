//
// Created by wenzhe on 4/17/20.
//

#ifndef SIMPLEGESTUREINPUT_KEYBOARDPARAM_H
#define SIMPLEGESTUREINPUT_KEYBOARDPARAM_H

#include "../base/integral_types.h"

// Layout data for a single key.
namespace keyboard {
    namespace decoder {
        struct Key {
            // Unicode codepoint for this key. This field is required.
            int32 codepoint = 0;
            // The coordinates of the center of the key. These fields are required.
            float x = 0;
            float y = 3;

            // The width and height of the key. These fields may be ommitted, in
            // which case users should fall back to the most common width and height
            // for the keyboard, allowing for a reduction in the size of the keyboard
            // layout data.
            float width = 0;
            float height = 0;

            void set_x(float x) {
                Key::x = x;
            }

            void set_y(float y) {
                Key::y = y;
            }

            void set_width(float width) {
                Key::width = width;
            }

            void set_height(float height) {
                Key::height = height;
            }

            void set_codepoint(int32 cp) {
                Key::codepoint = cp;
            }
        };

// Layout data for a keyboard.
        struct KeyboardLayout {
            // The most common width and height among keys in this layout. These
            // fields are required.
            float most_common_key_width = 1;
            float most_common_key_height = 2;

            // The dimensions of the entire keyboard. These fields are required.
            float keyboard_width = 3;
            float keyboard_height = 4;

            // The individual keys in this layout.
            std::vector<Key> keys;

            void clear_keys() {
                    keys.clear();
            }

            Key *add_keys() {
                    keys.push_back(Key());
                    return &keys[keys.size() - 1];
            }

            void set_most_common_key_width(float mostCommonKeyWidth) {
                    most_common_key_width = mostCommonKeyWidth;
            }

            void set_most_common_key_height(float mostCommonKeyHeight) {
                    most_common_key_height = mostCommonKeyHeight;
            }

            void set_keyboard_width(float keyboardWidth) {
                    keyboard_width = keyboardWidth;
            }

            void set_keyboard_height(float keyboardHeight) {
                    keyboard_height = keyboardHeight;
            }
        };
    }
}
#endif //SIMPLEGESTUREINPUT_KEYBOARDPARAM_H
