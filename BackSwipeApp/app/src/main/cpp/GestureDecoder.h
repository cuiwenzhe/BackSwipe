//
// Created by wenzhe on 4/9/20.
//

#ifndef SIMPLEGESTUREINPUT_GESTUREDECODER_H
#define SIMPLEGESTUREINPUT_GESTUREDECODER_H
#include "internal/Louds/LoudsLmParams.h"
#include "internal/Louds/LoudsLmParams.h"
#include <string>
#include <map>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include "internal/lexicon-interface.h"
#include "internal/language-model-interface.h"
#include "internal/DecoderParams.h"
#include "internal/languageModel/interpolated-lm.h"
#include "internal/keyboardSetting/keyboard.h"
#include "internal/keyboardSetting/KeyboardParam.h"
#include "internal/language-model-interface.h"
#include "internal/base/basictypes.h"
#include "internal/touch-sequence.h"
#include "internal/token.h"
#include "internal/base/hash.h"
#include "internal/languageModel/top_n.h"
#include "internal/decoder-result.h"
//
//using keyboard::decoder::LanguageModelInterface;
//using keyboard::decoder::LexiconInterface;
//using keyboard::decoder::KeyboardLayout;
//using keyboard::decoder::LanguageModelScorerInterface;

using keyboard::decoder::lm::InterpolatedLm;

namespace keyboard {
namespace decoder {
    // Orders DecoderResults by score, returns true if the left Result has a greater
    // expected_terminal_score than the right one.
    struct ResultGreater {
        bool operator()(const DecoderResult& left, const DecoderResult& right) const {
            return left.score() > right.score();
        }
    };

    // An TopN beam for search Tokens.
    typedef TopN<Token, TokenGreater> TokenBeam;

    // A pool of pre-allocated tokens to be used in the decoding search space.

    // A DecoderState represents a potential suggestion. It encodes a lexicon link
    // (lexicon_id and node_id) that represents the current term, and a
    // word_history_id that represents the previous terms (if any) decoded before
    // the current term. At each point in time, only one best Token is kept for each
    // active DecoderState.
    //
    // Note: For gesture input only, the token state also incorporates the
    // aligned_key. This is used to distinguish gesture tokens that are in-transit
    // to different possible keys for the same lexical codepoint (e.g., the Spanish
    // keyboard layout has keys for 'n' and 'ñ', which can both align to 'ñ').
    //
    // Note: This currently does not support digraphs with repeated keys or larger
    // multigraphs. In order to do so, we would need to add a digaph_index.
    //
    // Also, because the state does include the prev_key, the gesture decoding
    // process can be slightly greedy in that it only maintains the one best way of
    // reaching the next lexical state. For example, in the case of the word 'über',
    // the [üb] token can be reached by either an alignment to [u] or [ue]. When
    // both [u] and [ue] drop out of the beam, the [üb] token is stuck with the
    // best scoring of the two possible prev_keys. Since the gesture transit
    // score depends on the previous key, this can affect the score for the
    // remaining gesture points leading up to 'b'. In practice this should rarely be
    // a problem. If the two possible prev_keys [u] and [e] are not right next to
    // each other, then the correct prev_key one should have a much better alignment
    // score. If, on the other hand, the two prev_keys are right next to each other
    // (or even overlapping), then the future transit-scores for each prev_key
    // should be very similar and any difference is thus unlikely to affect the
    // final results.
    //
    // TODO(ouyang): Consider adding the additional parameters to the DecoderState,
    // such as prev_key and digaph/multigraph_index.
    struct DecoderState {
        int8 lexicon_id;
        uint64 node_id;
        int32 word_history_id;
        KeyId aligned_key;

        bool operator==(const DecoderState& s) const {
            return node_id == s.node_id && word_history_id == s.word_history_id &&
                   lexicon_id == s.lexicon_id && aligned_key == s.aligned_key;
        }
    };

