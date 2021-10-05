// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// #status: RECOMMENDED
// #category: operations on strings
// #summary: Functions for splitting strings into substrings.
//
// This file contains functions for splitting strings. The new and recommended
// API for string splitting is the strings::Split() function. The old API is a
// large collection of standalone functions declared at the bottom of this file
// in the global scope.
//
// TODO(user): Rough migration plan from old API to new API
// (1) Add comments to old Split*() functions showing how to do the same things
//     with the new API.
// (2) Reimplement some of the old Split*() functions in terms of the new
//     Split() API. This will allow deletion of code in split.cc.
// (3) (Optional) Replace old Split*() API calls at call sites with calls to new
//     Split() API.
//
#ifndef STRINGS_SPLIT_H_
#define STRINGS_SPLIT_H_

#include <stddef.h>
#include <algorithm>
using std::copy;
using std::max;
using std::min;
using std::reverse;
using std::sort;
using std::swap;
#include <ext/hash_map>
using __gnu_cxx::hash;
using __gnu_cxx::hash_map;
#include <ext/hash_set>
using __gnu_cxx::hash;
using __gnu_cxx::hash_set;
using __gnu_cxx::hash_multiset;
#include <iterator>
using std::back_insert_iterator;
using std::iterator_traits;
#include <map>
using std::map;
using std::multimap;
#include <set>
using std::multiset;
using std::set;
#include <string>
using std::string;
#include <utility>
using std::make_pair;
using std::pair;
#include <vector>
using std::vector;

#include "../base/integral_types.h"
#include "../base/logging.h"
#include "../base/charset.h"
#include "split_internal.h"
#include "../base/stringpiece.h"
#include "strip.h"

namespace strings {

