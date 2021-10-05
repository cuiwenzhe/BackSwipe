//
// Copyright (C) 1999 and onwards Google, Inc.
//
// Author: Craig Silverstein
//
// This file contains routines for hashing and fingerprinting.
//
// A hash function takes an arbitrary input bitstring (string, char*,
// number) and turns it into a hash value (a fixed-size number) such
// that unequal input values have a high likelihood of generating
// unequal hash values.  A fingerprint is a hash whose design is
// biased towards avoiding hash collisions, possibly at the expense of
// other characteristics such as execution speed.
//
// In general, if you are only using the hash values inside a single
// executable -- you're not writing the values to disk, and you don't
// depend on another instance of your program, running on another
// machine, generating the same hash values as you -- you want to use
// a HASH.  Otherwise, you want to use a FINGERPRINT.
//
// RECOMMENDED HASH FOR STRINGS:    GoodFastHash
//
// It is a functor, so you can use it like this:
//     hash_map<string, xxx, GoodFastHash<string> >
//     hash_set<char *, GoodFastHash<char*> >
//
// RECOMMENDED HASH FOR NUMBERS:    hash<>
//
// Note that this is likely the identity hash, so if your
// numbers are "non-random" (especially in the low bits), another
// choice is better.  You can use it like this:
//     hash_map<int, xxx>
//     hash_set<uint64>
//
// RECOMMENDED HASH FOR POINTERS:    hash<>
//
// This is a lightly mixed hash. An identity function can be a poor choice
// because pointers can have low entropy in the least significant bits, and
// those bits are important for efficiency in some uses, e.g., dense_hash_map<>.
//
// RECOMMENDED HASH FOR std::pair<T, U>: GoodFastHash<std::pair<T, U>>.
//
// This is significantly faster than hash<std::pair<T, U>> and calls
// GoodFastHash<T> and GoodFastHash<U> (instead of hash<T> and hash<U>), where
// they are available.
//
// RECOMMENDED HASH FOR std::tuple: hash<std::tuple<T, U, ...>>
//
// RECOMMENDED HASH FOR std::array: hash<std::array<T, N>>
//
// RECOMMENDED HASH FOR STRUCTS: ::util_hash::Hash
//
// ::util_hash::Hash(x, y, z) hashes x, y, and z (using GoodFastHash<> if it's
// available; otherwise using hash<>) and then combines the hashes.
//
// RECOMMENDED FINGERPRINT:
//
// For string input, use Fingerprint2011 from fingerprint2011.h. Do *not* use
// Fingerprint in new code; it has problems with excess collisions (see below).
//
// For integer input, Fingerprint is still recommended though collisions are
// possible.
//
// OTHER HASHES AND FINGERPRINTS:
//
// See http://wiki/Main/GdhChoosingAHashFunction
//
// The wiki page also has good advice for when to use a fingerprint vs
// a hash.
//
//
// Note: if your file declares hash_map<string, ...> or
// hash_set<string>, it will use the default hash function,
// hash<string>.  This is not a great choice.  Always provide an
// explicit functor, such as GoodFastHash, as a template argument.
// (Either way, you will need to #include this file to get the
// necessary definition.)
//
// Some of the hash functions below are documented to be fixed
// forever; the rest (whether they're documented as so or not) may
// change over time.  "Change over time" here means that the hash may
// depend on a value that's reseeded at process startup, and thus may
// not be consistent even across multiple runs of the same binary.  If
// you require a hash function that does not change over time, you
// should have unittests enforcing this property.  We already have
// several such functions; see hash_unittest.cc for the details and
// unittests.

#ifndef UTIL_HASH_HASH_H_
#define UTIL_HASH_HASH_H_

#include <stddef.h>
#include <stdint.h>     // for uintptr_t
#include <string.h>
#include <algorithm>
//#include <hash_map>     // hacky way to make sure we import standard hash<> fns
//#include <hash_set>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "casts.h"
#include "int128.h"
#include "integral_types.h"
#include "macros.h"
#include "port.h"
#include "builtin_type_hash.h"
#include "city.h"
#include "hash128to64.h"
#include "jenkins.h"
#include "jenkins_lookup2.h"
#include "string_hash.h"