    class TokenPool {
    public:
        explicit TokenPool(const int capacity) : tokens_(), free_tokens_() {
            Reset(capacity);
        }

        // Releases all tokens from the pool and resets the size. Note that after
        // calling this method no previously allocated tokens can be used.
        void Reset(const int capacity) {
            if (capacity == tokens_.size() && capacity == free_tokens_.size()) {
                // No need to reset.
                return;
            }
            tokens_.resize(capacity);
            tokens_.shrink_to_fit();
            free_tokens_.clear();
            for (auto& token : tokens_) {
                free_tokens_.push_back(&token);
            }
        }

        // Allocate an new token from the pool. Returns null If no new tokens are
        // available. Any tokens allocated using this method will need to be returned
        // by calling ReleasePooledToken.
        inline Token* NewPooledToken() {
            if (free_tokens_.empty()) {
                return nullptr;
            }
            Token* const token = free_tokens_.back();
            free_tokens_.pop_back();
            return token;
        }

        // Release a token that has been allocated from the pool by NewPooledToken().
        // The token must not be used after being released.
        inline void ReleasePooledToken(Token* token) {
            free_tokens_.push_back(token);
        }

        // Returns the number of tokens that are still free and available to be
        // allocated.
        inline int FreeCount() const { return free_tokens_.size(); }

        // Returns the maximum capacity of the token pool.
        inline int capacity() const { return tokens_.size(); }

        //private:
        std::vector<Token> tokens_;
        std::deque<Token*> free_tokens_;
    };

    struct DecoderStateHasher {
        std::size_t operator()(const DecoderState& c) const {
            return util_hash::Hash(c.lexicon_id, c.node_id, c.word_history_id);
        }
    };

    typedef std::unordered_map<DecoderState, Token*, DecoderStateHasher>
            StateToTokenMap;

    class GestureDecoder {
    public:
        GestureDecoder(bool isTest);

        void setMainParams(LoudsLmParams params);

        void AddLexiconAndLm(const std::string &lm_name, LexiconInterface *lexicon,
                             std::unique_ptr<LanguageModelInterface> lm);

        void RecreateDecoderForActiveLms();

        void SetKeyboardLayout(KeyboardLayout layout) {
            keyboard_layout_ = layout;
            gesture_keyboard_.reset(Keyboard::CreateKeyboardOrNull(keyboard_layout_).release());
        }
        vector<DecoderResult> DecodeTouch(TouchSequence* sequence, Utf8String prev);

        float GetAutocorrectThreshold(float top_result_score, int touch_points_size){
            const float autocorrect_threshold =
                            params_.autocorrect_threshold_base +
                            touch_points_size *
                            params_.autocorrect_threshold_adjustment_per_tap;
            return std::min(1.0f, autocorrect_threshold / top_result_score * 0.5f);
        };

        void PredictNextTerm(
                const vector<Utf8StringPiece>& decoded_terms, int max_predictions,
                std::map<Utf8String, LogProbFloat>* top_predictions);

        // Initializes the given token to the root(s) of the lexicon(s). This function
        // will cache the root token at the first call and uses the cached value in
        // following calls.
        void GetRootToken(Token* token);

        // Returns a new search token from a pre-allocated token pool.
        // Returns null if no new search tokens are available.
        // Note that this token needs to be added to the search space using the method
        // DecoderSession::AddSearchTokenToSearchSpace.
        inline Token* NewSearchToken() {
            return search_space_token_pool_->NewPooledToken();
        }

        // Add a new search token to the search space. Note that the token must have
        // been allocated by using DecoderSession::NewSearchToken.
        void AddSearchTokenToSearchSpace(Token* token);

        // Get the DecoderState for the given node and history_id.
        DecoderState GetDecoderStateForNode(const CodepointNode& node,
                                            const KeyId aligned_key,
                                            const int history_id) const;

