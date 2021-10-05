// An interface for defining a lexicon trie (based on UTF-8 chars) that will be
// used during keyboard decoding. It will support certain node level operations
// like GetChildren(...).
//
// The lexicon may also store complete-term or prefix log probabilities,
// which will be used by the decoder if available.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LEXICON_INTERFACE_H_
#define INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LEXICON_INTERFACE_H_

#include <vector>

#include "base/integral_types.h"
#include "base/macros.h"
#include "base/basictypes.h"
#include "lexicon-node.h"

namespace keyboard {
namespace decoder {

    using std::vector;
    // A general interface for the lexicon. Implementations are not required to be
    // thread-safe.
    //
    // Warning: mutable operations in implementations that add them may invalidate
    // existing LexiconNode objects.
    class LexiconInterface {
    public:
        virtual ~LexiconInterface() {}

        // Get the root node of the lexicon trie.
        virtual LexiconNode GetRootNode() const ABSTRACT;

        // Get the UTF-8 string key associated with the given node.
        virtual Utf8String GetKey(const LexiconNode& node) const ABSTRACT;

        // Get the set of children for the given node.  The children will be
        // appended to the output vector.
        virtual void GetChildren(const LexiconNode& node,
                                 vector<LexiconNode>* children) const ABSTRACT;

        // Get the complete term log probability for a given node.
        //
        // Returns false if the node is a prefix instead of a complete term.  See
        // also: IsEndOfTerm().
        virtual bool TermLogProb(const LexiconNode& node, float* prob) const ABSTRACT;

        // Get the prefix log probability for a node. This is the maximum log
        // probability of all terms that start with this prefix.
        //
        // Returns false if the node does not have a prefix probability. In this case,
        // the decoder will use the prefix probability of the parent prefix. This
        // allows the trie to save space by not adding redundant prefixes.
        // (E.g., the prefix "therefo" does not need to be inserted if it has the same
        // probability as the parent prefix "theref").
        //
        // Note: Should always return false if HasPrefixProbabilities() is false.
        virtual bool PrefixLogProb(const LexiconNode& node,
                                   float* prob) const ABSTRACT;

        // Returns true if node is a complete term in the lexicon as opposed to
        // a strict prefix.  (E.g., "therefore" vs. "theref".)
        virtual bool IsEndOfTerm(const LexiconNode& node) const {
            float unused_prob;
            return TermLogProb(node, &unused_prob);
        }

        // Whether the lexicon encodes prefix probabilities.
        virtual bool HasPrefixProbabilities() const ABSTRACT;

        // Whether the lexicon nodes encode unicode codepoints (rather than UTF-8
        // chars).
        virtual bool EncodesCodepoints() const ABSTRACT;
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LEXICON_INTERFACE_H_
