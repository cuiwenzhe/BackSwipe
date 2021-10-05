//
// Created by wenzhe on 4/16/20.
//

#ifndef SIMPLEGESTUREINPUT_DECODERPARAMS_H
#define SIMPLEGESTUREINPUT_DECODERPARAMS_H
#include "base/constants.h"
class DecoderParams{
public:
    DecoderParams(){}
public:
    // The standard deviation of the error between the gesture path and the
    // intended key centers, as a fraction of the most common key width. A
    // lower value means a higher penalty for deviating away from the keys.
    const float key_error_sigma = 0.9;

    // The standard deviation of the error in the direction of the gesture,
    // relative to the ideal direction between pairs of intended keys. A
    // lower value means a higher penalty for gesturing in a direction that
    // does not match the ideal path.
    const float direction_error_sigma = 0.7;

    // A score penalty for skipping (i.e. treating as an in-transit point)
    // a touch point where the user paused.
    const float skip_pause_score = -2.0;

    // A score penalty for skipping (i.e. treating as an in-transit point)
    // a touch point that is considered a corner.
    const float skip_corner_score = -4.0;

    // The minimum curvature for a point to be considered a corner. Note that
    // corners must also be a local maxima in curvature.
    const float min_curvature_for_corner = 0.392699; // (PI/4)

    // A point may be a pause if its duration is greater than this value.
    // Note that pauses must also be a local maxima in duration.
    const float pause_duration_in_millis = 200;

    // This parameter only affects the intermediate scores during the decoder's
    // search, not the scores in the final results. However, setting this weight
    // too high may cause the decoder to prematurely prune rare prefixes, make
    // some terms completely ungesturable.
    const float prefix_lm_weight = 0.5;


    // The maximum number of tokens that the decoder can maintain at any time.
    // While only the top 'active_beam_width' tokens are advanced each time frame,
    // more tokens are used to represent intermediate states during decoding.
    // Note that the algorithm starts pruning low-scoring tokens when the capacity
    // is reached. For environments where memory is plentiful, a pool capacity of
    // about 20x the active_beam_width is usually enough to avoid pruning.
    int token_pool_capacity =  1000;

    /****
     * Android Decoder
     */
    // The maximum number of next word predictions to consider.
    const int kMaxNextWordPredictions = 100;
    // The decoder will try to return at least this many completions for a prefix.
    const int kMinCompletions = 3;
    // The size of the search beam for enumerating lexical completions. This
    // is kept small for performance reasons.
    const int kCompletionBeamSize = 20;
    // The maximum number of lexicons supported.
    const int kMaxLexicons = 127;

    // The base autocorrection threshold (for tap typing only). When the top
    // result's score is above the autocorrect_threshold, the decoder returns
    // an autocorrect_confidence that is above 0.5 (autocorrect suggested).
    //
    // The autocorrect_confidence is scaled such that:
    // autocorrect_confidence = min(1.0, autocorrect_threshold / top_score * 0.5)
    //
    // Examples:
    //   top_score >= autocorrect_threshold / 2,    autocorrect_confidence = 1.0
    //   top_score == autocorrect_threshold,        autocorrect_confidence = 0.5
    //   top_score == autocorrect_threshold * 2,    autocorrect_confidence = 0.25
    //
    float autocorrect_threshold_base = -10.0;

    // The adjustment to the autocorrection threshold for each tap.
    // This is to account for the additional negative spatial score that we
    // expect to accumulate for each new tap.
    float autocorrect_threshold_adjustment_per_tap = -2.0;

    // The interpolation weight for the static (i.e., main language) LMs.
    float static_lm_interpolation_weight = 1.0;

    // The default interpolation weight for the dynamic LMs.
    float dynamic_lm_interpolation_weight = 0.2;

    // The number of candidate terms the decoder should return.
    const int num_suggestions_to_return = 20;

    // The maximum number of multi-term terminal tokens considered each time
    // tokens are passed to the next time frame.
    const int max_multi_term_terminals = 10;

    // The width of the beam used for active search states. At each point in time,
    // the decoder will advance at most this many states.
    int active_beam_width = 100;

    // Whether to consider multi-term candidates for the spaceless input.
    bool allow_multi_term = false;

    // A token must score higher than the best token plus this value to be
    // considered, otherwise it will be pruned from the search.
    float score_to_beat_offset = -12.0;

    // A token must score higher than the best token plus this value to be
    // considered as a correction (e.g., insertions, omissions, etc).
    float score_to_beat_offset_for_corrections = -6.0;

    // A token must score higher than this value to be considered, otherwise it
    // will be pruned from the search.
    float score_to_beat_absolute = keyboard::decoder::NEG_INF;

    // The extra spatial score weight for the first point.
    float first_point_weight = 2.0;

    // Whether multi-term decoding requires users to gesture through the space key
    // between words. If this false, multi-term decoding will not use the space
    // key at all. Has no effect if allow_multi_term is false.
    bool use_space_for_multi_term = false;

    // The minimum align score to consider a touch alignment to a key.
    float min_align_key_score = -8.0;

    // The spatial score penalty for each extra term generated in a multi-term
    // continuous gesture. This is applied regardless of use_space_for_multi_term.
    float extra_term_score = 0.0;

    // The minimum alignment score to the space key for multi-term decoding. This
    // can require the user to gesture very close to the spacebar before the
    // decoder considers a multi-term phrase candidate. A lower value can reduce
    // false word segmentations and improve performance. Note: Has no effect if
    // allow_multi_term or use_space_for_multi_term is false.
    float min_space_align_score = -1.0;

    // The extra spatial score penalty for an omission.
    float omission_score = -5.0;

    // The backoff penalty when there is no language model score for the decoded
    // term(s) and the decoder backs off unigram logp from the lexicon.
    float lexicon_unigram_backoff = -5.0;

    // The width of the beam used for prefix states. The decoder will generate
    // completions for these top-n prefixes.
    int prefix_beam_width = 3;
    // The maximum spatial score penalty for imprecise gesture matches.
    //
    // The per-term adjustment scales linearly within the spatial score:
    // For a messy gesture (spatial_score <= precise_match_threshold):
    //    penalty = max_imprecise_match_penalty
    // For a relatively precise gesture (spatial_score == threshold * 0.5):
    //    penalty = max_precise_match_adjustment * 0.5
    // For a perfect gesture (spatial_score == 0.0):
    //    penalty = 0
    float max_imprecise_match_penalty = -4.0;


    // The minimum spatial score for a gesture to be considered a precise match.
    //
    // For imprecise gesture matches, we negatively adjust the spatial model
    // score distinguish them from matches where the user is gesturing carefully.
    // Otherwise, some words can become essentially un-gesturable because they are
    // always mis-recognized as a similar (but much more frequent) competing word
    // (e.g., "three"/"the", "I'd"/"is").
    float precise_match_threshold = -2.0;

    // The extra spatial score penalty for an non-literal match.
    float non_literal_match_penalty = -4.0;

    // The extra spatial score penalty for performing an auto-completion.
    float completion_score = -4.0;


    // Exclude this number of points from the start and end of the touch sequence
    // when search for local maxima.
    float endpoint_offset_for_maxima = 2;

    // The decoder should suppress uppercase suggestions (e.g., "You", "YOU") that
    // score less than this far below their lowercase variant (e.g., "you").
    // Note: Lowercase suggestions are never suppressed.
    float uppercase_suppression_score_threshold = -100.0;
};
#endif //SIMPLEGESTUREINPUT_DECODERPARAMS_H