        // Prune the search tokens if the free token ratio falls below this value.
        static constexpr float kPruneWhenFreeRatioBelow = 0.1f;

        // Prune this proportion of the worst search tokens.
        static constexpr float kPruneRatio = 0.5f;

    private:
        // Processes the touch point at the supplied index.  Assumes that all tokens
        // in the search space have been processed up to index - 1 (if index > 0).
        // This involves the following steps:
        //
        // 1. If index > 0, advance all tokens in the search space to their
        //    alignment for the point at index - 1 and remove tokens for which no
        //    alignment was computed (because they scored too poorly).
        //
        // 2. Extract the set of top-scoring tokens for point index.  This set is
        //    the search beam for this index.
        //
        // 3. Pass the tokens in the beam to index, i.e. create alignments for the
        //    case where these tokens are still in transit to the next key.
        //
        // 4. Expand all tokens in the beam to their children, pruning the search
        //    space whenever necessary.  (Including next-term children.)
        //
        // The decoder_debug argument is optional and may be nullptr.
        void ProcessNextTouchPoint(const int index);

        // For gestures, passes the tokens in the supplied beam to the touch point
        // at index.  Assumes all tokens have advanced to (index - 1) before this
        // method is called.  This method updates active_beam_min_score_, which
        // allows us to do skip expanding tokens whose children wouldn't make it
        // into the beam.
        void PassGestureTokensInBeam(const std::unordered_set<Token*>& beam,
                                     const int index);

        // Advance all of the active tokens to the next index. It prunes out
        // any tokens that are now obsolete (e.g., were not advanced at all in the
        // the previous time frame). For still-active tokens, it replaces their
        // cur_alignment with next_alignment, and clears next_alignment (in
        // preparation for the next touch point).
        //
        // The decoder_debug argument is optional and may be nullptr.
        //
        // Returns:
        //   A set of the top active tokens to be advanced by the decoder. This
        //   typically represents a subset of all active states/tokens maintained in
        //   the search space, to speed up decoding. Only these tokens will be
        //   specifically activated and passed to the next time frame. However, a
        //   token that did not make it into this top set is still useful because it
        //   may be activated by another token (e.g., as a lexical child).
        //
        //   Note: The ownership of the tokens is retained by this class, and the
        //   pointers are only guaranteed to be valid only the next call to
        //   DecoderSession::AdvanceToNextIndex.
        std::unordered_set<Token*> AdvanceToNextIndexAndReturnTopTokens(
                const int next_index);

        // Enumerates the different types of character alignments that can be applied
        // to a token.
        enum DecoderAlignType {
            // Treat the next touch point as a regular alignment to the next character.
                    ALIGN_NORMAL,
            // Treat the next touch point as an alignment to the next-next character,
            // skipping the next character: ("ths" -> "this").
                    ALIGN_OMISSION,
            // Treat the next touch point as the start of a new term that has just re-
            // entered the lexicon at the root (with or without an explicit space).
                    ALIGN_REENTRY,
        };

        // Expands token to all of its possible destinations for the next touch
        // point (i.e., time frame).  Adds any terminal tokens that were reached to
        // top_terminals.
        //
        // Args:
        //   next_index      - The touch index for the next time frame.
        //   align_type      - The type of alignment that resulted in the expansion.
        //   token           - The token to expand.
        //   reentry_tokens  - Populated with the top reentry tokens that have already
        //                     reached the end of the previous word.
        void ExpandToken(const int next_index, const DecoderAlignType align_type, Token* token,
                         TokenBeam* reentry_tokens);

        // The version of ExpandToken for gesture typing input.
        void ExpandTokenGesture(const int decoded_size, Token* token,
                                const DecoderAlignType align_type,
                                TokenBeam* reentry_tokens);

        // Whether the token should be expanded to its children. This returns false
        // for gesture tokens that are still in-transit to the current key and do
        // not need to be expanded yet, as well as gesture tokens that wouldn't
        // score well enough to make it into the active beam if aligned to the
        // previous point.
        bool ShouldExpandToChildren(const Token* token) const;

