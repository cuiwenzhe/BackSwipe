// Created by utf8tablebuilder version 2.95
// See util/utf8/public/utf8statetable.h for usage
//
//  Rejects all codes from file:
//    blaze-out/gcc-4.X.Y-crosstool-v18-hybrid-grtev4-k8-opt/genfiles/util/utf8/preprocess/unilib_src/is_fullwidth_an.txt
//  Accepts all other UTF-8 codes 0000..10FFFF
//  All bytes are checked for structurally valid UTF-8
//  Table entries are absolute statetable subscripts

#ifndef BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8SCANNOTFW_AN_H__
#define BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8SCANNOTFW_AN_H__

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

//  Entire table has 10 state blocks of 256 entries each

static const unsigned int utf8scannotfw_an_STATE0 = 0;		// state[0]
static const unsigned int utf8scannotfw_an_STATE0_SIZE = 256;	// =[1]
static const unsigned int utf8scannotfw_an_TOTAL_SIZE = 2560;
static const unsigned int utf8scannotfw_an_MAX_EXPAND_X4 = 0;
static const unsigned int utf8scannotfw_an_SHIFT = 8;
static const unsigned int utf8scannotfw_an_BYTES = 1;
static const unsigned int utf8scannotfw_an_LOSUB = 0x20202020;
static const unsigned int utf8scannotfw_an_HIADD = 0x00000000;

static const uint8 utf8scannotfw_an[] = {
        // state[0] 0x000000 Byte 1
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        2,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  7,
        4,  5,  5,  5,  6,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[1] 0x000080 Byte 2 of 2
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[2] 0x000000 Byte 2 of 3
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[3] 0x001000 Byte 2 of 3
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[4] 0x000000 Byte 2 of 4
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        3,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  3,
        3,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  3,
        3,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  3,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[5] 0x040000 Byte 2 of 4
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        3,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  3,
        3,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  3,
        3,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  3,
        3,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  3,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[6] 0x100000 Byte 2 of 4
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        3,  3,  3,  3,  3,  3,  3,  3,   3,  3,  3,  3,  3,  3,  3,  3,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[7] 0x00f000 Byte 2 of 3
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,   1,  1,  1,  1,  8,  9,  1,  1,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[8] 0x00ff00 Byte 3 of 3
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_, RJ_,RJ_,  0,  0,  0,  0,  0,  0,
        0,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_, RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,
        RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_, RJ_,RJ_,RJ_,  0,  0,  0,  0,  0,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        // state[9] 0x00ff40 Byte 3 of 3
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

        0,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_, RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,
        RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_,RJ_, RJ_,RJ_,RJ_,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,
        X__,X__,X__,X__,X__,X__,X__,X__, X__,X__,X__,X__,X__,X__,X__,X__,

};

// Remap base[0] = (del, add, string_offset)
static const RemapEntry utf8scannotfw_an_remap_base[] = {
        {0,0,0} };

// Remap string[0]
static const unsigned char utf8scannotfw_an_remap_string[] = {
        0 };

static const unsigned char utf8scannotfw_an_fast[256] = {
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

static const UTF8ScanObj utf8scannotfw_an_obj = {
        utf8scannotfw_an_STATE0,
        utf8scannotfw_an_STATE0_SIZE,
        utf8scannotfw_an_TOTAL_SIZE,
        utf8scannotfw_an_MAX_EXPAND_X4,
        utf8scannotfw_an_SHIFT,
        utf8scannotfw_an_BYTES,
        utf8scannotfw_an_LOSUB,
        utf8scannotfw_an_HIADD,
        utf8scannotfw_an,
        utf8scannotfw_an_remap_base,
        utf8scannotfw_an_remap_string,
        utf8scannotfw_an_fast
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

// Table has 2816 bytes, Hash = 0CB3-0776

#endif  // BLAZE_OUT_GCC_4_X_Y_CROSSTOOL_V18_HYBRID_GRTEV4_K8_OPT_GENFILES_UTIL_UTF8_PREPROCESS_UNILIB_SRC_UTF8SCANNOTFW_AN_H__
