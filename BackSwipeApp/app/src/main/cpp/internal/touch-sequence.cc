#include "touch-sequence.h"

#include <algorithm>

#include "base/stringprintf.h"
#include "keyboardSetting/keyboard.h"
#include "base/latinime-charconverter.h"
#include "math-utils.h"
#include "DecoderParams.h"
//#include "inputmethod/keyboard/decoder/proto/touch-points.pb.h"
#include "base/constants.h"

namespace keyboard {
namespace decoder {

    using base::StringPrintf;

    TouchSequence::TouchSequence(const vector<int>& xs, vector<int>& ys, vector<int>& times, const int pointer_id,
                                 const float gesture_sample_dist)
            : is_gesture_(true),
              pointer_id_(pointer_id),
              last_update_size_(0) {
        const float sample_dist = is_gesture_ ? gesture_sample_dist : 0.0;
        for (int i = 0; i < xs.size(); ++i) {
            TouchAction action = TOUCH_MOVE;
            if(i == 0){
                action = TOUCH_DOWN;
            }else if(i == xs.size() - 1){
                action = TOUCH_UP;
            }
            AddPoint(action, xs[i], ys[i],
                         times[i], sample_dist);
        }
    }


//    TouchSequence::TouchSequence(const TouchData& touch_data, const int pointer_id,
//                                 const float gesture_sample_dist)
//            : is_gesture_(touch_data.is_gesture()),
//              pointer_id_(pointer_id),
//              last_update_size_(0) {
//        const float sample_dist = is_gesture_ ? gesture_sample_dist : 0.0;
//        for (int i = 0; i < touch_data.touch_points_size(); ++i) {
//            const TouchPoint& touch_point = touch_data.touch_points(i);
//            if (touch_point.pointer() != pointer_id) {
//                continue;
//            }
//            if (is_gesture_ || touch_point.action() == TouchPoint::ACTION_DOWN) {
//                // For non-gesture (tap) data, treat the ACTION_DOWN event as the
//                // representative point for the key press.
//                AddPoint(touch_point.action(), touch_point.x(), touch_point.y(),
//                         touch_point.time(), sample_dist);
//                if (!is_gesture_) {
//                    tapped_codes_.push_back(touch_point.tapped_code());
//                }
//            }
//        }
//    }
//
//    void TouchSequence::ExtendTouchPoints(const TouchData& touch_data,
//                                          const int start_index,
//                                          const float gesture_sample_dist) {
//        const float sample_dist = is_gesture_ ? gesture_sample_dist : 0.0;
//        for (int i = start_index; i < touch_data.touch_points_size(); ++i) {
//            const TouchPoint& touch_point = touch_data.touch_points(i);
//            if (touch_point.pointer() != pointer_id_) {
//                continue;
//            }
//            if (is_gesture_ || touch_point.action() == TouchPoint::ACTION_DOWN) {
//                // For non-gesture (tap) data, treat the ACTION_DOWN event as the
//                // representative point for the key press.
//                AddPoint(touch_point.action(), touch_point.x(), touch_point.y(),
//                         touch_point.time(), sample_dist);
//                if (!is_gesture_) {
//                    tapped_codes_.push_back(touch_point.tapped_code());
//                }
//            }
//        }
//    }

    void TouchSequence::AddPoint(const int action, const float x, const float y,
                                 const int time, const float sample_dist) {
        float length = 0;
        const int point_count = xs_.size();
        if (point_count > 0) {
            const int last = point_count - 1;
            const int last_length = lengths_[last];
            const float distance = MathUtils::Distance(x, y, xs_[last], ys_[last]);
            length = last_length + distance;
            if (distance < sample_dist) {
                if (action == TOUCH_UP) {
                    actions_[last] = action;
                    xs_[last] = x;
                    ys_[last] = y;
                    times_[last] = time;
                    lengths_[last] = length;
                }
                return;
            }
        }
        actions_.push_back(action);
        xs_.push_back(x);
        ys_.push_back(y);
        times_.push_back(time);
        lengths_.push_back(length);
    }

    float TouchSequence::PointDistance(const int i, const int j) const {
        return MathUtils::Distance(xs_[i], ys_[i], xs_[j], ys_[j]);
    }

    float TouchSequence::PointAngle(const int i, const int j) const {
        return MathUtils::GetAngle(xs_[i], ys_[i], xs_[j], ys_[j]);
    }