        // Returns the score to beat for non-correction candidates.
        float ScoreToBeat() const {
            return best_score_ + params_.score_to_beat_offset;
        }
        // Returns the score to beat for multi-term candidates.
        float ScoreToBeatForMultiTerm() const {
            return best_score_ + params_.score_to_beat_offset_for_corrections;
        }

        float GetConditionalLanguageModelScore(
                const vector<Utf8StringPiece>& term_sequence,
                const Token& terminal_token);

        // Returns the (log of the linear-interpolated) conditional probability of the
        // last term in the term sequence from the language model scorers.
        float DecodedTermsConditionalLogProb(const vector<Utf8StringPiece>& terms);

        // Returns the set of possible keys that can align to the given code.
        // This is cached within the DecoderSession to speed up repeated calls.
        //
        // See Keyboard::GetPossibleKeysForCode.
        const vector<KeyId>& GetPossibleKeysForCode(const char32 code);


        // Pass next_token to the next touch point alignment at next_index, given
        // original_token, which is aligned to the previous touch point.
        //
        // If next_token == token, then we are staying in the same state. Treat the
        // next_index touch point as still "in-transit" to the same key as before.
        //
        // If next_token != token, then we are moving to a new state. Treat the
        // previous touch point as the end of original_token's alignment to its key,
        // and the next touch point as the first point advancing towards
        // next_token's key.
        //
        // Note that in each time frame, a single DecoderState may receive updates
        // from multiple original_tokens. This allows the decoder to maintain
        // multiple paths for reaching the same state, and select the optimal
        // Viterbi path. See the paper reference in decoder.h for more details.
        //
        // Args:
        //   original_token  - The original token to be advanced.
        //   next_index      - The index for the next touch point.
        //   touch_sequence  - The input touch_sequence.
        //   next_token      - The destination token for the next touch point.
        //
        // Returns: Whether the next_token was updated.
        bool PassTokenGesture(const Token* original_token, const int next_index,
                              const TouchSequence* touch_sequence,
                              Token* next_token) const;

        // Whether the token should be considered. Tokens that are too far from the
        // top active token will be pruned from the search.
        bool ShouldConsiderToken(const Token* token) const;

        // Whether the (terminal) token should be considered for extension to more
        // terms.
        bool ShouldConsiderMultiTerm(const Token* token) const;

        // Gets the unigram score for the token from its lexicon(s).
        static float GetUnigramScore(const Token& token);

        // Returns whether or not the given code is skippable, meaning that it may
        // be omitted in gesture or touch typing. For example users may omit the
        // apostrophe, typing the letters [hes] for the word [he's].
        static inline bool IsSkippableCharCode(const int code) {
            return (code == '\'' || code == '-');
        }

        /*************************************
         * Token Passing Algorithm functions *
         ************************************/

        // Initialize a next-word token for re-entry into the root of the lexicon.
        // This treats the terminal_token as the end of the previous word.
        void InitializeReentryToken(const Token& terminal_token, const float penalty,
                                    const int next_key, Token* reentry_token);


        // Finds the search token that matches the given input. Returns nullptr if no
        // such token exists in the search space.
        Token* FindSearchToken(const vector<CodepointNode>& nodes,
                               const int word_history_id, const int next_key);

        // Get the sequence of decoded terms for the given token.
        //
        // Args:
        //   token              - The token from which to extract the terms.
        //   last_term_storage  - The string in which to store the last term,
        //                        representing the token's current lexical state.
        //   decoded_terms      - The sequence of all decoded terms in the token
        //                        (including last_term_storage as the last term).
        //
        // Note: The output parameter decoded_terms is only valid while
        // last_term_storage is valid.
        void GetDecodedTerms(const Token& token, Utf8String* last_term_storage,
                             vector<Utf8StringPiece>* decoded_terms) const;

