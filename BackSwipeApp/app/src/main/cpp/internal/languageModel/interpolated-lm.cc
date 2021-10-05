#include "interpolated-lm.h"

#include <cmath>
#include <map>
#include <set>

#include "../base/logging.h"
#include "../base/constants.h"

namespace keyboard {
namespace decoder {
namespace lm {

    class InterpolatedLmScorer : public LanguageModelScorerInterface {
    public:
        // Creates a new scorer that combines the given LanguageModelInterfaces
        // and their respective interpolation weights.
        //
        // Note: The weights for the constituent scorers are normalized so that
        // the sum of all weights is equal to 1.0.
        InterpolatedLmScorer(
                const vector<pair<const LanguageModelInterface*, float>>& weighted_lms,
                const Utf8StringPiece& preceding_text,
                const Utf8StringPiece& following_text)
                : weighted_scorers_(), supports_next_word_predictions_() {
            float sum_weights = 0;
            for (const auto& weighted_lm : weighted_lms) {
                sum_weights += weighted_lm.second;
            }
            for (const auto& weighted_lm : weighted_lms) {
                LanguageModelScorerInterface* scorer =
                        weighted_lm.first->NewScorerOrNull(preceding_text, following_text);
                if (scorer == nullptr) {
                    LOG(ERROR) << "NewScorerOrNull should not return null";
                    weighted_scorers_.clear();
                    return;
                }
                const float normalized_weight = weighted_lm.second / sum_weights;
                weighted_scorers_.push_back(
                        {std::unique_ptr<LanguageModelScorerInterface>(scorer),
                         normalized_weight});
                supports_next_word_predictions_.push_back(
                        weighted_lm.first->SupportsPredictions());
            }
        }

        // Returns the number of scorers.
        int size() const { return weighted_scorers_.size(); }

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
        // The vector of scorers and their respective weights to interpolate over.
        vector<pair<std::unique_ptr<LanguageModelScorerInterface>, float>>
                weighted_scorers_;

        // A boolean vector that indicates whether each of the constituent LMs
        // support next word predictions.
        vector<bool> supports_next_word_predictions_;
    };

    LogProbFloat InterpolatedLmScorer::DecodedTermsLogProb(
            const vector<Utf8StringPiece>& decoded_terms) {
        float interpolated_prob = 0;
        for (int i = 0; i < weighted_scorers_.size(); ++i) {
            const LogProbFloat logp =
                    weighted_scorers_[i].first->DecodedTermsLogProb(decoded_terms);
            const float weight = weighted_scorers_[i].second;
            interpolated_prob += exp(logp) * weight;
        }
        if (interpolated_prob == 0) {
            return NEG_INF;
        }
        return log(interpolated_prob);
    }

    LogProbFloat InterpolatedLmScorer::DecodedTermsConditionalLogProb(
            const vector<Utf8StringPiece>& decoded_terms) {
        float interpolated_prob = 0;
        for (int i = 0; i < weighted_scorers_.size(); ++i) {
            const LogProbFloat logp =
                    weighted_scorers_[i].first->DecodedTermsConditionalLogProb(
                            decoded_terms);
            const float weight = weighted_scorers_[i].second;
            interpolated_prob += exp(logp) * weight;
        }
        if (interpolated_prob == 0) {
            return NEG_INF;
        }
        return log(interpolated_prob);
    }

    void InterpolatedLmScorer::PredictNextTerm(
            const vector<Utf8StringPiece>& decoded_terms, const int max_predictions,
            vector<pair<Utf8String, LogProbFloat>>* results) {
        results->clear();
        // A map between predictions and their interpolated (non-log) probability.
        std::map<Utf8String, float> interpolated_probs;
        // A map between predictions and the scorers that predicted them.
        std::map<Utf8String, std::set<int>> prediction_scorers;
        for (int i = 0; i < weighted_scorers_.size(); ++i) {
            if (supports_next_word_predictions_[i]) {
                const float weight = weighted_scorers_[i].second;
                vector<pair<Utf8String, LogProbFloat>> predictions;
                weighted_scorers_[i].first->PredictNextTerm(
                        decoded_terms, max_predictions, &predictions);
                for (const auto& prediction : predictions) {
                    interpolated_probs[prediction.first] += exp(prediction.second) * weight;
                    prediction_scorers[prediction.first].insert(i);
                }
            }
        }
        for (const auto& entry : interpolated_probs) {
            if (weighted_scorers_.size() > 1) {
                float interpolated_prob = entry.second;
                // Take into account the contribution from any scorers that didn't
                // contribute to the interpolated probability. These can even include
                // scorers that don't support next word prediction.
                //
                // Note: Avoid re-scoring the on any scorers that already contributed
                // to the interpolated_prob. This is especially important since some LMs
                // (i.e., LoudsLm) may have special handling for next-word predictions,
                // which would be lost when calling DecodedTermsConditionalLogProb.
                vector<Utf8StringPiece> terms_with_predicted_term(decoded_terms);
                terms_with_predicted_term.push_back(entry.first);
                const std::set<int>& scorers =
                        prediction_scorers.find(entry.first)->second;
                for (int i = 0; i < weighted_scorers_.size(); ++i) {
                    if (scorers.find(i) == scorers.end()) {
                        const LogProbFloat logp =
                                weighted_scorers_[i].first->DecodedTermsConditionalLogProb(
                                        terms_with_predicted_term);
                        const float weight = weighted_scorers_[i].second;
                        interpolated_prob += exp(logp) * weight;
                    }
                }
                results->push_back({entry.first, log(interpolated_prob)});
            } else {
                results->push_back({entry.first, log(entry.second)});
            }
        }
    }

    LanguageModelScorerInterface* InterpolatedLm::NewScorerOrNull(
            const Utf8StringPiece& preceding_text,
            const Utf8StringPiece& following_text) const {
        InterpolatedLmScorer* scorer =
                new InterpolatedLmScorer(weighted_lms_, preceding_text, following_text);
        if (scorer->size() == 0) {
            delete scorer;
            return nullptr;
        }
        return scorer;
    }

}  // namespace lm
}  // namespace decoder
}  // namespace keyboard
