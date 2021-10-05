// Copyright 2011 Google Inc. All Rights Reserved.
// MOE:begin_strip
// Author: chandlerc@google.com (Chandler Carruth)
//
// Contains the legacy Bob Jenkins Lookup2-based hashing routines. These need to
// always return the same results as their values have been recorded in various
// places and cannot easily be updated.
//
// Original Author: Sanjay Ghemawat
//
// This is based on Bob Jenkins newhash function
// see: http://burtleburtle.net/bob/hash/evahash.html
// According to http://burtleburtle.net/bob/c/lookup2.c,
// his implementation is public domain.
//
// The implementation here is backwards compatible with the google1
// implementation.  The google1 implementation used a 'signed char *'
// to load words from memory a byte at a time.  See gwshash.cc for an
// implementation that is compatible with Bob Jenkins' lookup2.c.
// MOE:end_strip

#include "jenkins.h"

// MOE:begin_strip

#if !defined(AVOID_TRADE_SECRET_CODE)
#include "integral_types.h"
#include "macros.h"
#include "util_endian.h"
#include "jenkins_lookup2.h"

static inline uint32 char2unsigned(char c) {
    return static_cast<uint32>(static_cast<unsigned char>(c));
}

static inline uint64 char2unsigned64(char c) {
    return static_cast<uint64>(static_cast<unsigned char>(c));
}

uint32 Hash32StringWithSeed(const char *s, size_t len, uint32 c) {
    uint32 a = 0x9e3779b9UL, b = 0x9e3779b9UL;
    size_t keylen = len;
    while (keylen >= 12) {
        a += Google1At(s);
        b += Google1At(s + 4);
        c += Google1At(s + 8);
        s += 12;
        mix(a, b, c);
        keylen -= 12;
    }
    c += len;
    switch (keylen) {  // deal with rest.
        case 0:
            break;
        case 1:
            a += char2unsigned(s[0]);
            break;
        case 2:
            a += char2unsigned(s[0]);
            a += char2unsigned(s[1]) << 8;
            break;
        case 3:
            a += char2unsigned(s[0]);
            a += char2unsigned(s[1]) << 8;
            a += char2unsigned(s[2]) << 16;
            break;
        case 4:
            a += Google1At(s);
            break;
        case 5:
            b += char2unsigned(s[4]);
            a += Google1At(s);
            break;
        case 6:
            b += char2unsigned(s[4]);
            b += char2unsigned(s[5]) << 8;
            a += Google1At(s);
            break;
        case 7:
            b += LittleEndian::Load32(s + 3) >> 8;
            a += Google1At(s);
            break;
        case 8:
            a += Google1At(s);
            b += Google1At(s + 4);
            break;
        case 9:
            a += Google1At(s);
            b += Google1At(s + 4);
            c += char2unsigned(s[8]) << 8;
            break;
        case 10:
            a += Google1At(s);
            b += Google1At(s + 4);
            c += char2unsigned(s[8]) << 8;
            c += char2unsigned(s[9]) << 16;
            break;
        case 11:
            a += Google1At(s);
            b += Google1At(s + 4);
            c += LittleEndian::Load32(s + 7) & 0xffffff00;
            break;
    }
    mix(a, b, c);
    return c;
}

uint64 Hash64StringWithSeed(const char *s, size_t len, uint64 c) {
    uint64 a, b;
    size_t keylen;

    a = b = GG_ULONGLONG(0xe08c1d668b756f82);   // the golden ratio; an arbitrary value

    for ( keylen = len;  keylen >= 3 * sizeof(a);
          keylen -= 3 * static_cast<uint32>(sizeof(a)), s += 3 * sizeof(a) ) {
        a += Word64At(s);
        b += Word64At(s + sizeof(a));
        c += Word64At(s + sizeof(a) * 2);
        mix(a,b,c);
    }

    c += len;
    switch (keylen) {  // deal with rest.
        case 23:
            c += char2unsigned64(s[22]) << 56;
            FALLTHROUGH_INTENDED;
        case 22:
            c += char2unsigned64(s[21]) << 48;
            FALLTHROUGH_INTENDED;
        case 21:
            c += char2unsigned64(s[20]) << 40;
            FALLTHROUGH_INTENDED;
        case 20:
            c += char2unsigned64(s[19]) << 32;
            FALLTHROUGH_INTENDED;
        case 19:
            c += char2unsigned64(s[18]) << 24;
            FALLTHROUGH_INTENDED;
        case 18:
            c += char2unsigned64(s[17]) << 16;
            FALLTHROUGH_INTENDED;
        case 17:
            c += char2unsigned64(s[16]) << 8;
            // the first byte of c is reserved for the length
            FALLTHROUGH_INTENDED;
        case 16:
            b += Word64At(s + 8);
            a += Word64At(s);
            break;
        case 15:
            b += char2unsigned64(s[14]) << 48;
            FALLTHROUGH_INTENDED;
        case 14:
            b += char2unsigned64(s[13]) << 40;
            FALLTHROUGH_INTENDED;
        case 13:
            b += char2unsigned64(s[12]) << 32;
            FALLTHROUGH_INTENDED;
        case 12:
            b += char2unsigned64(s[11]) << 24;
            FALLTHROUGH_INTENDED;
        case 11:
            b += char2unsigned64(s[10]) << 16;
            FALLTHROUGH_INTENDED;
        case 10:
            b += char2unsigned64(s[9]) << 8;
            FALLTHROUGH_INTENDED;
        case 9:
            b += char2unsigned64(s[8]);
            FALLTHROUGH_INTENDED;
        case 8:
            a += Word64At(s);
            break;
        case 7:
            a += char2unsigned64(s[6]) << 48;
            FALLTHROUGH_INTENDED;
        case 6:
            a += char2unsigned64(s[5]) << 40;
            FALLTHROUGH_INTENDED;
        case 5:
            a += char2unsigned64(s[4]) << 32;
            FALLTHROUGH_INTENDED;
        case 4:
            a += char2unsigned64(s[3]) << 24;
            FALLTHROUGH_INTENDED;
        case 3:
            a += char2unsigned64(s[2]) << 16;
            FALLTHROUGH_INTENDED;
        case 2:
            a += char2unsigned64(s[1]) << 8;
            FALLTHROUGH_INTENDED;
        case 1:
            a += char2unsigned64(s[0]);
            // case 0: nothing left to add
    }
    mix(a,b,c);
    return c;
}

