// Copyright 2011 Google Inc. All Rights Reserved.
// Author: chandlerc@google.com (Chandler Carruth)
//
// This is the legacy unified hash library implementation. Its components are
// being split up into smaller, dedicated libraries. What remains here are
// things still being migrated.
//
// To find the implementation of the core Bob Jenkins lookup2 hash, look in
// jenkins.cc.

#ifdef __SSE2__
#include <emmintrin.h>
#endif  // __SSE2__

#ifdef __ALTIVEC__
#include <altivec.h>
#endif  // __ALTIVEC__

#include "hash.h"

// MOE:begin_strip

#if !defined(AVOID_TRADE_SECRET_CODE)
#include "base/integral_types.h"
    #include "base/logging.h"
    #include "util/endian/endian.h"
    #include "util/hash/jenkins.h"
    #include "util/hash/jenkins_lookup2.h"

    // For components that ship code externally (notably the Google Search
    // Appliance) we want to change the fingerprint function so that
    // attackers cannot mount offline attacks to find collisions with
    // google.com internal fingerprints (most importantly, for URL
    // fingerprints).
    #ifdef GOOGLECLIENT
    #error Do not compile this into binaries that we deliver to users!
    #error Instead, use //depot/googleclient/google3/util/hash.cc
    #endif
    #ifdef EXTERNAL_FP
    static const uint32 kFingerprintSeed0 = 0xabc;
    static const uint32 kFingerprintSeed1 = 0xdef;
    #else
    static const uint32 kFingerprintSeed0 = 0;
    static const uint32 kFingerprintSeed1 = 102072;
    #endif

    static inline uint32 char2unsigned(char c) {
      return static_cast<uint32>(static_cast<unsigned char>(c));
    }

    static inline uint64 char2unsigned64(char c) {
      return static_cast<uint64>(static_cast<unsigned char>(c));
    }

    uint64 FingerprintReferenceImplementation(const char *s, size_t len) {
      uint32 hi = Hash32StringWithSeed(s, len, kFingerprintSeed0);
      uint32 lo = Hash32StringWithSeed(s, len, kFingerprintSeed1);
      return CombineFingerprintHalves(hi, lo);
    }

    // An SSE2 implementation of the 32-bit mix() method in jenkins_lookup2.h .
    // It uses SSE2 to mix four values in parallel. This is equivalent to calling
    // mix four times on each of the four vector elements.
    #ifdef __SSE2__
    // Short-hand for the "x -= y" operation.
    #define MINUS_IS(x, y) *x = _mm_sub_epi32(*x, *y)
    static inline void Mix32SSE2(__m128i* __restrict__ a, __m128i* __restrict__ b,
                                 __m128i* __restrict__ c) {
      MINUS_IS(a, b); MINUS_IS(a, c);
      *a = _mm_xor_si128(*a, _mm_srli_epi32(*c, 13));
      MINUS_IS(b, c); MINUS_IS(b, a);
      *b = _mm_xor_si128(*b, _mm_slli_epi32(*a, 8));
      MINUS_IS(c, a); MINUS_IS(c, b);
      *c = _mm_xor_si128(*c, _mm_srli_epi32(*b, 13));
      MINUS_IS(a, b); MINUS_IS(a, c);
      *a = _mm_xor_si128(*a, _mm_srli_epi32(*c, 12));
      MINUS_IS(b, c); MINUS_IS(b, a);
      *b = _mm_xor_si128(*b, _mm_slli_epi32(*a, 16));
      MINUS_IS(c, a); MINUS_IS(c, b);
      *c = _mm_xor_si128(*c, _mm_srli_epi32(*b, 5));
      MINUS_IS(a, b); MINUS_IS(a, c);
      *a = _mm_xor_si128(*a, _mm_srli_epi32(*c, 3));
      MINUS_IS(b, c); MINUS_IS(b, a);
      *b = _mm_xor_si128(*b, _mm_slli_epi32(*a, 10));
      MINUS_IS(c, a); MINUS_IS(c, b);
      *c = _mm_xor_si128(*c, _mm_srli_epi32(*b, 15));
    }
    #undef MINUS_IS
    #endif  // __SSE2__

    // An Altivec implementation of the 32-bit mix() method in jenkins_lookup2.h .
    // It uses Altivec to mix four values in parallel. This is equivalent to calling
    // mix four times on each of the four vector elements.
    #ifdef __ALTIVEC__
    // Short-hand for the "x -= y" operation.
    #define MINUS_IS(x, y) *x = vec_sub(*x, *y)
    static inline void Mix32Altivec(__vector unsigned int* __restrict__ a,
                                    __vector unsigned int* __restrict__ b,
                                    __vector unsigned int* __restrict__ c) {
      MINUS_IS(a, b);
      MINUS_IS(a, c);
      *a = vec_xor(*a, vec_sr(*c, vec_splats(static_cast<uint32>(13))));
      MINUS_IS(b, c);
      MINUS_IS(b, a);
      *b = vec_xor(*b, vec_sl(*a, vec_splats(static_cast<uint32>(8))));
      MINUS_IS(c, a);
      MINUS_IS(c, b);
      *c = vec_xor(*c, vec_sr(*b, vec_splats(static_cast<uint32>(13))));
      MINUS_IS(a, b);
      MINUS_IS(a, c);
      *a = vec_xor(*a, vec_sr(*c, vec_splats(static_cast<uint32>(12))));
      MINUS_IS(b, c);
      MINUS_IS(b, a);
      *b = vec_xor(*b, vec_sl(*a, vec_splats(static_cast<uint32>(16))));
      MINUS_IS(c, a);
      MINUS_IS(c, b);
      *c = vec_xor(*c, vec_sr(*b, vec_splats(static_cast<uint32>(5))));
      MINUS_IS(a, b);
      MINUS_IS(a, c);
      *a = vec_xor(*a, vec_sr(*c, vec_splats(static_cast<uint32>(3))));
      MINUS_IS(b, c);
      MINUS_IS(b, a);
      *b = vec_xor(*b, vec_sl(*a, vec_splats(static_cast<uint32>(10))));
      MINUS_IS(c, a);
      MINUS_IS(c, b);
      *c = vec_xor(*c, vec_sr(*b, vec_splats(static_cast<uint32>(15))));
    }
    #undef MINUS_IS
    #endif  // __SSE2__

    #ifdef __SSE2__
    // Sets the lower two 32-bit lanes of an __m128i equal to a.
    static inline __m128i SetLowerTwoLanes(uint32 a) {
      const __m128i t = _mm_cvtsi32_si128(a);
      return _mm_unpacklo_epi32(t, t);
    }
    // Implements "a += b" on the lower two lanes where a is a __m128i and b is a
    // uint32 scalar.
    static inline void Vec128PlusIs32(__m128i* __restrict__ a, uint32 b) {
      *a = _mm_add_epi32(*a, SetLowerTwoLanes(b));
    }
    #endif  // __SSE2__

    #ifdef __ALTIVEC__
    // Implements "a += b" on the vector lanes where a is a __vector unsigned int
    // and b is a uint32 scalar.
    static inline void Vec128PlusIs32(__vector unsigned int* __restrict__ a,
                                      uint32 b) {
      // GCC does not understand that vec_splats uses b. Avoid a false-positive
      // warning which is upgraded to an error.
      (void)b;
      *a = vec_add(*a, vec_splats(b));
    }
    #endif  // __ALTIVEC__

    // This is a faster version of FingerprintReferenceImplementation(),
    // making use of the fact that we're hashing the same string twice.
    uint64 FingerprintInterleavedImplementation(const char *s, size_t keylen) {
    #ifdef __SSE2__
      // When SSE2 is available, run the two hashes with different seeds in
      // parallel. Note that Mix32SSE actually works with four seeds in parallel,
      // of which we use only two. Still, this is not only faster than the default
      // implementation; it is faster than running just two hashes in parallel
      // using MMX.
      // The golden ratio; an arbitrary value.
      __m128i a = SetLowerTwoLanes(0x9e3779b9UL),
              b = SetLowerTwoLanes(0x9e3779b9UL);
      __m128i c = _mm_set_epi32(0, 0, kFingerprintSeed1, kFingerprintSeed0);
      const size_t orig_len = keylen;
      while (keylen >= 12) {
        Vec128PlusIs32(&a, Google1At(s));
        Vec128PlusIs32(&b, Google1At(s + 4));
        Vec128PlusIs32(&c, Google1At(s + 8));
        s += 12;
        Mix32SSE2(&a, &b, &c);
        keylen -= 12;
      }
      Vec128PlusIs32(&c, orig_len);
      switch (keylen) {  // deal with rest.
        case 0:
          break;
        case 1:
          Vec128PlusIs32(&a, char2unsigned(s[0]));
          break;
        case 2:
          Vec128PlusIs32(&a, char2unsigned(s[0]));
          Vec128PlusIs32(&a, char2unsigned(s[1]) << 8);
          break;
        case 3:
          Vec128PlusIs32(&a, char2unsigned(s[0]));
          Vec128PlusIs32(&a, char2unsigned(s[1]) << 8);
          Vec128PlusIs32(&a, char2unsigned(s[2]) << 16);
          break;
        case 4:
          Vec128PlusIs32(&a, Google1At(s));
          break;
        case 5:
          Vec128PlusIs32(&b, char2unsigned(s[4]));
          Vec128PlusIs32(&a, Google1At(s));
          break;
        case 6:
          Vec128PlusIs32(&b, char2unsigned(s[4]));
          Vec128PlusIs32(&b, char2unsigned(s[5]) << 8);
          Vec128PlusIs32(&a, Google1At(s));
          break;
        case 7:
          Vec128PlusIs32(&b, LittleEndian::Load32(s + 3) >> 8);
          Vec128PlusIs32(&a, Google1At(s));
          break;
        case 8:
          Vec128PlusIs32(&a, Google1At(s));
          Vec128PlusIs32(&b, Google1At(s + 4));
          break;
        case 9:
          Vec128PlusIs32(&a, Google1At(s));
          Vec128PlusIs32(&b, Google1At(s + 4));
          Vec128PlusIs32(&c, char2unsigned(s[8]) << 8);
          break;
        case 10:
          Vec128PlusIs32(&a, Google1At(s));
          Vec128PlusIs32(&b, Google1At(s + 4));
          Vec128PlusIs32(&c, char2unsigned(s[8]) << 8);
          Vec128PlusIs32(&c, char2unsigned(s[9]) << 16);
          break;
        case 11:
          Vec128PlusIs32(&a, Google1At(s));
          Vec128PlusIs32(&b, Google1At(s + 4));
          Vec128PlusIs32(&c, LittleEndian::Load32(s + 7) & 0xffffff00);
          break;
      }
      Mix32SSE2(&a, &b, &c);
      // Obtain the lower two 32-bit lanes, and combine the hash values.
      // NOTE: One would like to use _mm_cvtsi128_si64 here, in order to generate
      // a movq r64, xmm here, but some of our 32-bit compilers do not handle this
      // intrinsic. Therefore, we use _mm_storel_epi64; this generates a
      // movq m64, xmm on 32-bit hardware, while the compiler is still savvy enough
      // to generate a movq r64, xmm in 64-bit mode.
      uint32 val[2];
      _mm_storel_epi64(reinterpret_cast<__m128i*>(val), c);
      return CombineFingerprintHalves(val[0], val[1]);
    #elif defined __ALTIVEC__
      // When Altivec is available, run the two hashes with different seeds in
      // parallel. Note that Mix32Altivec actually works with four seeds in
      // parallel,
      // of which we use only two. Still, this is not only faster than the default
      // implementation.
      // The golden ratio; an arbitrary value.
      __vector unsigned int a = vec_splats(0x9e3779b9U);
      __vector unsigned int b = vec_splats(0x9e3779b9U);
      __vector unsigned int c =
          (__vector unsigned int){kFingerprintSeed0, kFingerprintSeed1, 0, 0};
      const size_t orig_len = keylen;
      while (keylen >= 12) {
        Vec128PlusIs32(&a, Google1At(s));
        Vec128PlusIs32(&b, Google1At(s + 4));
        Vec128PlusIs32(&c, Google1At(s + 8));
        s += 12;
        Mix32Altivec(&a, &b, &c);
        keylen -= 12;
      }
      Vec128PlusIs32(&c, orig_len);
      switch (keylen) {  // deal with rest.
        case 0:
          break;
        case 1:
          Vec128PlusIs32(&a, char2unsigned(s[0]));
          break;
        case 2:
          Vec128PlusIs32(&a, char2unsigned(s[0]));
          Vec128PlusIs32(&a, char2unsigned(s[1]) << 8);
          break;
        case 3:
          Vec128PlusIs32(&a, char2unsigned(s[0]));
          Vec128PlusIs32(&a, char2unsigned(s[1]) << 8);
          Vec128PlusIs32(&a, char2unsigned(s[2]) << 16);
          break;
        case 4:
          Vec128PlusIs32(&a, Google1At(s));
          break;
        case 5:
          Vec128PlusIs32(&b, char2unsigned(s[4]));
          Vec128PlusIs32(&a, Google1At(s));
          break;
        case 6:
          Vec128PlusIs32(&b, char2unsigned(s[4]));
          Vec128PlusIs32(&b, char2unsigned(s[5]) << 8);
          Vec128PlusIs32(&a, Google1At(s));
          break;
        case 7:
          Vec128PlusIs32(&b, LittleEndian::Load32(s + 3) >> 8);
          Vec128PlusIs32(&a, Google1At(s));
          break;
        case 8:
          Vec128PlusIs32(&a, Google1At(s));
          Vec128PlusIs32(&b, Google1At(s + 4));
          break;
        case 9:
          Vec128PlusIs32(&a, Google1At(s));
          Vec128PlusIs32(&b, Google1At(s + 4));
          Vec128PlusIs32(&c, char2unsigned(s[8]) << 8);
          break;
        case 10:
          Vec128PlusIs32(&a, Google1At(s));
          Vec128PlusIs32(&b, Google1At(s + 4));
          Vec128PlusIs32(&c, char2unsigned(s[8]) << 8);
          Vec128PlusIs32(&c, char2unsigned(s[9]) << 16);
          break;
        case 11:
          Vec128PlusIs32(&a, Google1At(s));
          Vec128PlusIs32(&b, Google1At(s + 4));
          Vec128PlusIs32(&c, LittleEndian::Load32(s + 7) & 0xffffff00);
          break;
      }
      Mix32Altivec(&a, &b, &c);
      uint32 c_0;
      uint32 c_1;
      vec_ste(vec_splat(c, 0), 0, &c_0);
      vec_ste(vec_splat(c, 1), 0, &c_1);
      return CombineFingerprintHalves(c_0, c_1);
    #else
      // Non-vector implementation: the code is tedious to read, but it's just two
      // interleaved copies of Hash32StringWithSeed().
      uint32 a, b, c, d, e, f;

      a = b = d = e = 0x9e3779b9UL;   // the golden ratio; an arbitrary value

      if (keylen >= 12) {
        c = kFingerprintSeed0;
        f = kFingerprintSeed1;
        size_t orig_len = keylen;
        do {
          // d -= a;
          // a += Google1At(s);
          // d += a;
          // The 3 lines above are functionally equivalent with these 2 lines.
          // a += Google1At(s);
          // d += Google1At(s);
          // Using a local var instead of -=/+= trick increases register
          // pressure and leads to 10 % slower fingerprinting (2800 cores).
          d -= a;
          e -= b;
          f -= c;
          a += Google1At(s);
          b += Google1At(s + 4);
          c += Google1At(s + 8);
          d += a;
          e += b;
          s += 12;
          f += c;
          mix(a, b, c);
          mix(d, e, f);
          keylen -= 12;
        } while (keylen >= 12);
        c += orig_len;
        f += orig_len;
      } else {
        c = kFingerprintSeed0 + keylen;
        f = kFingerprintSeed1 + keylen;
      }
      switch (keylen) {  // deal with rest.
        case 0:
          break;
        case 1 :
          a += char2unsigned(s[0]);
          d += char2unsigned(s[0]);
          break;
        case 2 :
          d -= a;
          a += char2unsigned(s[0]);
          a += char2unsigned(s[1]) << 8;
          d += a;
          break;
        case 3 :
          d -= a;
          a += char2unsigned(s[0]);
          a += char2unsigned(s[1]) << 8;
          a += char2unsigned(s[2]) << 16;
          d += a;
          break;
        case 4 :
          a += Google1At(s);
          d += Google1At(s);
          break;
        case 5 :
          a += Google1At(s);
          d += Google1At(s);
          b += char2unsigned(s[4]);
          e += char2unsigned(s[4]);
          break;
        case 6 :
          e -= b;
          a += Google1At(s);
          d += Google1At(s);
          b += char2unsigned(s[4]);
          b += char2unsigned(s[5]) << 8;
          e += b;
          break;
        case 7 :
          e -= b;
          a += Google1At(s);
          d += Google1At(s);
          b += char2unsigned(s[4]);
          b += char2unsigned(s[5]) << 8;
          b += char2unsigned(s[6]) << 16;
          e += b;
          break;
        case 8 :
          d -= a;
          e -= b;
          a += Google1At(s);
          b += Google1At(s+4);
          d += a;
          e += b;
          break;
        case 9 :
          d -= a;
          e -= b;
          a += Google1At(s);
          b += Google1At(s+4);
          c += char2unsigned(s[8]) << 8;
          f += char2unsigned(s[8]) << 8;
          d += a;
          e += b;
          break;
        case 10:
          d -= a;
          e -= b;
          f -= c;
          a += Google1At(s);
          b += Google1At(s+4);
          c += char2unsigned(s[8]) << 8;
          c += char2unsigned(s[9]) << 16;
          d += a;
          e += b;
          f += c;
          break;
        case 11:
          d -= a;
          e -= b;
          f -= c;
          a += Google1At(s);
          b += Google1At(s+4);
          c += char2unsigned(s[8]) << 8;
          c += char2unsigned(s[9]) << 16;
          c += char2unsigned(s[10]) << 24;
          d += a;
          e += b;
          f += c;
          break;
      }
      mix(a, b, c);
      mix(d, e, f);
      return CombineFingerprintHalves(c, f);
    #endif  // __SSE2__
    }

    // Extern template definitions.

    #if defined(__GNUC__) && !defined(__QNX__)
    HASH_NAMESPACE_DECLARATION_START
    template class hash_set<string>;
    template class hash_map<string, string>;
    HASH_NAMESPACE_DECLARATION_END
    #endif

