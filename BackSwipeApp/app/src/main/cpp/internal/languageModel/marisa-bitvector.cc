#include "marisa-bitvector.h"

#include "../base/logging.h"
#include "marisa/io.h"

namespace keyboard {
namespace lm {
namespace louds {

    using marisa::grimoire::io::Mapper;
    using marisa::grimoire::io::Reader;
    using marisa::grimoire::io::Writer;

    MarisaBitVector::MarisaBitVector()
            : bit_vector_(new marisa::grimoire::vector::BitVector<32>) {}

    std::size_t MarisaBitVector::rank1(std::size_t i) const {
                DCHECK_LT(i, bit_vector_->size());
        return bit_vector_->rank1(i);
    }

    std::size_t MarisaBitVector::rank0(std::size_t i) const {
                DCHECK_LT(i, bit_vector_->size());
        return bit_vector_->rank0(i);
    }

    std::size_t MarisaBitVector::select1(std::size_t i) const {
                DCHECK_LT(i, bit_vector_->size());
        return bit_vector_->select1(i);
    }

    std::size_t MarisaBitVector::select0(std::size_t i) const {
                DCHECK_LT(i, bit_vector_->size());
        return bit_vector_->select0(i);
    }

    bool MarisaBitVector::operator[](std::size_t i) const {
                DCHECK_LT(i, bit_vector_->size()) << "Index exceeds size: " << i;
        return (*bit_vector_)[i];
    }

    std::size_t MarisaBitVector::size() const { return bit_vector_->size(); }

    void MarisaBitVector::push_back(bool bit) { bit_vector_->push_back(bit); }

    void MarisaBitVector::map(MarisaMapper* mapper) {
        bit_vector_->map(*mapper->mapper());
    }

    void MarisaBitVector::read(MarisaReader* reader) {
        bit_vector_->read(*reader->reader());
    }

    void MarisaBitVector::write(MarisaWriter* writer) const {
        bit_vector_->write(*writer->writer());
    }

    void MarisaBitVector::build() {
        // Does not throw exceptions.
        bit_vector_->build(true /* enables_select0 */, true /* enables_select1 */);
    }

}  // namespace louds
}  // namespace lm
}  // namespace keyboard
