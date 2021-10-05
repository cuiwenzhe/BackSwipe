#include "keyboard.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <set>

#include "../base/logging.h"
#include "../base/latinime-charconverter.h"
#include "KeyboardParam.h"
#include "../base/constants.h"

namespace keyboard {
namespace decoder {

    // static
    std::unique_ptr<Keyboard> Keyboard::CreateKeyboardOrNull(
            const KeyboardLayout& keyboard_layout) {
        std::unique_ptr<Keyboard> keyboard(new Keyboard(keyboard_layout));
        if (keyboard->num_keys() <= 0) {
            LOG(ERROR) << "Cannot create a keyboard with 0 valid keys";
            keyboard = nullptr;
        } else {
            // Update geometric properties for the keyboard.
            keyboard->UpdateGeometricProperties();
        }
        return keyboard;
    }

    Keyboard::Keyboard(const KeyboardLayout& keyboard_layout)
            : most_common_key_width_(keyboard_layout.most_common_key_width),
              most_common_key_height_(keyboard_layout.most_common_key_height),
              keyboard_width_(keyboard_layout.keyboard_width),
              keyboard_height_(keyboard_layout.keyboard_height) {
        // Add the keys from the KeyboardLayout to the Keyboard.
        num_keys_ = 0;
        for (const auto& key : keyboard_layout.keys) {
            AddKey(key.codepoint, key.x, key.y, key.width, key.height);
        }
    }

    char32 Keyboard::GetNearestKeyCode(const float x, const float y) const {
        float min_distance = INF;
        char32 nearest_key = 0;
        for (int i = 0; i < num_keys_; ++i) {
            const float distance =
                    MathUtils::Distance(x, y, center_xs_[i], center_ys_[i]);
            // Calculate probability based on bivariate Gaussian distribution
            if (min_distance == INF || distance < min_distance) {
                nearest_key = key_codes_[i];
                min_distance = distance;
            }
        }
        //LOG(ERROR) << "nearest: " << (char) nearest_key;
        return nearest_key;
    }

    KeyId Keyboard::GetKeyIndex(const char32 code) const {
        for (int i = 0; i < num_keys_; ++i) {
            if (key_codes_[i] == code) {
                return i;
            }
        }
        return kInvalidKeyId;
    }

    float Keyboard::KeyToKeyDirectionByCode(const KeyId c1, const KeyId c2) const {
        const int i = GetKeyIndex(c1);
        const int j = GetKeyIndex(c2);
        return (IsValidKeyIndex(i) && IsValidKeyIndex(j))
               ? KeyToKeyDirectionByIndex(i, j)
               : 0;
    }

    float Keyboard::KeyToSegmentDistanceSq(const KeyId key, const KeyId key1,
                                           const KeyId key2) const {
                DCHECK(IsValidKeyIndex(key));
                DCHECK(IsValidKeyIndex(key1));
                DCHECK(IsValidKeyIndex(key2));
        if (key1 == key2) {
            return sqr(KeyToKeyDistanceByIndex(key, key1));
        }
        const float x = center_xs_[key];
        const float y = center_ys_[key];
        const float x1 = center_xs_[key1];
        const float y1 = center_ys_[key1];
        const float x2 = center_xs_[key2];
        const float y2 = center_ys_[key2];
        return MathUtils::PointToSegmentDistSq(x, y, x1, y1, x2, y2);
    }

    float Keyboard::PointToSegmentDistanceSq(const float x, const float y,
                                             const KeyId key1,
                                             const KeyId key2) const {
                DCHECK(IsValidKeyIndex(key1));
                DCHECK(IsValidKeyIndex(key2));
        const float x1 = center_xs_[key1];
        const float y1 = center_ys_[key1];
        if (key1 == key2) {
            return MathUtils::DistanceSq(x, y, x1, y1);
        }
        const float x2 = center_xs_[key2];
        const float y2 = center_ys_[key2];
        return MathUtils::PointToSegmentDistSq(x, y, x1, y1, x2, y2);
    }

    float Keyboard::PointToSegmentDistance(const float x, const float y,
                                           const KeyId key1,
                                           const KeyId key2) const {
        return sqrt(PointToSegmentDistanceSq(x, y, key1, key2));
    }

    float Keyboard::KeyToSegmentDistance(const KeyId key, const KeyId key1,
                                         const KeyId key2) const {
        return sqrt(KeyToSegmentDistanceSq(key, key1, key2));
    }

