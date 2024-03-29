/**
 * Copyright 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// Printf variants that place their output in a C++ string.
//
// Usage:
//      string result = StringPrintf("%d %s\n", 10, "hello");
//      SStringPrintf(&result, "%d %s\n", 10, "hello");
//      StringAppendF(&result, "%d %s\n", 20, "there");
// Reference:
// https://github.com/google/supersonic/blob/master/supersonic/utils/stringprintf.h

#ifndef BASE_STRINGPRINTF_H_
#define BASE_STRINGPRINTF_H_

#include <stdarg.h>
#include <string>
#include <vector>
#include "port.h"

namespace base {
    // Return a C++ string
    extern string StringPrintf(const char* format, ...)
    // Tell the compiler to do printf format string checking.
    PRINTF_ATTRIBUTE(1,2);

    // Store result into a supplied string and return it
    extern const string& SStringPrintf(string* dst, const char* format, ...)
    // Tell the compiler to do printf format string checking.
    PRINTF_ATTRIBUTE(2,3);

    // Append result to a supplied string
    extern void StringAppendF(string* dst, const char* format, ...)
    // Tell the compiler to do printf format string checking.
    PRINTF_ATTRIBUTE(2,3);

    // Lower-level routine that takes a va_list and appends to a specified
    // string.  All other routines are just convenience wrappers around it.
    extern void StringAppendV(string* dst, const char* format, va_list ap);

    // The max arguments supported by StringPrintfVector
    extern const int kStringPrintfVectorMaxArgs;

    // You can use this version when all your arguments are strings, but
    // you don't know how many arguments you'll have at compile time.
    // StringPrintfVector will LOG(FATAL) if v.size() > kStringPrintfVectorMaxArgs
    extern string StringPrintfVector(
            const char* format, const std::vector<string>& v);
}  // namespace base

#endif // BASE_STRINGPRINTF_H_