#ifdef LANG_CXX11
#include <array>
#include <tuple>
#endif

// For now, we always assume we need to use the January 2016 versions
// of hash<string>, GoodFastHash<string>, CityHash, hash<Tuple<T>>, ...
// But we'll flip the default after a cleanup.  Bug 27047089.
// Also, for now, one may define NO_GOLDEN_TEST to get the new behavior.
#ifndef NO_GOLDEN_TEST
#define GOLDEN_TEST 1
#endif

// TODO(jyrki): This is one of the legacy hashes, kept here only
// temporarily for making it easier to remove the physical dependency
// to legacy_hash.h
inline uint32 HashTo32(const char *s, size_t slen) {
    uint32 retval = Hash32StringWithSeed(s, slen, MIX32);
    return retval == kIllegalHash32 ? static_cast<uint32>(retval-1) : retval;
}

inline uint64 Hash64StringWithSeed(const string& s, uint64 seed) {
    return Hash64StringWithSeed(s.data(), s.size(), seed);
}

namespace util_hash {
// Don't bother with HashBase unless you're supporting
// the legacy stdext::hash_* containers on MSVC.
// A class derived from HashBase conforms to the C++11 standard
// Hash concept: [http://en.cppreference.com/w/cpp/concept/Hash].
// On MSVC builds, it additionally meets the "hash traits" requirements:
// [https://msdn.microsoft.com/en-us/library/1s1byw77.aspx].
// In the MSVC hash containers, a single 'hash_compare'
// template parameter provides both key hashing and key
// equivalence. It also controls the container's bucket geometry.
#ifdef _MSC_VER
    template <typename Key, typename StrictWeakOrder = std::less<Key> >
struct HashBase : ::stdext::hash_compare<Key, StrictWeakOrder> {};
#else
    template <typename Key, typename Ignored = void>
    struct HashBase {
    private:
        class CannotBeCalled {
            // 'explicit' so that hasher({}) is unambiguous.
            explicit CannotBeCalled() {}  // NOLINT(runtime/explicit)
        };
    public:
        // HashBase-derived classes must have a
        // "using HashBase<Key>::operator()" to properly overload with,
        // rather than hide, inherited call operators. So a call
        // operator is provided here that exists, but always loses.
        void operator()(CannotBeCalled) const;
    };
#endif  // _MSC_VER

// A strict weak order based on strcmp.
    struct ByStrCmp {
        bool operator()(const char* a, const char* b) const {
            return strcmp(a, b) < 0;
        }
    };

// A few hash specializations just perform static_cast<size_t>.
    template <typename T>
    struct TrivialHashBase : HashBase<T> {
        using HashBase<T>::operator();
        size_t operator()(const T& x) const { return static_cast<size_t>(x); }
    };
}  // namespace util_hash

HASH_NAMESPACE_DECLARATION_START

//// STLport and MSVC 10.0 above already define these.
//#if !defined(_MSC_VER)
//#if !defined(_STLP_LONG_LONG)
//template <> struct hash<long long>  // NOLINT(runtime/int)
//        : util_hash::TrivialHashBase<long long> {};  // NOLINT(runtime/int)
//template <> struct hash<unsigned long long>  // NOLINT(runtime/int)
//        : util_hash::TrivialHashBase<unsigned long long> {};  // NOLINT(runtime/int)
//#endif  // !defined(_STLP_LONG_LONG)
//template <> struct hash<bool> : util_hash::TrivialHashBase<bool> {};
//
//// This intended to be a "good" hash function.  It may change from time to time.
//// TODO(aiuto): If the need arises to use hash_map with unit128 in apps that
//// build for Windows, then define the equivalent as stdext::hash_compare
//template <> struct hash<uint128>
//        : util_hash::HashBase<uint128> {
//    using util_hash::HashBase<uint128>::operator();
//    size_t operator()(const uint128& x) const {
//        if (sizeof(&x) == 8) {  // 64-bit systems have 8-byte pointers.
//            return Hash128to64(x);
//        } else {
//            uint32 a = static_cast<uint32>(Uint128Low64(x)) +
//                       static_cast<uint32>(0x9e3779b9UL);
//            uint32 b = static_cast<uint32>(Uint128Low64(x) >> 32) +
//                       static_cast<uint32>(0x9e3779b9UL);
//            uint32 c = static_cast<uint32>(Uint128High64(x)) + MIX32;
//            mix(a, b, c);
//            a += static_cast<uint32>(Uint128High64(x) >> 32);
//            mix(a, b, c);
//            return c;
//        }
//    }
//};
//
//// Hash pointers as integers, but bring more entropy to the lower bits.
//template <typename T> struct hash<T*>
//        : util_hash::HashBase<T*> {
//    using util_hash::HashBase<T*>::operator();
//    size_t operator()(T *x) const {
//        size_t k = static_cast<size_t>(reinterpret_cast<uintptr_t>(x));
//        return k + (k >> 6);
//    }
//};
//#endif  // !defined(_MSC_VER)

