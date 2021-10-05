// Description:
//   Defines a keyboard layout (e.g., a list of key positions) used for touch
//   and gesture decoding.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_KEYBOARD_H_
#define INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_KEYBOARD_H_

#include <memory>
#include <string>
#include <vector>

#include "../base/integral_types.h"
#include "../base/logging.h"
#include "../base/macros.h"
#include "../math-utils.h"

namespace keyboard {
namespace decoder {

    using std::vector;
    class KeyboardLayout;

        // This class doesn't have a public constructor.  Use CreateKeyboardOrNull
        // instead to instantiate it.

        typedef int16 KeyId;

        class Keyboard {
        public:
            // An invalid id for a key not on the keyboard.
            static constexpr KeyId kInvalidKeyId = -1;

            // Creates and returns a new keyboard, or nullptr if keyboard_layout was
            // malformed.  The caller assumes ownership of the returned pointer.
            static std::unique_ptr<Keyboard> CreateKeyboardOrNull(
                    const KeyboardLayout& keyboard_layout);

            // Get the code of the key nearest to the coordinate (x, y).
            char32 GetNearestKeyCode(const float x, const float y) const;

            // Get the key index for the given character code.
            KeyId GetKeyIndex(const char32 code) const;

            // Get the char code for the given key index.
            char32 GetKeyCode(const KeyId key_id) const {
                        DCHECK(IsValidKeyIndex(key_id));
                return key_codes_[key_id];
            }

            // Returns true if given key index is valid.
            bool IsValidKeyIndex(const KeyId key) const {
                return key >= 0 && key < num_keys_;
            }

            // Get the (raw) distance between the key indices i and j.
            float KeyToKeyDistanceByIndex(const KeyId i, const KeyId j) const {
                        DCHECK(IsValidKeyIndex(i));
                        DCHECK(IsValidKeyIndex(j));
                return key_key_distances_[i][j];
            }

            // Get the direction of the line between the key indices i and j.
            float KeyToKeyDirectionByIndex(const KeyId i, const KeyId j) const {
                        DCHECK(IsValidKeyIndex(i));
                        DCHECK(IsValidKeyIndex(j));
                return key_key_directions_[i][j];
            }

            // Get the direction of the line between the keys for character codes i and j.
            float KeyToKeyDirectionByCode(const KeyId i, const KeyId j) const;

            // Get the distance between the point (x, y) and the key index.
            float PointToKeyDistanceByIndex(const float x, const float y,
                                            const KeyId key) const {
                        DCHECK(IsValidKeyIndex(key)) << "Invalid key " << key;
                const float width = widths_[key];
                if (width <= most_common_key_width_ * 2) {
                    // FOR INVISIBLE GESTURE KEYBOARD:
                    return MathUtils::DistanceStep(x, y, center_xs_[key], center_ys_[key], (int) key);
                    // return MathUtils::Distance(x, y, center_xs_[key], center_ys_[key], (int) key);
                } else {
                    // Special handling for keys wider than 2x the standard key width.
                    const float span = (width - most_common_key_width_) / 2;
                    const float left_x = center_xs_[key] - span;
                    const float right_x = center_xs_[key] + span;
                    return sqrt(MathUtils::PointToSegmentDistSq(x, y, left_x, center_ys_[key],
                                                                right_x, center_ys_[key]));
                }
            }

            // FOR INVISIBLE GESTURE KEYBOARD:
            // Get the probability between the point (x, y) and the key index.
            // Key index ordered is from Q to Z.
            float PointToKeyDistanceByRange(const float x, const float y, const KeyId key,
                                            vector<double> keyboard_range, const double keyboard_count) const {
                DCHECK(IsValidKeyIndex(key)) << "Invalid key " << key;
                const float width = widths_[key];
                if (width <= most_common_key_width_ * 2) {
                    return MathUtils::PointToRangeDistance(x, y, center_xs_[key], center_ys_[key], (int) key,
                                                           keyboard_range, keyboard_count);
                } else {
                    // Special handling for keys wider than 2x the standard key width.
                    const float span = (width - most_common_key_width_) / 2;
                    const float left_x = center_xs_[key] - span;
                    const float right_x = center_xs_[key] + span;
                    return sqrt(MathUtils::PointToSegmentDistSq(x, y, left_x, center_ys_[key],
                                                                right_x, center_ys_[key]));
                }
            }

            // Get the probability between the point (x, y) and the key index.
            float PointToKeyProbByIndex(const float x, const float y, const int key,
                                        const char code) const {
                DCHECK(IsValidKeyIndex(key)) << "Invalid key " << key;
                const float width = widths_[key];
                if (width <= most_common_key_width_ * 2) {
                    if ((int)code <= (int)'z' && (int)code >= (int)'a') {
                        int ind = (int)code - (int)'a';
                        return MathUtils::Probability(x, y, ind);
                    }
                    else{
                        return MathUtils::Distance(x, y, center_xs_[key], center_ys_[key]);
                    }
                } else {
                    // Special handling for keys wider than 2x the standard key width.
                    const float span = (width - most_common_key_width_) / 2;
                    const float left_x = center_xs_[key] - span;
                    const float right_x = center_xs_[key] + span;
                    return sqrt(MathUtils::PointToSegmentDistSq(x, y, left_x, center_ys_[key],
                                                                right_x, center_ys_[key]));
                }
            }

