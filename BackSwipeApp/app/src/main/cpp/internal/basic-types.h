// Basic types used to encode data in LOUDS language models.

#ifndef INPUTMETHOD_KEYBOARD_LM_LOUDS_BASIC_TYPES_H_
#define INPUTMETHOD_KEYBOARD_LM_LOUDS_BASIC_TYPES_H_


namespace keyboard {
namespace lm {
namespace louds {

    /////////////////////// Trie key representations ////////////////////////

    // A TermChar is used as the node label for a UTF-8 string trie/lexicon.
    // The decoder supports multi-byte UTF-8 codes, and will automatically convert
    // them to char32 codepoints if needed during the traversal.
    typedef char TermChar;

    // A TermId16 is used as the node label for an n-gram trie, and references a
    // term in the lexicon.
    typedef uint16_t TermId16;


    /////////////////////// Probability representations ////////////////////////

    // Represents a log probability quantized into 8 bits.
    typedef uint8_t QuantizedLogProb;

    // Used to emphasize that a float represents a log probability.
    typedef float LogProbFloat;

}  // namespace louds
}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_LOUDS_BASIC_TYPES_H_
