//
// Created by wenzhe on 4/9/20.
//
#include "GestureDecoder.h"
#include "internal/lexicon-interface.h"
#include "internal/language-model-interface.h"
#include "internal/touch-sequence.h"
#include "internal/token.h"
#include "internal/char-utils.h"
#include "internal/base/join.h"
#include "internal/result-utils.h"

//using keyboard::decoder::LexiconInterface;
//using keyboard::decoder::LanguageModelInterface;
//using keyboard::decoder::TouchSequence;
namespace keyboard {
namespace decoder {
    bool StringVectorEquals(const vector<Utf8String>& v1,
                            const vector<Utf8String>& v2) {
        if (v1.size() != v2.size()) return false;
        // Starts comparison from the back, because decoded results trend to have the
        // same prefix.
        for (int i = static_cast<int>(v1.size()) - 1; i >= 0; --i) {
            if (v1[i] != v2[i]) {
                return false;
            }
        }
        return true;
    }

    void AddResultIfBetter(const DecoderResult& result,
                           vector<DecoderResult>* results) {
        for (auto& old_result : *results) {
            if (result.word() == old_result.word()) {
                if (result.score() > old_result.score()) {
                    old_result = result;
                }
                return;
            }
        }
        results->push_back(result);
    }
    // Comparator that orders nodes by increasing prefix logp.
    struct OrderByPrefixProb {
        bool operator()(const CodepointNode& left, const CodepointNode& right) {
            return left.PrefixLogProb() > right.PrefixLogProb();
        }
    };

    // Comparator that orders nodes by increasing term logp.
    struct OrderByTermProb {
        bool operator()(const CodepointNode& left, const CodepointNode& right) {
            float left_prob;
            float right_prob;
            left.TermLogProb(&left_prob);
            right.TermLogProb(&right_prob);
            return left_prob > right_prob;
        }
    };

    GestureDecoder::GestureDecoder(bool isTest):
                                                touch_sequence_(),

                                                preceding_text_(),
                                                following_text_(),
                                                search_space_(),
                                                search_space_token_pool_(new TokenPool(params_.token_pool_capacity)),
                                                best_score_(NEG_INF),
                                                decoded_index_(0),
                                                word_histories_(),
                                                next_word_history_id_(0),
                                                top_tokens_set_(),
                                                temp_scores_(),
                                                active_beam_min_score_(NEG_INF),
                                                codes_to_keys_map_(),
                                                root_token_cache_(nullptr),
                                                next_word_predictions_() {
        //params_(decoder->params()),

    }

    void GestureDecoder::setMainParams(LoudsLmParams params) {
        main_lm_params = params;

    }

    void GestureDecoder::RecreateDecoderForActiveLms() {
        std::vector<std::pair<const LanguageModelInterface *, float>> weighted_lms;
        for (auto &entry : static_lexicons_) {
            lexicon_interfaces_.push_back(entry.second);
        }
        for (auto &entry : static_lms_) {
            weighted_lms.push_back(
                    {entry.second.get(), params_.static_lm_interpolation_weight});
        }
        //TODO: Implement DynamicLM - Wenzhe
        interpolated_lm_.reset(new InterpolatedLm(weighted_lms));
        lm_interfaces_.push_back(interpolated_lm_.get());

//    std::unique_ptr<DecoderInterface>(new Decoder(params, lexicons, lms));
//    ResetAllDecoderSessions();
        if (lexicon_interfaces_.size() > params_.kMaxLexicons) {
            lexicon_interfaces_.resize(params_.kMaxLexicons);
        }

        for (auto lm : lm_interfaces_) {
            LanguageModelScorerInterface *scorer =
                    lm->NewScorerOrNull(preceding_text_, following_text_);
            if (scorer != nullptr) {
                lm_scorers_.push_back(
                        std::unique_ptr<LanguageModelScorerInterface>(scorer));
            }
        }

    }

    void GestureDecoder::AddLexiconAndLm(const std::string &lm_name, LexiconInterface *lexicon,
                                         std::unique_ptr<LanguageModelInterface> lm) {
        // Acquire a write lock before adding the LM.
        if (lexicon != nullptr) {
            static_lexicons_[lm_name] = lexicon;
        }
        if (lm != nullptr) {
            static_lms_[lm_name] = std::move(lm);
        }
    }

