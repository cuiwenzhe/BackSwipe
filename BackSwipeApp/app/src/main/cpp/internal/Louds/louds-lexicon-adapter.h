// A wrapper for LoudsLexicon that allows it to be used in the keyboard decoder.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_LM_LOUDS_LOUDS_LEXICON_ADAPTER_H_
#define INPUTMETHOD_KEYBOARD_DECODER_LM_LOUDS_LOUDS_LEXICON_ADAPTER_H_

#include <memory>
#include <vector>

#include "../lexicon-interface.h"
#include "../lexicon-node.h"
#include "louds-lexicon.h"

namespace keyboard {
namespace decoder {
namespace lm {

    using keyboard::lm::louds::LexiconTermId;
    using keyboard::lm::louds::LoudsLexicon;
    using keyboard::lm::louds::Utf8CharTrie;
    using namespace std;

    class LoudsLexiconAdapter : public LexiconInterface {
    public:
        // Creates a new LoudsLexiconInterface for the given LoudsLexicon.
        // Note: Does not take ownership of the LoudsLexicon.
        explicit LoudsLexiconAdapter(const LoudsLexicon* lexicon)
                : lexicon_(lexicon) {}

        ~LoudsLexiconAdapter() override;

        // Returns the underlying LoudsLexicon.
        const LoudsLexicon* louds_lexicon() const { return lexicon_; }

        // Returns whether or not the term is in the lexicon.
        bool IsInVocabulary(const Utf8StringPiece& term) const {
            if (keyboard::lm::IsReservedTerm(term)) {
                return true;
            }
            const LexiconTermId node_id = lexicon_->KeyToNodeId(term);
            if (node_id == Utf8CharTrie::kInvalidId) {
                return false;
            }
            float unused_logp;
            return lexicon_->TermLogProbForNodeId(node_id, &unused_logp);
        }

        ////////////////////////////////////////////////////////////////////////////
        // The following methods are inherited from LexiconInterface.
        // Please refer to the comments for that class.
        ////////////////////////////////////////////////////////////////////////////

        LexiconNode GetRootNode() const override;

        Utf8String GetKey(const LexiconNode& node) const override {
            return lexicon_->NodeIdToKey(node.id);
        }

        void GetChildren(const LexiconNode& node,
                         vector<LexiconNode>* children) const override;

        bool TermLogProb(const LexiconNode& node, float* prob) const override;

        bool PrefixLogProb(const LexiconNode& node, float* prob) const override;

        bool HasPrefixProbabilities() const override {
            return lexicon_->has_prefix_unigrams();
        }

        bool EncodesCodepoints() const override { return false; }

    private:
        // The underlying LoudsLexicon.
        const LoudsLexicon* lexicon_;
    };

}  // namespace lm
}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_LM_LOUDS_LOUDS_LEXICON_ADAPTER_H_
