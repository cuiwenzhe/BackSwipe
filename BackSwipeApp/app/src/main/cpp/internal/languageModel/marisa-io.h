// Wrap classes from "third_party/marisa/v0_2_0/lib/marisa/grimoire/io/".
//
// Removes exceptions from the marisa library and adds checks in their place.
// Also limits access to the methods not needed by the louds lm/lexicon
// implementation.

#ifndef INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_IO_H_
#define INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_IO_H_

#include <memory>

// Must be before any headers in "third_party/marisa" to remove exceptions.
#include "remove-marisa-exceptions.h"
#include "marisa/io.h"

namespace keyboard {
namespace lm {
namespace louds {

    #define BYTES_TO_NEXT_8_BYTE_MULTIPLE(size) (8 - (size % 8)) % 8

    // A class for mapping data sequentially from a memory mapped buffer.
    class MarisaMapper {
    public:
        MarisaMapper();

        // Sequentially maps an object of type T. The method then ensures that the
        // mapped region is a multiple of 8-bytes (expected by the marisa library),
        // seeking forward as needed.
        template <typename T>
        void map(T *obj) {
            const size_t size = sizeof(T);
            mapper_->map(obj);
            // Marisa expects I/O to occur in multiples of 8-bytes, so seek() skips
            // the appropriate number of bytes (see marisa's Vector::map_).
            mapper_->seek(BYTES_TO_NEXT_8_BYTE_MULTIPLE(size));
        }

        // Opens the mapper by creating a new memory map for the given file.
        // Returns whether or not the file was opened successfully.
        bool open(const char *filename);

        // Opens the mapper for a pointer to a memory mapped buffer.
        void open(const void *ptr, std::size_t size);

        // Returns the raw Marisa mapper.
        marisa::grimoire::io::Mapper *mapper() { return mapper_.get(); }

    private:
        // The underlying marisa Mapper
        std::unique_ptr<marisa::grimoire::io::Mapper> mapper_;
    };

    // A class for reading data sequentially from a file or stream.
    class MarisaReader {
    public:
        MarisaReader();

        // Sequentially reads an object of type T. The method then ensures that the
        // read region is a multiple of 8-bytes (expected by the marisa library),
        // seeking forward as needed.
        template <typename T>
        void read(T *obj) {
            const size_t size = sizeof(T);
            reader_->read(obj);
            // Marisa expects I/O to occur in multiples of 8-bytes, so seek() skips
            // the appropriate number of bytes (see marisa's Vector::read_).
            reader_->seek(BYTES_TO_NEXT_8_BYTE_MULTIPLE(size));
        }

        // Opens the reader for an input file.
        // Returns whether or not the file was opened successfully.
        bool open(const char *filename);

        // Opens the reader for an input stream.
        void open(std::istream *stream);

        // Returns the raw Marisa reader.
        marisa::grimoire::io::Reader *reader() { return reader_.get(); }

    private:
        // The underlying marisa Reader
        std::unique_ptr<marisa::grimoire::io::Reader> reader_;
    };

    // A class for writing data sequentially to a file.
    class MarisaWriter {
    public:
        MarisaWriter();

        // Sequentially writes an object of type T. The method then ensures that the
        // written region is a multiple of 8-bytes (expected by the marisa library),
        // writing extra 0's as needed.
        template <typename T>
        void write(const T &obj) {
            const size_t size = sizeof(T);
            writer_->write(obj);
            // Marisa expects I/O to occur in multiples of 8-bytes, so seek() pads the
            // output with the appropriate number of bytes. (see marisa's
            // Vector::write_)
            writer_->seek(BYTES_TO_NEXT_8_BYTE_MULTIPLE(size));
        }

        // Opens the writer for an output file.
        void open(const char *filename);
        // Opens the writer for an ostream, e.g. a Google File.
        void open(std::ostream &stream);

        // Returns the raw Marisa writer.
        marisa::grimoire::io::Writer *writer() { return writer_.get(); }

    private:
        // The underlying marisa Writer
        std::unique_ptr<marisa::grimoire::io::Writer> writer_;
    };

}  // namespace louds
}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_LOUDS_MARISA_IO_H_
