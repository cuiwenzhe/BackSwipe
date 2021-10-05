// Description:
//   A token incorporates knowledge about the current alignment score (i.e., log
//   likelihood) for a given search position in the lexicon. This is very
//   similar to the token passing model used in speech recognition (Young et
//   al., Token Passing: a Simple Conceptual Model for Connected Speech
//   Recognition Systems. 1989).

#ifndef INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_TOKEN_H_
#define INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_TOKEN_H_

#include <map>
#include <memory>
#include <vector>

#include "base/integral_types.h"
#include "alignment.h"
#include "codepoint-node.h"
#include "keyboardSetting/keyboard.h"
#include "DecoderParams.h"
#include "base/constants.h"

namespace keyboard {
namespace decoder {

    using namespace std;

    // A token incorporates knowledge about the current alignment score (i.e., log
    // likelihood) for a given search position (node) in the prefix trie. Tokens can
    // be "passed" to connected child nodes (i.e., possible next letters), which
    // also advances the alignment of the gesture to the new character.
    class Token {
    public:
        // Default constructor. Note that a token created by this constructor is not
        // usable since it does not reference any lexical nodes. Requires a call to
        // InitializeAsRoot, InitializeAsChild, or InitializeAsNextTerm.
        Token()
                : nodes_(),
                  aligned_key_(-1),
                  prev_aligned_key_(-1),
                  omitted_key_(-1),
                  prefix_lm_score_(0),
                  prev_lm_score_(0),
                  word_history_id_(-1),
                  cur_alignment_(),
                  next_alignment_(),
                  children_(nullptr) {}

        // Create a token for the given nodes, copy properties from the parent
        // node, and set the aligned key to the given key id.
        Token(const vector<CodepointNode>& nodes, const Token& parent,
              const int aligned_key, const DecoderParams& params)
                : nodes_(nodes),
                  aligned_key_(aligned_key),
                  prev_aligned_key_(parent.prev_aligned_key_),
                  omitted_key_(parent.omitted_key_),
                  prefix_lm_score_(parent.prefix_lm_score_),
                  prev_lm_score_(parent.prev_lm_score_),
                  word_history_id_(parent.word_history_id_),
                  cur_alignment_(parent.cur_alignment_),
                  next_alignment_(parent.next_alignment_),
                  children_(nullptr) {
            UpdatePrefixLmScore(params);
        }

        // Copy constructor.
        Token(const Token& token)
                : nodes_(token.nodes_),
                  aligned_key_(token.aligned_key_),
                  prev_aligned_key_(token.prev_aligned_key_),
                  omitted_key_(token.omitted_key_),
                  prefix_lm_score_(token.prefix_lm_score_),
                  prev_lm_score_(token.prev_lm_score_),
                  word_history_id_(token.word_history_id_),
                  cur_alignment_(token.cur_alignment_),
                  next_alignment_(token.next_alignment_),
                  children_(token.children_) {}

        // Initializes the token as the root of the lexical trie, with no key, empty
        // scores, and empty alignments. If the given children is not nullptr, the
        // token will use it instead of extracting children from nodes.
        void InitializeAsRoot(
                const vector<CodepointNode>& nodes, const DecoderParams& params) {
            nodes_ = nodes;
            aligned_key_ = -1;
            prev_aligned_key_ = -1;
            omitted_key_ = -1;
            prefix_lm_score_ = 0;
            prev_lm_score_ = 0;
            word_history_id_ = -1;
            cur_alignment_.Clear();
            next_alignment_.Clear();
            children_ = nullptr;
            UpdatePrefixLmScore(params);
        }

        // Initialize this token as a lexical child of the parent token.
        // Copies properties from the parent node.
        void InitializeAsChild(const vector<CodepointNode>& nodes,
                               const Token& parent, const int aligned_key,
                               const DecoderParams& params);

        // Initialize this token as a new term following the given terminal
        // (end-of-word) token.
        //
        // If a next_key is provided (i.e., next_key != Keyboard::kInvalidKey), the
        // reentry token will be initialized as in-transit to that key. Otherwise, the
        // token will have the same alignment as the input terminal_token.
        void InitializeAsNextTerm(const Token& terminal_token,
                                  const int new_word_history_id,
                                  const float last_lm_score,
                                  const int next_key);

        // Initialize the child token that represents a repeat of the last letter
        // of the parent token (e.g., "so" to "soo") or a different letter but is
        // overlapped with the last letter of parent token in keyboard layout (e.g.,
        // "h" to "hi" in 9key layout).
        //
        // Since this child token and its parent end in identical keys, we assume
        // that the expanded token should have the same align score as its parent.
        //
        // Returns true if current alignment of this token is updated to a better
        // alignment and needs to be expanded.
        bool InitializeAsRepeatedLetterIfNeeded(const Token& parent,
                                                const DecoderParams& params) {
            prev_aligned_key_ = parent.prev_aligned_key_;

            // We can use parent's next/current align score as this token's based on
            // our assumption.
            // If the alignment of this expansion is better than existing alignment, we
            // need to update token to new alignment. Otherwise, we can skip this
            // expansion.
            if (next_alignment_.BestScore() < parent.next_alignment_.BestScore()) {
                next_alignment_ = parent.next_alignment_;
            }

            if (cur_alignment_.BestScore() <= parent.cur_alignment_.BestScore()) {
                cur_alignment_ = parent.cur_alignment_;
                return true;
            }
            return false;
        }

        // The nodes represented by this token.
        const vector<CodepointNode>* nodes() const { return &nodes_; }

        // The last key aligned by this token, referenced by key index (see
        // Keyboard::GetKeyIndex).
        int aligned_key() const { return aligned_key_; }