#else
// MOE:end_strip

#include "util/hash/jenkins_lookup2.h"
// These functions can be updated using farmhash later.
// The current version is copied from mobile/util/hash/hash.cc.
static const uint32 kPrimes32[16] ={
  65537, 65539, 65543, 65551, 65557, 65563, 65579, 65581,
  65587, 65599, 65609, 65617, 65629, 65633, 65647, 65651,
};

static const uint64 kPrimes64[] ={
  GG_ULONGLONG(4294967311), GG_ULONGLONG(4294967357),
  GG_ULONGLONG(4294967371), GG_ULONGLONG(4294967377),
  GG_ULONGLONG(4294967387), GG_ULONGLONG(4294967389),
  GG_ULONGLONG(4294967459), GG_ULONGLONG(4294967477),
  GG_ULONGLONG(4294967497), GG_ULONGLONG(4294967513),
  GG_ULONGLONG(4294967539), GG_ULONGLONG(4294967543),
  GG_ULONGLONG(4294967549), GG_ULONGLONG(4294967561),
  GG_ULONGLONG(4294967563), GG_ULONGLONG(4294967569)
};

uint32 Hash32StringWithSeedReferenceImplementation(const char *s,
                                                   size_t len,
                                                   uint32 seed) {
  uint32 n = seed;
  size_t prime1 = 0, prime2 = 8;  // Indices into kPrimes32
  union {
    uint16 n;
    char bytes[sizeof(uint16)];
  } chunk;
  for (const char *i = s, *const end = s + len; i != end; ) {
    chunk.bytes[0] = *i++;
    chunk.bytes[1] = i == end ? 0 : *i++;
    n = n * kPrimes32[prime1++] ^ chunk.n * kPrimes32[prime2++];
    prime1 &= 0x0F;
    prime2 &= 0x0F;
  }
  return n;
}

uint32 Hash32StringWithSeed(const char *s, size_t len, uint32 seed) {
  uint32 a, b;
  b = 0x12f905ffUL;
  uint32 c = seed;
  while (len > 12) {
    a = Hash32StringWithSeedReferenceImplementation(s, 12, c);
    mix(a, b, c);
    s += 12;
    len -= 12;
  }
  if (len > 0) {
    a = Hash32StringWithSeedReferenceImplementation(s, len, c);
    mix(a, b, c);
  }
  return c;
}

uint64 Hash64StringWithSeedReferenceImplementation(const char *s,
                                                   size_t len,
                                                   uint64 seed) {
  uint64 n = seed;
  size_t prime1 = 0, prime2 = 8;  // Indices into kPrimes64
  union {
    uint32 n;
    char bytes[sizeof(uint32)];
  } chunk;
  for (const char *i = s, *const end = s + len; i != end; ) {
    chunk.bytes[0] = *i++;
    chunk.bytes[1] = i == end ? 0 : *i++;
    chunk.bytes[2] = i == end ? 0 : *i++;
    chunk.bytes[3] = i == end ? 0 : *i++;
    n = n * kPrimes64[prime1++] ^ chunk.n * kPrimes64[prime2++];
    prime1 &= 0x0F;
    prime2 &= 0x0F;
  }
  return n;
}

uint64 Hash64StringWithSeed(const char *s, size_t len, uint64 seed) {
  uint64 a, b;
  b = 0x2c2ca38cd0cc731bULL;
  uint64 c = seed;
  while (len > 24) {
    a = Hash64StringWithSeedReferenceImplementation(s, 24, c);
    mix(a, b, c);
    s += 24;
    len -= 24;
  }
  if (len > 0) {
    a = Hash64StringWithSeedReferenceImplementation(s, len, c);
    mix(a, b, c);
  }
  return c;
}

// MOE:begin_strip
#endif  // !defined(AVOID_TRADE_SECRET_CODE)
// MOE:end_strip
