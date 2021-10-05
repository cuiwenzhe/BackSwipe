// Description:
//   Encodes the sequence of touch points that forms a gesture or tapped word.
//   Each touch sequence captures the touch points from a single pointer (i.e.,
//   finger).
//
//   It also calculates several geometric properties that are used during
//   decoding, such as directions, curvatures, locations of pauses and corners,
//   etc.
//
//   In addition, this class pre-computes two scores used in decoding:
//   - Alignment scores between each point and each key
//     (calculated based on squared Euclidean error)
//   - In-transit scores between each point and each pair of keys
//     (calculated based on the squared direction error)

#ifndef INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_TOUCH_SEQUENCE_H_
#define INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_TOUCH_SEQUENCE_H_

#include <string>
#include <vector>

#include "base/logging.h"
#include "keyboardSetting/keyboard.h"
#include "DecoderParams.h"
//#include "testing/production_stub/public/gunit_prod.h"

namespace keyboard {
namespace decoder {

    class Keyboard;

    class TouchSequence {
    public:
        enum TouchAction {TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP};
// The number of training points to recompute per update, since some of the
        // metrics calculated (like direction and curvature) depend on future points.
        static constexpr int kPointsToRecompute = 3;

        // Create an empty touch sequence.
        explicit TouchSequence(bool is_gesture)
                : is_gesture_(is_gesture),
                  pointer_id_(0),
                  last_update_size_(0),
                  num_keys_(0) {}

        TouchSequence(const vector<int> &xs, vector<int> &ys, vector<int> &times,
                      const int pointer_id,
                      const float gesture_sample_dist);
//        // Create a TouchSequence from the input TouchData. If the input is a gesture,
//        // the stroke will be sub-sampled so that the minimum distance between
//        // successive points is sample_dist.
//        // Note: The gesture_sample_dist parameter is in pixels, and is usually
//        // equal to GestureParams.sample_distance_in_key_widths() *
//        // Keyboard.most_common_key_width()
//        TouchSequence(const TouchData& touch_data, const int pointer_id,
//                      const float gesture_sample_dist);

//        // Extends the TouchSequence by adding input TouchData, starting with
//        // touch_data.touch_points()[start_index].
//        void ExtendTouchPoints(const TouchData& touch_data, const int start_index,
//                               const float gesture_sample_dist);

        // Adds a point to the TouchSequence. The new point will only be added if it
        // its distance to the previous point is greater than sample_dist.
        void AddPoint(const int action, const float x, const float y, const int time,
                      const float sample_dist);

        // Update the geometric properties of this TouchSequence. Note that if the
        // touch sequence was previously updated and then extended, this method only
        // needs to update the newly added points.
        // These properties are used during the decoding process, and some depend on
        // the keyboard and params.
        void UpdateProperties(const Keyboard& keyboard, const DecoderParams& params,
                              bool is_three_decoder_enabled);

        // Note that the following methods are only valid after UpdateProperties
        // has been called.

        // The distance between the i-th and j-th points.
        float PointDistance(const int i, const int j) const;

        // The angle between the i-th and j-th points.
        float PointAngle(const int i, const int j) const;

        // Whether the touch sequence is a gesture.
        bool is_gesture() const { return is_gesture_; }

        // The number of points.
        int size() const { return xs_.size(); }

        // The pointer id for the touch sequence. Only touch points from this
        // pointer id are included.
        int pointer_id() const { return pointer_id_; }

        // The x-coordinate of the i-th point.
        float xs(const int i) const { return xs_[i]; }

        // The y-coordinate of the i-th point.
        float ys(const int i) const { return ys_[i]; }

        // The cumulative length from the first point to the i-th point.
        float lengths(const int i) const { return lengths_[i]; }

        // The direction (in radians) of the i-th point.
        float direction(const int i) const { return directions_[i]; }

        // The curvature (in radians) of the i-th point.
        float curvature(const int i) const { return curvatures_[i]; }

