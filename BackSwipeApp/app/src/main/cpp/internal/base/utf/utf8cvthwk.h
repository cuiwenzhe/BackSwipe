// Created by utf8tablebuilder version 2.95
// See util/utf8/public/utf8statetable.h for usage
// 
//  Replaces all codes from file:
//    util/utf8/preprocess/proptables/google_halfwidth_katakana.txt
//  Accepts all other UTF-8 codes 0000..10FFFF
//  Exit optimized -- exits after four times in state 0
//  Space optimized
//
// ** ASSUMES INPUT IS STRUCTURALLY VALID UTF-8 **
//
//  Table entries are absolute statetable subscripts

#ifndef BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8CVTHWK_H__
#define BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8CVTHWK_H__

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

//  Entire table has 29 state blocks of 64 entries each

static const unsigned int utf8cvthwk_STATE0 = 0;		// state[0]
static const unsigned int utf8cvthwk_STATE0_SIZE = 832;	// =[13]
static const unsigned int utf8cvthwk_TOTAL_SIZE = 1856;
static const unsigned int utf8cvthwk_MAX_EXPAND_X4 = 12;
static const unsigned int utf8cvthwk_SHIFT = 6;
static const unsigned int utf8cvthwk_BYTES = 1;
static const unsigned int utf8cvthwk_LOSUB = 0x20202020;
static const unsigned int utf8cvthwk_HIADD = 0x00000000;

static const uint8 utf8cvthwk[] = {
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
        15, 16, 16, 20, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
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
        15, 16, 16, 20, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
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
        15, 16, 16, 20, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
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
        15, 16, 16, 20, 16, 16, 16, 16,  16, 16, 16, 16, 16, 16, 16, 16,
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

        // state[20 + 2] 0x003000 Byte 2 of 3
        21, 14, 23, 25, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,  14, 14, 14, 14, 14, 14, 14, 14,

        // state[21 + 2] 0x003000 Byte 3 of 3
        0,T1_,T1_,  0,  0,  0,  0,  0,   0,  0,  0,  0,T1_,T1_,T1_,T1_,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

        0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x02,0x03,0x02,0x03,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

        // state[23 + 2] 0x003080 Byte 3 of 3
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,T1_,T1_,T1_,T1_,  0,  0,  0,
        0,T1_,T1_,T1_,T1_,T1_,T1_,T1_, T1_,T1_,T1_,T1_,T1_,T1_,T1_,T1_,
        T1_,T1_,T1_,T1_,T1_,T1_,T1_,T1_, T1_,T1_,T1_,T1_,T1_,T1_,T1_,T1_,

        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x04,0x05,0x04,0x05,0x00,0x00,0x00,
        0x00,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c, 0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,
        0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c, 0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,

        // state[25 + 2] 0x0030c0 Byte 3 of 3
        T1_,T1_,T1_,T1_,T1_,T1_,T1_,T1_, T1_,T1_,T1_,T1_,T1_,T1_,T1_,T1_,
        T1_,T1_,T1_,T1_,T1_,T1_,T1_,T1_, T1_,T1_,T1_,T1_,T1_,T1_,T1_,T1_,
        T1_,T1_,T1_,T1_,T1_,T1_,T1_,T1_, T1_,T1_,T1_,T1_,T1_,T1_,  0,T1_,
        0,  0,T1_,T1_,T1_,  0,  0,T1_,   0,  0,T1_,T1_,T1_,  0,  0,  0,

        0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c, 0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,
        0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c, 0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,
        0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c, 0x4d,0x4e,0x4f,0x50,0x51,0x52,0x00,0x53,
        0x00,0x00,0x54,0x55,0x56,0x00,0x00,0x57, 0x00,0x00,0x58,0x59,0x5a,0x00,0x00,0x00,

};

// Remap base[91] = (del, add, string_offset)
static const RemapEntry utf8cvthwk_remap_base[] = {
        {3,3,   0}, {3,3,   3}, {3,3,   6}, {3,3,   9},
        {3,3,  12}, {3,3,  15}, {3,3,  18}, {3,3,  21},
        {3,3,  24}, {3,3,  27}, {3,3,  30}, {3,3,  33},
        {3,3,  36}, {3,3,  39}, {3,3,  42}, {3,3,  45},

        {3,3,  48}, {3,6,  51}, {3,3,  57}, {3,6,  60},
        {3,3,  66}, {3,6,  69}, {3,3,  75}, {3,6,  78},
        {3,3,  84}, {3,6,  87}, {3,3,  93}, {3,6,  96},
        {3,3, 102}, {3,6, 105}, {3,3, 111}, {3,6, 114},

        {3,3, 120}, {3,6, 123}, {3,3, 129}, {3,6, 132},
        {3,3, 138}, {3,6, 141}, {3,3, 147}, {3,6, 150},
        {3,3, 156}, {3,3, 159}, {3,6, 162}, {3,3, 168},
        {3,6, 171}, {3,3, 177}, {3,6, 180}, {3,3, 186},

        {3,3, 189}, {3,3, 192}, {3,3, 195}, {3,3, 198},
        {3,3, 201}, {3,6, 204}, {3,6, 210}, {3,3, 216},
        {3,6, 219}, {3,6, 225}, {3,3, 231}, {3,6, 234},
        {3,6, 240}, {3,3, 246}, {3,6, 249}, {3,6, 255},


        {3,3, 261}, {3,6, 264}, {3,6, 270}, {3,3, 276},
        {3,3, 279}, {3,3, 282}, {3,3, 285}, {3,3, 288},
        {3,3, 291}, {3,3, 294}, {3,3, 297}, {3,3, 300},
        {3,3, 303}, {3,3, 306}, {3,3, 309}, {3,3, 312},

        {3,3, 315}, {3,3, 318}, {3,3, 321}, {3,3, 324},
        {3,3, 327}, {3,3, 330}, {3,6, 333}, {3,6, 339},
        {3,6, 345}, {3,3, 351}, {3,3, 354}, {0,0,0} };

