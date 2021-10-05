// Copyright 2010 Google Inc. All Rights Reserved.
// Authors: jyrki@google.com (Jyrki Alakuijala), gpike@google.com (Geoff Pike)

#ifndef UTIL_HASH_HASH128TO64_H_
#define UTIL_HASH_HASH128TO64_H_

// MOE:begin_strip
// NOTE: for the speed of the build, this section of AVOID_TRADE_SECRET_CODE
// definition is duplicated from avoid_tradesecret.h, make sure that the two
// are in sync.
#undef AVOID_TRADE_SECRET_CODE
// For now use the external global build definition to avoid trade secrets.
// blaze build --define DEPLOYMENT=EXTERNAL_DEPLOYMENT
// TODO(whua|wuu): Switch to checking for a single preprocessor symbol when such
// symbol exists.
#if defined(EXTERNAL_DEPLOYMENT) || defined(__ANDROID__) ||            \
    defined(__APPLE__) || defined(OS_ANDROID) || defined(OS_CYGWIN) || \
    !defined(OS_LINUX) || !defined(GOOGLE_GLIBCXX_VERSION) || \
    defined(__native_client__)
#define AVOID_TRADE_SECRET_CODE 1
#ifndef EXTERNAL_FP
#define EXTERNAL_FP
#endif
#endif  // EXTERNAL_DEPLOYMENT

#if !defined(AVOID_TRADE_SECRET_CODE)
#include "base/int128.h"
#include "base/integral_types.h"

// Hash 128 input bits down to 64 bits of output.
// This is intended to be a reasonably good hash function.
// It may change from time to time.
inline uint64 Hash128to64(const uint128& x) {
  // Murmur-inspired hashing.
  const uint64 kMul = 0xc6a4a7935bd1e995ULL;
  uint64 a = (Uint128Low64(x) ^ Uint128High64(x)) * kMul;
  a ^= (a >> 47);
  uint64 b = (Uint128High64(x) ^ a) * kMul;
  b ^= (b >> 47);
  b *= kMul;
  return b;
}

#else
// MOE:end_strip

#include "int128.h"
#include "integral_types.h"
#include "builtin_type_hash.h"  // for HashSeed()

// Based on a hash function in FarmHash.
inline uint64 Hash128to64(const uint128& x) {
    // Murmur-inspired hashing.
    const uint64 kMul = 0x9ddfea08eb382d69ULL;
    uint64 a =
            (Uint128Low64(x) + Uint128High64(x) + util_hash::HashSeed()) * kMul;
    a ^= (a >> 46);
    uint64 b = (Uint128High64(x) ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}

// MOE:begin_strip
#endif  // !defined(AVOID_TRADE_SECRET_CODE)
// MOE:end_strip

#endif  // UTIL_HASH_HASH128TO64_H_