    //                              The new Split API
    //                                  aka Split2
    //                              aka strings::Split()
    //
    // This string splitting API consists of a Split() function in the ::strings
    // namespace and a handful of delimiter objects in the ::strings::delimiter
    // namespace (more on delimiter objects below). The Split() function always
    // takes two arguments: the text to be split and the delimiter on which to split
    // the text. An optional third argument may also be given, which is a Predicate
    // functor that will be used to filter the results, e.g., to skip empty strings
    // (more on predicates below). The Split() function adapts the returned
    // collection to the type specified by the caller.
    //
    // Example 1:
    //   // Splits the given string on commas. Returns the results in a
    //   // vector of strings.
    //   vector<string> v = strings::Split("a,b,c", ",");
    //   assert(v.size() == 3);
    //
    // Example 2:
    //   // By default, empty strings are *included* in the output. See the
    //   // strings::SkipEmpty predicate below to omit them.
    //   vector<string> v = strings::Split("a,b,,c", ",");
    //   assert(v.size() == 4);  // "a", "b", "", "c"
    //   v = strings::Split("", ",");
    //   assert(v.size() == 1);  // v contains a single ""
    //
    // Example 3:
    //   // Splits the string as in the previous example, except that the results
    //   // are returned as StringPiece objects. Note that because we are storing
    //   // the results within StringPiece objects, we have to ensure that the input
    //   // string outlives any results.
    //   vector<StringPiece> v = strings::Split("a,b,c", ",");
    //   assert(v.size() == 3);
    //
    // Example 4:
    //   // Stores results in a set<string>.
    //   set<string> a = strings::Split("a,b,c,a,b,c", ",");
    //   assert(a.size() == 3);
    //
    // Example 5:
    //   // Stores results in a map. The map implementation assumes that the input
    //   // is provided as a series of key/value pairs. For example, the 0th element
    //   // resulting from the split will be stored as a key to the 1st element. If
    //   // an odd number of elements are resolved, the last element is paired with
    //   // a default-constructed value (e.g., empty string).
    //   map<string, string> m = strings::Split("a,b,c", ",");
    //   assert(m.size() == 2);
    //   assert(m["a"] == "b");
    //   assert(m["c"] == "");  // last component value equals ""
    //
    // Example 6:
    //   // Splits on the empty string, which results in each character of the input
    //   // string becoming one element in the output collection.
    //   vector<string> v = strings::Split("abc", "");
    //   assert(v.size() == 3);
    //
    // Example 7:
    //   // Stores first two split strings as the members in an std::pair.
    //   std::pair<string, string> p = strings::Split("a,b,c", ",");
    //   EXPECT_EQ("a", p.first);
    //   EXPECT_EQ("b", p.second);
    //   // "c" is omitted because std::pair can hold only two elements.
    //
    // As illustrated above, the Split() function adapts the returned collection to
    // the type specified by the caller. The returned collections may contain
    // string, StringPiece, Cord, or any object that has a constructor (explicit or
    // not) that takes a single StringPiece argument. This pattern works for all
    // standard STL containers including vector, list, deque, set, multiset, map,
    // and multimap, non-standard containers including hash_set and hash_map, and
    // even std::pair which is not actually a container.
    //
    // Splitting to std::pair is an interesting case because it can hold only two
    // elements and is not a collection type. When splitting to an std::pair the
    // first two split strings become the std::pair's .first and .second members
    // respectively. The remaining split substrings are discarded. If there are less
    // than two split substrings, the empty string is used for the corresponding
    // std::pair member.
    //
    // The strings::Split() function can be used multiple times to perform more
    // complicated splitting logic, such as intelligently parsing key-value pairs.
    // For example
    //
    //   // The input string "a=b=c,d=e,f=,g" becomes
    //   // { "a" => "b=c", "d" => "e", "f" => "", "g" => "" }
    //   map<string, string> m;
    //   for (StringPiece sp : strings::Split("a=b=c,d=e,f=,g", ",")) {
    //     m.insert(strings::Split(sp, strings::delimiter::Limit("=", 1)));
    //   }
    //   EXPECT_EQ("b=c", m.find("a")->second);
    //   EXPECT_EQ("e", m.find("d")->second);
    //   EXPECT_EQ("", m.find("f")->second);
    //   EXPECT_EQ("", m.find("g")->second);
    //
    // The above example stores the results in an std::map. But depending on your
    // data requirements, you can just as easily store the results in an
    // std::multimap or even a vector<std::pair<>>.
    //
    //
    //                                  Delimiters
    //
    // The Split() function also takes a second argument that is a delimiter. This
    // delimiter is actually an object that defines the boundaries between elements
    // in the provided input. If a string (const char*, ::string, or StringPiece) is
    // passed in place of an explicit Delimiter object, the argument is implicitly
    // converted to a ::strings::delimiter::Literal.
    //
    // With this split API comes the formal concept of a Delimiter (big D). A
    // Delimiter is an object with a Find() function that knows how find the first
    // occurrence of itself in a given StringPiece. Models of the Delimiter concept
    // represent specific kinds of delimiters, such as single characters,
    // substrings, or even regular expressions.
    //
    // The following Delimiter objects are provided as part of the Split() API:
    //
    //   - Literal (default)
    //   - AnyOf
    //   - Limit
    //   - FixedLength
    //
    // The following are examples of using some provided Delimiter objects:
    //
    // Example 1:
    //   // Because a string literal is converted to a strings::delimiter::Literal,
    //   // the following two splits are equivalent.
    //   vector<string> v1 = strings::Split("a,b,c", ",");           // (1)
    //   using ::strings::delimiter::Literal;
    //   vector<string> v2 = strings::Split("a,b,c", Literal(","));  // (2)
    //
    // Example 2:
    //   // Splits on any of the characters specified in the delimiter string.
    //   using ::strings::delimiter::AnyOf;
    //   vector<string> v = strings::Split("a,b;c-d", AnyOf(",;-"));
    //   assert(v.size() == 4);
    //
    // Example 3:
    //   // Uses the Limit meta-delimiter to limit the number of matches a delimiter
    //   // can have. In this case, the delimiter of a Literal comma is limited to
    //   // to matching at most one time. The last element in the returned
    //   // collection will contain all unsplit pieces, which may contain instances
    //   // of the delimiter.
    //   using ::strings::delimiter::Limit;
    //   vector<string> v = strings::Split("a,b,c", Limit(",", 1));
    //   assert(v.size() == 2);  // Limited to 1 delimiter; so two elements found
    //   assert(v[0] == "a");
    //   assert(v[1] == "b,c");
    //
    // Example 4:
    //   // Splits into equal-length substrings.
    //   using ::strings::delimiter::FixedLength;
    //   vector<string> v = strings::Split("12345", FixedLength(2));
    //   assert(v.size() == 3);
    //   assert(v[0] == "12");
    //   assert(v[1] == "34");
    //   assert(v[2] == "5");
    //
    //                                  Predicates
    //
    // Predicates can filter the results of a Split() operation by determining
    // whether or not a resultant element is included in the result set. A predicate
    // may be passed as an *optional* third argument to the Split() function.
    //
    // Predicates are unary functions (or functors) that take a single StringPiece
    // argument and return bool indicating whether the argument should be included
    // (true) or excluded (false).
    //
    // One example where this is useful is when filtering out empty substrings. By
    // default, empty substrings may be returned by strings::Split(), which is
    // similar to the way split functions work in other programming languages. For
    // example:
    //
    //   // Empty strings *are* included in the returned collection.
    //   vector<string> v = strings::Split(",a,,b,", ",");
    //   assert(v.size() ==  5);  // v[0] == "", v[1] == "a", v[2] == "", ...
    //
    // These empty strings can be filtered out of the results by simply passing the
    // provided SkipEmpty predicate as the third argument to the Split() function.
    // SkipEmpty does not consider a string containing all whitespace to be empty.
    // For that behavior use the SkipWhitespace predicate. For example:
    //
    // Example 1:
    //   // Uses SkipEmpty to omit empty strings. Strings containing whitespace are
    //   // not empty and are therefore not skipped.
    //   using strings::SkipEmpty;
    //   vector<string> v = strings::Split(",a, ,b,", ",", SkipEmpty());
    //   assert(v.size() == 3);
    //   assert(v[0] == "a");
    //   assert(v[1] == " ");  // <-- The whitespace makes the string not empty.
    //   assert(v[2] == "b");
    //
    // Example 2:
    //   // Uses SkipWhitespace to skip all strings that are either empty or contain
    //   // only whitespace.
    //   using strings::SkipWhitespace;
    //   vector<string> v = strings::Split(",a, ,b,", ",",  SkipWhitespace());
    //   assert(v.size() == 2);
    //   assert(v[0] == "a");
    //   assert(v[1] == "b");
    //
    //
    //                     Differences between Split1 and Split2
    //
    // Split2 is the strings::Split() API described above. Split1 is a name for the
    // collection of legacy Split*() functions declared later in this file. Most of
    // the Split1 functions follow a set of conventions that don't necessarily match
    // the conventions used in Split2. The following are some of the important
    // differences between Split1 and Split2:
    //
    // Split1 -> Split2
    // ----------------
    // Append -> Assign:
    //   The Split1 functions all returned their output collections via a pointer to
    //   an out parameter as is typical in Google code. In some cases the comments
    //   explicitly stated that results would be *appended* to the output
    //   collection. In some cases it was ambiguous whether results were appended.
    //   This ambiguity is gone in the Split2 API as results are always assigned to
    //   the output collection, never appended.
    //
    // AnyOf -> Literal:
    //   Most Split1 functions treated their delimiter argument as a string of
    //   individual byte delimiters. For example, a delimiter of ",;" would split on
    //   "," and ";", not the substring ",;". This behavior is equivalent to the
    //   Split2 delimiter strings::delimiter::AnyOf, which is *not* the default. By
    //   default, strings::Split() splits using strings::delimiter::Literal() which
    //   would treat the whole string ",;" as a single delimiter string.
    //
    // SkipEmpty -> allow empty:
    //   Most Split1 functions omitted empty substrings in the results. To keep
    //   empty substrings one would have to use an explicitly named
    //   Split*AllowEmpty() function. This behavior is reversed in Split2. By
    //   default, strings::Split() *allows* empty substrings in the output. To skip
    //   them, use the strings::SkipEmpty predicate.
    //
    // string -> user's choice:
    //   Most Split1 functions return collections of string objects. Some return
    //   char*, but the type returned is dictated by each Split1 function. With
    //   Split2 the caller can choose which string-like object to return. (Note:
    //   char* C-strings are not supported in Split2--use StringPiece instead).
    //