//#if defined(__GNUC__)
//#if !defined(STLPORT)
//// Use our nice hash function for strings
//template <typename CharT, typename Traits, typename Alloc>
//struct hash<basic_string<CharT, Traits, Alloc> >
//        : util_hash::HashBase<basic_string<CharT, Traits, Alloc> > {
//    using util_hash::HashBase<basic_string<CharT, Traits, Alloc> >::operator();
//    size_t operator()(const basic_string<CharT, Traits, Alloc>& k) const {
//        return Hash32StringWithSeed(k.data(), k.size(), MIX32);
//    }
//};
//#endif  // !defined(STLPORT)
//#endif  // defined(__GNUC__)

// MSVC's STL requires an ever-so slightly different decl
#if defined(STL_MSVC)
template <> struct hash<const char*>
    : util_hash::HashBase<const char*, util_hash::ByStrCmp> {
  using util_hash::HashBase<const char*, util_hash::ByStrCmp>::operator();
  size_t operator()(const char* k) const { return HashTo32(k, strlen(k)); }
};

// MSVC 10.0 and above have already defined this.
#if !defined(_MSC_VER)
template <> struct hash<string>
    : util_hash::HashBase<string> {
  using util_hash::HashBase<string>::operator();
  size_t operator()(const string& k) const {
    return HashTo32(k.data(), k.size());
  }
};
#endif  // !defined(_MSC_VER)

#endif  // defined(STL_MSVC)

// Hasher for STL pairs. Requires hashers for both members to be defined.
// Prefer GoodFastHash, particularly if speed is important.
//template <typename First, typename Second>
//struct hash<std::pair<First, Second> >
//        : util_hash::HashBase<std::pair<First, Second> > {
//    using util_hash::HashBase<std::pair<First, Second> >::operator();
//    size_t operator()(const std::pair<First, Second>& p) const {
//        size_t h1 = HashPart(p.first);
//        size_t h2 = HashPart(p.second);
//        // The decision below is at compile time
//        return (sizeof(h1) <= sizeof(uint32)) ?
//               Hash32NumWithSeed(h1, h2)
//                                              : Hash64NumWithSeed(h1, h2);
//    }
//
//private:
//    template <typename T>
//    static size_t HashPart(const T& x) { return hash<T>()(x); }
//};

HASH_NAMESPACE_DECLARATION_END

// If you want an excellent string hash function, and you don't mind if it
// might change when you sync and recompile, please use GoodFastHash<>.
// For most applications, GoodFastHash<> is a good choice, better than
// hash<string> or hash<char*> or similar.  GoodFastHash<> can change
// from time to time and may differ across platforms, and we'll strive
// to keep improving it.
//
// By the way, when deleting the contents of a hash_set of pointers, it is
// unsafe to delete *iterator because the hash function may be called on
// the next iterator advance.  Use STLDeleteContainerPointers().

template <typename X> struct GoodFastHash;

