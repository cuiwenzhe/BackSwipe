// Copyright 2010 Google Inc. All Rights Reserved.
// Authors: gpike@google.com (Geoff Pike), jyrki@google.com (Jyrki Alakuijala)
//
// This file provides a few functions for hashing strings.  On x86-64
// hardware as of early 2010, CityHash64() is much faster than
// MurmurHash64(), and passes the quality-of-hash tests in
// ./hasheval/hasheval_test.cc, among others, with flying colors.  The
// difference in speed can be a factor of two for strings of 50 to 64
// bytes, and sometimes even more for cache-resident longer strings.
//
// CityHash128() is optimized for relatively long strings and returns
// a 128-bit hash.  For strings more than about 2000 bytes it can be
// faster than CityHash64().
//
// Functions in the CityHash family are not suitable for cryptography.
//
// By the way, for some hash functions, given strings a and b, the hash
// of a+b is easily derived from the hashes of a and b.  This property
// doesn't hold for any hash functions in this file.
//
// Note that the hash functions defined here are related to, but NOT the same
// as the functions defined in the open source version in third_party/cityhash
//
// Also note that the hash functions defined here (due to their use of
// HashSeed, see util/hash/builtin_type_hash.h) are not guaranteed consistent
// across different processes, or different executions of the same binary.

#ifndef UTIL_HASH_CITY_H_
#define UTIL_HASH_CITY_H_

#include <stddef.h>  // for size_t.

#include "int128.h"
#include "integral_types.h"

namespace util_hash {

    // Hash function for a byte array.
    // The mapping may change from time to time.
    uint64 CityHash64(const char *s, size_t len);

    // Hash function for a byte array.  For convenience, a 64-bit seed is also
    // hashed into the result.  The mapping may change from time to time.
    uint64 CityHash64WithSeed(const char *s, size_t len, uint64 seed);

    // Hash function for a byte array.  For convenience, two seeds are also
    // hashed into the result.  The mapping may change from time to time.
    uint64 CityHash64WithSeeds(const char *s, size_t len,
                               uint64 seed0, uint64 seed1);

    // Hash function for a byte array.  The mapping will never change, but
    // please note that it is a different never-changing function if
    // EXTERNAL_DEPLOYMENT is defined.
    uint128 CityHash128(const char *s, size_t len);

    // Hash function for a byte array.  For convenience, a 128-bit seed is also
    // hashed into the result.  The mapping will never change, but please note that
    // it is a different never-changing function if EXTERNAL_DEPLOYMENT is defined.
    uint128 CityHash128WithSeed(const char *s, size_t len, uint128 seed);

    // Hash function for a byte array.
    // The mapping may change from time to time.
    uint32 CityHash32(const char *s, size_t len);

    // Hash function for a byte array.
    // The mapping may change from time to time.
    uint32 CityHash32WithSeed(const char *s, size_t len, uint32 seed);

}  // namespace util_hash

#endif  // UTIL_HASH_CITY_H_