    namespace delimiter {
        // A Delimiter object tells the splitter where a string should be broken. Some
        // examples are breaking a string wherever a given character or substring is
        // found, wherever a regular expression matches, or simply after a fixed length.
        // All Delimiter objects must have the following member:
        //
        //   StringPiece Find(StringPiece text);
        //
        // This Find() member function should return a StringPiece referring to the next
        // occurrence of the represented delimiter, which is the location where the
        // input string should be broken. The returned StringPiece may be zero-length if
        // the Delimiter does not represent a part of the string (e.g., a fixed-length
        // delimiter). If no delimiter is found in the given text, a zero-length
        // StringPiece referring to text.end() should be returned (e.g.,
        // StringPiece(text.end(), 0)). It is important that the returned StringPiece
        // always be within the bounds of the StringPiece given as an argument--it must
        // not refer to a string that is physically located outside of the given string.
        // The following example is a simple Delimiter object that is created with a
        // single char and will look for that char in the text given to the Find()
        // function:
        //
        //   struct SimpleDelimiter {
        //     const char c_;
        //     explicit SimpleDelimiter(char c) : c_(c) {}
        //     StringPiece Find(StringPiece text) {
        //       int pos = text.find(c_);
        //       if (pos == StringPiece::npos) return StringPiece(text.end(), 0);
        //       return StringPiece(text, pos, 1);
        //     }
        //   };

