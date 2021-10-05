// Description:
//   Math an geometry utilities used by the keyboard decoder.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_MATH_UTILS_H_
#define INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_MATH_UTILS_H_

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

#include "base/macros.h"

#include <android/log.h>
#undef LOG_TAG
#define LOG_TAG "math-utils"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

using std::vector;

namespace keyboard {
namespace decoder {

    static constexpr float PI = 3.14159265358979323846f;
    static inline float sqr(const float x) { return x * x; }
    static inline double sqr(const double x) { return x * x; }

    class MathUtils {
    public:
        // FOR INVISIBLE GESTURE KEYBOARD
        static vector<double> mean_xs_;
        static vector<double> mean_ys_;
        static vector<double> sd_xs_;
        static vector<double> sd_ys_;
        static vector<double> rhos_;

        // Get the angle for the line connecting points (x1, y1) and point (x2, y2).
        static float GetAngle(const float x1, const float y1, const float x2,
                              const float y2) {
            const float dx = x2 - x1;
            const float dy = y2 - y1;
            if (dx == 0 && dy == 0) return 0.0;
            return atan2(dy, dx);
        }

        // Get the difference between two angles, bounded between 0 and 2 * PI.
        static float GetAngleDiff(const float a1, const float a2) {
            const float diff = fabs(a1 - a2);
            if (diff >= PI) {
                return 2.0f * PI - diff;
            }
            return diff;
        }

        // Get the squared Euclidean distance between two points.
        static float DistanceSq(const float x1, const float y1, const float x2,
                                const float y2) {
            const float dx = x2 - x1;
            const float dy = y2 - y1;
            return dx * dx + dy * dy;
        }

        // Get the Euclidean distance between two points.
        static float Distance(const float x1, const float y1, const float x2,
                              const float y2) {
            return hypot(x1 - x2, y1 - y2);
        }

        // FOR INVISIBLE GESTURE KEYBOARD:
        // Get the Euclidean distance between a point (x1, y1) and the key center at index.
        // The key center is retrieved from the updated mean_xs_ and mean_ys_ array.
        static float Distance(const float x1, const float y1, const float x2,
                              const float y2, const int index) {
            if (index < mean_xs_.size()) {
                float x_mean = (float) mean_xs_[index];
                float y_mean = (float) mean_ys_[index];
                return hypot(x1 - x_mean, y1 - y2);
            }
            return hypot(x1 - x2, y1 - y2);
        }

        static void PrintMeans() {
            for (int k = 0; k < mean_xs_.size(); k++) {
                LOGD("*** key[%d], mean (%f, %f)", k, mean_xs_[k], mean_ys_[k]);
            }
        }

        // FOR INVISIBLE GESTURE KEYBOARD:
        // A step function of the Euclidean distance between two points.
        static float DistanceStep(const float x1, const float y1,
                                  const float center_x, const float center_y, const int index) {
            float distance;
            float x_mean = center_x, y_mean = center_y;
            if (index < mean_xs_.size()) {
                float x_mean = (float) mean_xs_[index];
                // mean_ys_ is normalized to the keyboard at a fixed position:
                //  normalized keyboard             imaginary keyboard
                //                                  ___________________
                //  ___________________            |___________________|
                // |___________________|           |___________________|
                // |___________________|           |___________________|
                // |___________________|
                // q_center = 345, mean_ys[i]      q_center = center_y, y_mean=?
                // 345 - center_y = mean_ys[i] - ?
                float y_mean = (float) mean_ys_[index] + (center_y - 345);
            }

            if (index < sd_ys_.size() && index < sd_xs_.size()) {
                double x_threshold = sd_xs_[index];
                double y_threshold = sd_ys_[index];
                if (((x1 - x_mean) * (x1 - x_mean) / x_threshold * x_threshold +
                     (y1 - y_mean) * (y1 - y_mean) / y_threshold * y_threshold) <= 1) {
                    return 0.0;
                }
            }
            return hypot(x1 - x_mean, y1 - y_mean);
        }

        // FOR INVISIBLE GESTURE KEYBOARD:
        // Get the probability between a point (x1, y1) and the key center at index.
        // The key center is decided by a range of possible keyboard positions.
        // The result is normalized across all keyboard possibilities.
        static float PointToRangeDistance(const float x1, const float y1,
                                          const float x2, const float y2, const int index,
                                          const vector<double> keyboard_range, const double keyboard_count) {
            double total = 0;
            // Use data collection results as the key centers
            if (index < mean_xs_.size()) {
                for (double center : keyboard_range) {
                    double y_mean = mean_ys_[index] + (center - 345);
                    total += MathUtils::DistanceStep(x1, y1, mean_xs_[index], y_mean, index);
                }
                return total / keyboard_count;
            } else {
                LOGE("Not an alphabetical key.");
                return 0;
            }
        }

