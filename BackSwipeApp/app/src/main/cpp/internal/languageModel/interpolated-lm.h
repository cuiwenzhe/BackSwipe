// A combined LanguageModelInterface that contains multiple individual
// LanguageModelInterfaces. It performs linear weighted interpolation across its
// child language models (in regular probability space).

#ifndef INPUTMETHOD_KEYBOARD_DECODER_LM_INTERPOLATED_INTERPOLATED_LM_H_
#define INPUTMETHOD_KEYBOARD_DECODER_LM_INTERPOLATED_INTERPOLATED_LM_H_

#include <memory>
#include <utility>
#include <vector>

#include "../base/basictypes.h"
#include "../language-model-interface.h"

using namespace std;

namespace keyboard {
namespace decoder {
namespace lm {

    class InterpolatedLm : public LanguageModelInterface {
    public:
        // Creates an interpolated LM that combines the given LanguageModelInterfaces
        // and their respective interpolation weights.
        //
        // Note: Weighted interpolation is performed in regular (non-log) probability
        // space.
        explicit InterpolatedLm(
                const vector<pair<const LanguageModelInterface*, float>>& weighted_lms)
                : weighted_lms_(weighted_lms) {}

        ////////////////////////////////////////////////////////////////////////////
        // The following method is inherited from LanguageModelInterface.
        // Please refer to the comments for that class.
        ////////////////////////////////////////////////////////////////////////////

        LanguageModelScorerInterface* NewScorerOrNull(
                const Utf8StringPiece& preceding_text,
                const Utf8StringPiece& following_text) const override;

    private:
        // The vector of LMs and their respective weights to interpolate over.
        vector<pair<const LanguageModelInterface*, float>> weighted_lms_;
    };

}  // namespace lm
}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_LM_INTERPOLATED_INTERPOLATED_LM_H_