#else

#ifdef EXTERNAL_FP
// MOE:end_strip
static const uint32 kFingerprintSeed0 = 0xabc;
static const uint32 kFingerprintSeed1 = 0xdef;
// MOE:begin_strip
#else
static const uint32 kFingerprintSeed0 = 0;
    static const uint32 kFingerprintSeed1 = 102072;
#endif
// MOE:end_strip

uint64 FingerprintReferenceImplementation(const char *s, size_t len) {
    uint32 hi = Hash32StringWithSeed(s, len, kFingerprintSeed0);
    uint32 lo = Hash32StringWithSeed(s, len, kFingerprintSeed1);
    return CombineFingerprintHalves(hi, lo);
}

uint64 FingerprintInterleavedImplementation(const char *s, size_t len) {
    uint32 hi = Hash32StringWithSeed(s, len, kFingerprintSeed0);
    uint32 lo = Hash32StringWithSeed(s, len, kFingerprintSeed1);
    return CombineFingerprintHalves(hi, lo);
}

// Remove duplicate definitions.
//#if defined(__GNUC__) && !defined(__QNX__)
//HASH_NAMESPACE_DECLARATION_START
//template class hash_set<string>;
//template class hash_map<string, string>;
//HASH_NAMESPACE_DECLARATION_END
//#endif

// MOE:begin_strip
#endif  // !defined(AVOID_TRADE_SECRET_CODE)
// MOE:end_strip

