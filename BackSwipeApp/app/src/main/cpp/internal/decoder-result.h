// A result represents a token that has reached the end of the touch sequence.
// The output of the decoding process is a list of n-best results,
// each representing a unique word (or word prefix) candidate.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_DECODER_RESULT_H_
#define INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_DECODER_RESULT_H_

#include <string>

#include "base/basictypes.h"

namespace keyboard {
namespace decoder {

    class LexiconInterface;

    class DecoderResult {
    public:
        // Creates a new DecoderResult with the given values.
        //
        // Args:
        //   word            - The suggested word.
        //   spatial_score   - The spatial model score.
        //   lm_score        - The language model score.
        //
        // TODO(ouyang): Rename 'word' now that we support multi-term decoding.
        DecoderResult(const Utf8String& word, float spatial_score, float lm_score)
                : word_(word), spatial_score_(spatial_score), lm_score_(lm_score) {}

        DecoderResult() : word_(), spatial_score_(0), lm_score_(0) {}

        // The suggested word.
        const Utf8String& word() const { return word_; }

        void set_word(const Utf8String& word) { word_ = word; }

        // The spatial model score.
        float spatial_score() const { return spatial_score_; }

        void set_spatial_score(const float spatial_score) {
            spatial_score_ = spatial_score;
        }

        // The language model score.
        float lm_score() const { return lm_score_; }

        void set_lm_score(const float lm_score) { lm_score_ = lm_score; }

        void adjust_spatial_score(const float adjustment) {
            spatial_score_ += adjustment;
        }

        // The total score.
        float score() const { return spatial_score_ + lm_score_; }

    private:
        // These fields are documented in their accessor functions above.
        Utf8String word_;
        float spatial_score_;
        float lm_score_;
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_DECODER_RESULT_H_
