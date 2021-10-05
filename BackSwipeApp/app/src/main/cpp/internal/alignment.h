// Description:
//   The alignment keeps track of the current alignment between the token's
//   prefix and the current point in the input sequence. This alignment is
//   updated each time a new time frame (e.g., touch-event) is processed.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_ALIGNMENT_H_
#define INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_ALIGNMENT_H_

#include <algorithm>

#include "base/integral_types.h"
#include "base/constants.h"

namespace keyboard {
namespace decoder {

    class Alignment {
    public:
        // Creates a new alignment with default values.
        Alignment() : index_(-1), align_score_(0), transit_score_(0) {}

        // Creates a new alignment with the given values.
        //
        // Args:
        //   index         - The alignment's index in the touch sequence.
        //   align_score   - The alignment's spatial score if the key is reached.
        //   transit_score - The alignment's spatial score if the key is not
        //                   reached yet.
        Alignment(int index, float align_score, float transit_score)
                : index_(index),
                  align_score_(align_score),
                  transit_score_(transit_score) {}

        // The alignment's spatial score, when treating the current touch point as the
        // final alignment point for the current key.
        float align_score() const { return align_score_; }

        // The alignment's spatial score, when treating the current touch point as
        // still "in-transit" to the current key. This is only used for gestures.
        float transit_score() const { return transit_score_; }

        // The alignment's index in the touch sequence.
        int index() const { return index_; }

        // The alignment's best score = max(align_score, transit_score).
        float BestScore() const { return std::max(align_score_, transit_score_); }

        // Resets the current score of the alignment to NEG_INF.
        void InvalidateScore() {
            transit_score_ = NEG_INF;
            align_score_ = NEG_INF;
        }

        // Add the score to the current alignment.
        void AddScore(const float score) {
            align_score_ += score;
            transit_score_ += score;
        }

        // Initialize this as a repeated letter.
        void InitializeAsRepeatedLetter(const Alignment& prev_alignment) {
            index_ = prev_alignment.index_;
            align_score_ = prev_alignment.align_score_;
            transit_score_ = prev_alignment.transit_score_;
        }

        // Clear the alignment to an invalid index.
        void Clear() {
            index_ = -1;
            align_score_ = 0;
            transit_score_ = 0;
        }

        void InvalidateTransitScore() { transit_score_ = NEG_INF; }

    private:
        // The alignment's index in the touch sequence.
        int16 index_;

        // The alignment's spatial score.
        float align_score_;

        // The alignment's transit score.
        float transit_score_;
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_ALIGNMENT_H_