    vector<char32> TouchSequence::GetLiteralCodes() const {
        vector<char32> literals;
        const int point_count = size();
        if (nearest_keys_.size() == point_count) {
            int prev_code = 0;
            for (int i = 0; i < point_count; ++i) {
                if (is_gesture_ && !is_corners_[i] && !is_pauses_[i] && i != 0 &&
                    i != point_count - 1) {
                    continue;
                }
                if (nearest_keys_[i] > 0) {
                    const int base_lower_code =
                            LatinImeCharConverter::toBaseLowerCase(nearest_keys_[i]);
                    if (!is_gesture_ || base_lower_code != prev_code) {
                        // Skip repeated insertions of the same code if this is a gesture.
                        literals.push_back(base_lower_code);
                    }
                    prev_code = base_lower_code;
                }
            }
        }
        return literals;
    }

    bool TouchSequence::IsMidGesture() const {
        return is_gesture_ && !actions_.empty();
//        return is_gesture_ && !actions_.empty() &&
//               actions_.back() != TouchPoint::ACTION_UP;
    }

    void TouchSequence::UpdateProperties(const Keyboard& keyboard,
                                         const DecoderParams& params,
                                         bool is_three_decoder_enabled) {
        const int point_count = xs_.size();

        nearest_keys_.resize(point_count);
        curvatures_.resize(point_count);
        directions_.resize(point_count);
        durations_.resize(point_count);
        is_corners_.resize(point_count);
        is_pauses_.resize(point_count);
        num_keys_ = keyboard.num_keys();

        const int start_index = std::max(0, last_update_size_ - kPointsToRecompute);
        for (int i = start_index; i < point_count; ++i) {
            nearest_keys_[i] = keyboard.GetNearestKeyCode(xs_[i], ys_[i]);
        }

        if (is_gesture_) {
            UpdateGestureGeometry(start_index, keyboard, params);
            UpdateGestureScores(keyboard, params);
        }

        UpdateAlignScores(keyboard, params, is_three_decoder_enabled);
        last_update_size_ = point_count;
    }

    void TouchSequence::UpdateGestureGeometry(const int start_index,
                                              const Keyboard& keyboard_layout,
                                              const DecoderParams& params) {
        const int point_count = xs_.size();
        if (point_count < 2) {
            return;
        }
        const float corner_curvature =
                params.min_curvature_for_corner;
        const float pause_duration =
                params.pause_duration_in_millis;

        // Compute directions
        for (int i = std::max(1, start_index); i < point_count - 1; ++i) {
            directions_[i] = PointAngle(i - 1, i + 1);
        }
        directions_[0] = directions_[1];
        directions_[point_count - 1] = directions_[point_count - 2];
        // Compute curvatures
        for (int i = std::max(1, start_index); i < point_count - 1; ++i) {
            curvatures_[i] =
                    MathUtils::GetAngleDiff(directions_[i - 1], directions_[i + 1]);
        }
        // Compute durations
        for (int i = std::max(1, start_index); i < point_count - 1; ++i) {
            durations_[i] = (times_[i + 1] - times_[i - 1]);
        }
        // Find pauses and corners
        for (int i = std::max(2, start_index); i < point_count - 1; ++i) {
            is_pauses_[i] = durations_[i] >= pause_duration &&
                            durations_[i] > durations_[i - 1] &&
                            durations_[i] >= durations_[i + 1];
            is_corners_[i] = curvatures_[i] >= corner_curvature &&
                             curvatures_[i] > curvatures_[i - 1] &&
                             curvatures_[i] >= curvatures_[i + 1];
        }
    }

    //TODO: calculate spatial (transit, align) scores for gesture input.
    void TouchSequence::UpdateGestureScores(const Keyboard& keyboard_layout,
                                            const DecoderParams& params) {
        const float key_sigma = params.key_error_sigma;
        const float direction_sigma = params.direction_error_sigma;
        const float skip_pause_score = params.skip_pause_score;
        const float skip_corner_score = params.skip_corner_score;
        const float key_width = keyboard_layout.most_common_key_width();
        const int num_keys = keyboard_layout.num_keys();
        const int point_count = xs_.size();

        transit_scores_.resize(point_count);
        align_scores_.resize(point_count);

        const int start_index = std::max(0, last_update_size_ - kPointsToRecompute);

        // These variables are defined to accelerate the calculation process.
        const int transit_score_count = num_keys * num_keys;
        const float root_direction_error_weight = sqr(1 / direction_sigma);
        const float direction_error_weight_scale =
                root_direction_error_weight / key_width;

        for (int i = start_index; i < point_count; ++i) {
            const float direction_error_weight =
                    (i == 0)
                    ? root_direction_error_weight
                    : (lengths_[i] - lengths_[i - 1]) * direction_error_weight_scale;
            transit_scores_[i].resize(transit_score_count);
            const float pause_score = is_pauses_[i] ? skip_pause_score : 0.0;
            const float corner_score =
                    is_corners_[i] ? curvatures_[i] * skip_corner_score : 0.0;
            for (int k1 = 0; k1 < num_keys; ++k1) {
                for (int k2 = 0; k2 < num_keys; ++k2) {
                    if (k1 != k2) {
                        const float ideal_direction =
                                keyboard_layout.KeyToKeyDirectionByIndex(k1, k2);
                        const float direction_error = std::min(
                                PI / 4, MathUtils::GetAngleDiff(directions_[i], ideal_direction));
                        const float direction_score =
                                -sqr(direction_error) * direction_error_weight;
                        set_transit_score(i, k1, k2,
                                          direction_score + pause_score + corner_score);
                    }
                }
            }
        }

        // FOR INVISIBLE GESTURE KEYBOARD
        const float spatial_model_weight = .7f;
        const float distance_weight = 1 * spatial_model_weight / (key_width * key_sigma);
        for (int i = start_index; i < point_count; ++i) {
            align_scores_[i].resize(num_keys);
            const float x = xs_[i];
            const float y = ys_[i];
            for (int k1 = 0; k1 < num_keys; ++k1) {
                const char key_code = keyboard_layout.GetKeyCode(k1);
                const float score =
                        -sqr(keyboard_layout.PointToKeyDistanceByIndex(x, y, k1) *
                             distance_weight);
                set_align_score(i, k1, score);
            }
        }
    }