        // Returns the score for interpreting the touch point at index as an alignment
        // to the space key.
        float GetAlignToSpaceScore(const int index) const;

        // Get the word history (i.e., sequence of previously decoded terms)
        // associated with the given id.
        const vector<Utf8String>& GetWordHistory(int id) const;

        // Find or create the child token (referenced by the given nodes) for the
        // given parent token. This will only create a new Token if the associated
        // DecoderState has no Token assigned to it yet. Otherwise, it will return
        // a pointer to the pre-existing Token. This allows the decoder to explore
        // multiple ways of reaching the same child Token at each time frame.
        //
        // Args:
        //   nodes       - The nodes that represent the new lexicon term.
        //   parent      - The parent token that was passed this state. The new token
        //                 will share the parent's word_history_id.
        //   next_key    - The new aligned key for the new Token.
        //
        // Returns:
        //   A pointer to the unique Token associated with the child node's
        //   DecoderState. Note that the ownership of the tokens is retained by this
        //   class, and the pointers are only guaranteed to be valid only the next
        //   call to DecoderSession::AdvanceToNextIndex.
        //
        //   Note: May return nullptr if there are no free search tokens in the token
        //   pool (though this should not happen in practice with proper pruning).
        Token* FindOrCreateChildToken(const vector<CodepointNode>& nodes,
                                      const Token& parent, const int next_key);

        // Get the top 'max_completions' completions from the given prefix node based
        // on the unigram prefix probabilities. These results will likely need to be
        // rescored by the full language model.
        void GetBestCompletionsForNode(
                const CodepointNode& start_node, int max_completions,
                std::map<Utf8String, LogProbFloat>* completions) const;
        /*************************************
        *    Result Processing Functions     *
        *************************************/
        // Get or create a new word_history by appending a new term to a previous
        // word_history referenced by prev_word_history_id. Returns a new word_history
        // id for the combined terms. If prev_word_history_id does not exist, then
        // only the new term will be used.
        int GetOrAddWordHistory(int prev_word_history_id, const Utf8String& new_term);


        // At the end of the decoding process, add any prefix-completions to the
        // suggested results (e.g., "birthd" -> "birthday")
        void ProcessPrefixCompletions(TokenBeam* top_prefixes,
                                      vector<DecoderResult>* prediction_results);

        // Applies final spatial score adjustments to the set of results. This is
        // mainly to promote certain results that represent perfectly typed letters or
        // very precise gestures.
        void ApplyScoreAdjustments(vector<DecoderResult>* results) const;

        // Process a token that has reached the end of the input. Populates the
        // results vector with any DecoderResults from the token (if it is a terminal)
        // and the top_prefixes beam if the token is a prefix of a longer term.
        void ProcessEndOfInput(Token* token, vector<DecoderResult>* results,
                               TokenBeam* top_prefixes);

        // Process the token as an end-of-input terminal, meaning that it represents
        // a complete word. The terminal is added to the output results vector.
        void ExtractEndOfInputTerminal(const Token& terminal_token,
                                       vector<DecoderResult>* results);

        /************************************
         *        Decoding variables        *
         ***********************************/

        // The input touch sequence representation for the search.
        const TouchSequence* touch_sequence() const { return touch_sequence_.get(); }

        // The keyboard representation for the search.
        const Keyboard* keyboard() const { return gesture_keyboard_.get(); }

        LoudsLmParams main_lm_params;
        DecoderParams params_;

        // The interpolated LM for the currently active LMs.
        std::unique_ptr<InterpolatedLm> interpolated_lm_;

        // The map between static language model names and their respective
        // LanguageModelInterfaces. This map has ownership of the actual LMs.
        // Note: static LMs are always considered "active" for decoding.
        std::map<string, std::unique_ptr<LanguageModelInterface>> static_lms_;

