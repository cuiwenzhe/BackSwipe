// Copyright 2006 Google Inc.
//
// Hand-edited by dsites on 2006-09-14 10:01:13
// See util/utf8/public/utf8statetable.h for usage
//
//  Rejects all codes from file:
//    util/utf8/unilib/is_nointerchange7bit.txt
//  Rejects all non-seven-bit ASCII

#ifndef UTIL_UTF8_UNILIB_SRC_UTF8ACCEPTINTERCHANGE7BIT_H_
#define UTIL_UTF8_UNILIB_SRC_UTF8ACCEPTINTERCHANGE7BIT_H_

#include "../integral_types.h"
#include "../utf8statetable.h"

#define X__ (kExitIllegalStructure)
#define RJ_ (kExitReject)
#define S1_ (kExitReplace1)
#define S2_ (kExitReplace2)
#define S3_ (kExitReplace3)
#define S21 (kExitReplace21)
#define S31 (kExitReplace31)
#define S32 (kExitReplace32)
#define T1_ (kExitReplaceOffset1)
#define T2_ (kExitReplaceOffset2)
#define S11 (kExitReplace1S0)
#define SP_ (kExitSpecial)
#define D__ (kExitDoAgain)
#define RJA (kExitRejectAlt)

//  Entire table has 1 state block of 256 entries each

static const unsigned int utf8acceptinterchange7bit_STATE0 = 0;		// state[0]
static const unsigned int utf8acceptinterchange7bit_STATE0_SIZE = 256;	// =[1]
static const unsigned int utf8acceptinterchange7bit_TOTAL_SIZE = 256;
static const unsigned int utf8acceptinterchange7bit_MAX_EXPAND_X4 = 0;
static const unsigned int utf8acceptinterchange7bit_SHIFT = 8;
static const unsigned int utf8acceptinterchange7bit_BYTES = 1;
static const unsigned int utf8acceptinterchange7bit_LOSUB = 0x20202020;
static const unsigned int utf8acceptinterchange7bit_HIADD = 0x01010101;

static const uint8 utf8acceptinterchange7bit[] = {
        // state[0] 0x000000 Byte 1
        RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_, RJ_,0  ,0  ,RJ_,0  ,0  ,RJ_,RJ_,
        RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_, RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,RJ_,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

};

static const unsigned char utf8acceptinterchange7bit_fast[256] = {
        1,1,1,1,1,1,1,1, 1,0,0,1,0,0,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,

        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

};


static const UTF8ScanObj utf8acceptinterchange7bit_obj = {
        utf8acceptinterchange7bit_STATE0,
        utf8acceptinterchange7bit_STATE0_SIZE,
        utf8acceptinterchange7bit_TOTAL_SIZE,
        utf8acceptinterchange7bit_MAX_EXPAND_X4,
        utf8acceptinterchange7bit_SHIFT,
        utf8acceptinterchange7bit_BYTES,
        utf8acceptinterchange7bit_LOSUB,
        utf8acceptinterchange7bit_HIADD,
        utf8acceptinterchange7bit,
        nullptr,
        nullptr,
        utf8acceptinterchange7bit_fast
};


#undef X__
#undef RJ_
#undef S1_
#undef S2_
#undef S3_
#undef S21
#undef S31
#undef S32
#undef T1_
#undef T2_
#undef S11
#undef SP_
#undef D__
#undef RJA

// Table has 512 bytes, Hash = xxxx-xxxx

#endif  // UTIL_UTF8_UNILIB_SRC_UTF8ACCEPTINTERCHANGE7BIT_H_