    vector<DecoderResult> GestureDecoder::DecodeTouch(TouchSequence* touch_sequence, Utf8String prev) {
        vector<DecoderResult> unfiltered_results;
        vector<DecoderResult>* results = &unfiltered_results;
        bool session_three_decoder_enabled = false;
        touch_sequence->UpdateProperties(*gesture_keyboard_, params_, session_three_decoder_enabled);
        if (touch_sequence->size() == 0) {
            map<Utf8String, LogProbFloat> predictions;
            PredictNextTerm({prev}, params_.num_suggestions_to_return, &predictions);
        }
        touch_sequence_.reset(touch_sequence);
        Token* root = NewSearchToken();
        if (root == nullptr) {
            // Could not allocate a root token from the token pool. This should not
            // happen in practice.
            return unfiltered_results;
        }
        GetRootToken(root);
        if (root->nodes()->size() == 0) {
            // There were no root nodes (which can happen when the decoder has no
            // lexicons). Return without populating any results.
            search_space_token_pool_->ReleasePooledToken(root);
            return unfiltered_results;
        }
        AddSearchTokenToSearchSpace(root);
        int start = 0;
        int end = touch_sequence->size();

        // If decoding for the session so far has already been performed, there is
        // nothing to do.  Otherwise, decode incremental input.
        // search_space_ is empty initially; and will be filled with tokens
        // obtained in the following steps (mostly ProcessNextTouchPoint()).
        if (start < end) {
            // Decode the input into a set of candidates.
            for (int i = start; i < end; ++i) {
                ProcessNextTouchPoint(i);
            }
            for (auto& entry : search_space_) {
                entry.second->AdvanceToNextAlignment();
            }
        }

        // Extract and re-score the top prefixes and populate the results vector.
        TokenBeam top_prefixes(params_.prefix_beam_width);
        for (auto& entry : search_space_) {
            if (entry.second->index() == end - 1) {
                ProcessEndOfInput(entry.second, results, &top_prefixes);
            }
        }

        // Transform prefixes into their completions, if necessary.
        ProcessPrefixCompletions(&top_prefixes, results);
        ApplyScoreAdjustments(results);
        const size_t num_results =
                std::min<size_t>(params_.num_suggestions_to_return, results->size());
        std::partial_sort(results->begin(), results->begin() + num_results,
                          results->end(), ResultGreater());
        if (results->size() > params_.num_suggestions_to_return) {
            results->resize(params_.num_suggestions_to_return);
        }

        unfiltered_results = SuppressUppercaseResults(unfiltered_results,
                                                      params_.uppercase_suppression_score_threshold);

        for(DecoderResult res: unfiltered_results){
            std::string word = res.word();
        }

        const float autocorrect_threshold =
                params_.autocorrect_threshold_base +
                touch_sequence->size() *
                params_.autocorrect_threshold_adjustment_per_tap;

        return unfiltered_results;
    }

    void GestureDecoder::PredictNextTerm(const vector<Utf8StringPiece> &decoded_terms,
                                         int max_predictions,
                                         std::map<Utf8String, LogProbFloat> *top_predictions) {
        for (auto& scorer : lm_scorers_) {
            vector<pair<Utf8String, LogProbFloat>> predictions;
            scorer->PredictNextTerm(decoded_terms, max_predictions, &predictions);
            for (const auto& prediction : predictions) {
                const auto& it = top_predictions->find(prediction.first);
                if (it != top_predictions->end() && it->second > prediction.second) {
                    // Do nothing if the prediction already exists in the top_predictions
                    // list and the existing one has a higher score.
                    continue;
                } else {
                    (*top_predictions)[prediction.first] = prediction.second;
                }
            }
        }
    };

    void GestureDecoder::GetRootToken(Token* token) {
        if (root_token_cache_ == nullptr) {
            vector<CodepointNode> root_nodes;
            for (const auto* lexicon : lexicon_interfaces_) {
                root_nodes.push_back(CodepointNode::GetRootNode(lexicon));
            }
            root_token_cache_.reset(new Token());
            root_token_cache_->InitializeAsRoot(root_nodes, params_);
        }
        *token = *root_token_cache_;
    }

    void GestureDecoder::AddSearchTokenToSearchSpace(Token *token) {
        const DecoderState& key = GetDecoderStateForNode(
                (*token->nodes())[0], token->aligned_key(), token->word_history_id());
        search_space_[key] = token;
    }

    DecoderState GestureDecoder::GetDecoderStateForNode(
            const CodepointNode &node, const KeyId aligned_key,const int history_id) const {
            const int8 lexicon_id = GetLexiconId(node.lexicon());
            // Only include the aligned_key in the state for gesture input. This can
            // result in separate gesture tokens for the same lexical prefix.
            // Due to the instantaneous nature of tap input, the same token can be used to
            // capture multiple possible key alignments.
            const KeyId state_key = aligned_key;
            return {lexicon_id, node.GetNodeData(), history_id, state_key};
    }

