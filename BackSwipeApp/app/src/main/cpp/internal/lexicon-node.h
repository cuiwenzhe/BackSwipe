#ifndef INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LEXICON_NODE_H_
#define INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LEXICON_NODE_H_

#include "base/integral_types.h"

namespace keyboard {
namespace decoder {

    // A LexiconNode to reference a specific node (e.g., term or prefix) in a
    // LexiconInterface.
    //
    // Note that LexiconNodes are only valid for the LexiconInterface that created
    // them (e.g., via LexiconInterface::GetRootNode).
    //
    // Members:
    //   c  - The character of the node. May be a UTF-8 char or a unicode
    //        codepoint, depending on LexiconInterface::EncodesCodepoints.
    //   id - A 64-bit field that uniquely identifies the node within its lexicon.
    //        The lexicon implementation can use this field to store an internal
    //        reference to the node, such as an address or index. If two nodes have
    //        the same ID, they must also have the same character. Distinct nodes
    //        belonging to different lexicons may have the same ID.
    struct LexiconNode {
        char32 c;
        uint64 id;
    };

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_LEXICON_NODE_H_
