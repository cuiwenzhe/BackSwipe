// Copyright 2007 Google Inc. All Rights Reserved.
// Author: smiles@google.com (Stewart Miles)

#include "scoped_mmap.h"

#include <errno.h>
#include <sys/mman.h>

ScopedMmap::ScopedMmap() {
    Reset();
}


ScopedMmap::~ScopedMmap() {
    if (IsMapped())
        Unmap();
}


void* ScopedMmap::Map(const int    file_descriptor,
                      const size_t offset,
                      const size_t size,
                      const size_t alignment,
                      const int    mmap_prot,
                      const int    mmap_flags) {
    CHECK_GE(file_descriptor, 0);
    CHECK(size);
    CHECK(alignment);

    // align map offset to specified alignment
    const size_t file_map_offset          = offset % alignment;
    const size_t aligned_file_map_offset  = offset - file_map_offset;
    // adjust the mapped region size so the entire
    // offset...offset+size region is included
    aligned_file_map_size_ = size + file_map_offset;

    // map the specified page(s) into this process' address space as read only
    process_address_ = mmap(0, aligned_file_map_size_, mmap_prot,
                            mmap_flags, file_descriptor,
                            aligned_file_map_offset);
    if (MAP_FAILED == process_address_) {
        LOG(WARNING) << "Failed to map file region " << offset <<
                     " length " << size;
        Reset();
        return NULL;
    }
    return static_cast<uint8*>(process_address_) + file_map_offset;
}


void ScopedMmap::Unmap() {
    // unmap page(s)
    if (munmap(process_address_, aligned_file_map_size_) < 0) {
        // TODO: Removed partial log message
        LOG(WARNING) << "Failed to unmap address.";
//        LOG(WARNING) << "Failed to unmap address " <<
//                     process_address_ << " size " << aligned_file_map_size_ <<
//                     " errno=" << errno;
    }
    Reset();
}


bool ScopedMmap::IsMapped() const {
    return process_address_ && aligned_file_map_size_;
}


void ScopedMmap::Reset() {
    process_address_       = NULL;
    aligned_file_map_size_ = 0;
}