    void GestureDecoder::ProcessNextTouchPoint(const int index) {
        AdvanceToNextIndexAndReturnTopTokens(index);
        TokenBeam top_reentry_tokens(params_.max_multi_term_terminals);

        // For gestures, pass the next alignments for top_tokens.  This computes
        // alignments as though the tokens are still in transit to their key.  Since
        // a token's spatial score is max(align_score, transit_score), it cannot
        // decrease when it is aligned to the current touch point.
        //
        // Therefore, if the beam is full, the lowest score in the beam after this
        // operation is a lower bound on the score needed to make it into the beam
        // at point index + 1.  PassGestureTokensInBeam stores this lower bound in
        // active_beam_min_score_ and we subsequently use it to avoid expanding
        // tokens that wouldn't make it into the beam.  (If the beam isn't full, we
        // set this threshold to NEG_INF and we also place an upper bound on it of
        // ScoreToBeatForMultiTerm when multi-term corrections should be
        // considered.)
        //
        // TODO(lhellsten): See if we can do something similar for tap typing.
        PassGestureTokensInBeam(top_tokens_set_, index);

        for (auto& token : top_tokens_set_) {
            // Ensure that we have enough free search tokens in the token pool.
            // Note: It is only safe to call PruneSearchTokensIfNeeded here because we
            // will not reference past token_pool_ tokens directly except those
            // remaining in the top_token_set_.
            PruneSearchTokensOutsideTopTokensSet();
            ExpandToken(index, ALIGN_NORMAL, token, &top_reentry_tokens);
        }

        // Process the top reentry tokens (if any) that were generated for the
        // previous index. They have just re-entered the root of the lexicon, and need
        // to be expanded to the start of the next term.
//        std::unique_ptr<vector<Token>> reentry_tokens(top_reentry_tokens.Extract());
        std::unique_ptr<vector<Token>> reentry_tokens(new vector<Token>(top_reentry_tokens.Take()));
//        std::unique_ptr<vector<Token>> reentry_tokens(new vector<Token>(top_reentry_tokens.TakeNondestructive()));
        for (auto& token : *reentry_tokens) {
            // Note: It is safe to call PruneSearchTokensIfNeeded here because
            // reentry_tokens are not part of the search space and will not be pruned.
            PruneSearchTokensOutsideTopTokensSet();
            ExpandToken(index, ALIGN_REENTRY, &token, nullptr);
        }
    }

    std::unordered_set<Token *>
    GestureDecoder::AdvanceToNextIndexAndReturnTopTokens(const int next_index) {
        decoded_index_ = next_index;
        std::unordered_set<int> active_histories;
        best_score_ = NEG_INF;
        auto token_iter = search_space_.begin();
        temp_scores_.clear();
        top_tokens_set_.clear();
        while (token_iter != search_space_.end()) {
            Token* token = token_iter->second;
            if (token->index() < next_index - 1 &&
                token->next_index() == next_index - 1) {
                // The token has a next-alignment entry that matches the current index,
                // but has not been advanced yet (this happens when continuing from a
                // saved state). Advance the token to its next alignment.
                token->AdvanceToNextAlignment();
            }
            if (token->index() == next_index - 1) {
                // The token's alignment is up-to-date given the current index, meaning
                // it is still active. Keep it and add it to the top_tokens beam.
                const float score = token->TotalScore();
                temp_scores_.push_back(score);
                if (token->word_history_id() >= 0) {
                    active_histories.insert(token->word_history_id());
                }
                if (score > best_score_) {
                    best_score_ = score;
                }
                ++token_iter;
            } else {
                // The token's alignment is out-dated given the current index, meaning
                // it was never passed to the most recent time frame (e.g., its score
                // was too poor to be worth considering). Remove it from the search space.
                search_space_token_pool_->ReleasePooledToken(token);
                search_space_.erase(token_iter++);
            }
        }

        auto history_iter = word_histories_.begin();
        while (history_iter != word_histories_.end()) {
            if (active_histories.find(history_iter->first) == active_histories.end()) {
                word_histories_.erase(history_iter++);
            } else {
                ++history_iter;
            }
        }

        int beam_cutoff = temp_scores_.size() - params_.active_beam_width;
        float score_threshold = params_.score_to_beat_absolute;
        if (beam_cutoff > 0) {
            std::nth_element(temp_scores_.begin(), temp_scores_.begin() + beam_cutoff,
                             temp_scores_.end());
            score_threshold = std::max(score_threshold, temp_scores_[beam_cutoff]);
        }

        token_iter = search_space_.begin();
        while (token_iter != search_space_.end()) {
            Token* token = token_iter->second;
            const float score = token->TotalScore();
            if (score >= score_threshold) {
                top_tokens_set_.insert(token);
            }
            ++token_iter;
        }

        return top_tokens_set_;
    }