        // The map between static lexicon names and their respective pointers.
        // Note: this map does not own the actual lexicons. E.g., in the case of
        // LoudsLmAdapter, the lexicon is owned by the parent LoudsLm.
        std::map<string, LexiconInterface *> static_lexicons_;
        // The list of lexicons to use during decoding.
        std::vector<const LexiconInterface *> lexicon_interfaces_;

        // Get the index of the given lexicon.
        int GetLexiconId(const LexiconInterface* lexicon) const {
            for (size_t i = 0; i < lexicon_interfaces_.size(); ++i) {
                if (lexicon_interfaces_[i] == lexicon) {
                    return i;
                }
            }
            CHECK(false) << "Lexicon not found.";
        }

        // The list of language models to use during decoding.
        std::vector<const LanguageModelInterface *> lm_interfaces_;

        KeyboardLayout keyboard_layout_;

        // The language model scorer(s) for this search.
        vector<std::unique_ptr<LanguageModelScorerInterface>> lm_scorers_;

        // The preceding text for this search.
        Utf8String preceding_text_;
        // The following text for this search.
        Utf8String following_text_;

        // A cached token that represent the root(s) of lexicon(s).
        std::unique_ptr<Token> root_token_cache_;
        // A pool of pre-allocated tokens to be used in the search space. All of the
        // tokens added to the search_space_ should come from this pool.
        std::unique_ptr<TokenPool> search_space_token_pool_;

        // The map between active search states and their corresponding tokens. Only
        // the top scoring token is kept for each state (e.g., lexical node).
        // Note that all of the token pointers in this map should come from the
        // search_space_token_pool_, and will be released when the state is no
        // longer active.
        StateToTokenMap search_space_;

        // Stores the worst score of the tokens being processed, i.e. the current
        // beam, or NEG_INF if the beam is not full.  Used to avoid generating child
        // tokens that won't score well enough to be retained.  Currently this is
        // only used for gestures.
        float active_beam_min_score_;

        // A vector of temporary scores used for ranking and pruning.
        std::vector<float> temp_scores_;

        // The set of top tokens that should be advanced by the decoder in each time
        // frame. The size should be equal to DecoderParams::active_beam_width.
        std::unordered_set<Token*> top_tokens_set_;

        // Prunes the specified ratio of search tokens from the search space, freeing
        // up spaces in the token pool for new tokens.
        //
        // WARNING: This call may invalidate any token not in the top_tokens_set_.
        // The caller must ensure that these tokens will no longer be referenced
        // directly. Pruned tokens may be re-created (as needed) by
        // FindOrCreateChildToken.
        void PruneSearchTokensOutsideTopTokensSet();

        // The best score for the current active tokens.
        float best_score_;

        // The index of the last point in the touch sequence that has been decoded
        // (i.e., the index of the last call to AdvanceAllTokensInBeam). All active
        // tokens advance at the same rate, so they should all share this index.
        int decoded_index_;

        // A map to store the possible word_histories (i.e., previous term sequences)
        // for the active tokens in the session. This is highly efficient since many
        // tokens will share the same word_history. Also, the word_history_id key is
        // used to identify unique DecoderStates, distinguishing between Tokens that
        // have the same nodes (i.e., current term) but different word histories.
        std::unordered_map<int, vector<Utf8String>> word_histories_;

        // The input touch sequence representation for the search.
        std::unique_ptr<TouchSequence> touch_sequence_;

        std::unique_ptr<const Keyboard> gesture_keyboard_;

        // A local cache that stores the mapping between codepoints and their
        // corresponding keyboard keys.
        std::unordered_map<char32, vector<KeyId>> codes_to_keys_map_;

        // A counter that keeps track of the next word_history_id to be assigned.
        int next_word_history_id_;

        // The next word predictions for the current session.
        map<Utf8String, LogProbFloat> next_word_predictions_;

    };
}
}


#endif //SIMPLEGESTUREINPUT_GESTUREDECODER_H
