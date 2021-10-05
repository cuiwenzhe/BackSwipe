#include "token.h"

namespace keyboard {
namespace decoder {

    void Token::InitializeAsChild(const vector<CodepointNode>& nodes,
                                  const Token& parent,
                                  const int aligned_key,
                                  const DecoderParams& params) {
        nodes_ = nodes;
        aligned_key_ = aligned_key;
        prev_aligned_key_ = parent.aligned_key_;
        omitted_key_ = parent.omitted_key_;
        prefix_lm_score_ = parent.prefix_lm_score_;
        prev_lm_score_ = parent.prev_lm_score_;
        word_history_id_ = parent.word_history_id_;
        cur_alignment_ = parent.cur_alignment_;
        next_alignment_ = parent.next_alignment_;
        children_ = nullptr;
        UpdatePrefixLmScore(params);
    }

    void Token::InitializeAsNextTerm(const Token& terminal_token,
                                     const int new_word_history_id,
                                     const float term_lm_score,
                                     const int next_key) {
        omitted_key_ = -1;
        prefix_lm_score_ = 0.0f;
        word_history_id_ = new_word_history_id;
        prev_lm_score_ = terminal_token.prev_lm_score_ + term_lm_score;
        if (next_key != Keyboard::kInvalidKeyId) {
            aligned_key_ = next_key;
            prev_aligned_key_ = terminal_token.aligned_key_;
            cur_alignment_ = {terminal_token.index(), NEG_INF,
                              terminal_token.align_score()};
            next_alignment_.InvalidateScore();
        } else {
            aligned_key_ = terminal_token.aligned_key_;
            prev_aligned_key_ = terminal_token.prev_aligned_key_;
            cur_alignment_ = terminal_token.cur_alignment_;
            next_alignment_ = terminal_token.next_alignment_;
            cur_alignment_.InvalidateTransitScore();
            next_alignment_.InvalidateTransitScore();
        }
    }

    void Token::ExtractChildrenIfNeeded() {
        if (children_ != nullptr) {
            return;
        }
        children_ = std::make_shared<map<char32, vector<CodepointNode>>>();
        vector<CodepointNode> child_nodes;
        for (const CodepointNode& node : nodes_) {
            node.GetChildCodepoints(&child_nodes);
        }
        // Combine all child nodes with the same character key into one token.
        // This enables simultaneous traversal of multiple language models, and
        // reduces the need to perform redundant alignments on common prefixes.
        for (const auto& child_node : child_nodes) {
            const int child_code = child_node.codepoint();
            (*children_)[child_code].push_back(child_node);
        }
    }

}  // namespace decoder
}  // namespace keyboard