            // Get the edit distance in the ideal (keycenter to keycenter) gestures for
            // the two words. This is calculated using the Dynamic Time Warping (DTW), a
            // dynamic programming algorithm to find the optimal alignment between two
            // temporal sequences.
            //
            // Note: This metric is similar to the Levenshtein distance for strings.
            // Except in this case, insertions, substitutions, and deletions are scored
            // according to the gesture path. There is no cost for edits that do not cause
            // a visible difference in the paths (e.g., "pt" -> "pit" on QWERTY).
            float GestureEditDistance(const vector<char32>& word1,
                                      const vector<char32>& word2) const;

            // Get the minimum distance between the key and the line segment connecting
            // key1 and key2.
            float KeyToSegmentDistance(const KeyId key, const KeyId key1,
                                       const KeyId key2) const;

            // Get the minimum distance between the point (x, y) and the line segment
            // connecting key1 and key2.
            float PointToSegmentDistance(const float x, const float y, const KeyId key1,
                                         const KeyId key2) const;

            // If the input code has a digraph, and the aligned_key is at the first
            // digraph-key, this method returns the second digraph key.
            // Otherwise, returns Keyboard::kInvalidKeyId.
            // TODO(ouyang): Expand to support larger multi-graphs.
            KeyId GetSecondDigraphKeyForCode(const char32 code,
                                             const KeyId aligned_key) const;


            // Returns whether or not the give code can be aligned to the given key.
            // This does not consider any digraph associated with the code.
            bool CodeAlignsToKey(const char32 code, const KeyId key) const;

            // Returns the set of possible keys that can align to the given code.
            //
            // Example: On certain Spanish keyboards, the code 'ñ' (LATIN SMALL LETTER N
            // WITH TILDE) can align to either the special 'ñ' key or the internal.base 'n' key.
            vector<KeyId> GetPossibleKeysForCode(const char32 code) const;

            // Get the diagonal length of the entire keyboard.
            float keyboard_diagonal_length() const {
                return MathUtils::Length(keyboard_width_, keyboard_height_);
            }

            // Get the width of the entire keyboard.
            float keyboard_width() const { return keyboard_width_; }

            // Get the height of the entire keyboard.
            float keyboard_height() const { return keyboard_height_; }

            // Get the center x coordinate for the given key id.
            // Returns -1 for keys not on the keyboard.
            inline float center_xs(const KeyId id) const {
                if (!IsValidKeyIndex(id)) {
                    return -1.0f;
                }
                return center_xs_[id];
            }

            // Get the center y coordinate for the given key id.
            // Returns -1 for keys not on the keyboard.
            inline float center_ys(const KeyId id) const {
                if (!IsValidKeyIndex(id)) {
                    return -1;
                }
                return center_ys_[id];
            }

            // Return the number of keys in the keyboard.
            int num_keys() const { return num_keys_; }

            // Return the width of a typical letter key on the keyboard.
            float most_common_key_width() const { return most_common_key_width_; }

            // Return the height of a typical letter key on the keyboard.
            float most_common_key_height() const { return most_common_key_height_; }

        private:
            // Creates the keyboard from the keyboard layout data.  Note, that the
            // keyboard is not fully initialized after construction, so callers should use
            // CreateKeyboardOrNull.
            explicit Keyboard(const KeyboardLayout& keyboard_layout);

            // Updates the geometric properties of the keyboard.  This is used as part of
            // initialization.
            void UpdateGeometricProperties();

            // Adds a key to the keyboard.
            //
            // Args:
            //   code      - The unicode codepoint for the key.
            //   center_x  - The key-center x coordinate.
            //   center_y  - The key-center y coordinate.
            //   width     - The actual (not just visible) key width, including gaps.
            //   height    - The actual (not just visible) key height, including gaps.
            void AddKey(const char32 code, const float center_x, const float center_y,
                        const float width, const float height);

            // Get the minimum squared distance between the key and the line segment
            // connecting key1 and key2.
            float KeyToSegmentDistanceSq(const KeyId key, const KeyId key1,
                                         const KeyId key2) const;

            // Get the minimum squared distance between the point (x, y) and the line
            // segment connecting key1 and key2.
            float PointToSegmentDistanceSq(const float x, const float y, const KeyId key1,
                                           const KeyId key2) const;

            // The total number of keys in the keyboard.
            int num_keys_;

            // The width of a standard key. This is the actual touchable size of the key,
            // including any gaps between keys.
            float most_common_key_width_;

            // The height of a standard key. This is the actual touchable size of the key,
            // including any gaps between keys.
            float most_common_key_height_;

            // The total width of the keyboard.
            float keyboard_width_;

            // The total height of the keyboard.
            float keyboard_height_;

            // A vector of unicode characters codes for each key index.
            vector<char32> key_codes_;

            // A vector of key center x coordinates for each key index.
            vector<float> center_xs_;

            // A vector of key center y coordinates for each key index.
            vector<float> center_ys_;

            // A vector of key widths for each key index.
            vector<float> widths_;

            // A vector of key heights for each key index.
            vector<float> heights_;

            // A 2D vector of key to key distances for each pair of key indices.
            vector<vector<float>> key_key_distances_;

            // A 2D vector of key to key directions for each pair of key indices.
            // This is defined as the direction of a straight line going from the first
            // key index to the second key index.
            vector<vector<float>> key_key_directions_;

            DISALLOW_COPY_AND_ASSIGN(Keyboard);
        };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_KEYBOARD_H_