    void GestureDecoder::PassGestureTokensInBeam(const std::unordered_set<Token *> &beam,
                                                 const int index) {

        // Choose a safe initial value for the active_beam_min_score_ threshold,
        // which is updated below by iterating over the tokens in the beam.
        //
        // If the beam isn't full, we can't compute a lower bound on its worst entry
        // yet, so we set the threshold to NEG_INF.  If it is full, we still want
        // our threshold to be low enough to consider terminal nodes if multi-term
        // correction is enabled, i.e. at most ScoreToBeatForMultiTerm().  This
        // accounts for the occasional case where the full LM score of a term is
        // much better than the prefix LM score.
        active_beam_min_score_ = NEG_INF;
        if (beam.size() >= params_.active_beam_width) {
            active_beam_min_score_ = params_.allow_multi_term
                                     ? ScoreToBeatForMultiTerm()
                                     : (float)0.0;
        }

        for (Token* token : beam) {
            if (token->aligned_key() >= 0 && token->transit_score() > NEG_INF) {
                // Advance the token to the current touch point.
                PassTokenGesture(token, index, touch_sequence(), token);
                if (token->NextTotalScore() < active_beam_min_score_) {
                    active_beam_min_score_ = token->NextTotalScore();
                }
            } else {
                // This token is either the root token, or a key aligned to the first
                // touch point that we're about to expand.  It cannot be in transit, and
                // so it will drop out of the beam, meaning we can't yet determine a
                // lower bound.  This only happens at indices 0 (the root token) and 1
                // (tokens aligned to index 0 that are to be expanded).
                active_beam_min_score_ = NEG_INF;
            }
        }


        // TODO(lhellsten): There's potential for a small additional win by using
        // the best score so for the next beam + score_to_beat_offset if it's a
        // tighter bound.

    }

    bool GestureDecoder::PassTokenGesture(const Token *original_token, const int next_index,
                                          const TouchSequence *touch_sequence,
                                          Token *next_token) const {
        if (next_index >= touch_sequence->size()) {
            return false;
        }
        const Alignment* alignment = original_token->alignment();
        Alignment* next_alignment = next_token->mutable_next_alignment();
        const KeyId next_key = next_token->aligned_key();
        const float point_align_score =
                (next_key >= 0) ? touch_sequence->align_score(next_index, next_key)
                                : NEG_INF;

        const KeyId prev_key = next_token->prev_aligned_key();
        if (prev_key == -1) {
            // If there is no previous key, then only align to the first touch point.
            const float new_align_score =
                    (next_index == 0)
                    ? point_align_score * params_.first_point_weight
                    : NEG_INF;
            *next_alignment = {next_index, new_align_score, NEG_INF};
            return true;
        }

        const float point_transit_score =
                touch_sequence->transit_score(next_index, prev_key, next_key);
        float next_align_score, next_transit_score;

        const KeyId original_key = original_token->aligned_key();
        const bool is_same_key =
                original_key == next_key ||
                keyboard()->KeyToKeyDistanceByIndex(original_key, next_key) == 0;

        if (is_same_key) {
            // Update the token's alignment to the same key, given the new touch point.
            next_align_score = alignment->transit_score() + point_align_score;
            next_transit_score = alignment->transit_score() + point_transit_score;
        } else {
            // Advance the alignment from the original token to the next_token's key.
            next_align_score = alignment->align_score() + point_align_score;
            next_transit_score = alignment->align_score() + point_transit_score;
        }

        // Update the next alignment for next_token if this is the best alignment
        // found so far.
        if (std::max(next_align_score, next_transit_score) >
            next_alignment->BestScore()) {
            *next_alignment = {next_index, next_align_score, next_transit_score};
            return true;
        }
        return false;
    }


    void GestureDecoder::PruneSearchTokensOutsideTopTokensSet() {
        if (search_space_token_pool_->FreeCount() >
            search_space_token_pool_->capacity() * kPruneWhenFreeRatioBelow) {
            return;
        }
        temp_scores_.clear();
        auto it = search_space_.begin();
        while (it != search_space_.end()) {
            Token* token = it->second;
            const bool can_prune = top_tokens_set_.find(token) == top_tokens_set_.end();
            if (can_prune) {
                const float score = token->NextTotalScore() > NEG_INF
                                    ? token->NextTotalScore()
                                    : token->TotalScore();
                temp_scores_.push_back(score);
            }
            ++it;
        }
        if (temp_scores_.size() == 0) {
            // No tokens to prune.
            return;
        }
        const int prune_index = temp_scores_.size() * kPruneRatio;
        std::nth_element(temp_scores_.begin(), temp_scores_.begin() + prune_index,
                         temp_scores_.end());
        const float prune_score = temp_scores_[prune_index];
        it = search_space_.begin();
        while (it != search_space_.end()) {
            Token* token = it->second;
            const bool can_prune = top_tokens_set_.find(token) == top_tokens_set_.end();
            if (can_prune) {
                const float score = token->NextTotalScore() > NEG_INF
                                    ? token->NextTotalScore()
                                    : token->TotalScore();
                if (score < prune_score) {
                    search_space_token_pool_->ReleasePooledToken(token);
                    search_space_.erase(it++);
                    continue;
                }
            }
            ++it;
        }

    }

    void GestureDecoder::ExpandToken(const int next_index, const DecoderAlignType align_type, Token *token,
                                     TokenBeam *reentry_tokens) {
        if (!ShouldConsiderToken(token)) return;
        ExpandTokenGesture(next_index, token, align_type, reentry_tokens);

    }

