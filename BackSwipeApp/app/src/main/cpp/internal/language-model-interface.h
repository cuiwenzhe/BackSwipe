// Defines an interface for computing the language model probability of
// decoded terms, together with preceding and following context that is
// considered already committed.  The interface is optimized for repeated
// probability computations using the same context; callers create a new
// language model scorer object that is initialized with supplied preceding
// and following text.
//
// Sample usage:
//
//   Utf8String Decode(const LanguageModelInterface& lang_model,
//                     const DecodeRequest& request) {
//     std::unique_ptr<LanguageModelScorerInterface> scorer(
//         lang_model.NewScorerOrNull(request.preceding_text(),
//                                    request.following_text()));
//     LogProbFloat best_score = kLogProbFloatZero;
//     vector<Utf8StringPiece> best_candidate;
//     for (const vector<const Utf8StringPiece>& candidate :
//          GenerateCandidates(request)) {
//       LogProbFloat score = scorer->DecodedTermsLogProb(candidate);
//       if (score >= best_score) {
//         best_score = score;
//         best_candidate = candidate;
//       }
//     }
//     return strings::Join(best_candidate, " ");
//   }

#ifndef INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LANGUAGE_MODEL_INTERFACE_H_
#define INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LANGUAGE_MODEL_INTERFACE_H_

#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/basictypes.h"
#include "base/constants.h"
//#include "base/stringpiece.h"

namespace keyboard {
namespace decoder {

    using namespace std;

    // An interface for classes that can score decoded terms and predict the
    // next term.  This class is not thread-safe.
    //
    // Methods are non-const and implementations may modify internal state
    // that's used for caching computations.  However, calling methods on this
    // class must not have side effects on the probabilities or predictions
    // returned by subsequent calls.
    class LanguageModelScorerInterface {
    public:
        virtual ~LanguageModelScorerInterface() {}

        // Computes the probability under this scorer of a sequence of decoded
        // terms.
        //
        // Args:
        //   decoded_terms - The sequence of terms to score.  Must be non-empty.
        //
        // Returns:
        //   The log probability under this language model of decoded_terms.
        virtual LogProbFloat DecodedTermsLogProb(
                const vector<Utf8StringPiece>& decoded_terms) ABSTRACT;

        // Computes the conditional probability under this scorer of the last term in
        // the sequence of decoded terms.
        //
        // Args:
        //   decoded_terms - The sequence of terms to score.  Must be
        //   non-empty.
        //
        // Returns:
        //   The conditional log probability under this language model of the last
        //   term in decoded_terms sequence.
        virtual LogProbFloat DecodedTermsConditionalLogProb(
                const vector<Utf8StringPiece>& decoded_terms) ABSTRACT;

        // Computes up to the top max_predictions terms that most likely follow
        // decoded_terms under this scorer.
        //
        // Args:
        //   decoded_terms   - A sequence of terms that have been decoded so far,
        //                     and immediately precede the predicted terms.  May
        //                     be empty.
        //   max_predictions - Maximum number of results to return.  Must be > 0.
        //   predictions     - Cleared and populated with an entry for each term
        //                     predicted.  The first pair is the term's text, and
        //                     the second is its conditional log probability given
        //                     supplied the decoded terms and any additional context
        //                     this object was initialized with.  Ties in log
        //                     probability for the max_predictions-th entry may be
        //                     broken arbitrarily.
        //
        // Implementations are not required to implement this method; the default
        // implementation returns no predictions.
        virtual void PredictNextTerm(
                const vector<Utf8StringPiece>& decoded_terms, const int max_predictions,
                vector<pair<Utf8String, LogProbFloat>>* predictions) {}
    };

    // The core language model interface.  Implementations are not required to be
    // thread-safe.
    //
    // Warning: mutable operations in implementations that add them may invalidate
    // existing LanguageModelScorerInterface objects.
    class LanguageModelInterface {
    public:
        virtual ~LanguageModelInterface() {}

        // Creates a new scorer object optimized for the supplied context
        // strings.  The scorer computes joint probabilities of decoded terms and
        // the context, and generates predictions (if supported) taking into
        // account the context.
        //
        // Args:
        //   preceding_text - UTF-8 text that precedes decoded text.
        //   following_text - UTF-8 text that follows decoded text.
        //
        // Returns:
        //   A new LanguageModelScorerInterface.  The caller assumes ownership.
        //   May return nullptr if the arguments are invalid, e.g. if they fail
        //   to normalize.
        //
        // Note: implementations must assume the strings backing the supplied
        // StringPieces are only valid during this method call.
        virtual LanguageModelScorerInterface* NewScorerOrNull(
                const Utf8StringPiece& preceding_text,
                const Utf8StringPiece& following_text) const ABSTRACT;

        // Indicates whether this language model implementation supports
        // look-ahead predictions, i.e. the PredictNextTerm method.
        virtual bool SupportsPredictions() const { return false; }

        // Returns whether or not the term is in the vocabulary.
        // TODO(ouyang): Make this method ABSTRACT.
        virtual bool IsInVocabulary(const Utf8StringPiece& term) const {
            return false;
        }
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LANGUAGE_MODEL_INTERFACE_H_
