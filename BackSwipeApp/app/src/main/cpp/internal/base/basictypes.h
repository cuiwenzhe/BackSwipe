// Baisc type definitions for the keyboard decoder.

#ifndef INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_BASICTYPES_H_
#define INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_BASICTYPES_H_

#include "stringpiece.h"

namespace keyboard {
namespace decoder {

    ///////////////////////// Unicode representations //////////////////////////

    // The following typedefs used to emphasize that a string (or string-like
    // object) stores Unicode encoded in UTF-8.
    typedef std::string Utf8String;
    typedef const char* Utf8ConstCString;
    typedef StringPiece Utf8StringPiece;

    /////////////////////// Probability representations ////////////////////////

    // Used to emphasize that a float represents a log probability.
    typedef float LogProbFloat;

}  // namespace decoder
}  // namespace keyboard

#endif  // INPUTMETHOD_KEYBOARD_DECODER_PUBLIC_BASICTYPES_H_
