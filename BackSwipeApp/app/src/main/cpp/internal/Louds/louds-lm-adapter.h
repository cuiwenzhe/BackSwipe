// A wrapper for LoudsLm that allows it to be used in the keyboard decoder.
#ifndef INPUTMETHOD_KEYBOARD_DECODER_LM_LOUDS_LOUDS_LM_ADAPTER_H_
#define INPUTMETHOD_KEYBOARD_DECODER_LM_LOUDS_LOUDS_LM_ADAPTER_H_

#include <memory>
#include <utility>
#include <vector>

#include "louds-lexicon-adapter.h"
#include "../base/basictypes.h"
#include "../language-model-interface.h"
#include "../basic-types.h"
#include "louds-lm.h"

namespace keyboard {
namespace decoder {
namespace lm {

    using keyboard::decoder::LanguageModelInterface;
    using keyboard::decoder::LanguageModelScorerInterface;
    using keyboard::decoder::LogProbFloat;
    using keyboard::decoder::Utf8StringPiece;
    using keyboard::lm::louds::LoudsLm;
    using keyboard::lm::louds::TermId16;

    class LoudsLmAdapter : public LanguageModelInterface {
    public:
        // Creates a new LoudsLmAdapter for the given LoudsLm language model.
        explicit LoudsLmAdapter(std::unique_ptr<LoudsLm> lm);

        // Returns the LoudsLexiconAdapter associated with the LoudsLm.
        LoudsLexiconAdapter* lexicon() const { return lexicon_.get(); }

        // Returns the base LoudsLm that is wrapped by this class.
        const LoudsLm* louds_lm() const { return louds_lm_.get(); }

        ////////////////////////////////////////////////////////////////////////////
        // The following methods are inherited from LanguageModelInterface.
        // Please refer to the comments for that class.
        ////////////////////////////////////////////////////////////////////////////

        // Note: The current LoudsLm implementation does not use the following_text.
        LanguageModelScorerInterface* NewScorerOrNull(
                const Utf8StringPiece& preceding_text,
                const Utf8StringPiece& following_text) const override;

        bool SupportsPredictions() const override { return true; }

        bool IsInVocabulary(const Utf8StringPiece& term) const override {
            return lexicon_->IsInVocabulary(term);
        }

    private:
        // The underlying LoudsLm.
        std::unique_ptr<LoudsLm> louds_lm_;

        // The underlying LoudsLexicon (which also serves as the term to term-id map).
        std::unique_ptr<LoudsLexiconAdapter> lexicon_;
    };

    class LoudsLmScorer : public LanguageModelScorerInterface {
    public:
        // Note: following_text currently not used by the decoder.
        explicit LoudsLmScorer(const LoudsLmAdapter* lm,
                               const vector<TermId16>& preceding_term_ids)
                : lm_(lm), preceding_term_ids_(preceding_term_ids) {}

        ////////////////////////////////////////////////////////////////////////////
        // The following methods are inherited from LanguageModelScorerInterface.
        // Please refer to the comments for that class.
        ////////////////////////////////////////////////////////////////////////////

        LogProbFloat DecodedTermsLogProb(
                const vector<Utf8StringPiece>& decoded_terms) override;

        LogProbFloat DecodedTermsConditionalLogProb(
                const vector<Utf8StringPiece>& decoded_terms) override;

        void PredictNextTerm(
                const vector<Utf8StringPiece>& decoded_terms, const int max_predictions,
                vector<pair<Utf8String, LogProbFloat>>* predictions) override;

    private:
        // The parent LoudsLm interface.
        const LoudsLmAdapter* lm_;

        // The term_ids for the preceding terms.
        vector<TermId16> preceding_term_ids_;
    };

}  // namespace lm
}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_LM_LOUDS_LOUDS_LM_ADAPTER_H_