        // Represents a literal string delimiter. Examples:
        //
        //   using ::strings::delimiter::Literal;
        //   vector<string> v = strings::Split("a=>b=>c", Literal("=>"));
        //   assert(v.size() == 3);
        //   assert(v[0] == "a");
        //   assert(v[1] == "b");
        //   assert(v[2] == "c");
        //
        // The next example uses the empty string as a delimiter.
        //
        //   using ::strings::delimiter::Literal;
        //   vector<string> v = strings::Split("abc", Literal(""));
        //   assert(v.size() == 3);
        //   assert(v[0] == "a");
        //   assert(v[1] == "b");
        //   assert(v[2] == "c");
        //
        // This is the *default* delimiter used if a literal string or string-like
        // object is used where a Delimiter object is expected. For example, the
        // following calls are equivalent.
        //
        //   vector<string> v = strings::Split("a,b", ",");
        //   vector<string> v = strings::Split("a,b", strings::delimiter::Literal(","));
        //
        class Literal {
        public:
            explicit Literal(StringPiece sp);
            StringPiece Find(StringPiece text) const;

        private:
            const string delimiter_;
        };

        namespace internal {
            // A traits-like metafunction for selecting the default Delimiter object type
            // for a particular Delimiter type. The base case simply exposes type Delimiter
            // itself as the delimiter's Type. However, there are specializations for
            // string-like objects that map them to the Literal delimiter object. This
            // allows functions like strings::Split() and strings::delimiter::Limit() to
            // accept string-like objects (e.g., ",") as delimiter arguments but they will
            // be treated as if a Literal delimiter was given.
            template <typename Delimiter>
            struct SelectDelimiter {
                typedef Delimiter Type;
            };
            template <> struct SelectDelimiter<const char*> { typedef Literal Type; };
            template <> struct SelectDelimiter<StringPiece> { typedef Literal Type; };
            template <> struct SelectDelimiter<std::string> { typedef Literal Type; };
#if defined(HAS_GLOBAL_STRING)
            template <> struct SelectDelimiter<string> { typedef Literal Type; };
#endif
        }  // namespace internal

        // Represents a delimiter that will match any of the given byte-sized
        // characters. AnyOf is similar to Literal, except that AnyOf uses
        // StringPiece::find_first_of() and Literal uses StringPiece::find(). AnyOf
        // examples:
        //
        //   using ::strings::delimiter::AnyOf;
        //   vector<string> v = strings::Split("a,b=c", AnyOf(",="));
        //
        //   assert(v.size() == 3);
        //   assert(v[0] == "a");
        //   assert(v[1] == "b");
        //   assert(v[2] == "c");
        //
        // If AnyOf is given the empty string, it behaves exactly like Literal and
        // matches each individual character in the input string.
        //
        // Note: The string passed to AnyOf is assumed to be a string of single-byte
        // ASCII characters. AnyOf does not work with multi-byte characters.
        class AnyOf {
        public:
            explicit AnyOf(StringPiece sp);
            StringPiece Find(StringPiece text) const;

        private:
            const string delimiters_;
        };

        // A delimiter for splitting into equal-length strings. The length argument to
        // the constructor must be greater than 0. This delimiter works with ascii
        // string data, but does not work with variable-width encodings, such as UTF-8.
        // Examples:
        //
        //   using ::strings::delimiter::FixedLength;
        //   vector<string> v = strings::Split("123456789", FixedLength(3));
        //   assert(v.size() == 3);
        //   assert(v[0] == "123");
        //   assert(v[1] == "456");
        //   assert(v[2] == "789");
        //
        // Note that the string does not have to be a multiple of the fixed split
        // length. In such a case, the last substring will be shorter.
        //
        //   using ::strings::delimiter::FixedLength;
        //   vector<string> v = strings::Split("12345", FixedLength(2));
        //   assert(v.size() == 3);
        //   assert(v[0] == "12");
        //   assert(v[1] == "34");
        //   assert(v[2] == "5");
        //
        class FixedLength {
        public:
            explicit FixedLength(int length);
            StringPiece Find(StringPiece text) const;

