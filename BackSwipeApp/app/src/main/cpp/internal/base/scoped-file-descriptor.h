// A scoped file descriptor that automatically closes its POSIX file descriptor
// when it goes out of scope.

#ifndef INPUTMETHOD_KEYBOARD_LM_BASE_SCOPED_FILE_DESCRIPTOR_H_
#define INPUTMETHOD_KEYBOARD_LM_BASE_SCOPED_FILE_DESCRIPTOR_H_

#include <unistd.h>

#include "macros.h"

namespace keyboard {
namespace lm {

    // Note: This class is not thread-safe.
    class ScopedFileDescriptor {
    public:
        // Creates a ScopedFd that takes ownership of the give fd.
        explicit ScopedFileDescriptor(int fd) : fd_(fd) {}

        ~ScopedFileDescriptor() {
            if (fd_ >= 0) {
                close(fd_);
            }
        }

        // Returns whether or not the stored fd is valid (>= 0).
        bool is_valid() const { return fd_ >= 0; }

        // Returns the stored fd.
        int operator*() const { return fd_; }

        // Returns the stored fd.
        int get() const { return fd_; }

        // Releases ownership of and returns the stored fd.
        int release() {
            int fd = fd_;
            fd_ = -1;
            return fd;
        }

    private:
        // The stored file descriptor.
        int fd_;

        DISALLOW_COPY_AND_ASSIGN(ScopedFileDescriptor);
    };

}  // namespace lm
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_LM_BASE_SCOPED_FILE_DESCRIPTOR_H_
