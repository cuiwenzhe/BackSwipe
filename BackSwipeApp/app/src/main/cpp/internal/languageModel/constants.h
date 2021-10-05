// Description:
//   Declares common constants used by the keyboard language models.

#ifndef INPUTMETHOD_KEYBOARD_LM_BASE_CONSTANTS_H_
#define INPUTMETHOD_KEYBOARD_LM_BASE_CONSTANTS_H_

#include <limits>

#include "../base/integral_types.h"
#include "../base/stringpiece.h"

namespace keyboard {
namespace lm {

    typedef uint32 TermId;

    // The reserved term id for the beginning of sentence term (<S>)
    constexpr TermId kBOSId = 0;

    // The reserved term id for the end of sentence term (</S>)
    constexpr TermId kEOSId = 1;

    // The reserved term id for a term that is not in the lexicon (<UNK>)
    // Note: Also used for terms that do not have an externally visible term_id
    // according to max_num_term_ids_.
    constexpr TermId kUnkId = 2;

    // The special reserved term id indicating that there is no id (<NONE>)
    // Note: Not intended for regular OoV terms that do not have a term_id. For
    // those use <UNK> instead.
    constexpr TermId kNoneId = 3;

    // There are several reserved term ids that cannot be allocated to actual
    // terms. This is the first unreserved term id.
    constexpr TermId kFirstUnreservedId = 4;

    // LINT.IfChange
    // The beginning of sentence term (see kBOSId).
    const char kBOS[] = "<S>";

    // The end of sentence term (see kEOSId).
    const char kEOS[] = "</S>";
    // LINT.ThenChange(//depot/google3/java/com/google/android/apps/inputmethod/libs/delight4/TextTokenizer.java)

    // The unknown term (see kUnkId).
    const char kUnk[] = "<UNK>";

    // The none term (see kNoneId).
    const char kNone[] = "<NONE>";

    // Returns whether or not the given term is a special reserved term.
    // bool IsReservedTerm(const StringPiece& term);
    bool IsReservedTerm(const StringPiece& term);

    // If the input term is a special reserved term, returns its termid.
    // Otherwise, returns kFirstUnreservedId.
    // TermId ReservedTermToTermId(const StringPiece& term);
    TermId ReservedTermToTermId(const StringPiece& term);

    // Returns the term for the reserved termid.
    // Returns the empty string if the termid is not a reserved term.
    std::string ReservedTermIdToTerm(const TermId termid);

}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_BASE_CONSTANTS_H_
