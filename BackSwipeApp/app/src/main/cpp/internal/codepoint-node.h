// A wrapper for LexiconNode that allows the decoder to traverse codepoints
// rather than UTF-8 characters.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_CODEPOINT_NODE_H_
#define INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_CODEPOINT_NODE_H_

#include <vector>

#include "base/integral_types.h"
#include "lexicon-interface.h"
//#include "testing/production_stub/public/gunit_prod.h"

namespace keyboard {
namespace decoder {

    using std::vector;

    // A wrapper for a LexiconNode that supports lookups and node-by-node
    // traversals based on unicode codepoints (char32) rather than by UTF8
    // characters (char8).
    class CodepointNode {
    public:
        // Constructor used by GetRootNode, GetChildCodepoints and tests.
        CodepointNode(const LexiconNode& lexicon_node, const char32 codepoint,
                      const LexiconInterface* lexicon)
                : lexicon_node_(lexicon_node),
                  codepoint_(codepoint),
                  prefix_logp_(0.0f),
                  lexicon_(lexicon) {}

        // Get the root node of the lexicon, which is initialized with a codepoint
        // value of 0 and a PrefixLogProb of 0.0. Note that the parent lexicon must
        // remain valid during the lifetime of this node and any of its descendents.
        static CodepointNode GetRootNode(const LexiconInterface* lexicon);

        // Returns the unicode codepoint represented by this node.
        char32 codepoint() const { return codepoint_; }

        // Returns the parent Lexicon for this node.
        const LexiconInterface* lexicon() const { return lexicon_; }

        // Returns the prefix probability for the codepoint node.
        // For LexiconNodes that do not have a prefix logprob, this will return the
        // prefix logprob of the nearest ancestor. If Lexicon::HasPrefixProbabilities
        // is false, then it returns 0.0.
        float PrefixLogProb() const { return prefix_logp_; }

        // Retrieves the complete term log probability for the codepoint node.
        // Returns false if not a complete term (see LexiconInterface::PrefixLogProb).
        bool TermLogProb(float* value) const {
            return lexicon_->TermLogProb(lexicon_node_, value);
        }

        // Gets the key string associated with this node.
        Utf8String GetKey() const { return lexicon_->GetKey(lexicon_node_); }

        // See LexiconInterface::IsEndOfTerm.
        bool IsEndOfTerm() const { return lexicon_->IsEndOfTerm(lexicon_node_); }

        // Get a list of child codepoint nodes for this node. This automatically
        // expands multi-byte characters until it reaches the end of the codepoint.
        void GetChildCodepoints(vector<CodepointNode>* children) const;

        // Get the unique identifier for the underlying LexiconNode with respect to
        // the lexicon containing it.
        uint64 GetNodeData() const { return lexicon_node_.id; }

    private:
        // Expand a node that represents the start of (one or more) multi-byte UTF8
        // characters. This method automatically converts the bytes it finds into
        // the final unicode codepoint.
        void ExpandUTF8Node(const int remaining_bytes,
                            vector<CodepointNode>* results) const;

        LexiconNode lexicon_node_;
        char32 codepoint_;
        float prefix_logp_;
        const LexiconInterface* lexicon_;
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_INTERNAL_CODEPOINT_NODE_H_
