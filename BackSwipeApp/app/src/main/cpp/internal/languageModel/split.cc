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

#include "split.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <iterator>
using std::back_insert_iterator;
using std::iterator_traits;
#include <limits>
using std::numeric_limits;

#include "../base/integral_types.h"
#include "../base/logging.h"
#include "../base/macros.h"
#include "../base/strtoint.h"
#include "../base/ascii_ctype.h"
#include "../base/util.h"
#include "../base/hash.h"

// Implementations for some of the Split2 API. Much of the Split2 API is
// templated so it exists in header files, either strings/split.h or
// strings/split_internal.h.
namespace strings {
namespace delimiter {
    namespace {

        // This GenericFind() template function encapsulates the finding algorithm
        // shared between the Literal and AnyOf delimiters. The FindPolicy template
        // parameter allows each delimiter to customize the actual find function to use
        // and the length of the found delimiter. For example, the Literal delimiter
        // will ultimately use StringPiece::find(), and the AnyOf delimiter will use
        // StringPiece::find_first_of().
        template <typename FindPolicy>
        StringPiece GenericFind(
                StringPiece text,
                StringPiece delimiter,
                FindPolicy find_policy) {
            if (delimiter.empty() && text.length() > 0) {
                // Special case for empty string delimiters: always return a zero-length
                // StringPiece referring to the item at position 1.
                return StringPiece(text.begin() + 1, 0);
            }
            int found_pos = StringPiece::npos;
            StringPiece found(text.end(), 0);  // By default, not found
            found_pos = find_policy.Find(text, delimiter);
            if (found_pos != StringPiece::npos) {
                found.set(text.data() + found_pos, find_policy.Length(delimiter));
            }
            return found;
        }

        // Finds using StringPiece::find(), therefore the length of the found delimiter
        // is delimiter.length().
        struct LiteralPolicy {
            int Find(StringPiece text, StringPiece delimiter) {
                return text.find(delimiter);
            }
            int Length(StringPiece delimiter) {
                return delimiter.length();
            }
        };

        // Finds using StringPiece::find_first_of(), therefore the length of the found
        // delimiter is 1.
        struct AnyOfPolicy {
            int Find(StringPiece text, StringPiece delimiter) {
                return text.find_first_of(delimiter);
            }
            int Length(StringPiece delimiter) {
                return 1;
            }
        };

    }  // namespace

        //
        // Literal
        //
        Literal::Literal(StringPiece sp) : delimiter_(sp.ToString()) {
        }

        StringPiece Literal::Find(StringPiece text) const {
            return GenericFind(text, delimiter_, LiteralPolicy());
        }

        //
        // AnyOf
        //
        AnyOf::AnyOf(StringPiece sp) : delimiters_(sp.ToString()) {
        }

        StringPiece AnyOf::Find(StringPiece text) const {
            return GenericFind(text, delimiters_, AnyOfPolicy());
        }

        //
        // FixedLength
        //
        FixedLength::FixedLength(int length) : length_(length) {
            CHECK_GT(length, 0);
        }

        StringPiece FixedLength::Find(StringPiece text) const {
            // If the string is shorter than the chunk size we say we
            // "can't find the delimiter" so this will be the last chunk.
            if (text.length() <= length_)
                return StringPiece(text.end(), 0);

            return StringPiece(text.begin() + length_, 0);
        }

}  // namespace delimiter
}  // namespace strings
