// Wrap third_party/marisa/v0_2_0/lib/marisa/grimoire/vector/bit-vector.h.
//
// Removes exceptions from the marisa library and adds checks in their place.
// Also limits access to the methods not needed by the louds lm/lexicon
// implementation.

#ifndef INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_VECTOR_H_
#define INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_VECTOR_H_

#include <memory>

#include "../base/integral_types.h"
#include "../base/logging.h"
#include "marisa-io.h"
// This must be before any headers in "third_party/marisa" to remove exceptions.
#include "remove-marisa-exceptions.h"
#include "marisa/io.h"
#include "marisa/vector.h"

namespace keyboard {
namespace lm {
namespace louds {

    // A vector that can be read, written, and memory mapped using the classes from
    // marisa-io.h. It is intended to store node and term level information in a
    // LOUDS trie, and work in conjunction with MarisaBitVector.
    template <typename T>
    class MarisaVector {
    public:
        // Creates a new empty vector.
        MarisaVector() : vector_(new marisa::grimoire::vector::Vector<T>) {}

        // Memory maps the contents of the vector from a MarisaMapper.
        // The vector should not be modified (e.g., using push_back) after this
        // method has been called.
        void map(MarisaMapper* mapper) { vector_->map(*mapper->mapper()); }

        // Reads the contents of the vector from a MarisaReader.
        // The vector should not be modified (e.g., using push_back) after this
        // method has been called.
        void read(MarisaReader* reader) { vector_->read(*reader->reader()); }

        // Writes the contents of the vector to a MarisaWriter.
        void write(MarisaWriter* writer) const { vector_->write(*writer->writer()); }

        // Pushes a new value to the back of the vector.
        // This should only be called on a new MarisaVector, not one that has been
        // read or memory mapped.
        void push_back(const T& x) { vector_->push_back(x); }

        // Returns the value at position i.
        const T &operator[](std::size_t i) const {
                    DCHECK_LT(i, vector_->size());
            const marisa::grimoire::vector::Vector<T>* const_vector = vector_.get();
            return (*const_vector)[i];
        }

        // Returns the size of the vector.
        std::size_t size() const { return vector_->size(); }

    private:
        // The underlying marisa Vector
        std::unique_ptr<marisa::grimoire::vector::Vector<T>> vector_;
    };

}  // namespace louds
}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_VECTOR_H_
