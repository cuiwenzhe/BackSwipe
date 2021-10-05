#include "louds-lexicon-adapter.h"
#include "../basic-types.h"
#include "louds-trie.h"
//#include "thread/threadlocal.h"
//#include <boost/thread/tss.hpp>

namespace keyboard {
namespace decoder {
namespace lm {

    using keyboard::lm::louds::LoudsNodeId;
    using keyboard::lm::louds::QuantizedLogProb;
    using keyboard::lm::louds::TermChar;

    using keyboard::decoder::LexiconInterface;
    using keyboard::decoder::LexiconNode;
    using keyboard::decoder::Utf8String;

    // A reusable per-thread cache to store intermediate child labels.
    //static ThreadLocal<vector<TermChar>> child_labels_cache_;
    //static boost::thread_specific_ptr<vector<TermChar>> child_labels_cache_;
    static vector<TermChar>* child_labels_cache_;

    // A reusable per-thread cache to store intermediate child node ids.
    //static ThreadLocal<vector<LoudsNodeId>> child_node_ids_cache_;
    //static boost::thread_specific_ptr<vector<LoudsNodeId>> child_node_ids_cache_;
    static vector<LoudsNodeId>* child_node_ids_cache_;

    LoudsLexiconAdapter::~LoudsLexiconAdapter() {}

    LexiconNode LoudsLexiconAdapter::GetRootNode() const {
        return {'\0', keyboard::lm::louds::Utf8CharTrie::kRootNodeId};
    }

    void LoudsLexiconAdapter::GetChildren(const LexiconNode& node,
                                          vector<LexiconNode>* children) const {
        //vector<TermChar>* child_labels = child_labels_cache_.pointer();
        //vector<LoudsNodeId>* child_node_ids = child_node_ids_cache_.pointer();
        if(child_labels_cache_ == nullptr)
            child_labels_cache_ = new vector<TermChar>();
        vector<TermChar>* child_labels = child_labels_cache_;
        if(child_node_ids_cache_ == nullptr)
            child_node_ids_cache_ = new vector<LoudsNodeId>();
        vector<LoudsNodeId>* child_node_ids = child_node_ids_cache_;
        child_labels->clear();
        child_node_ids->clear();
        const LoudsNodeId node_id = static_cast<LoudsNodeId>(node.id);
        lexicon_->GetChildren(node_id, child_labels, child_node_ids);
        children->reserve(child_labels->size());
        for (int i = 0; i < child_labels->size(); ++i) {
            children->push_back(
                    {(*child_labels)[i], static_cast<uint64_t>((*child_node_ids)[i])});
        }
    }

    bool LoudsLexiconAdapter::TermLogProb(const LexiconNode& node,
                                          float* prob) const {
        return lexicon_->TermLogProbForNodeId(node.id, prob);
    }

    bool LoudsLexiconAdapter::PrefixLogProb(const LexiconNode& node,
                                            float* prob) const {
        if (!lexicon_->has_prefix_unigrams()) {
            return false;
        }
        return lexicon_->PrefixLogProbForNodeId(node.id, prob);
    }

}  // namespace lm
}  // namespace decoder
}  // namespace keyboard
