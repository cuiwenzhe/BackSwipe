// Copyright 2005 Google Inc.
// All Rights Reserved.
//
// Author: Thomas Marsden <marsden@google.com>
// Owner: Thorsten Brants <brants@google.com>
//
// This file provides quantizers for storing floating point numbers (float)
// from a given range as integers with given numbers of bits.

#ifndef NLP_COMMON_PUBLIC_QUANTIZER_H__
#define NLP_COMMON_PUBLIC_QUANTIZER_H__

#include <stdint.h>

//#include "base/basictypes.h"
#include "integral_types.h"

// Quantizer
//
// Virtual base class for quantizers
// A quantizer provides functions for encoding floating point numbers that
// are in the range [0 .. some_maximum] as integers with given numbers
// of bits, and for converting back the integers into floats.
// Values smaller or larger than the defined range are encoded as 0 or the
// maximum, respectively.
class Quantizer {
public:
    // Constructor for an empty quantizer.
    // It is the user's responsibility to call 'init()' before use. Using
    // an un-initialized quantizer has undefined results.
    Quantizer();

    // Constructor with initialization:
    // 'max': maximum value to encode (minimum is 0)
    // 'nbits': number of bits to use for encoding
    Quantizer(float max, int nbits);

    virtual ~Quantizer();

    // Initialize the quantizer to use 'max' as maximum value and encode
    // values with 'nbits' bits. May be called repeatedly to change settings.
    virtual void init(float max, int nbits);

    // Check whether the quantizer has been initialized with valid values
    // (either by providing parameters to the constructor or calling 'init()'
    // directly)
    //virtual bool is_initialized() const;

    // Accessors the the quantizer's parameters
    virtual float max() const;
    virtual int nbits() const;

    // Pure virtual methods to be implemented

    // Encode a float using the number of bits provided during initialization
    // Special cases:
    // - Encode() of a value >= 'max': returns the largest quantized value
    //   (the user may compare with Quantizer::max() to do something else)
    // - Encode() of a value < 0: returns 0
    virtual uint32 Encode(float f) const = 0;

    // Decode a uint32 (which uses the given number of bits) back into a float
    // Special case:
    // - Decode() of a value >= 2^'nbits': undefined
    virtual float Decode(uint32 i) const = 0;

protected:
    float max_;  // The max value for the quantizer
    int nbits_;  // The number of nbits this quantizer uses
};

// EqualSizeBinQuantizer
//
// Allows the use of 1 to 32 bits to store float values.
// The encoded value 0 represents 0.0, the encoded value 2^nbits-1 represents
// the given maximum value, and inbetween 2^nbits-3 values are chosen
// that evenly cover the range.
//
// Example:
//   max = 0.3
//   nbits = 2
// Decodes the following values:
//   0 --> 0.0
//   1 --> 0.1
//   2 --> 0.2
//   3 --> 0.3
// Encodes the following ranges:
//   (-inf, 0.05) --> 0
//   [0.05, 0.15) --> 1
//   [0.15, 0.25) --> 2
//   [0.25, +inf) --> 3
// The boundaries between the different quantized values may be slightly
// different because of floating point inaccuracies
class EqualSizeBinQuantizer : public Quantizer {
public:
    // All methods inherited from 'Quantizer'.
    // Please consult documentation above.
    EqualSizeBinQuantizer();
    EqualSizeBinQuantizer(float max, int nbits);
    virtual void init(float max, int nbits);
    virtual uint32  Encode(float f) const;
    virtual float Decode(uint32 i) const;

private:
    uint32 max_encoded_;    // maximum encoded integer value (2^nbits - 1)
    float encoding_const_;
};

#endif  // NLP_COMMON_PUBLIC_QUANTIZER_H__