    void GestureDecoder::ExpandTokenGesture(const int next_index, Token *token,
                                            const DecoderAlignType align_type, TokenBeam *reentry_tokens) {
        if (!ShouldExpandToChildren(token)) return;
        const int kCodeSpace = ' ';
        const KeyId space_key = keyboard()->GetKeyIndex(kCodeSpace);

        if (params_.allow_multi_term) {
            const bool use_space_multiterm =
                    params_.use_space_for_multi_term &&
                    space_key != Keyboard::kInvalidKeyId;

            if (reentry_tokens != nullptr && ShouldConsiderMultiTerm(token) &&
                token->IsTerminal()) {
                Token reentry_token;
                const KeyId next_key =
                        use_space_multiterm ? space_key : Keyboard::kInvalidKeyId;
                InitializeReentryToken(*token,
                                       params_.extra_term_score,
                                       next_key, &reentry_token);
                reentry_tokens->push(reentry_token);
            }

            if (use_space_multiterm && token->aligned_key() == space_key) {
                if (align_type == ALIGN_REENTRY) {
                    // This is a new reentry token that needs to be 1) aligned to the next
                    // touch point and 2) added to the search space.
                    PassTokenGesture(token, next_index, touch_sequence(), token);
                    Token* search_token = FindSearchToken(
                            *token->nodes(), token->word_history_id(), space_key);
                    if (search_token != nullptr) {
                        if (token->NextTotalScore() > search_token->NextTotalScore()) {
                            *search_token = *token;
                        }
                    } else {
                        search_token = NewSearchToken();
                        *search_token = *token;
                        AddSearchTokenToSearchSpace(search_token);
                    }
                    return;
                } else if (next_index > 1) {
                    // This is a token that is in-transit to the space key.
                    const float prev_space_score = GetAlignToSpaceScore(next_index - 1);
                    const float space_score = GetAlignToSpaceScore(next_index);
                    if (space_score < params_.min_space_align_score ||
                        space_score > prev_space_score) {
                        // If the gesture has not reached the space key yet, then we only
                        // need to consider the in-transit alignment (already done in
                        // ProcessNextTouchPoint). There is no need to expand to the child
                        // tokens.
                        return;
                    }
                }
            }
        }

        const KeyId next_digraph_key = keyboard()->GetSecondDigraphKeyForCode(
                token->nodes()->back().codepoint(), token->aligned_key());
        if (next_digraph_key != Keyboard::kInvalidKeyId) {
            // If the token is currently aligned to a first digraph key, consider the
            // possibility that it is advancing to the second digraph key.
            Token* child_token =
                    FindOrCreateChildToken(*token->nodes(), *token, next_digraph_key);
            if (child_token != nullptr) {
                PassTokenGesture(token, next_index, touch_sequence(), child_token);
            }
            if (!keyboard()->CodeAlignsToKey(token->nodes()->back().codepoint(),
                                             token->aligned_key())) {
                return;
            }
        }

        for (const auto& code_to_nodes_entry : token->children()) {
            const char32 code = code_to_nodes_entry.first;
            const vector<KeyId>& possible_keys = GetPossibleKeysForCode(code);
            const vector<CodepointNode>& nodes = code_to_nodes_entry.second;
            const KeyId prev_key = token->aligned_key();
            for (KeyId next_key : possible_keys) {
                const bool is_repeated_key =
                        prev_key == next_key ||
                        (prev_key >= 0 &&
                         keyboard()->KeyToKeyDistanceByIndex(prev_key, next_key) == 0);
                Token* child_token = FindOrCreateChildToken(nodes, *token, next_key);
                if (child_token == nullptr) {
                    continue;
                }
                if (is_repeated_key) {
                    // Special handling is needed when the current key of a gesture is
                    // repeated (e.g., "l[l]ama", "so[o]n", "thre[e]") or if the previous
                    // key overlaps the next key (e.g., for 9-key layout). Explicitly skip
                    // the repeated key and expand to all possible next keys.
                    if (child_token->InitializeAsRepeatedLetterIfNeeded(*token, params_)) {
                        ExpandTokenGesture(next_index, child_token, ALIGN_NORMAL,
                                           reentry_tokens);
                    }
                } else {
                    PassTokenGesture(token, next_index, touch_sequence(), child_token);
                }
            }
            if (IsSkippableCharCode(code) || possible_keys.empty()) {
                // Skippable character omission (no penalty) or non-letter omission
                // (with penalty).
                Token omission_token(nodes, *token, token->aligned_key(), params_);
                if (!IsSkippableCharCode(code)) {
                    omission_token.AddScore(params_.omission_score);
                }
                PassTokenGesture(&omission_token, next_index, touch_sequence(),
                                 &omission_token);
                ExpandTokenGesture(next_index, &omission_token, ALIGN_NORMAL,
                                   reentry_tokens);
            }
        }

    }



    bool GestureDecoder::ShouldConsiderToken(const Token *token) const {
        return token->TotalScore() >= ScoreToBeat();
    }