    float Keyboard::GestureEditDistance(const vector<char32>& word1,
                                        const vector<char32>& word2) const {
        vector<int> keys_i;
        vector<int> keys_j;
        for (const auto& code : word1) {
            const int key_index = GetKeyIndex(code);
            if (key_index != kInvalidKeyId) {
                keys_i.push_back(key_index);
            }
        }
        for (const auto& code : word2) {
            const int key_index = GetKeyIndex(code);
            if (key_index != kInvalidKeyId) {
                keys_j.push_back(key_index);
            }
        }
        const int size_i = keys_i.size();
        const int size_j = keys_j.size();
        const int cols = (size_j + 1);
        const int matrix_size = (size_i + 1) * (size_j + 1);

        vector<float> dist_matrix(matrix_size);

        for (int i = 1; i <= size_i; ++i) {
            dist_matrix[i * cols] = INF;
        }
        for (int j = 1; j <= size_j; ++j) {
            dist_matrix[j] = INF;
        }
        for (int i = 0; i < size_i; ++i) {
            const int key_i = keys_i[i];
            const int next_key_i = keys_i[std::min(size_i - 1, i + 1)];
            const float dir_i = KeyToKeyDirectionByIndex(key_i, next_key_i);
            for (int j = 0; j < size_j; ++j) {
                const int key_j = keys_j[j];
                const int next_key_j = keys_j[std::min(size_j - 1, j + 1)];
                const float dir_j = KeyToKeyDirectionByIndex(key_j, next_key_j);
                const bool same_direction = MathUtils::GetAngleDiff(dir_i, dir_j) < PI;
                const float align_cost =
                        dist_matrix[i * cols + j] + KeyToKeyDistanceByIndex(key_i, key_j);
                float skip_i_cost = INF;
                float skip_j_cost = INF;
                if (i < size_i && same_direction) {
                    skip_i_cost = dist_matrix[i * cols + (j + 1)] +
                                  KeyToSegmentDistance(key_i, key_j, next_key_j);
                }
                if (j < size_j && same_direction) {
                    skip_j_cost = dist_matrix[(i + 1) * cols + j] +
                                  KeyToSegmentDistance(key_j, key_i, next_key_i);
                }
                dist_matrix[(i + 1) * cols + (j + 1)] =
                        std::min(align_cost, std::min(skip_i_cost, skip_j_cost));
            }
        }
        return dist_matrix[size_i * cols + size_j] / most_common_key_width_;
    }

    void Keyboard::UpdateGeometricProperties() {
        key_key_distances_.resize(num_keys_);
        key_key_directions_.resize(num_keys_);
        for (int i = 0; i < num_keys_; ++i) {
            key_key_distances_[i].resize(num_keys_);
            key_key_directions_[i].resize(num_keys_);
            for (int j = 0; j < num_keys_; ++j) {
                if (i == j) {
                    key_key_distances_[i][j] = 0;
                    key_key_directions_[i][j] = 0;
                } else {
                    const float x1 = center_xs_[i];
                    const float y1 = center_ys_[i];
                    const float x2 = center_xs_[j];
                    const float y2 = center_ys_[j];
                    key_key_distances_[i][j] = MathUtils::Distance(x1, y1, x2, y2);
                    key_key_directions_[i][j] = MathUtils::GetAngle(x1, y1, x2, y2);
                }
            }
        }
    }

    void Keyboard::AddKey(const char32 code, const float center_x,
                          const float center_y, const float width,
                          const float height) {
        key_codes_.push_back(code);
        center_xs_.push_back(center_x);
        center_ys_.push_back(center_y);
        widths_.push_back(width);
        heights_.push_back(height);
        ++num_keys_;
    }

    bool Keyboard::CodeAlignsToKey(const char32 code, const KeyId key) const {
        const char32 lower_code = LatinImeCharConverter::toLowerCase(code);
        const char32 base_lower_code =
                LatinImeCharConverter::toBaseLowerCase(lower_code);
        const KeyId base_lower_key = GetKeyIndex(base_lower_code);
        if (base_lower_key == key) {
            return true;
        }
        const KeyId lower_key = GetKeyIndex(lower_code);
        if (lower_key == key) {
            return true;
        }
        return false;
    }

    vector<KeyId> Keyboard::GetPossibleKeysForCode(const char32 code) const {
        vector<KeyId> possible_keys;
        const char32 lower_code = LatinImeCharConverter::toLowerCase(code);
        const char32 base_lower_code =
                LatinImeCharConverter::toBaseLowerCase(lower_code);
        const KeyId base_lower_key = GetKeyIndex(base_lower_code);

        if (base_lower_key >= 0) {
            // Always consider the internal.base lowercase version of the code.
            possible_keys.push_back(base_lower_key);
            if (base_lower_code == lower_code) {
                return possible_keys;
            }
        }

        // If the code has a digraph, add the first digraph key:
        const vector<char32>& digraph_codes =
                LatinImeCharConverter::GetDigraphForCode(lower_code);
        if (!digraph_codes.empty()) {
            const KeyId first_digraph_key = GetKeyIndex(digraph_codes[0]);
            if (first_digraph_key >= 0 && first_digraph_key != base_lower_key) {
                possible_keys.push_back(first_digraph_key);
            }
        }
        // Check whether the (non-internal.base) lowercase version of the code is also on the
        // keyboard. For example, some Spanish keyboards have both 'ñ' and 'n' keys.
        const KeyId lower_key = GetKeyIndex(lower_code);
        if (lower_key >= 0 && lower_key != base_lower_key) {
            possible_keys.push_back(lower_key);
        }
        return possible_keys;
    }

    KeyId Keyboard::GetSecondDigraphKeyForCode(const char32 code,
                                               const KeyId aligned_key) const {
        const char32 lower_code = LatinImeCharConverter::toLowerCase(code);
        const vector<char32>& digraph_codes =
                LatinImeCharConverter::GetDigraphForCode(lower_code);
        if (!digraph_codes.empty()) {
            const KeyId first_digraph_key = GetKeyIndex(digraph_codes[0]);
            const KeyId second_digraph_key = GetKeyIndex(digraph_codes[1]);
            if (aligned_key == first_digraph_key) {
                return second_digraph_key;
            }
        }
        return Keyboard::kInvalidKeyId;
    }

}  // namespace decoder
}  // namespace keyboard
