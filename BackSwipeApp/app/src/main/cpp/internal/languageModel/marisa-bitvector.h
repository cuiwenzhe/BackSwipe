// Wrapper for third_party/marisa/v0_2_0/lib/marisa/grimoire/vector/bit-vector.h
//
// Removes exceptions from the marisa library and adds checks in their place.
// Also limits access to the methods not needed by the louds lm/lexicon
// implementation.

#ifndef INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_BITVECTOR_H_
#define INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_BITVECTOR_H_

#include <memory>

#include "../base/integral_types.h"
#include "marisa-io.h"
// This must be before any headers in "third_party/marisa" to remove exceptions.
#include "remove-marisa-exceptions.h"
#include "marisa/bit-vector.h"

namespace keyboard {
namespace lm {
namespace louds {

    // A bit-vector that supports LOUDS operations for rank and select.
    class MarisaBitVector {
    public:
        // Creates a new empty bit vector.
        MarisaBitVector();

        // Returns the number of 1 bits to the left of, not including, position i
        // Note: only supported after calling MarisaBitVector::build().
        std::size_t rank1(std::size_t i) const;

        // Returns the number of 0 bits to the left of, not including, position i
        // Note: only supported after calling MarisaBitVector::build().
        std::size_t rank0(std::size_t i) const;

        // Returns the position of the i-th 1 bit in the bit vector (0-indexed)
        // Note: only supported after calling MarisaBitVector::build().
        std::size_t select1(std::size_t i) const;

        // Returns the position of the i-th 0 bit in the bit vector (0-indexed)
        // Note: only supported after calling MarisaBitVector::build().
        std::size_t select0(std::size_t i) const;

        // Returns the bit value at position i
        bool operator[](std::size_t i) const;

        // Returns the size of the bit vector
        std::size_t size() const;

        // Pushes a new bit to the back of the bit vector.
        // Should only be called before calling MarisaBitVector::build().
        void push_back(bool bit);

        // Memory maps the contents of the bit vector from a MarisaMapper.
        void map(MarisaMapper* mapper);

        // Reads the contents of the bit vector from a MarisaReader.
        void read(MarisaReader* reader);

        // Writes the contents of the bit vector to a MarisaWriter.
        void write(MarisaWriter* writer) const;

        // Builds the rank and select indices for the bit vector.
        void build();

    private:
        // The underlying marisa BitVector
        std::unique_ptr<marisa::grimoire::vector::BitVector<32>> bit_vector_;
    };

}  // namespace louds
}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_BITVECTOR_H_
