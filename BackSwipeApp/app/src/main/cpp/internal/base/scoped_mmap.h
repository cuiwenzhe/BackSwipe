// Copyright 2007 Google Inc. All Rights Reserved.
// Author: smiles@google.com (Stewart Miles)

#ifndef UTIL_MEMORY_SCOPED_MMAP_H__
#define UTIL_MEMORY_SCOPED_MMAP_H__

#include <stddef.h>

#include "integral_types.h"
#include "logging.h"
#include "macros.h"

// A wrapper around mmap and munmap which unmaps it's mapped memory
// when it's destructor is called.
// https://www.corp.google.com/eng/designdocs/platforms/taxonomist/mmio.html
//
// Recommended pattern of usage:
//   /* map and copy 4KB from the physical address 1MB */
//   const int    file        = open("/dev/mem", O_RDWR);
//   const size_t copyAddress = 0x00100000;
//   const size_t copySize    = 0x00001000;
//   uint8*       localcopy   = new uint8[copySize];
//   {
//     platform::ScopedMmap scoped_mmap;
//     void *address = scoped_mmap.Map(file, 0x10000, 0x10, getpagesize(),
//                                     PROT_READ, MAP_SHARED);
//     memcpy(localcopy, address, copySize);
//   } /* unmaps mapped memory here */
class ScopedMmap {
public:
    // initializes the memory mapper to unmapped
    ScopedMmap();

    // if memory has been mapped by this object, unmap it
    ~ScopedMmap();

    // Map the data specified by the byte offset and size from the
    // file_descriptor into the calling process' address space ensuring the
    // mapped region is aligned to the specified byte alignment
    // returning the address of the mapped region in the process' address space
    // if successful.  If mapping fails this function returns NULL.
    // file_descriptor must be a valid file descriptor and
    // size should be greater than 0.
    // alignment should be the page size returned by getpagesize().
    // mmap_prot specifies the memory protection mode (prot) passed to mmap().
    // mmap_flags specifies the type of mapped object flags (flags) passed
    // to mmap().
    // See mmap() documentation (info mmap) for more information.
    void* Map(const int    file_descriptor,
              const size_t offset,
              const size_t size,
              const size_t alignment,
              const int    mmap_prot,
              const int    mmap_flags);

    // unmap the mapped file descriptor
    void Unmap();

    // determine whether this object has mapped a region of a file into the
    // calling process' address space
    bool IsMapped() const;


protected:
    void*  process_address_;
    size_t aligned_file_map_size_;

    // reset the internal object's state to unmapped
    void Reset();

    DISALLOW_COPY_AND_ASSIGN(ScopedMmap);
};

#endif  // UTIL_MEMORY_SCOPED_MMAP_H__