namespace util_hash {
    namespace internal {

// Uses GoodFastHash<T> if available, otherwise HASH_NAMESPACE::hash<T>.
// Pre-C++11, GoodFastHash<T> cannot be detected, so it's
// always HASH_NAMESPACE::hash<T>.
// This is a *private* internal detail of the util/hash library.
        template <typename T> size_t ChooseHasher(const T& x);

    }  // namespace internal
}  // namespace util_hash

// This intended to be a "good" hash function.  It may change from time to time.
template <> struct GoodFastHash<char*>
        : util_hash::HashBase<char*, util_hash::ByStrCmp> {
    using util_hash::HashBase<char*, util_hash::ByStrCmp>::operator();
    size_t operator()(char* s) const {
        return HashStringThoroughly(s, strlen(s));
    }
};

// This intended to be a "good" hash function.  It may change from time to time.
template <> struct GoodFastHash<const char*>
        : util_hash::HashBase<const char*, util_hash::ByStrCmp> {
    using util_hash::HashBase<const char*, util_hash::ByStrCmp>::operator();
    size_t operator()(const char* s) const {
        return HashStringThoroughly(s, strlen(s));
    }
};

// This intended to be a "good" hash function.  It may change from time to time.
template <typename CharT, typename Traits, typename Alloc>
struct GoodFastHash<basic_string<CharT, Traits, Alloc> >
        : util_hash::HashBase<basic_string<CharT, Traits, Alloc> > {
    using util_hash::HashBase<basic_string<CharT, Traits, Alloc> >::operator();
    size_t operator()(const basic_string<CharT, Traits, Alloc>& k) const {
        return HashStringThoroughly(k.data(), k.size() * sizeof(k)); //TODO: sizeof(k[0]) -> sizeof(k)
    }
};

// This intended to be a "good" hash function; it is much faster than
// hash<std::pair<T, U>>.  It may change from time to time.  Requires
// GoodFastHash<> or hash<> to be defined for T and U.
template <typename T, typename U>
struct GoodFastHash<std::pair<T, U> >
        : util_hash::HashBase<std::pair<T, U> > {
    using util_hash::HashBase<std::pair<T, U> >::operator();
    size_t operator()(const std::pair<T, U>& k) const {
        size_t h1 = util_hash::internal::ChooseHasher(k.first);
        size_t h2 = util_hash::internal::ChooseHasher(k.second);

        // Mix the hashes together.  Multiplicative hashing mixes the high-order
        // bits better than the low-order bits, and rotating moves the high-order
        // bits down to the low end, where they matter more for most hashtable
        // implementations.
        static const size_t kMul = static_cast<size_t>(0xc6a4a7935bd1e995ULL);
        if (std::is_integral<T>::value) {
            // We want to avoid GoodFastHash({x, y}) == 0 for common values of {x, y}.
            // hash<X> is the identity function for integral types X, so without this,
            // GoodFastHash({0, 0}) would be 0.
            h1 += 109;
        }
        h1 = h1 * kMul;
        h1 = (h1 << 21) | (h1 >> (std::numeric_limits<size_t>::digits - 21));
        return h1 + h2;
    }
};

#ifdef LANG_CXX11

namespace util_hash {
    namespace internal {

        // Lightly hash two hash codes together. When used repetitively to mix more
        // than two values, the new values should be in the first argument.
        inline size_t Mix(size_t new_hash, size_t accu) {
            static const size_t kMul = static_cast<size_t>(0xc6a4a7935bd1e995ULL);
            // Multiplicative hashing will mix bits better in the msb end ...
            accu *= kMul;
            // ... and rotating will move the better mixed msb-bits to lsb-bits.
            return ((accu << 21) |
                    (accu >> (std::numeric_limits<size_t>::digits - 21))) +
                   new_hash;
        }

        // Iteratively hash and mix, starting with the end of the tuple.
        template <typename Tup, size_t I = 0, size_t N = std::tuple_size<Tup>::value>
        struct HashTupleImpl {
            size_t operator()(const Tup& t, size_t seed) const {
                using std::get;
                return Mix(ChooseHasher(get<I>(t)), HashTupleImpl<Tup, I + 1>{}(t, seed));
            }
        };
        template <typename Tup, size_t N>
        struct HashTupleImpl<Tup, N, N> {
            size_t operator()(const Tup&, size_t seed) const { return seed; }
        };

