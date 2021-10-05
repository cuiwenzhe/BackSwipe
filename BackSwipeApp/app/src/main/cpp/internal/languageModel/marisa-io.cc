#include "marisa-io.h"

#include <unistd.h>
#include <cstddef>

#include "../base/logging.h"
#include "marisa/mapper.h"
#include "marisa/reader.h"
#include "marisa/writer.h"

namespace keyboard {
namespace lm {
namespace louds {

    static inline bool FileExists(const char *filename) {
        return access(filename, F_OK) == 0;
    }

    MarisaMapper::MarisaMapper() : mapper_(new marisa::grimoire::io::Mapper()) {}

    bool MarisaMapper::open(const char *filename) {
        if (!FileExists(filename)) {
            LOG(ERROR) << "Error opening file " << filename;
            return false;
        }
        mapper_->open(filename);
        return true;
    }

    void MarisaMapper::open(const void *ptr, std::size_t size) {
        mapper_->open(ptr, size);
    }

    MarisaReader::MarisaReader() : reader_(new marisa::grimoire::io::Reader()) {}

    bool MarisaReader::open(const char *filename) {
        if (!FileExists(filename)) {
            LOG(ERROR) << "Error opening file " << filename;
            return false;
        }
        reader_->open(filename);
        return true;
    }

    void MarisaReader::open(std::istream *stream) { reader_->open(*stream); }

    MarisaWriter::MarisaWriter() : writer_(new marisa::grimoire::io::Writer()) {}

    void MarisaWriter::open(const char *filename) { writer_->open(filename); }

    void MarisaWriter::open(std::ostream &stream) { writer_->open(stream); }

}  // namespace louds
}  // namespace lm
}  // namespace keyboard