    bool GestureDecoder::ShouldExpandToChildren(const Token *token) const {
        int kCodeSpace = ' ';
        if (token->aligned_key() < 0 || !touch_sequence_->is_gesture() ||
            token->aligned_key() == keyboard()->GetKeyIndex(kCodeSpace)) {
            return true;
        }
        const float last_align_score =
                touch_sequence()->align_score(token->index(), token->aligned_key());
        if (last_align_score < params_.min_align_key_score)
            return false;

        // Specific to gestures: if we expand token to its children, that implies
        // we're committing to aligning it to the previous point.  If that score
        // wouldn't be enough to beat out the worst token in the beam then there's
        // no point in generating its children, as they won't be retained.  Note
        // that if the beam is not full, active_beam_min_score_ == NEG_INF.
        float child_score_upper_bound = token->align_score() + token->lm_score();
        if (child_score_upper_bound < active_beam_min_score_) return false;

        return true;
    }

    bool GestureDecoder::ShouldConsiderMultiTerm(const Token *token) const {
        if (token->aligned_key() < 0) {
            return false;
        }

        if (!params_.allow_multi_term) {
            return false;
        }
        return token->TotalScore() >= ScoreToBeatForMultiTerm();
    }

    void GestureDecoder::InitializeReentryToken(const Token &terminal_token, const float penalty,
                                                const int next_key, Token *reentry_token) {
        vector<Utf8StringPiece> decoded_terms;
        Utf8String last_term;
        GetDecodedTerms(terminal_token, &last_term, &decoded_terms);
        const float conditional_lm_score =
                GetConditionalLanguageModelScore(decoded_terms, terminal_token);
        int word_history_id =
                GetOrAddWordHistory(terminal_token.word_history_id(), last_term);
        GetRootToken(reentry_token);
        reentry_token->InitializeAsNextTerm(terminal_token, word_history_id,
                                            conditional_lm_score, next_key);
        if (penalty != 0) {
            reentry_token->AddScore(penalty);
        }
    }