        template <typename Tup>
        size_t HashTuple(const Tup& t, size_t seed) {
            return 0xc6a4a7935bd1e995ULL * HashTupleImpl<Tup>{}(t, seed);
        }

    }  // namespace internal

    // ::util_hash::Hash(a, b, c, ...) hashes a, b, c, and so on (using
    // GoodFastHash<>, if it's available for the given type, otherwise using
    // hash<>), and then combines the individual hashes.  This is intended to be a
    // pretty good hash function, which may change from time to time.  (Its quality
    // mostly depends on the quality of GoodFastHash<> and/or hash<>.)
    //
    // In the somewhat unusual case of nested calls to Hash(), it is best if
    // the new values should appear first in the arguments list.  For example:
    //
    //  size_t Hash(int x, int y, vector<T> v, vector<T> w) {
    //    auto combine = [](size_t h, const T& elem) {
    //      return util_hash::Hash(elem, h);  // Note that elem is the first arg.
    //    };
    //    size_t vh =
    //        std::accumulate(v.begin(), v.end(), static_cast<size_t>(0), combine);
    //    size_t wh =
    //        std::accumulate(w.begin(), w.end(), static_cast<size_t>(0), combine);
    //    // Note that x and y come before vh and wh.
    //    return util_hash::Hash(x, y, vh, wh);
    //  }
    //
    // A stronger (and slower) way to combine multiple hash codes together is to
    // use hash<uint128>.  The order of args in hash<uint128> doesn't matter.  For
    // example:
    //
    //  size_t Hash(T x, U y) {
    //    return hash<uint128>()(uint128(util_hash::Hash(x), util_hash::Hash(y)));
    //  }

    inline size_t Hash() {
        return 113;
    }

    template <typename First, typename... T>
    size_t Hash(const First& f, const T&... t) {
        return internal::Mix(internal::ChooseHasher(f), Hash(t...));
    }
}  // namespace util_hash

//HASH_NAMESPACE_DECLARATION_START
//
//// Hash functions for tuples.  These are intended to be "good" hash functions.
//// They may change from time to time.  GoodFastHash<> or hash<> must be defined
//// for the tuple elements.
//template <typename... T>
//struct hash<std::tuple<T...>>
//        : util_hash::HashBase<std::tuple<T...>> {
//    using util_hash::HashBase<std::tuple<T...>>::operator();
//    size_t operator()(const std::tuple<T...>& t) const {
//        return util_hash::internal::HashTuple(t, 113);
//    }
//};
//
//// Hash functions for std::array.  These are intended to be "good" hash
//// functions.  They may change from time to time.  GoodFastHash<> or hash<> must
//// be defined for T.
//template <typename T, std::size_t N>
//struct hash<std::array<T, N>>
//        : util_hash::HashBase<std::array<T, N>> {
//    using util_hash::HashBase<std::array<T, N>>::operator();
//    size_t operator()(const std::array<T, N>& t) const {
//        return util_hash::internal::HashTuple(t, 71);
//    }
//};
//
//HASH_NAMESPACE_DECLARATION_END

#endif  // LANG_CXX11

namespace util_hash {
    namespace internal {

#ifdef LANG_CXX11
        template <typename T, decltype(GoodFastHash<T>()(std::declval<T>())) = 0>
        size_t ChooseHasherImpl(const T& v, int) {
            return GoodFastHash<T>()(v);
        }
#endif  // LANG_CXX11

        template <typename T>
        size_t ChooseHasherImpl(const T& v, ...) {
            return std::hash<T>()(v);
        }

        template <typename T>
        size_t ChooseHasher(const T& x) {
            return ChooseHasherImpl<T>(x, 0);
        }

    }  // namespace internal
}  // namespace util_hash

