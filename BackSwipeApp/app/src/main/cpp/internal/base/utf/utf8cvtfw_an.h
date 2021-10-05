// Created by utf8tablebuilder version 2.95
// See util/utf8/public/utf8statetable.h for usage
// 
//  Replaces all codes from file:
//    blaze-out/gcc-4.X.Y-crosstool-v18-hybrid-grtev4-k8-opt/genfiles/util/utf8/preprocess/unilib_src/is_fullwidth_an.txt
//  Accepts all other UTF-8 codes 0000..10FFFF
//  Exit optimized -- exits after four times in state 0
//  Space optimized
//
// ** ASSUMES INPUT IS STRUCTURALLY VALID UTF-8 **
//
//  Table entries are absolute statetable subscripts

#ifndef BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8CVTFW_AN_H__
#define BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8CVTFW_AN_H__

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

//  Entire table has 27 state blocks of 64 entries each

static const unsigned int utf8cvtfw_an_STATE0 = 0;		// state[0]
static const unsigned int utf8cvtfw_an_STATE0_SIZE = 832;	// =[13]
static const unsigned int utf8cvtfw_an_TOTAL_SIZE = 1728;
static const unsigned int utf8cvtfw_an_MAX_EXPAND_X4 = 12;
static const unsigned int utf8cvtfw_an_SHIFT = 6;
static const unsigned int utf8cvtfw_an_BYTES = 1;
static const unsigned int utf8cvtfw_an_LOSUB = 0x20202020;
static const unsigned int utf8cvtfw_an_HIADD = 0x00000000;

static const uint8 utf8cvtfw_an[] = {
        // state[0] 0x000000 Byte 1
        4,  4,  4,  4,  4,  4,  4,  4,   4,  4,  4,  4,  4,  4,  4,  4,
        4,  4,  4,  4,  4,  4,  4,  4,   4,  4,  4,  4,  4,  4,  4,  4,
        4,  4,  4,  4,  4,  4,  4,  4,   4,  4,  4,  4,  4,  4,  4,  4,
        4,  4,  4,  4,  4,  4,  4,  4,   4,  4,  4,  4,  4,  4,  4,  4,

        4,  4,  4,  4,  4,  4,  4,  4,   4,  4,  4,  4,  4,  4,  4,  4,
        4,  4,  4,  4,  4,  4,  4,  4,   4,  4,  4,  4,  4,  4,  4,  4,
        4,  4,  4,  4,  4,  4,  4,  4,   4,  4,  4,  4,  4,  4,  4,  4,
        4,  4,  4,  4,  4,  4,  4,  4,   4,  4,  4,  4,  4,  4,  4,  4,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        15, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 20,
        17, 18, 18, 18, 19,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[4] 0x000000 Byte 1
        8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,

        8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
        8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        15, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 20,
        17, 18, 18, 18, 19,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[8] 0x000000 Byte 1
        12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12,

        12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,  12, 12, 12, 12, 12, 12, 12, 12,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        15, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 20,
        17, 18, 18, 18, 19,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[12] 0x000000 Byte 1
        D__,D__,D__,D__,D__,D__,D__,D__, D__,D__,D__,D__,D__,D__,D__,D__,
        D__,D__,D__,D__,D__,D__,D__,D__, D__,D__,D__,D__,D__,D__,D__,D__,
        D__,D__,D__,D__,D__,D__,D__,D__, D__,D__,D__,D__,D__,D__,D__,D__,
        D__,D__,D__,D__,D__,D__,D__,D__, D__,D__,D__,D__,D__,D__,D__,D__,

        D__,D__,D__,D__,D__,D__,D__,D__, D__,D__,D__,D__,D__,D__,D__,D__,
        D__,D__,D__,D__,D__,D__,D__,D__, D__,D__,D__,D__,D__,D__,D__,D__,
        D__,D__,D__,D__,D__,D__,D__,D__, D__,D__,D__,D__,D__,D__,D__,D__,
        D__,D__,D__,D__,D__,D__,D__,D__, D__,D__,D__,D__,D__,D__,D__,D__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        15, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 20,
        17, 18, 18, 18, 19,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[14 + 2] 0x000080 Byte 2 of 2
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

        // state[15 + 2] 0x000000 Byte 2 of 3
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,

        // state[16 + 2] 0x001000 Byte 2 of 3
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,

        // state[17 + 2] 0x000000 Byte 2 of 4
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,

        // state[18 + 2] 0x040000 Byte 2 of 4
        16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,

        // state[19 + 2] 0x100000 Byte 2 of 4
        16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[20 + 2] 0x00f000 Byte 2 of 3
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 21, 23, 14, 14,

        // state[21 + 2] 0x00ff00 Byte 3 of 3
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        S31,S31,S31,S31,S31,S31,S31,S31, S31,S31,  0,  0,  0,  0,  0,  0,
        0,S31,S31,S31,S31,S31,S31,S31, S31,S31,S31,S31,S31,S31,S31,S31,
        S31,S31,S31,S31,S31,S31,S31,S31, S31,S31,S31,  0,  0,  0,  0,  0,

        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37, 0x38,0x39,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x41,0x42,0x43,0x44,0x45,0x46,0x47, 0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
        0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57, 0x58,0x59,0x5a,0x00,0x00,0x00,0x00,0x00,

        // state[23 + 2] 0x00ff40 Byte 3 of 3
        0,S31,S31,S31,S31,S31,S31,S31, S31,S31,S31,S31,S31,S31,S31,S31,
        S31,S31,S31,S31,S31,S31,S31,S31, S31,S31,S31,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

        0x00,0x61,0x62,0x63,0x64,0x65,0x66,0x67, 0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77, 0x78,0x79,0x7a,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

};

// Remap base[0] = (del, add, string_offset)
static const RemapEntry utf8cvtfw_an_remap_base[] = {
        {0,0,0} };

// Remap string[0]
static const unsigned char utf8cvtfw_an_remap_string[] = {
        0 };

static const unsigned char utf8cvtfw_an_fast[256] = {
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,

};

static const UTF8ReplaceObj utf8cvtfw_an_obj = {
        utf8cvtfw_an_STATE0,
        utf8cvtfw_an_STATE0_SIZE,
        utf8cvtfw_an_TOTAL_SIZE,
        utf8cvtfw_an_MAX_EXPAND_X4,
        utf8cvtfw_an_SHIFT,
        utf8cvtfw_an_BYTES,
        utf8cvtfw_an_LOSUB,
        utf8cvtfw_an_HIADD,
        utf8cvtfw_an,
        utf8cvtfw_an_remap_base,
        utf8cvtfw_an_remap_string,
        utf8cvtfw_an_fast
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

// Table has 1984 bytes, Hash = F6E0-C58E

#endif  // BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8CVTFW_AN_H__