        // The duration (in milliseconds) of the i-th point.
        float durations(const int i) const { return durations_[i]; }

        // The nearest key (char code) of the i-th point.
        char32 nearest_key_codes(const int i) const { return nearest_keys_[i]; }

        // Whether the i-th point is a corner.
        bool is_corner(const int i) const { return is_corners_[i]; }

        // Whether the i-th point is a pause.
        bool is_pause(const int i) const { return is_pauses_[i]; }

        // Dumps the properties of the touch points into a string.
        string DebugString() const;

        // The total stroke length of the touch sequence.
        float TotalLength() const;

        // The 'literal' char codes for this touch sequence. This is the sequence of
        // the nearest (base lower) codes that the sequence passes through.
        vector<char32> GetLiteralCodes() const;

        // Returns the tapped code at the given touch index. This is the actual
        // character code that was generated by the keyboard client, and may
        // include characters that were not entered on the standard keyboard layout
        // (i.e., the long-press diacritics menu, secondary symbol layout, etc).
        char32 GetTappedCodeAt(int index) const {
            if (index >= tapped_codes_.size()) {
                return -1;
            }
            return tapped_codes_[index];
        }

        // Returns the likelihood that point [i] in the gesture is in-transit between
        // the key indices key_1 and key_2 on the keyboard (see
        // Keyboard::GetKeyIndex).
        inline float transit_score(const int i, const KeyId key_1,
                                   const KeyId key_2) const {
                    DCHECK_GE(i, 0);
                    DCHECK_LT(i, size());
                    DCHECK_GE(key_1, 0);
                    DCHECK_LT(key_1, num_keys_);
                    DCHECK_GE(key_2, 0);
                    DCHECK_LT(key_2, num_keys_);
            return transit_scores_[i][key_2 * num_keys_ + key_1];
        }

        // Returns likelihood that point [i] in the touch sequence represents a tap or
        // gesture alignment to the given key.
        inline float align_score(const int i, const KeyId key) const {
                    DCHECK_GE(i, 0);
                    DCHECK_LT(i, size());
                    DCHECK_GE(key, 0);
                    DCHECK_LT(key, num_keys_);
            return align_scores_[i][key];
        }

        // Returns whether the touch sequence ends at the middle of a gesture (before
        // reaching the final up event).
        bool IsMidGesture() const;

    private:
        void UpdateGestureGeometry(const int start_index,
                                   const Keyboard& keyboard_layout,
                                   const DecoderParams& params);

        // Updates the pre-computed transit scores between each point and each pair of
        // keys on the keyboard. See transit_score(...) for more details.
        void UpdateGestureScores(const Keyboard& keyboard_layout,
                                 const DecoderParams& params);

        // Updates the pre-computed alignment scores between each point and each key.
        // See align_score(...) for more details.
        void UpdateAlignScores(const Keyboard& keyboard_layout,
                               const DecoderParams& params,
                               bool is_three_decoder_enabled);

        inline void set_transit_score(const int i, const int key_index_1,
                                      const int key_index_2, const float value) {
            transit_scores_[i][key_index_2 * num_keys_ + key_index_1] = value;
        }

        inline void set_align_score(const int i, const int key_index,
                                    const float value) {
            align_scores_[i][key_index] = value;
        }

        const bool is_gesture_;

        int pointer_id_;

        // The size of the touch_sequence when its properties were last updated.
        int last_update_size_;

        int num_keys_;

        vector<int> actions_;
        vector<float> xs_;
        vector<float> ys_;
        vector<int> times_;
        vector<float> lengths_;

        vector<char32> nearest_keys_;
        vector<float> curvatures_;
        vector<float> directions_;
        vector<float> durations_;

        vector<bool> is_corners_;
        vector<bool> is_pauses_;

        vector<vector<float>> align_scores_;
        vector<vector<float>> transit_scores_;

        vector<char32> tapped_codes_;
    };

}  // namespace decoder
}  // namespace keyboard
#endif  // INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_TOUCH_SEQUENCE_H_