        // FOR INVISIBLE GESTURE KEYBOARD:
        // Wrapper function called by AndroidDecoder::DecoderSession::Keyboard.
        // If within one SD range, return probability 1.
        static float Probability(const float x, const float y, const int index) {
            if (index < mean_xs_.size()) {
                float x_mean = (float) mean_xs_[index];
                float y_mean = (float) mean_ys_[index];
                double x_sd = sd_xs_[index];
                double y_sd = sd_ys_[index];
                if (((x - x_mean) * (x - x_mean) / x_sd * x_sd +
                     (y - y_mean) * (y - y_mean) / y_sd * y_sd) <= 1) {
                    return -0.2f;
                } else {
                    return MathUtils::Probability(x, y, x_mean, y_mean,
                                                  x_sd, y_sd, rhos_[index]);
                }
            } else {
                return 0.0f;
            }
        }

        // Get the log probability of aligning a point to a key, given its distribution.
        static float Probability(const float x, const float y,
                                 const double miu1, const double miu2,
                                 const double sig1, const double sig2,
                                 const double rho) {
            double p = 1.0 - sqr(rho);
            double A = (sqr(x - miu1) / sqr(sig1)) + (sqr(y - miu2) / sqr(sig2))
                       - (2.0 * rho * (x - miu1) * (y - miu2) / (sig1 * sig2));
            double B = (-0.5) / p;

            float res = (float) log(0.5f / (PI * sig1 * sig2 * sqrt(p))) + (A * B);
            return 100.0f / res; // TODO: adjust params
        }

        // Get the mean of the vector values from start to end.
        static float Mean(const vector<float>& values, const int start,
                          const int end) {
            float mean = 0;
            for (int i = start; i < end; ++i) {
                mean += values[i];
            }
            return mean / (end - start);
        }

        // Get the median value of the array.
        static float Median(const float* array, const int size) {
            vector<float> values_vector(array, array + size);
            return Median(values_vector);
        }

        // Get the median value of the vector.
        static float Median(vector<float> values) {
            const int size = values.size();
            const int midpoint = size / 2;
            std::nth_element(values.begin(), values.begin() + midpoint, values.end());
            return values[midpoint];
        }

        // The Sigmoid density function.
        static inline float Sigmoid(float value, float midpoint, float slope) {
            return (1.0 / (1.0 + exp(-slope * (value - midpoint))));
        }

        // The Gaussian density function.
        static inline float Gaussian(float x, float sigma) {
            return exp(-(sqr(x)) / (2 * sqr(sigma))) / (sigma * sqrt(2 * PI));
        }

        // Get the Euclidean length of the line from (0, 0) to (x, y).
        static inline float Length(float x, float y) { return sqrt(sqr(x) + sqr(y)); }

        // Get the squared distance between the point (px, py) and the line
        // segment (x1, y1)-(x2, y2).
        static inline float PointToSegmentDistSq(float px, float py, float x1,
                                                 float y1, float x2, float y2) {
            if (x1 == x2 && y1 == y2) {
                return DistanceSq(px, py, x1, y1);
            }
            const float dx_p = px - x1;
            const float dy_p = py - y1;
            const float dx_l = x2 - x1;
            const float dy_l = y2 - y1;
            const float t = (dx_p * dx_l + dy_p * dy_l) / (sqr(dx_l) + sqr(dy_l));

            if (t < 0) {
                return DistanceSq(px, py, x1, y1);
            } else if (t > 1) {
                return DistanceSq(px, py, x2, y2);
            } else {
                const float px_l = x1 + dx_l * t;
                const float py_l = y1 + dy_l * t;
                return DistanceSq(px, py, px_l, py_l);
            }
        }

        // Get the distance between the point (px, py) and the line
        // segment (x1, y1)-(x2, y2).
        static inline float PointToLineDist(float px, float py, float x1, float y1,
                                            float x2, float y2) {
            return sqrt(PointToSegmentDistSq(px, py, x1, y1, x2, y2));
        }

    private:

        DISALLOW_IMPLICIT_CONSTRUCTORS(MathUtils);
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_MATH_UTILS_H_