        private:
            const int length_;
        };

        // Wraps another delimiter and sets a max number of matches for that delimiter.
        // Create LimitImpls using the Limit() function. Example:
        //
        //   using ::strings::delimiter::Limit;
        //   vector<string> v = strings::Split("a,b,c,d", Limit(",", 2));
        //
        //   assert(v.size() == 3);  // Split on 2 commas, giving a vector with 3 items
        //   assert(v[0] == "a");
        //   assert(v[1] == "b");
        //   assert(v[2] == "c,d");
        //
        template <typename Delimiter>
        class LimitImpl {
        public:
            LimitImpl(Delimiter delimiter, int limit)
                    : delimiter_(delimiter), limit_(limit), count_(0) {}
            StringPiece Find(StringPiece text) {
                if (count_++ == limit_) {
                    return StringPiece(text.end(), 0);  // No more matches.
                }
                return delimiter_.Find(text);
            }

        private:
            Delimiter delimiter_;
            const int limit_;
            int count_;
        };

        // Limit() function to create LimitImpl<> objects.
        template <typename Delimiter>
        inline LimitImpl<typename internal::SelectDelimiter<Delimiter>::Type>
        Limit(Delimiter delim, int limit) {
            typedef typename internal::SelectDelimiter<Delimiter>::Type DelimiterType;
            return LimitImpl<DelimiterType>(DelimiterType(delim), limit);
        }

    }  // namespace delimiter

    //
    // Predicates are functors that return bool indicating whether the given
    // StringPiece should be included in the split output. If the predicate returns
    // false then the string will be excluded from the output from strings::Split().
    //

    // Always returns true, indicating that all strings--including empty
    // strings--should be included in the split output. This predicate is not
    // strictly needed because this is the default behavior of the strings::Split()
    // function. But it might be useful at some call sites to make the intent
    // explicit.
    //
    // vector<string> v = Split(" a , ,,b,", ",", AllowEmpty());
    // EXPECT_THAT(v, ElementsAre(" a ", " ", "", "b", ""));
    struct AllowEmpty {
        bool operator()(StringPiece sp) const {
            return true;
        }
    };

    // Returns false if the given StringPiece is empty, indicating that the
    // strings::Split() API should omit the empty string.
    //
    // vector<string> v = Split(" a , ,,b,", ",", SkipEmpty());
    // EXPECT_THAT(v, ElementsAre(" a ", " ", "b"));
    struct SkipEmpty {
        bool operator()(StringPiece sp) const {
            return !sp.empty();
        }
    };

    // Returns false if the given StringPiece is empty or contains only whitespace,
    // indicating that the strings::Split() API should omit the string.
    //
    // vector<string> v = Split(" a , ,,b,", ",", SkipWhitespace());
    // EXPECT_THAT(v, ElementsAre(" a ", "b"));
    struct SkipWhitespace {
        bool operator()(StringPiece sp) const {
            StripWhiteSpace(&sp);
            return !sp.empty();
        }
    };

    // Definitions of the main Split() function. The use of SelectDelimiter<> allows
    // these functions to be called with a Delimiter template paramter that is an
    // actual Delimiter object (e.g., Literal or AnyOf), OR called with a
    // string-like delimiter argument, (e.g., ","), in which case the delimiter used
    // will default to strings::delimiter::Literal.
    template <typename Delimiter>
    inline internal::Splitter<
            typename delimiter::internal::SelectDelimiter<Delimiter>::Type>
    Split(StringPiece text, Delimiter d) {
        typedef typename delimiter::internal::SelectDelimiter<Delimiter>::Type
                DelimiterType;
        return internal::Splitter<DelimiterType>(text, DelimiterType(d));
    }

    template <typename Delimiter, typename Predicate>
    inline internal::Splitter<
            typename delimiter::internal::SelectDelimiter<Delimiter>::Type, Predicate>
    Split(StringPiece text, Delimiter d, Predicate p) {
        typedef typename delimiter::internal::SelectDelimiter<Delimiter>::Type
                DelimiterType;
        return internal::Splitter<DelimiterType, Predicate>(
                text, DelimiterType(d), p);
    }

}  // namespace strings

#endif  // STRINGS_SPLIT_H_
