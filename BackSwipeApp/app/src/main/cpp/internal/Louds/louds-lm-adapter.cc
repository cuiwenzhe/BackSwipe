#include <limits>
#include <map>
#include <string>

#include "louds-lm-adapter.h"
#include "../languageModel/case.h"
#include "../languageModel/split.h"

namespace keyboard {
namespace decoder {
namespace lm {

    LoudsLmAdapter::LoudsLmAdapter(std::unique_ptr<LoudsLm> lm)
            : louds_lm_(std::move(lm)),
              lexicon_(new LoudsLexiconAdapter(louds_lm_->lexicon())) {}

    LanguageModelScorerInterface* LoudsLmAdapter::NewScorerOrNull(
            const Utf8StringPiece& preceding_text,
            const Utf8StringPiece& following_text) const {
        vector<Utf8String> preceding_terms =
                strings::Split(preceding_text, " ", strings::SkipEmpty());
        const int max_preceding_terms = louds_lm_->max_n() - 1;
        if (preceding_terms.size() > max_preceding_terms) {
            const int start = preceding_terms.size() - max_preceding_terms;
            preceding_terms.assign(preceding_terms.begin() + start,
                                   preceding_terms.end());
        }
        vector<TermId16> preceding_term_ids =
                louds_lm_->TermsToTermIds(preceding_terms);
        return new LoudsLmScorer(this, preceding_term_ids);
    }

    LogProbFloat LoudsLmScorer::DecodedTermsLogProb(
            const vector<Utf8StringPiece>& decoded_terms) {
        return -numeric_limits<float>::infinity();
    }

    LogProbFloat LoudsLmScorer::DecodedTermsConditionalLogProb(
            const vector<Utf8StringPiece>& decoded_terms) {
        LogProbFloat logp;
        const bool found = lm_->louds_lm()->LookupConditionalLogProb(
                preceding_term_ids_, decoded_terms, &logp);
        if (found) {
            return logp;
        }
        return NEG_INF;
    }

    void LoudsLmScorer::PredictNextTerm(
            const vector<Utf8StringPiece>& decoded_terms, const int max_predictions,
            vector<pair<Utf8String, LogProbFloat>>* results) {
        results->clear();
        map<string, float> predictions;
        lm_->louds_lm()->PredictNextWords(preceding_term_ids_, decoded_terms,
                                          max_predictions, &predictions);
        for (const auto& prediction : predictions) {
            results->push_back(prediction);
        }
    }

}  // namespace lm
}  // namespace decoder
}  // namespace keyboard
