// Copyright 2011 Google Inc. All Rights Reserved.
// Author: chandlerc@google.com (Chandler Carruth)
//
// These are the core hashing routines which operate on strings. We define
// strings loosely as a sequence of bytes, and these routines are designed to
// work with the most fundamental representations of a string of bytes.
//
// These routines provide "good" hash functions in terms of both quality and
// speed. Their values can and will change as their implementations change and
// evolve.

#ifndef UTIL_HASH_STRING_HASH_H_
#define UTIL_HASH_STRING_HASH_H_

#include <limits.h>
#include <stddef.h>

#include "integral_types.h"
#include "city.h"

namespace hash_internal {

    template <size_t Bits = sizeof(size_t) * CHAR_BIT> struct Thoroughly;

    template <>
    struct Thoroughly<64> {
        static size_t Hash(const char* s, size_t len, size_t seed) {
            return static_cast<size_t>(util_hash::CityHash64WithSeed(s, len, seed));
        }
        static size_t Hash(const char* s, size_t len) {
            return static_cast<size_t>(util_hash::CityHash64(s, len));
        }
        static size_t Hash(const char* s, size_t len, size_t seed0, size_t seed1) {
            return static_cast<size_t>(util_hash::CityHash64WithSeeds(s, len,
                                                                      seed0, seed1));
        }
    };

    template <>
    struct Thoroughly<32> {
        static size_t Hash(const char* s, size_t len, size_t seed) {
            return static_cast<size_t>(
                    util_hash::CityHash32WithSeed(s, len, static_cast<uint32>(seed)));
        }
        static size_t Hash(const char* s, size_t len) {
            return static_cast<size_t>(util_hash::CityHash32(s, len));
        }
        static size_t Hash(const char* s, size_t len, size_t seed0, size_t seed1) {
            seed0 += Hash(s, len / 2, seed1);
            s += len / 2;
            len -= len / 2;
            return Hash(s, len, seed0);
        }
    };
}  // namespace hash_internal

// We use different algorithms depending on the size of size_t.
inline size_t HashStringThoroughlyWithSeed(const char* s, size_t len,
                                           size_t seed) {
    return hash_internal::Thoroughly<>::Hash(s, len, seed);
}

inline size_t HashStringThoroughly(const char* s, size_t len) {
    return hash_internal::Thoroughly<>::Hash(s, len);
}

inline size_t HashStringThoroughlyWithSeeds(const char* s, size_t len,
                                            size_t seed0, size_t seed1) {
    return hash_internal::Thoroughly<>::Hash(s, len, seed0, seed1);
}

#endif  // UTIL_HASH_STRING_HASH_H_