// ----------------------------------------------------------------------
// Fingerprint()
//   When used for string input, this is not recommended for new code.
//   Instead, use Fingerprint2011(), a higher-quality and faster hash function.
//   (See fingerprint2011.h.) The functions below that take integer input are
//   still recommended.
//
//   Fingerprinting a string (or char*) will never return 0 or 1,
//   in case you want a couple of special values.  However,
//   fingerprinting a numeric type may produce 0 or 1.
//   MOE:begin_strip
//
//   The hash mapping of Fingerprint() will never change.
//
//   Note: AVOID USING FINGERPRINT FOR STRING INPUT if at all possible.  Use
//   Fingerprint2011 (in fingerprint2011.h) instead.
//   Fingerprint() is susceptible to collisions for even short
//   strings with low edit distance; see
//     google3/experimental/users/mjansche/fingerprint/fprint_collision_test.cc
//   Example collisions:
//     "01056/02" vs. "11057/02"
//     "LTA 02" vs. "MTA 12"
//   The same study found only one collision each for CityHash64() and
//   MurmurHash64(), from more than 2^32 inputs, and on medium-length
//   strings with large edit distances. These issues, among others,
//   led to the recommendation that new code should avoid Fingerprint() for
//   string input.
//   MOE:end_strip
// ----------------------------------------------------------------------
uint64 FingerprintReferenceImplementation(const char *s, size_t len);

uint64 FingerprintInterleavedImplementation(const char *s, size_t len);

inline uint64 Fingerprint(const char *s, size_t len) {
    if (sizeof(s) == 8) {  // 64-bit systems have 8-byte pointers.
        // The better choice when we have a decent number of registers.
        return FingerprintInterleavedImplementation(s, len);
    } else {
        return FingerprintReferenceImplementation(s, len);
    }
}

// Routine that combines together the hi/lo part of a fingerprint
// and changes the result appropriately to avoid returning 0/1.
inline uint64 CombineFingerprintHalves(uint64 hi, uint32 lo) {
    uint64 result = (hi << 32) | lo;
    // (result >> 1) is here the same as (result > 1), but slightly faster.
    if (PREDICT_TRUE(result >> 1)) {
        return result;  // Not 0 or 1, return as is.
    }
    return result ^ GG_ULONGLONG(0x130f9bef94a0a928);
}

inline uint64 Fingerprint(const string &s) {
    return Fingerprint(s.data(), s.size());
}

inline uint64 Fingerprint(char c) {
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(signed char c) {
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(unsigned char c) {
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(short c) {  // NOLINT(runtime/int)
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(unsigned short c) {  // NOLINT(runtime/int)
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(int c) {
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(unsigned int c) {  // NOLINT(runtime/int)
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(long c) {  // NOLINT(runtime/int)
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(unsigned long c) {  // NOLINT(runtime/int)
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(long long c) {  // NOLINT(runtime/int)
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

inline uint64 Fingerprint(unsigned long long c) {  // NOLINT(runtime/int)
    return Hash64NumWithSeed(static_cast<uint64>(c), MIX64);
}

// This concatenates two 64-bit fingerprints. It is a convenience function to
// get a fingerprint for a combination of already fingerprinted components.
// It assumes that each input is already a good fingerprint itself.
// Note that this is legacy code and new code should use its replacement
// FingerprintCat2011().
//
// Note that in general it's impossible to construct Fingerprint(str)
// from the fingerprints of substrings of str.  One shouldn't expect
// FingerprintCat(Fingerprint(x), Fingerprint(y)) to indicate
// anything about Fingerprint(StrCat(x, y)).
inline uint64 FingerprintCat(uint64 fp1, uint64 fp2) {
    return Hash64NumWithSeed(fp1, fp2);
}

// Extern template declarations.
//
// gcc only for now.  msvc and others: this technique is likely to work with
// your compiler too.  changelists welcome.
//
// This technique is limited to template specializations whose hash key
// functions are declared in this file.

//#if defined(__GNUC__)
//HASH_NAMESPACE_DECLARATION_START
//extern template class hash_set<string>;
//extern template class hash_map<string, string>;
//HASH_NAMESPACE_DECLARATION_END
//#endif  // defined(__GNUC__)

#endif  // UTIL_HASH_HASH_H_