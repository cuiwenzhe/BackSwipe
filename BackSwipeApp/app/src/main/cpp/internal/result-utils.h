// Declares utility functions for processing decoder results.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_RESULT_UTILS_H_
#define INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_RESULT_UTILS_H_

#include <vector>

#include "decoder-result.h"

namespace keyboard {
namespace decoder {
    using namespace std;
    // Suppresses uppercase results that score too low below their lowercase
    // variants. This prevents the decoder from suggesting lots of uppercase
    // variants for the same word (e.g., you | You | YOU) unless there is a
    // reasonable likelihood that the uppercase variant will be useful.
    //
    // Args:
    //   results:
    //     The set of results to filter.
    //   uppercase_suppression_score_threshold:
    //     Suppress uppercase suggestions (e.g., "You", "YOU") that score less
    //     than this far below their lowercase variant (e.g., "you").
    //
    // Returns:
    //   A copy of results with uppercase variants suppressed.
    std::vector<DecoderResult> SuppressUppercaseResults(
            const std::vector<DecoderResult>& results,
            float uppercase_suppression_score_threshold);

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_RESULT_UTILS_H_