    Token *
    GestureDecoder::FindSearchToken(const vector<CodepointNode> &nodes, const int word_history_id,
                                    const int next_key) {
        const DecoderState& key =
                GetDecoderStateForNode(nodes[0], next_key, word_history_id);
        const auto& it = search_space_.find(key);
        if (it != search_space_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void GestureDecoder::GetDecodedTerms(const Token &token, Utf8String *last_term_storage,
                                         vector<Utf8StringPiece> *decoded_terms) const {
        const CodepointNode* terminal_node = &(token.nodes()->back());
        *last_term_storage = terminal_node->GetKey();
        if (token.word_history_id() != -1) {
            const vector<string>& prev_terms = GetWordHistory(token.word_history_id());
            decoded_terms->reserve(prev_terms.size() + 1);
            for (const auto& prev_term : prev_terms) {
                decoded_terms->emplace_back(prev_term);
            }
        }
        decoded_terms->emplace_back(*last_term_storage);
    }

    float GestureDecoder::GetAlignToSpaceScore(const int index) const {
        int kCodeSpace = ' ';
        const KeyId space_key = keyboard()->GetKeyIndex(kCodeSpace);
        if (space_key == Keyboard::kInvalidKeyId) {
            return NEG_INF;
        }
        return touch_sequence()->align_score(index, space_key);
    }

    const vector<Utf8String> &GestureDecoder::GetWordHistory(int id) const {
        const auto& it = word_histories_.find(id);
                DCHECK(it != word_histories_.end()) << "No word history found for id " << id;
        return it->second;
    }

    Token *
    GestureDecoder::FindOrCreateChildToken(const vector<CodepointNode> &nodes, const Token &parent,
                                           const int next_key) {
        const DecoderState& key =
                GetDecoderStateForNode(nodes[0], next_key, parent.word_history_id());
        const auto& it = search_space_.find(key);
        if (it != search_space_.end()) {
                    DCHECK_EQ((*it->second->nodes())[0].GetNodeData(), nodes[0].GetNodeData());
                    DCHECK_EQ(it->second->nodes()->size(), nodes.size());
            return it->second;
        } else {
            Token* child = NewSearchToken();
            if (child == nullptr) {
                return nullptr;
            }
            child->InitializeAsChild(nodes, parent, next_key, params_);
            child->InvalidateScores();
            search_space_[key] = child;
            return child;
        }
    }

    float
    GestureDecoder::GetConditionalLanguageModelScore(const vector<Utf8StringPiece> &term_sequence,
                                                     const Token &terminal_token) {
        if (!lm_scorers_.empty()) {
            const float conditional_lm_score =
                    DecodedTermsConditionalLogProb(term_sequence);
            if (conditional_lm_score > NEG_INF) {
                // Return the conditional logp for the last term.
                return conditional_lm_score;
            }
        }
        float unigram_score = GetUnigramScore(terminal_token);
        if (!lm_scorers_.empty()) {
            // If there were lm's available and none of them returned a valid logp,
            // apply a fixed backoff to lexicon unigram penalty.
            unigram_score += params_.lexicon_unigram_backoff;
        }
        return unigram_score;
    }

    float GestureDecoder::DecodedTermsConditionalLogProb(const vector<Utf8StringPiece> &terms) {
        // Note: Currently performs equally weighted linear interpolation of
        // probabilities when there are multiple language models.
        if (lm_scorers_.size() == 1) {
            return lm_scorers_[0]->DecodedTermsConditionalLogProb(terms);
        }
        float interpolated_prob = 0;
        for (auto& scorer : lm_scorers_) {
            const LogProbFloat logp = scorer->DecodedTermsConditionalLogProb(terms);
            interpolated_prob += exp(logp);
        }
        if (interpolated_prob == 0) {
            return NEG_INF;
        }
        return log(interpolated_prob / lm_scorers_.size());
    }

    const vector<KeyId> &GestureDecoder::GetPossibleKeysForCode(const char32 code) {
        const auto& entry = codes_to_keys_map_.find(code);
        if (entry != codes_to_keys_map_.end()) {
            return entry->second;
        }
        vector<KeyId> possible_keys = keyboard()->GetPossibleKeysForCode(code);
        codes_to_keys_map_[code] = possible_keys;
        return codes_to_keys_map_.find(code)->second;
    }

    float GestureDecoder::GetUnigramScore(const Token &token) {
        double max_logp = NEG_INF;
        for (const CodepointNode& node : *token.nodes()) {
            float unigram_logp;
            const bool is_term = node.TermLogProb(&unigram_logp);
            if (is_term) {
                if (unigram_logp > max_logp) {
                    max_logp = unigram_logp;
                }
            }
        }
        return max_logp;
    }

    int GestureDecoder::GetOrAddWordHistory(int prev_word_history_id, const Utf8String &new_term) {
        vector<Utf8String> words;
        if (prev_word_history_id >= 0) {
            words = GetWordHistory(prev_word_history_id);
        }
        words.push_back(new_term);
        for (const auto& pair : word_histories_) {
            if (StringVectorEquals(pair.second, words)) {
                return pair.first;
            }
        }
        word_histories_[++next_word_history_id_].swap(words);
        return next_word_history_id_;
    }

    void GestureDecoder::ProcessPrefixCompletions(TokenBeam *top_prefixes,
                                                  vector<DecoderResult> *prediction_results) {
        if (next_word_predictions_.empty()) {
            // Extract the top next word predictions from language model.
            PredictNextTerm({}, params_.kMaxNextWordPredictions,
                            &next_word_predictions_);
        }

        const float completion_score = params_.completion_score;

        int prediction_count = 0;
        //std::unique_ptr<vector<Token>> prefixes(top_prefixes->Extract());
        std::unique_ptr<vector<Token>> prefixes(new vector<Token>(top_prefixes->Take()));
//        std::unique_ptr<vector<Token>> prefixes(new vector<Token>(top_prefixes->TakeNondestructive()));
        for (const auto& prefix_token : *prefixes) {
            const Utf8String prefix_term = prefix_token.nodes()->back().GetKey();
            const double spatial_score = prefix_token.align_score() + completion_score;
            for (const auto& prediction : next_word_predictions_) {
                const Utf8StringPiece prediction_term = prediction.first;
                if (prediction_term.starts_with(prefix_term)) {
                    DecoderResult prediction_result(prediction.first, spatial_score,
                                                    prediction.second);
                    AddResultIfBetter(prediction_result, prediction_results);
                    ++prediction_count;
                }
            }
            if (prediction_count < params_.kMinCompletions) {
                for (const auto& node : *prefix_token.nodes()) {
                    map<Utf8String, LogProbFloat> completions;
                    GetBestCompletionsForNode(node, params_.kCompletionBeamSize,
                                              &completions);
                    for (const auto& completion : completions) {
                        const float lm_score =
                                DecodedTermsConditionalLogProb({completion.first});
                        const float completion_lm_score =
                                lm_score != NEG_INF ? lm_score : completion.second;
                        DecoderResult completion_result(completion.first, spatial_score,
                                                        completion_lm_score);
                        AddResultIfBetter(completion_result, prediction_results);
                    }
                }
            }
        }
    }

    void GestureDecoder::ApplyScoreAdjustments(vector<DecoderResult> *results) const {
        if (touch_sequence()->is_gesture()) {
            const float max_penalty =
                    params_.max_imprecise_match_penalty;
            for (DecoderResult& result : (*results)) {
                const float original_score = result.spatial_score();

                // Penalize non-precise gestures by max_penalty.
                float spatial_score_adjustment = max_penalty;

                if (original_score > params_.precise_match_threshold) {
                    // Penalize precise gestures by a fraction of max_penalty.
                    const float score_to_threshold_ratio =
                            original_score / params_.precise_match_threshold;
                    spatial_score_adjustment = score_to_threshold_ratio * max_penalty;
                }
                result.adjust_spatial_score(spatial_score_adjustment);
            }
        } else {
            const float non_literal_penalty =
                    params_.non_literal_match_penalty;
            vector<char32> literal_codes = touch_sequence()->GetLiteralCodes();
            for (auto& result : (*results)) {
                const float original_score = result.spatial_score();
                // Penalize non-literal matches by non_literal_penalty.
                float spatial_score_adjustment = non_literal_penalty;
                vector<char32> result_codes =
                        CharUtils::GetBaseLowerCodeSequence(result.word());
                if (CharUtils::ResultMatchesLiteral(result_codes, literal_codes)) {
                    // Do not penalize literal matches.
                    spatial_score_adjustment = 0.0f;
                }
                result.adjust_spatial_score(spatial_score_adjustment);
            }
        }
    }

    void GestureDecoder::GetBestCompletionsForNode(const CodepointNode &start_node, int max_completions,
                                                   std::map<Utf8String, LogProbFloat> *completions) const {
        // A TopN beam of active lexical nodes currently being explored.
        TopN<CodepointNode, OrderByPrefixProb> active_nodes(max_completions);
        // A TopN beam of full completions.
        TopN<CodepointNode, OrderByTermProb> top_completions(max_completions);

        active_nodes.push(start_node);
        float score_to_beat = NEG_INF;
        while (!active_nodes.empty()) {
//            const std::unique_ptr<vector<CodepointNode>> cur_predictions(
//                    active_nodes.Extract());
            const std::unique_ptr<vector<CodepointNode>> cur_predictions(
                    new vector<CodepointNode>(active_nodes.Take()));
//            const std::unique_ptr<vector<CodepointNode>> cur_predictions(
//                    new vector<CodepointNode>(active_nodes.TakeNondestructive()));
            active_nodes.Reset();
            for (const auto& node : *cur_predictions) {
                if (node.PrefixLogProb() <= score_to_beat) {
                    continue;
                }
                LogProbFloat logp = NEG_INF;
                // Check if we've reached the end of a term.
                if (node.TermLogProb(&logp)) {
                    top_completions.push(node);
                    if (top_completions.size() == max_completions) {
                        // Set the score to beat to the log probability of the worst
                        // completion in the beam.
                        top_completions.peek_bottom().TermLogProb(&score_to_beat);
                    }
                }
                vector<CodepointNode> child_nodes;
                node.GetChildCodepoints(&child_nodes);
                for (const auto& child : child_nodes) {
                    if (child.PrefixLogProb() > score_to_beat) {
                        active_nodes.push(child);
                    }
                }
            }
        }
//        const std::unique_ptr<vector<CodepointNode>> final_completions(
//                top_completions.Extract());
        const std::unique_ptr<vector<CodepointNode>> final_completions(
                new vector<CodepointNode>(top_completions.Take()));
//        const std::unique_ptr<vector<CodepointNode>> final_completions(
//                new vector<CodepointNode>(top_completions.TakeNondestructive()));
        for (const auto& node : *final_completions) {
            LogProbFloat logp = NEG_INF;
            if (node.TermLogProb(&logp)) {
                Utf8String term = node.GetKey();
                (*completions)[term] = logp + params_.lexicon_unigram_backoff;
            }
        }
    }

    void GestureDecoder::ProcessEndOfInput(Token *token, vector<DecoderResult> *results,
                                           TokenBeam *top_prefixes) {
        if (token->IsTerminal()) {
            ExtractEndOfInputTerminal(*token, results);
        }
        vector<CodepointNode> child_nodes;
        for (const CodepointNode& node : *(token->nodes())) {
            node.GetChildCodepoints(&child_nodes);
        }
        if (child_nodes.size() > 0 && !token->has_prev_terms()) {
            top_prefixes->push(*token);
        }
    }

    void GestureDecoder::ExtractEndOfInputTerminal(const Token &terminal_token,
                                                   vector<DecoderResult> *results) {
        vector<Utf8StringPiece> decoded_terms;
        Utf8String last_term;
        GetDecodedTerms(terminal_token, &last_term, &decoded_terms);

        const vector<CodepointNode>* nodes = terminal_token.nodes();
        float lm_score = NEG_INF;
        const float conditional_lm_score =
                GetConditionalLanguageModelScore(decoded_terms, terminal_token);
        lm_score = conditional_lm_score + terminal_token.prev_lm_score();
        float spatial_score = terminal_token.align_score();
        Utf8String decoded_terms_string = strings::Join(decoded_terms, " ");
        if (lm_score > NEG_INF) {
            DecoderResult result(decoded_terms_string, spatial_score, lm_score);
            const float terminal_score = result.score();
            if (terminal_score > NEG_INF && terminal_score) {
                AddResultIfBetter(result, results);
            }
        }
    }


}
}