        void set_aligned_key(KeyId key) { aligned_key_ = key; }

        // The previous key aligned by this token, referenced by key index (see
        // Keyboard::GetKeyIndex).
        int prev_aligned_key() const { return prev_aligned_key_; }

        // The recently omitted code (if any). This is used to handle transpositions,
        // where the user types keys out of order.
        int omitted_key() const { return omitted_key_; }

        void set_omitted_key(KeyId key) { omitted_key_ = key; }

        // Returns the child nodes for all of the nodes in this Token, mapped by
        // their codepoints.
        const map<char32, vector<CodepointNode>>& children() {
            ExtractChildrenIfNeeded();
            return *children_;
        }

        // Extract the child nodes for this token if it hasn't been done yet.
        void ExtractChildrenIfNeeded();

        // The index in the touch sequence for the current alignment.
        int index() const { return cur_alignment_.index(); }

        int next_index() const { return next_alignment_.index(); }

        // The total score for the token for ranking the search beam.
        float TotalScore() const { return spatial_score() + lm_score(); }

        // The next index score for the token for ranking the search beam.
        float NextTotalScore() const {
            return next_alignment_.BestScore() + lm_score();
        }

        // The align score for the token for ranking the search beam.
        float align_score() const { return cur_alignment_.align_score(); }

        // Sets the align score for the token for ranking the search beam.
        void set_align_score(float score) {
            cur_alignment_ = {cur_alignment_.index(), score, NEG_INF};
        }

        // The transit score for the token for ranking the search beam.
        float transit_score() const { return cur_alignment_.transit_score(); }

        // The spatial score for the token for ranking the search beam.
        float spatial_score() const { return cur_alignment_.BestScore(); }

        // The lm score of the token.
        float lm_score() const { return prev_lm_score() + prefix_lm_score(); }

        // The prefix lm score of the token. This is the maximum prefix log prob
        // of all of the codepoints in the token.
        float prefix_lm_score() const { return prefix_lm_score_; }

        // Adds an arbitrary spatial score adjustment to the current alignment.
        // This is used, for example, to penalize certain types of corrections.
        void AddScore(float score) { cur_alignment_.AddScore(score); }

        // Returns a const pointer to the current alignment.
        const Alignment* alignment() const { return &cur_alignment_; }

        // Returns a mutable pointer to the current alignment.
        Alignment* mutable_alignment() { return &cur_alignment_; }

        // Returns a const pointer to the best next alignment. This will replace
        // the current alignment when the decoding advances to the next touch
        // point.
        const Alignment* next_alignment() const { return &next_alignment_; }

        // Returns a mutable pointer to the best next alignment.
        Alignment* mutable_next_alignment() { return &next_alignment_; }

        // The sum of the language model (e.g., n-gram) scores for the previous
        // decoded terms, if any.
        float prev_lm_score() const { return prev_lm_score_; }

        // Get the word history id for the token. See DecoderSession::GetWordHistory.
        int word_history_id() const { return word_history_id_; }

        // Replace the current alignment with the next alignment and clear the next
        // alignment's scores.
        void AdvanceToNextAlignment() {
            cur_alignment_ = next_alignment_;
            next_alignment_.InvalidateScore();
        }

        // Invalidate the scores for this token.
        void InvalidateScores() {
            cur_alignment_.InvalidateScore();
            next_alignment_.InvalidateScore();
        }

        bool has_prev_terms() const { return word_history_id_ >= 0; }

        // Updates the prefix lm score for the token by querying the underlying
        // node(s). If the token contains multiple nodes (i.e., the prefix exists in
        // multiple lexicons), this method takes the maximum. The prefix logp is then
        // weighted by the prefix_lm_weight from the params.
        void UpdatePrefixLmScore(const DecoderParams& params) {
            prefix_lm_score_ = NEG_INF;
            for (const CodepointNode& node : nodes_) {
                const float score = node.PrefixLogProb();
                if (score > prefix_lm_score_) {
                    prefix_lm_score_ = score;
                }
            }
            prefix_lm_score_ = prefix_lm_score_ * params.prefix_lm_weight;
        }

        // Returns whether or not this token is at a terminal (i.e., at least one
        // of the lexicon nodes is at the end of a complete term).
        bool IsTerminal() const {
            for (const CodepointNode& node : nodes_) {
                if (node.IsEndOfTerm()) {
                    return true;
                }
            }
            return false;
        }

    protected:
        // The following are protected instead of private to allow exposing them for
        // testing purposes.
        void set_prev_aligned_key(KeyId prev_aligned_key) {
            prev_aligned_key_ = prev_aligned_key;
        }

        // These variables are documented in the comments for their accessor methods
        vector<CodepointNode> nodes_;

        KeyId aligned_key_;
        KeyId prev_aligned_key_;
        KeyId omitted_key_;

        float prefix_lm_score_;
        float prev_lm_score_;

        int word_history_id_;

        Alignment cur_alignment_;
        Alignment next_alignment_;

        // Use a shared_ptr because the children map can be shared among multiple
        // tokens.
        std::shared_ptr<map<char32, vector<CodepointNode>>> children_;
    };

// Orders Tokens by expected end_of_alignment score, returns true if the left
// token has a greater expected end_of_input score than the right one.
    struct TokenGreater {
        bool operator()(const Token& left, const Token& right) const {
            return left.TotalScore() > right.TotalScore();
        }
    };

    struct TokenPtrGreater {
        bool operator()(const Token* left, const Token* right) const {
            return left->TotalScore() > right->TotalScore();
        }
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_TOKEN_H_