    void TouchSequence::UpdateAlignScores(const Keyboard& keyboard_layout,
                                          const DecoderParams& params,
                                          bool is_three_decoder_enabled) {
        const float key_sigma = is_gesture_
                                ? params.key_error_sigma
                                : params.key_error_sigma;
        const float key_width = keyboard_layout.most_common_key_width();
        const int num_keys = keyboard_layout.num_keys();
        const int point_count = xs_.size();

        align_scores_.resize(point_count);

        int start_index = std::max(0, last_update_size_ - kPointsToRecompute);
        if (is_three_decoder_enabled) {
            start_index = 0;
        }

        const float spatial_model_weight = .7f;
        const float distance_weight = spatial_model_weight / (key_width * key_sigma);

        double keyboard_count = 5;
        vector<double> keyboard_range(keyboard_count);
        double keyboard_q_center = keyboard_layout.center_ys(0);
        double keyboard_range_step = 50; // from user study
        for (int i = 0; i < keyboard_count; i++) {
            keyboard_range[i] = keyboard_q_center + (i - 2) * keyboard_range_step;
        }

        for (int i = start_index; i < point_count; ++i) {
            align_scores_[i].resize(num_keys);
            const float x = xs_[i];
            const float y = ys_[i];
            for (int k = 0; k < num_keys; ++k) {
                const char key_code = keyboard_layout.GetKeyCode(k);
                float score;
                // FOR INVISIBLE GESTURE KEYBOARD
                if (is_three_decoder_enabled) {
                    score = -sqr(keyboard_layout.PointToKeyDistanceByIndex(x, y, k) *
                                         distance_weight);
                    set_align_score(i, k, score);
                } else {
                    if ((key_code <= (int) 'z' && key_code >= (int) 'a')) {
                        score = -sqr(keyboard_layout.PointToKeyDistanceByRange(x, y, k, keyboard_range, keyboard_count) *
                                     distance_weight);
                    } else {
                        score = -sqr(keyboard_layout.PointToKeyDistanceByIndex(x, y, k) *
                                     distance_weight);
                    }
                    set_align_score(i, k, score);
                }
            }
        }
    }

    float TouchSequence::TotalLength() const {
        const int size = lengths_.size();
        if (size == 0) {
            return 0;
        }
        return lengths_[size - 1];
    }

    string TouchSequence::DebugString() const {
        string result = StringPrintf("TouchSequence: (%zu)\n", xs_.size());
        const bool has_properties = (directions_.size() == xs_.size());
        for (int i = 0; i < xs_.size(); ++i) {
            if (has_properties) {
                result.append(StringPrintf(
                        "    %d\t%c (%4.4f, %4.4f), time: %.4d, dir: %4.4f, cur: %4.4f, "
                                "dur: %4.4f (%d, %d) \n",
                        i, nearest_keys_[i], xs_[i], ys_[i], times_[i], directions_[i],
                        curvatures_[i], durations_[i], static_cast<int>(is_pauses_[i]),
                        static_cast<int>(is_corners_[i])));
            } else {
                result.append(
                        StringPrintf("    %d\t (%.4f, %.4f), time: %d, length: %.4f\n", i,
                                     xs_[i], ys_[i], times_[i], lengths_[i]));
            }
        }
        return result;
    }

}  // namespace decoder
}  // namespace keyboard
