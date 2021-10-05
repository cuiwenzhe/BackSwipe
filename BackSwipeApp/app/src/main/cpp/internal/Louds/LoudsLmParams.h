//
// Created by wenzhe on 4/16/20.
//

#ifndef SIMPLEGESTUREINPUT_LOUDSLMPARAMS_H
#define SIMPLEGESTUREINPUT_LOUDSLMPARAMS_H
#include <string>
class LoudsLmParams{
public:
    LoudsLmParams() {}
public:
    // The stupid back factor (in log probability space).
    float stupid_backoff_logp = static_cast<float>(-1.0);

    // The language model quantizes log probabilities into 256 equally spaced bins
    // spanning the range: [-logp_quantizer_range, 0].
    float logp_quantizer_range = 25.0;

    // The maximum number of term_ids (limited to the 16-bit addressable space).
    int max_num_term_ids = 0x10000;

    // Whether the LM's lexicon should encode prefix unigrams.
    bool enable_prefix_unigrams = true;

    // Whether the LoudsLm stores backoff weights. If false, use stupid-backoff.
    // TODO(ouyang): We might want stupid backoff even when there are backoff
    // weights. E.g. we could potentially minimize storage by essentially pruning
    // the weights (only including the most useful ones vs an assumed stupid
    // backoff baseline).
    bool has_backoff_weights = false;

    // The autocorrect threshold to use for this model.
    // Note: Does not affect Fava, which defines this threshold on the client.
    float autocorrect_threshold = 0.45;

    // The autocorrection threshold to use in place of autocorrect_threshold
    // (see above) for single-letter tapped words. A higher threshold here
    // prevents unwanted corrections away from abbreviations, initialisms, or
    // acronyms, like "u" and "N.B.". Note that this does not affect
    // capitalization-only corrections such as "i" -> "I".
    //
    // Set this value to a value <= autocorrect_threshold (or leave unset) to use
    // the autocorrect_threshold field for single letter words.
    //
    // TODO(lhellsten): Adjust or remove this threshold as abbreviations start
    // being handled by the LM.
    //
    // Note: Does not affect Fava, which defines this threshold on the client
    // (with a value of 0.9).
    float autocorrect_threshold_for_single_letter = -1.0;

    // Whether or not to include unigram-level next-word predictions. If set to
    // true, PredictNextWords will include the top unigram words words in the
    // LM (e.g., "the", "to", "I"). This is useful for ensuing that the keyboard
    // shows a full set of predictions even if there are no higher-order ngrams.
    bool include_unigram_predictions = false;

    // If the top two results are case variants, the second result is lowercase,
    // and their score difference is less than this value, then the decoder will
    // swap the top two results.  This helps to avoid suggesting e.g. "Apple"
    // instead of "apple".  Set to 0 to disable this functionality.
    float swap_case_variants_score_diff_threshold = 0.0;

    // If set, enable a heuristic to generate compounds by fusing
    // multi-word DecoderResults. Consumed by AndroidDecoderWrapper.
    bool enable_auto_compounding = false;

    // Extra backoff weight when backing off to an uppercase unigram.
    // TODO(ouyang): Remove this parameter when no longer needed by the LMs.
    float uppercase_unigram_extra_backoff_weight = -0.0;

    // A minimum unigram logp threshold for next-word predictions generated from
    // higher order n-grams (3-grams and above, see LoudsLm::PredictNextWords).
    //
    // This can be used to prevent the LM from predicting less common words,
    // which are more likely to be offensive or embarrassing in context.
    //
    // WARNING: The default threshold is very conservative, and will only allow
    // the most frequent unigrams as next-word predictions. It should be
    // overridden whenever possible to provide the best tradeoff between
    // prediction coverage and quality for each language/LM.
    float min_unigram_logp_for_predictions = -100.0;

    // The format version numbers for known LoudsLm formats. They should match
    // the CL number that generated each format.
    //
    // A new format version number should be added only when the LoudsLm format
    // changes in such a way that it is no longer backward compatible. Old version
    // numbers should not be changed or removed if there's any chance that the
    // version is in circulation.
    enum FormatVersionNumber {
        INVALID = 0,           // An invalid version number that cannot be read.
        FAVA_BETA = 86736212  // The CL number that generated this format.
    };
    // The format version of the LoudsLm. Default value should always be equal to
    // the latest format version in source.
    FormatVersionNumber format_version = FAVA_BETA;
public:
    bool ParseFromString(std::string& str){
        return false;
    }
    std::string SerializeAsString(){
        return "";
    }
};
#endif //SIMPLEGESTUREINPUT_LOUDSLMPARAMS_H