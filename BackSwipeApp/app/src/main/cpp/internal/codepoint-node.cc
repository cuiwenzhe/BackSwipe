#include "codepoint-node.h"

#include "base/logging.h"
#include "lexicon-node.h"
//#include "thread/threadlocal.h"
//#include <boost/thread/tss.hpp>

namespace keyboard {
namespace decoder {

    // A reusable cache to store intermediate lexicon nodes.
    //static ThreadLocal<vector<LexiconNode>> lexicon_node_cache_;
    //static boost::thread_specific_ptr<vector<LexiconNode>> lexicon_node_cache_;
    static vector<LexiconNode>* lexicon_node_cache_;

    // static
    CodepointNode CodepointNode::GetRootNode(const LexiconInterface* lexicon) {
        LexiconNode root = lexicon->GetRootNode();
        return CodepointNode(root, 0, lexicon);
    }

#include <android/log.h>
    void CodepointNode::GetChildCodepoints(vector<CodepointNode>* children) const {
        if(lexicon_node_cache_ == nullptr){
            lexicon_node_cache_ = new vector<LexiconNode>();
        }
        vector<LexiconNode>* cache = lexicon_node_cache_;
//        if(lexicon_node_cache_.get() == NULL)
//            lexicon_node_cache_.reset(new vector<LexiconNode>());
//        vector<LexiconNode>* cache = lexicon_node_cache_.get();
        const int initial_children_size = children->size();
        cache->clear();
        lexicon_->GetChildren(lexicon_node_, cache);
        const int cached_node_count = cache->size();
        if (lexicon_->EncodesCodepoints()) {
            for (int i = 0; i < cached_node_count; ++i) {
                // No need to expand UTF-8 codes since the underlying lexicon already
                // encodes char32 codepoints.
                const auto& node = (*cache)[i];
                children->emplace_back(node, node.c, lexicon_);
            }
        } else {
            char32 codepoint = 0;
            for (int i = 0; i < cached_node_count; ++i) {
                const auto& node = (*cache)[i];
                int remaining_bytes = 0;
                const char c = node.c;
                if (c <= 0x7f) {
                    codepoint = c;
                    remaining_bytes = 0;
                } else if (c <= 0xbf) {
                    CHECK(false) << "First byte of utf8 character should not be between "
                            "0x80 and 0xbf.";
                    return;
                } else if (c <= 0xdf) {
                    codepoint = c & 0x1f;
                    remaining_bytes = 1;
                } else if (c <= 0xef) {
                    codepoint = c & 0x0f;
                    remaining_bytes = 2;
                } else {
                    codepoint = c & 0x07;
                    remaining_bytes = 3;
                }
                CodepointNode child_node(node, codepoint, lexicon_);
                if (remaining_bytes == 0) {
                    children->push_back(child_node);
                } else {
                    child_node.ExpandUTF8Node(remaining_bytes, children);
                }
            }
        }
        if (lexicon_->HasPrefixProbabilities()) {
            float child_prefix_logp;
            const int children_count = children->size();
            for (int i = initial_children_size; i < children_count; ++i) {
                CodepointNode& child_node = (*children)[i];
                const bool has_prefix_logp =
                        lexicon_->PrefixLogProb(child_node.lexicon_node_, &child_prefix_logp);
                child_node.prefix_logp_ =
                        has_prefix_logp ? child_prefix_logp : prefix_logp_;
            }
        }
    }

    void CodepointNode::ExpandUTF8Node(const int remaining_bytes,
                                       std::vector<CodepointNode>* results) const {
        std::vector<LexiconNode> next_utf8_bytes;
        lexicon_->GetChildren(lexicon_node_, &next_utf8_bytes);
        CHECK_GT(next_utf8_bytes.size(), 0);
        for (const auto& next_utf8_byte : next_utf8_bytes) {
            const char c = next_utf8_byte.c;
            const char32 new_codepoint = (codepoint_ << 6) | (c & 0x3f);
            CodepointNode child_node(next_utf8_byte, new_codepoint, lexicon_);
            if (remaining_bytes == 1) {
                results->push_back(child_node);
            } else {
                child_node.ExpandUTF8Node(remaining_bytes - 1, results);
            }
        }
    }

}  // namespace decoder
}  // namespace keyboard