// Remap string[357]
static const unsigned char utf8cvthwk_remap_string[] = {
        0xef,0xbd,0xa4,0xef,0xbd,0xa1,0xef,0xbd, 0xa2,0xef,0xbd,0xa3,0xef,0xbe,0x9e,0xef,
        0xbe,0x9f,0xef,0xbd,0xa7,0xef,0xbd,0xb1, 0xef,0xbd,0xa8,0xef,0xbd,0xb2,0xef,0xbd,
        0xa9,0xef,0xbd,0xb3,0xef,0xbd,0xaa,0xef, 0xbd,0xb4,0xef,0xbd,0xab,0xef,0xbd,0xb5,
        0xef,0xbd,0xb6,0xef,0xbd,0xb6,0xef,0xbe, 0x9e,0xef,0xbd,0xb7,0xef,0xbd,0xb7,0xef,

        0xbe,0x9e,0xef,0xbd,0xb8,0xef,0xbd,0xb8, 0xef,0xbe,0x9e,0xef,0xbd,0xb9,0xef,0xbd,
        0xb9,0xef,0xbe,0x9e,0xef,0xbd,0xba,0xef, 0xbd,0xba,0xef,0xbe,0x9e,0xef,0xbd,0xbb,
        0xef,0xbd,0xbb,0xef,0xbe,0x9e,0xef,0xbd, 0xbc,0xef,0xbd,0xbc,0xef,0xbe,0x9e,0xef,
        0xbd,0xbd,0xef,0xbd,0xbd,0xef,0xbe,0x9e, 0xef,0xbd,0xbe,0xef,0xbd,0xbe,0xef,0xbe,

        0x9e,0xef,0xbd,0xbf,0xef,0xbd,0xbf,0xef, 0xbe,0x9e,0xef,0xbe,0x80,0xef,0xbe,0x80,
        0xef,0xbe,0x9e,0xef,0xbe,0x81,0xef,0xbe, 0x81,0xef,0xbe,0x9e,0xef,0xbd,0xaf,0xef,
        0xbe,0x82,0xef,0xbe,0x82,0xef,0xbe,0x9e, 0xef,0xbe,0x83,0xef,0xbe,0x83,0xef,0xbe,
        0x9e,0xef,0xbe,0x84,0xef,0xbe,0x84,0xef, 0xbe,0x9e,0xef,0xbe,0x85,0xef,0xbe,0x86,

        0xef,0xbe,0x87,0xef,0xbe,0x88,0xef,0xbe, 0x89,0xef,0xbe,0x8a,0xef,0xbe,0x8a,0xef,
        0xbe,0x9e,0xef,0xbe,0x8a,0xef,0xbe,0x9f, 0xef,0xbe,0x8b,0xef,0xbe,0x8b,0xef,0xbe,
        0x9e,0xef,0xbe,0x8b,0xef,0xbe,0x9f,0xef, 0xbe,0x8c,0xef,0xbe,0x8c,0xef,0xbe,0x9e,
        0xef,0xbe,0x8c,0xef,0xbe,0x9f,0xef,0xbe, 0x8d,0xef,0xbe,0x8d,0xef,0xbe,0x9e,0xef,

        0xbe,0x8d,0xef,0xbe,0x9f,0xef,0xbe,0x8e, 0xef,0xbe,0x8e,0xef,0xbe,0x9e,0xef,0xbe,
        0x8e,0xef,0xbe,0x9f,0xef,0xbe,0x8f,0xef, 0xbe,0x90,0xef,0xbe,0x91,0xef,0xbe,0x92,
        0xef,0xbe,0x93,0xef,0xbd,0xac,0xef,0xbe, 0x94,0xef,0xbd,0xad,0xef,0xbe,0x95,0xef,
        0xbd,0xae,0xef,0xbe,0x96,0xef,0xbe,0x97, 0xef,0xbe,0x98,0xef,0xbe,0x99,0xef,0xbe,

        0x9a,0xef,0xbe,0x9b,0xef,0xbe,0x9c,0xef, 0xbd,0xa6,0xef,0xbe,0x9d,0xef,0xbd,0xb3,
        0xef,0xbe,0x9e,0xef,0xbe,0x9c,0xef,0xbe, 0x9e,0xef,0xbd,0xa6,0xef,0xbe,0x9e,0xef,
        0xbd,0xa5,0xef,0xbd,0xb0,0 };

static const unsigned char utf8cvthwk_fast[256] = {
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

static const UTF8ReplaceObj utf8cvthwk_obj = {
        utf8cvthwk_STATE0,
        utf8cvthwk_STATE0_SIZE,
        utf8cvthwk_TOTAL_SIZE,
        utf8cvthwk_MAX_EXPAND_X4,
        utf8cvthwk_SHIFT,
        utf8cvthwk_BYTES,
        utf8cvthwk_LOSUB,
        utf8cvthwk_HIADD,
        utf8cvthwk,
        utf8cvthwk_remap_base,
        utf8cvthwk_remap_string,
        utf8cvthwk_fast
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

// Table has 2833 bytes, Hash = BB9B-76EE

#endif  // BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8CVTHWK_H__
