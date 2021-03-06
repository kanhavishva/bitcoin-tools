/***********************************************************************************************************
* Pollard Kangaroo
* Compile with:
* gcc -O2 -I secp256k1/src/ -I secp256k1/ kangaroo.c bloom.c -lpthread -lm -lgmp -o kangaroo
* gcc -O3 -I secp256k1/src/ -I secp256k1/ kangaroo.c bloom.c -lpthread -lm -lgmp -o kangaroo -DFLAG_PROFILE=1 -DBLOOM_SIZE=32 -DCMPLEN=22
***********************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#include "libsecp256k1-config.h"
#include "include/secp256k1.h"
#include "secp256k1.c"
#include "bloom.h"


#define KANGAROO_VERSION "0.13"

#ifndef BLOOM_SIZE
#define BLOOM_SIZE 32 //  2* 4GB = 8GB
//#define BLOOM_SIZE 33 // 2* 8GB = 16GB
//#define BLOOM_SIZE 34 // 2*16GB = 32GB
//#define BLOOM_SIZE 35 // 2*32GB = 64GB
#endif

#ifndef CMPLEN
#define CMPLEN 20
#endif

#ifndef FLAG_PROFILE
#define FLAG_PROFILE 1 // "byPollard" // best, expected 2w^(1/2)/cores jumps
//#define FLAG_PROFILE 2 // "NaiveSplitRange" // norm, expected 2(w/cores)^(1/2) jumps
//#define FLAG_PROFILE 3 // "byOorschot&Wiener" // possibly bad implementation, low efficiency
//#define FLAG_PROFILE 4 // "stupid_random" // test for compare, not recommended
#endif

#define NUMPUBKEYS 160
const unsigned char rawpubkeys[NUMPUBKEYS][33] = {
{0x02,0x79,0xbe,0x66,0x7e,0xf9,0xdc,0xbb,0xac,0x55,0xa0,0x62,0x95,0xce,0x87,0x0b,0x07,0x02,0x9b,0xfc,0xdb,0x2d,0xce,0x28,0xd9,0x59,0xf2,0x81,0x5b,0x16,0xf8,0x17,0x98},//== #1
{0x02,0xf9,0x30,0x8a,0x01,0x92,0x58,0xc3,0x10,0x49,0x34,0x4f,0x85,0xf8,0x9d,0x52,0x29,0xb5,0x31,0xc8,0x45,0x83,0x6f,0x99,0xb0,0x86,0x01,0xf1,0x13,0xbc,0xe0,0x36,0xf9},//== #2
{0x02,0x5c,0xbd,0xf0,0x64,0x6e,0x5d,0xb4,0xea,0xa3,0x98,0xf3,0x65,0xf2,0xea,0x7a,0x0e,0x3d,0x41,0x9b,0x7e,0x03,0x30,0xe3,0x9c,0xe9,0x2b,0xdd,0xed,0xca,0xc4,0xf9,0xbc},//== #3
{0x02,0x2f,0x01,0xe5,0xe1,0x5c,0xca,0x35,0x1d,0xaf,0xf3,0x84,0x3f,0xb7,0x0f,0x3c,0x2f,0x0a,0x1b,0xdd,0x05,0xe5,0xaf,0x88,0x8a,0x67,0x78,0x4e,0xf3,0xe1,0x0a,0x2a,0x01},//== #4
{0x02,0x35,0x2b,0xbf,0x4a,0x4c,0xdd,0x12,0x56,0x4f,0x93,0xfa,0x33,0x2c,0xe3,0x33,0x30,0x1d,0x9a,0xd4,0x02,0x71,0xf8,0x10,0x71,0x81,0x34,0x0a,0xef,0x25,0xbe,0x59,0xd5},//== #5
{0x03,0xf2,0xda,0xc9,0x91,0xcc,0x4c,0xe4,0xb9,0xea,0x44,0x88,0x7e,0x5c,0x7c,0x0b,0xce,0x58,0xc8,0x00,0x74,0xab,0x9d,0x4d,0xba,0xeb,0x28,0x53,0x1b,0x77,0x39,0xf5,0x30},//== #6
{0x02,0x96,0x51,0x6a,0x8f,0x65,0x77,0x42,0x75,0x27,0x8d,0x0d,0x74,0x20,0xa8,0x8d,0xf0,0xac,0x44,0xbd,0x64,0xc7,0xba,0xe0,0x7c,0x3f,0xe3,0x97,0xc5,0xb3,0x30,0x0b,0x23},//== #7
{0x03,0x08,0xbc,0x89,0xc2,0xf9,0x19,0xed,0x15,0x88,0x85,0xc3,0x56,0x00,0x84,0x4d,0x49,0x89,0x09,0x05,0xc7,0x9b,0x35,0x73,0x22,0x60,0x9c,0x45,0x70,0x6c,0xe6,0xb5,0x14},//== #8
{0x02,0x43,0x60,0x1d,0x61,0xc8,0x36,0x38,0x74,0x85,0xe9,0x51,0x4a,0xb5,0xc8,0x92,0x4d,0xd2,0xcf,0xd4,0x66,0xaf,0x34,0xac,0x95,0x00,0x27,0x27,0xe1,0x65,0x9d,0x60,0xf7},//== #9
{0x03,0xa7,0xa4,0xc3,0x02,0x91,0xac,0x1d,0xb2,0x4b,0x4a,0xb0,0x0c,0x44,0x2a,0xa8,0x32,0xf7,0x79,0x4b,0x5a,0x09,0x59,0xbe,0xc6,0xe8,0xd7,0xfe,0xe8,0x02,0x28,0x9d,0xcd},//== #10
{0x03,0x8b,0x05,0xb0,0x60,0x3a,0xbd,0x75,0xb0,0xc5,0x74,0x89,0xe4,0x51,0xf8,0x11,0xe1,0xaf,0xe5,0x4a,0x87,0x15,0x04,0x5c,0xdf,0x48,0x88,0x33,0x3f,0x3e,0xbc,0x6e,0x8b},//== #11
{0x03,0x8b,0x00,0xfc,0xbf,0xc1,0xa2,0x03,0xf4,0x4b,0xf1,0x23,0xfc,0x7f,0x4c,0x91,0xc1,0x0a,0x85,0xc8,0xea,0xe9,0x18,0x7f,0x9d,0x22,0x24,0x2b,0x46,0x00,0xce,0x78,0x1c},//== #12
{0x03,0xaa,0xda,0xaa,0xb1,0xdb,0x8d,0x5d,0x45,0x0b,0x51,0x17,0x89,0xc3,0x7e,0x7c,0xfe,0xb0,0xeb,0x8b,0x3e,0x61,0xa5,0x7a,0x34,0x16,0x6c,0x5e,0xdc,0x9a,0x4b,0x86,0x9d},//== #13
{0x03,0xb4,0xf1,0xde,0x58,0xb8,0xb4,0x1a,0xfe,0x9f,0xd4,0xe5,0xff,0xbd,0xaf,0xae,0xab,0x86,0xc5,0xdb,0x47,0x69,0xc1,0x5d,0x6e,0x60,0x11,0xae,0x73,0x51,0xe5,0x47,0x59},//== #14
{0x02,0xfe,0xa5,0x8f,0xfc,0xf4,0x95,0x66,0xf6,0xe9,0xe9,0x35,0x0c,0xf5,0xbc,0xa2,0x86,0x13,0x12,0xf4,0x22,0x96,0x6e,0x8d,0xb1,0x60,0x94,0xbe,0xb1,0x4d,0xc3,0xdf,0x2c},//== #15
{0x02,0x9d,0x8c,0x5d,0x35,0x23,0x1d,0x75,0xeb,0x87,0xfd,0x2c,0x5f,0x05,0xf6,0x52,0x81,0xed,0x95,0x73,0xdc,0x41,0x85,0x32,0x88,0xc6,0x2e,0xe9,0x4e,0xb2,0x59,0x0b,0x7a},//== #16
{0x03,0x3f,0x68,0x8b,0xae,0x83,0x21,0xb8,0xe0,0x2b,0x7e,0x6c,0x0a,0x55,0xc2,0x51,0x5f,0xb2,0x5a,0xb9,0x7d,0x85,0xfd,0xa8,0x42,0x44,0x9f,0x7b,0xfa,0x04,0xe1,0x28,0xc3},//== #17
{0x02,0x0c,0xe4,0xa3,0x29,0x1b,0x19,0xd2,0xe1,0xa7,0xbf,0x73,0xee,0x87,0xd3,0x0a,0x6b,0xdb,0xc7,0x2b,0x20,0x77,0x1e,0x7d,0xff,0xf4,0x0d,0x0d,0xb7,0x55,0xcd,0x4a,0xf1},//== #18
{0x03,0x85,0x66,0x3c,0x8b,0x2f,0x90,0x65,0x9e,0x1c,0xca,0xb2,0x01,0x69,0x4f,0x4f,0x8e,0xc2,0x4b,0x37,0x49,0xcf,0xe5,0x03,0x0c,0x7c,0x36,0x46,0xa7,0x09,0x40,0x8e,0x19},//== #19
{0x03,0x3c,0x4a,0x45,0xcb,0xd6,0x43,0xff,0x97,0xd7,0x7f,0x41,0xea,0x37,0xe8,0x43,0x64,0x8d,0x50,0xfd,0x89,0x4b,0x86,0x4b,0x0d,0x52,0xfe,0xbc,0x62,0xf6,0x45,0x4f,0x7c},//== #20
{0x03,0x1a,0x74,0x6c,0x78,0xf7,0x27,0x54,0xe0,0xbe,0x04,0x61,0x86,0xdf,0x8a,0x20,0xcd,0xce,0x5c,0x79,0xb2,0xed,0xa7,0x60,0x13,0xc6,0x47,0xaf,0x08,0xd3,0x06,0xe4,0x9e},//== #21
{0x02,0x3e,0xd9,0x6b,0x52,0x4d,0xb5,0xff,0x4f,0xe0,0x07,0xce,0x73,0x03,0x66,0x05,0x2b,0x7c,0x51,0x1d,0xc5,0x66,0x22,0x7d,0x92,0x90,0x70,0xb9,0xce,0x91,0x7a,0xbb,0x43},//== #22
{0x03,0xf8,0x27,0x10,0x36,0x1b,0x8b,0x81,0xbd,0xed,0xb1,0x69,0x94,0xf3,0x0c,0x80,0xdb,0x52,0x24,0x50,0xa9,0x3e,0x8e,0x87,0xee,0xb0,0x7f,0x79,0x03,0xcf,0x28,0xd0,0x4b},//== #23
{0x03,0x6e,0xa8,0x39,0xd2,0x28,0x47,0xee,0x1d,0xce,0x3b,0xfc,0x5b,0x11,0xf6,0xcf,0x78,0x5b,0x06,0x82,0xdb,0x58,0xc3,0x5b,0x63,0xd1,0x34,0x2e,0xb2,0x21,0xc3,0x49,0x0c},//== #24
{0x03,0x05,0x7f,0xbe,0xa3,0xa2,0x62,0x33,0x82,0x62,0x8d,0xde,0x55,0x6b,0x2a,0x06,0x98,0xe3,0x24,0x28,0xd3,0xcd,0x22,0x5f,0x3b,0xd0,0x34,0xdc,0xa8,0x2d,0xd7,0x45,0x5a},//== #25
{0x02,0x4e,0x4f,0x50,0xa2,0xa3,0xec,0xcd,0xb3,0x68,0x98,0x8a,0xe3,0x7c,0xd4,0xb6,0x11,0x69,0x7b,0x26,0xb2,0x96,0x96,0xe4,0x2e,0x06,0xd7,0x13,0x68,0xb4,0xf3,0x84,0x0f},//== #26
{0x03,0x1a,0x86,0x4b,0xae,0x39,0x22,0xf3,0x51,0xf1,0xb5,0x7c,0xfd,0xd8,0x27,0xc2,0x5b,0x7e,0x09,0x3c,0xb9,0xc8,0x8a,0x72,0xc1,0xcd,0x89,0x3d,0x9f,0x90,0xf4,0x4e,0xce},//== #27
{0x03,0xe9,0xe6,0x61,0x83,0x8a,0x96,0xa6,0x53,0x31,0x63,0x7e,0x2a,0x3e,0x94,0x8d,0xc0,0x75,0x6e,0x50,0x09,0xe7,0xcb,0x5c,0x36,0x66,0x4d,0x9b,0x72,0xdd,0x18,0xc0,0xa7},//== #28
{0x02,0x6c,0xaa,0xd6,0x34,0x38,0x2d,0x34,0x69,0x1e,0x3b,0xef,0x43,0xed,0x4a,0x12,0x4d,0x89,0x09,0xa8,0xa3,0x36,0x2f,0x91,0xf1,0xd2,0x0a,0xba,0xaf,0x7e,0x91,0x7b,0x36},//== #29
{0x03,0x0d,0x28,0x2c,0xf2,0xff,0x53,0x6d,0x2c,0x42,0xf1,0x05,0xd0,0xb8,0x58,0x88,0x21,0xa9,0x15,0xdc,0x3f,0x9a,0x05,0xbd,0x98,0xbb,0x23,0xaf,0x67,0xa2,0xe9,0x2a,0x5b},//== #30
{0x03,0x87,0xdc,0x70,0xdb,0x18,0x06,0xcd,0x9a,0x9a,0x76,0x63,0x74,0x12,0xec,0x11,0xdd,0x99,0x8b,0xe6,0x66,0x58,0x48,0x49,0xb3,0x18,0x5f,0x7f,0x93,0x13,0xc8,0xfd,0x28},//== #31
{0x02,0x09,0xc5,0x82,0x40,0xe5,0x0e,0x3b,0xa3,0xf8,0x33,0xc8,0x26,0x55,0xe8,0x72,0x5c,0x03,0x7a,0x22,0x94,0xe1,0x4c,0xf5,0xd7,0x3a,0x5d,0xf8,0xd5,0x61,0x59,0xde,0x69},//== #32
{0x03,0xa3,0x55,0xaa,0x5e,0x2e,0x09,0xdd,0x44,0xbb,0x46,0xa4,0x72,0x2e,0x93,0x36,0xe9,0xe3,0xee,0x4e,0xe4,0xe7,0xb7,0xa0,0xcf,0x57,0x85,0xb2,0x83,0xbf,0x2a,0xb5,0x79},//== #33
{0x03,0x3c,0xdd,0x9d,0x6d,0x97,0xcb,0xfe,0x7c,0x26,0xf9,0x02,0xfa,0xf6,0xa4,0x35,0x78,0x0f,0xe6,0x52,0xe1,0x59,0xec,0x95,0x36,0x50,0xec,0x7b,0x10,0x04,0x08,0x27,0x90},//== #34
{0x02,0xf6,0xa8,0x14,0x8a,0x62,0x32,0x0e,0x14,0x9c,0xb1,0x5c,0x54,0x4f,0xe8,0xa2,0x5a,0xb4,0x83,0xa0,0x09,0x5d,0x22,0x80,0xd0,0x3b,0x8a,0x00,0xa7,0xfe,0xad,0xa1,0x3d},//== #35
{0x02,0xb3,0xe7,0x72,0x21,0x66,0x95,0x84,0x5f,0xa9,0xdd,0xa4,0x19,0xfb,0x5d,0xac,0xa2,0x81,0x54,0xd8,0xaa,0x59,0xea,0x30,0x2f,0x05,0xe9,0x16,0x63,0x5e,0x47,0xb9,0xf6},//== #36
{0x02,0x7d,0x2c,0x03,0xc3,0xef,0x0a,0xec,0x70,0xf2,0xc7,0xe1,0xe7,0x54,0x54,0xa5,0xdf,0xdd,0x0e,0x1a,0xde,0xa6,0x70,0xc1,0xb3,0xa4,0x64,0x3c,0x48,0xad,0x0f,0x12,0x55},//== #37
{0x03,0xc0,0x60,0xe1,0xe3,0x77,0x1c,0xbe,0xcc,0xb3,0x8e,0x11,0x9c,0x24,0x14,0x70,0x2f,0x3f,0x51,0x81,0xa8,0x96,0x52,0x53,0x88,0x51,0xd2,0xe3,0x88,0x6b,0xdd,0x70,0xc6},//== #38
{0x02,0x2d,0x77,0xcd,0x14,0x67,0x01,0x9a,0x6b,0xf2,0x8f,0x73,0x75,0xd0,0x94,0x9c,0xe3,0x0e,0x6b,0x58,0x15,0xc2,0x75,0x8b,0x98,0xa7,0x4c,0x27,0x00,0xbc,0x00,0x65,0x43},//== #39
{0x03,0xa2,0xef,0xa4,0x02,0xfd,0x52,0x68,0x40,0x0c,0x77,0xc2,0x0e,0x57,0x4b,0xa8,0x64,0x09,0xed,0xed,0xee,0x7c,0x40,0x20,0xe4,0xb9,0xf0,0xed,0xbe,0xe5,0x3d,0xe0,0xd4},//== #40
{0x03,0xb3,0x57,0xe6,0x84,0x37,0xda,0x27,0x3d,0xcf,0x99,0x5a,0x47,0x4a,0x52,0x44,0x39,0xfa,0xad,0x86,0xfc,0x9e,0xff,0xc3,0x00,0x18,0x3f,0x71,0x4b,0x09,0x03,0x46,0x8b},//== #41
{0x03,0xee,0xc8,0x83,0x85,0xbe,0x9d,0xa8,0x03,0xa0,0xd6,0x57,0x97,0x98,0xd9,0x77,0xa5,0xd0,0xc7,0xf8,0x09,0x17,0xda,0xb4,0x9c,0xb7,0x3c,0x9e,0x39,0x27,0x14,0x2c,0xb6},//== #42
{0x02,0xa6,0x31,0xf9,0xba,0x0f,0x28,0x51,0x16,0x14,0x90,0x4d,0xf8,0x0d,0x7f,0x97,0xa4,0xf4,0x3f,0x02,0x24,0x9c,0x89,0x09,0xda,0xc9,0x22,0x76,0xcc,0xf0,0xbc,0xda,0xed},//== #43
{0x02,0x5e,0x46,0x6e,0x97,0xed,0x0e,0x79,0x10,0xd3,0xd9,0x0c,0xeb,0x03,0x32,0xdf,0x48,0xdd,0xf6,0x7d,0x45,0x6b,0x9e,0x73,0x03,0xb5,0x0a,0x3d,0x89,0xde,0x35,0x73,0x36},//== #44
{0x02,0x6e,0xca,0xbd,0x2d,0x22,0xfd,0xb7,0x37,0xbe,0x21,0x97,0x5c,0xe9,0xa6,0x94,0xe1,0x08,0xeb,0x94,0xf3,0x64,0x9c,0x58,0x6c,0xc7,0x46,0x1c,0x8a,0xbf,0x5d,0xa7,0x1a},//== #45
{0x03,0xfd,0x54,0x87,0x72,0x2d,0x25,0x76,0xcb,0x6d,0x70,0x81,0x42,0x6b,0x66,0xa3,0xe2,0x98,0x6c,0x1c,0xe8,0x35,0x8d,0x47,0x90,0x63,0xfb,0x5f,0x2b,0xb6,0xdd,0x58,0x49},//== #46
{0x02,0x3a,0x12,0xbd,0x3c,0xaf,0x0b,0x0f,0x77,0xbf,0x4e,0xea,0x8e,0x7a,0x40,0xdb,0xe2,0x79,0x32,0xbf,0x80,0xb1,0x9a,0xc7,0x2f,0x5f,0x5a,0x64,0x92,0x5a,0x59,0x41,0x96},//== #47
{0x02,0x91,0xbe,0xe5,0xcf,0x4b,0x14,0xc2,0x91,0xc6,0x50,0x73,0x2f,0xaa,0x16,0x60,0x40,0xe4,0xc1,0x8a,0x14,0x73,0x1f,0x9a,0x93,0x0c,0x1e,0x87,0xd3,0xec,0x12,0xde,0xbb},//== #48
{0x02,0x59,0x1d,0x68,0x2c,0x3d,0xa4,0xa2,0xa6,0x98,0x63,0x3b,0xf5,0x75,0x17,0x38,0xb6,0x7c,0x34,0x32,0x85,0xeb,0xdc,0x34,0x92,0x64,0x5c,0xb4,0x46,0x58,0x91,0x14,0x84},//== #49
{0x03,0xf4,0x6f,0x41,0x02,0x7b,0xbf,0x44,0xfa,0xfd,0x6b,0x05,0x90,0x91,0xb9,0x00,0xda,0xd4,0x1e,0x68,0x45,0xb2,0x24,0x1d,0xc3,0x25,0x4c,0x7c,0xdd,0x3c,0x5a,0x16,0xc6},//== #50
{0x02,0x8c,0x6c,0x67,0xbe,0xf9,0xe9,0xee,0xbe,0x6a,0x51,0x32,0x72,0xe5,0x0c,0x23,0x0f,0x0f,0x91,0xed,0x56,0x0c,0x37,0xbc,0x9b,0x03,0x32,0x41,0xff,0x6c,0x3b,0xe7,0x8f},//== #51
{0x03,0x74,0xc3,0x3b,0xd5,0x48,0xef,0x02,0x66,0x7d,0x61,0x34,0x18,0x92,0x13,0x4f,0xcf,0x21,0x66,0x40,0xbc,0x22,0x01,0xae,0x61,0x92,0x8c,0xd0,0x87,0x4f,0x63,0x14,0xa7},//== #52
{0x02,0x0f,0xaa,0xf5,0xf3,0xaf,0xe5,0x83,0x00,0xa3,0x35,0x87,0x4c,0x80,0x68,0x1c,0xf6,0x69,0x33,0xe2,0xa7,0xae,0xb2,0x83,0x87,0xc0,0xd2,0x8b,0xb0,0x48,0xbc,0x63,0x49},//== #53
{0x03,0x4a,0xf4,0xb8,0x1f,0x8c,0x45,0x0c,0x2c,0x87,0x0c,0xe1,0xdf,0x18,0x4a,0xff,0x12,0x97,0xe5,0xfc,0xd5,0x49,0x44,0xd9,0x8d,0x81,0xe1,0xa5,0x45,0xff,0xb2,0x25,0x96},//== #54
{0x03,0x85,0xa3,0x0d,0x84,0x13,0xaf,0x4f,0x8f,0x9e,0x63,0x12,0x40,0x0f,0x2d,0x19,0x4f,0xe1,0x4f,0x02,0xe7,0x19,0xb2,0x4c,0x3f,0x83,0xbf,0x1f,0xd2,0x33,0xa8,0xf9,0x63},//== #55
{0x03,0x3f,0x2d,0xb2,0x07,0x4e,0x32,0x17,0xb3,0xe5,0xee,0x30,0x53,0x01,0xee,0xeb,0xb1,0x16,0x0c,0x4f,0xa1,0xe9,0x93,0xee,0x28,0x01,0x12,0xf6,0x34,0x86,0x37,0x99,0x9a},//== #56
{0x02,0xa5,0x21,0xa0,0x7e,0x98,0xf7,0x8b,0x03,0xfc,0x1e,0x03,0x9b,0xc3,0xa5,0x14,0x08,0xcd,0x73,0x11,0x9b,0x5e,0xb1,0x16,0xe5,0x83,0xfe,0x57,0xdc,0x8d,0xb0,0x7a,0xea},//== #57
{0x03,0x11,0x56,0x94,0x42,0xe8,0x70,0x32,0x6c,0xee,0xc0,0xde,0x24,0xeb,0x54,0x78,0xc1,0x9e,0x14,0x6e,0xcd,0x9d,0x15,0xe4,0x66,0x64,0x40,0xf2,0xf6,0x38,0x87,0x5f,0x42},//== #58
{0x02,0x41,0x26,0x7d,0x2d,0x7e,0xe1,0xa8,0xe7,0x6f,0x8d,0x15,0x46,0xd0,0xd3,0x0a,0xef,0xb2,0x89,0x2d,0x23,0x1c,0xee,0x0d,0xde,0x77,0x76,0xda,0xf9,0xf8,0x02,0x14,0x85},//== #59
{0x03,0x48,0xe8,0x43,0xdc,0x5b,0x1b,0xd2,0x46,0xe6,0x30,0x9b,0x49,0x24,0xb8,0x15,0x43,0xd0,0x2b,0x16,0xc8,0x08,0x3d,0xf9,0x73,0xa8,0x9c,0xe2,0xc7,0xeb,0x89,0xa1,0x0d},//== #60
{0x02,0x49,0xa4,0x38,0x60,0xd1,0x15,0x14,0x3c,0x35,0xc0,0x94,0x54,0x86,0x3d,0x6f,0x82,0xa9,0x5e,0x47,0xc1,0x16,0x2f,0xb9,0xb2,0xeb,0xe0,0x18,0x6e,0xb2,0x6f,0x45,0x3f},//== #61
{0x03,0x23,0x1a,0x67,0xe4,0x24,0xca,0xf7,0xd0,0x1a,0x00,0xd5,0xcd,0x49,0xb0,0x46,0x49,0x42,0x25,0x5b,0x8e,0x48,0x76,0x6f,0x96,0x60,0x2b,0xdf,0xa4,0xea,0x14,0xfe,0xa8},//== #62
{0x03,0x65,0xec,0x29,0x94,0xb8,0xcc,0x0a,0x20,0xd4,0x0d,0xd6,0x9e,0xdf,0xe5,0x5c,0xa3,0x2a,0x54,0xbc,0xbb,0xaa,0x6b,0x0d,0xdc,0xff,0x36,0x04,0x93,0x01,0xa5,0x45,0x79},//== #63
{},
{0x02,0x30,0x21,0x0c,0x23,0xb1,0xa0,0x47,0xbc,0x9b,0xdb,0xb1,0x34,0x48,0xe6,0x7d,0xed,0xdc,0x10,0x89,0x46,0xde,0x6d,0xe6,0x39,0xbc,0xc7,0x5d,0x47,0xc0,0x21,0x6b,0x1b},//== #65
{},
{},
{},
{},
{0x02,0x90,0xe6,0x90,0x0a,0x58,0xd3,0x33,0x93,0xbc,0x10,0x97,0xb5,0xae,0xd3,0x1f,0x2e,0x4e,0x7c,0xbd,0x3e,0x54,0x66,0xaf,0x95,0x86,0x65,0xbc,0x01,0x21,0x24,0x84,0x83},//== #70
{},
{},
{},
{},
{0x03,0x72,0x6b,0x57,0x4f,0x19,0x3e,0x37,0x46,0x86,0xd8,0xe1,0x2b,0xc6,0xe4,0x14,0x2a,0xde,0xb0,0x67,0x70,0xe0,0xa2,0x85,0x6f,0x5e,0x4a,0xd8,0x9f,0x66,0x04,0x47,0x55},//== #75
{},
{},
{},
{},
{0x03,0x7e,0x12,0x38,0xf7,0xb1,0xce,0x75,0x7d,0xf9,0x4f,0xaa,0x9a,0x2e,0xb2,0x61,0xbf,0x0a,0xeb,0x9f,0x84,0xdb,0xf8,0x12,0x12,0x10,0x4e,0x78,0x93,0x1c,0x2a,0x19,0xdc},//== #80
{},
{},
{},
{},
{0x03,0x29,0xc4,0x57,0x4a,0x4f,0xd8,0xc8,0x10,0xb7,0xe4,0x2a,0x4b,0x39,0x88,0x82,0xb3,0x81,0xbc,0xd8,0x5e,0x40,0xc6,0x88,0x37,0x12,0x91,0x2d,0x16,0x7c,0x83,0xe7,0x3a},//== #85
{},
{},
{},
{},
{0x03,0x5c,0x38,0xbd,0x9a,0xe4,0xb1,0x0e,0x8a,0x25,0x08,0x57,0x00,0x6f,0x3c,0xfd,0x98,0xab,0x15,0xa6,0x19,0x6d,0x9f,0x4d,0xfd,0x25,0xbc,0x7e,0xcc,0x77,0xd7,0x88,0xd5},//== #90
{},
{},
{},
{},
{0x02,0x96,0x7a,0x59,0x05,0xd6,0xf3,0xb4,0x20,0x95,0x9a,0x02,0x78,0x9f,0x96,0xab,0x4c,0x32,0x23,0xa2,0xc4,0xd2,0x76,0x2f,0x81,0x7b,0x78,0x95,0xc5,0xbc,0x88,0xa0,0x45},//== #95
{},
{},
{},
{},
{0x03,0xd2,0x06,0x3d,0x40,0x40,0x2f,0x03,0x0d,0x4c,0xc7,0x13,0x31,0x46,0x88,0x27,0xaa,0x41,0xa8,0xa0,0x9b,0xd6,0xfd,0x80,0x1b,0xa7,0x7f,0xb6,0x4f,0x8e,0x67,0xe6,0x17},//== #100
{},
{},
{},
{},
{0x03,0xbc,0xf7,0xce,0x88,0x7f,0xfc,0xa5,0xe6,0x2c,0x9c,0xab,0xbd,0xb7,0xff,0xa7,0x1d,0xc1,0x83,0xc5,0x2c,0x04,0xff,0x4e,0xe5,0xee,0x82,0xe0,0xc5,0x5c,0x39,0xd7,0x7b},//== #105
{},
{},
{},
{},
{0x03,0x09,0x97,0x6b,0xa5,0x57,0x09,0x66,0xbf,0x88,0x91,0x96,0xb7,0xfd,0xf5,0xa0,0xf9,0xa1,0xe9,0xab,0x34,0x05,0x56,0xec,0x29,0xf8,0xbb,0x60,0x59,0x96,0x16,0x16,0x7d},//== #110
{},
{},
{},
{},
{0x02,0x48,0xd3,0x13,0xb0,0x39,0x8d,0x49,0x23,0xcd,0xca,0x73,0xb8,0xcf,0xa6,0x53,0x2b,0x91,0xb9,0x67,0x03,0x90,0x2f,0xc8,0xb3,0x2f,0xd4,0x38,0xa3,0xb7,0xcd,0x7f,0x55},//== #115
{},
{},
{},
{},
{0x02,0xce,0xb6,0xcb,0xbc,0xdb,0xdf,0x5e,0xf7,0x15,0x06,0x82,0x15,0x0f,0x4c,0xe2,0xc6,0xf4,0x80,0x7b,0x34,0x98,0x27,0xdc,0xdb,0xdd,0x1f,0x2e,0xfa,0x88,0x5a,0x26,0x30},//== #120
{},
{},
{},
{},
{0x02,0x33,0x70,0x9e,0xb1,0x1e,0x0d,0x44,0x39,0xa7,0x29,0xf2,0x1c,0x2c,0x44,0x3d,0xed,0xb7,0x27,0x52,0x82,0x29,0x71,0x3f,0x00,0x65,0x72,0x1b,0xa8,0xfa,0x46,0xf0,0x0e},//== #125
{},
{},
{},
{},
{0x03,0x63,0x3c,0xbe,0x3e,0xc0,0x2b,0x94,0x01,0xc5,0xef,0xfa,0x14,0x4c,0x5b,0x4d,0x22,0xf8,0x79,0x40,0x25,0x96,0x34,0x85,0x8f,0xc7,0xe5,0x9b,0x1c,0x09,0x93,0x78,0x52},//== #130
{},
{},
{},
{},
{0x02,0x14,0x5d,0x26,0x11,0xc8,0x23,0xa3,0x96,0xef,0x67,0x12,0xce,0x0f,0x71,0x2f,0x09,0xb9,0xb4,0xf3,0x13,0x5e,0x3e,0x0a,0xa3,0x23,0x0f,0xb9,0xb6,0xd0,0x8d,0x1e,0x16},//== #135
{},
{},
{},
{},
{0x03,0x1f,0x6a,0x33,0x2d,0x3c,0x5c,0x4f,0x2d,0xe2,0x37,0x8c,0x01,0x2f,0x42,0x9c,0xd1,0x09,0xba,0x07,0xd6,0x96,0x90,0xc6,0xc7,0x01,0xb6,0xbb,0x87,0x86,0x0d,0x66,0x40},//== #140
{},
{},
{},
{},
{0x03,0xaf,0xdd,0xa4,0x97,0x36,0x9e,0x21,0x9a,0x2c,0x1c,0x36,0x99,0x54,0xa9,0x30,0xe4,0xd3,0x74,0x09,0x68,0xe5,0xe4,0x35,0x24,0x75,0xbc,0xff,0xce,0x31,0x40,0xda,0xe5},//== #145
{},
{},
{},
{},
{0x03,0x13,0x78,0x07,0x79,0x0e,0xa7,0xdc,0x6e,0x97,0x90,0x1c,0x2b,0xc8,0x74,0x11,0xf4,0x5e,0xd7,0x4a,0x56,0x29,0x31,0x5c,0x4e,0x4b,0x03,0xa0,0xa1,0x02,0x25,0x0c,0x49},//== #150
{},
{},
{},
{},
{0x03,0x5c,0xd1,0x85,0x4c,0xae,0x45,0x39,0x1c,0xa4,0xec,0x42,0x8c,0xc7,0xe6,0xc7,0xd9,0x98,0x44,0x24,0xb9,0x54,0x20,0x9a,0x8e,0xea,0x19,0x7b,0x9e,0x36,0x4c,0x05,0xf6},//== #155
{},
{},
{},
{},
{0x02,0xe0,0xa8,0xb0,0x39,0x28,0x2f,0xaf,0x6f,0xe0,0xfd,0x76,0x9c,0xfb,0xc4,0xb6,0xb4,0xcf,0x87,0x58,0xba,0x68,0x22,0x0e,0xac,0x42,0x0e,0x32,0xb9,0x1d,0xdf,0xa6,0x73},//== #160
};


struct Pairs {
    int z; // TAME/WILD num
    int t; // Thread num
    int tame_wild; // 0=tame 1=wild
    secp256k1_gej *K; // Pointer to Tame of Wild element
    uint128_t *n; // Pointer to count
};

typedef struct table_t {
    unsigned char x[32];
    uint128_t n;
} table_t;

table_t *A; // tame table
table_t *B; // wild table

uint64_t tame_count;
uint64_t wild_count;


bloom_t bloom_tame;
bloom_t bloom_wild;

secp256k1_ge pubkeys[NUMPUBKEYS];
secp256k1_ge P[256];
secp256k1_ge P0[256];
uint128_t dS[256];
uint128_t dS0[256];


time_t time0, time1, time2;

unsigned int THREADS, CHECK_BITS;
int DEBUG = 0;

uint64_t status[128]; // Max threads: 128
uint128_t status_old = 0;

int solved = 0;

uint128_t DPmodule;
uint32_t JmaxofSp;


void usage(unsigned char *name) {
    printf("Usage: %s -b [BITS] -m [BITS] [OPTION]...\n\n\
 -b BITS                     bits\n\
 -t THREADS                  number of threads\n\
 -d                          debug\n\
 -h                          show this help\n", name);
    exit(1);
}


void time_format(uint64_t t) {

    unsigned int n = 0;
    unsigned char str[100];

    if (t>(60*60*24*365)) {
        n += sprintf(str+n, " %02luy", (uint64_t) t/(60*60*24*365));
        t = t%(60*60*24*365);
    }

    if (t>(60*60*24*30)) {
        n += sprintf(str+n, " %02lum", (uint64_t) t/(60*60*24*30));
        t = t%(60*60*24*30);
    }

    if (t>(60*60*24)) {
        n += sprintf(str+n, " %02lud", (uint64_t) t/(60*60*24));
        t = t%(60*60*24);
    }

    n += sprintf(str+n, " %02lu", (uint64_t) t/(60*60));
    t = t%(60*60);

    n += sprintf(str+n, ":%02lu", (uint64_t) t/60);
    t = t%60;

    n += sprintf(str+n, ":%02lus", t);

    printf("%s", str);
}


static void secp256k1_ge_copy(secp256k1_ge *r, const secp256k1_ge *a) {
   r->infinity = a->infinity;
   r->x = a->x;
   r->y = a->y;
}


uint64_t secp256k1_fe_mod(secp256k1_fe *a, uint64_t m) {

    uint64_t result = 0;

    unsigned char tmp[33];
    secp256k1_fe_get_b32(tmp, a);

    for (uint32_t i=0;i<32;i++) {
        result = ((result<<8) + tmp[i]) % m;
    }

    return result;
}


static void secp256k1_gej_normalize(secp256k1_gej *a) {
    secp256k1_fe z2, z3;
    secp256k1_fe_inv(&a->z, &a->z);
    secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_mul(&z3, &a->z, &z2);
    secp256k1_fe_mul(&a->x, &a->x, &z2);
    secp256k1_fe_mul(&a->y, &a->y, &z3);
    secp256k1_fe_set_int(&a->z, 1);
}


static void secp256k1_ge_mul_k(secp256k1_ge *r, const secp256k1_ge *a, const uint64_t k) {

    secp256k1_ge T[256];
    secp256k1_gej rj;

    if (k==0) {
        fprintf(stderr,"secp256k1_ge_mul_k * 0\n");
        exit(1);
    } else if (k==1) {
        secp256k1_ge_copy(r, a);
        return;
    }

    // Precalculate 2**i table
    secp256k1_gej pt;
    secp256k1_gej_set_ge(&pt, a);

    for (int i = 0; i<128; i++) {
        secp256k1_ge_set_gej(&T[i], &pt);
        secp256k1_gej_double_var(&pt, &pt, NULL);
    }

    int pt_init = 0;
    for (uint64_t a = 0; a<64; a++) {
        if (k>>a & 1 == 1) {
            if (pt_init) {
                secp256k1_gej_add_ge_var(&rj, &rj, &T[a], NULL);
            } else {
                secp256k1_gej_set_ge(&rj, &T[a]);
                pt_init = 1;
            }
        }
    }

    secp256k1_ge_set_gej(r, &rj);
}

static void secp256k1_gej_mul_k(secp256k1_gej *rj, const secp256k1_ge *a, const uint64_t k) {
    secp256k1_ge r;
    secp256k1_ge_mul_k(&r, a, k);
    secp256k1_gej_set_ge(rj, &r);
}


uint128_t randint(uint128_t a, uint128_t b) {

    uint128_t number;
    FILE *fp;
    fp = fopen("/dev/urandom", "r");

    if (!fp) {
        fprintf(stderr,"Failed to generate random numbers\n");
        exit(1);
    } else {
        int r = fread(&number, 1, 16, fp);
        fclose(fp);
        if (r==0) {
            fprintf(stderr,"Failed to generate random numbers\n");
            exit(1);
        }

        number += a;
        number %= b;

        return number;
    }
}


uint32_t getJmaxofSp(uint128_t optimalmeanjumpsize) {
    printf("\033[0;35m[optimal_mean_jumpsize]\033[0m %lu\n", (uint64_t) optimalmeanjumpsize); //TODO: 128bits

    uint64_t sumjumpsize, now_meanjumpsize, next_meanjumpsize;
    sumjumpsize = 0;

    for (uint32_t i=1; i<128; i++) {
        sumjumpsize += dS[i-1];

        now_meanjumpsize = round((float)1.0*(sumjumpsize)/(i));
        next_meanjumpsize = round((float)1.0*(sumjumpsize + dS[i])/(i+1));

        if ((long int)(optimalmeanjumpsize - now_meanjumpsize) <= (long int)(next_meanjumpsize - optimalmeanjumpsize)) {
            printf("\033[0;35m[meanjumpsize#Sp[%d]]\033[0m %lu(now) <= %lu(optimal) <= %lu(next)\n", i, (uint64_t) now_meanjumpsize, (uint64_t) optimalmeanjumpsize, (uint64_t) next_meanjumpsize);
            return i;
        }
    }

    fprintf(stderr,"\n[FATAL_ERROR] JmaxofSp not defined!\n");
    exit(1);
}


int comparator(unsigned char *str, uint128_t Pindex, int tame_wild) {

    uint128_t d;

    printf("SEARCHING: ");
    for (uint32_t i=0;i<32;i++)
      printf("%02x", str[i]);
    printf("\n");

    if (tame_wild==1) {
        for (uint64_t k=0; k<tame_count; k++) {
            if (memcmp(str, A[k].x, 32)==0) {
                d = (uint128_t) ((uint128_t) A[k].n - (uint128_t) Pindex);
                printf("\nX: ");
                for (uint32_t i=0;i<32;i++)
                  printf("%02x", str[i]);
                printf("\nFound in TAME: %lu, WILD: %lu\n", k, wild_count);
                printf("A: %016lx%016lx\n", (uint64_t) (A[k].n>>64), (uint64_t) (A[k].n));
                printf("B: %016lx%016lx\n", (uint64_t) (Pindex>>64), (uint64_t) (Pindex));
                printf("SOLVED: 00000000000000000000000000000000%016lx%016lx\n", (uint64_t) (d>>64), (uint64_t) (d));

                FILE *fp = fopen("results.txt","a+");
                fprintf(fp,"----------------\nFound private key %2d\n", CHECK_BITS);
                fprintf(fp,"A: %016lx%016lx\n", (uint64_t) (A[k].n>>64), (uint64_t) (A[k].n));
                fprintf(fp,"B: %016lx%016lx\nX: ", (uint64_t) (Pindex>>64), (uint64_t) (Pindex));
                for (uint32_t i=0;i<32;i++)
                  fprintf(fp,"%02x", str[i]);
                fprintf(fp,"\n00000000000000000000000000000000%016lx%016lx\n", (uint64_t) (d>>64), (uint64_t) (d));
                fclose(fp);

                return 1;
            }
        }
    } else {
        for (uint64_t k=0; k<wild_count; k++) {
            if (memcmp(str, B[k].x, 32)==0) {
                d = (uint128_t) ((uint128_t) Pindex - (uint128_t) B[k].n);
                printf("\nX: ");
                for (uint32_t i=0;i<32;i++)
                  printf("%02x", str[i]);
                printf("\nFound in WILD: %lu, TAME: %lu\n", k, tame_count);
                printf("A: %016lx%016lx\n", (uint64_t) (Pindex>>64), (uint64_t) (Pindex));
                printf("B: %016lx%016lx\n", (uint64_t) (B[k].n>>64), (uint64_t) (B[k].n));
                printf("SOLVED: 00000000000000000000000000000000%016lx%016lx\n", (uint64_t) (d>>64), (uint64_t) (d));

                FILE *fp = fopen("results.txt","a+");
                fprintf(fp,"----------------\nFound private key %2d\n", CHECK_BITS);
                fprintf(fp,"A: %016lx%016lx\n", (uint64_t) (Pindex>>64), (uint64_t) (Pindex));
                fprintf(fp,"B: %016lx%016lx\nX: ", (uint64_t) (B[k].n>>64), (uint64_t) (B[k].n));
                for (uint32_t i=0;i<32;i++)
                  fprintf(fp,"%02x", str[i]);
                fprintf(fp,"\n00000000000000000000000000000000%016lx%016lx\n", (uint64_t) (d>>64), (uint64_t) (d));
                fclose(fp);

                return 1;
            }
        }
    }
    return 0;
}


void *KANGAROO(void *arg) {

    unsigned char tmp[32];
    uint64_t resultmod = 0;
    uint128_t nowjumpsize;
    uint64_t pw;
    int solved_thread;

    struct Pairs *my_pair = (struct Pairs*)arg;
    int d = (*my_pair).z;
    int t = (*my_pair).t;
    int tame_wild = (*my_pair).tame_wild;
    secp256k1_gej K = *((*my_pair).K);
    uint128_t n = *((*my_pair).n);
    free(arg);

    status[t] = 0;

    secp256k1_gej_normalize(&K);
    secp256k1_fe_get_b32(tmp, &K.x);

    if (tame_wild==0)
        printf("\033[0;31mTAME %02d\033[0m - 0x%016lx%016lx ", d, (uint64_t) (n>>64), (uint64_t) n);
    else
        printf("\033[0;33mWILD %02d\033[0m - 0x%016lx%016lx ", d, (uint64_t) (n>>64), (uint64_t) n);

    printf("X=0x");
    for (uint32_t i=0;i<32;i++)
      printf("%02x", tmp[i]);
    printf("\n");

    sleep(1);

    while (1) {

        status[t]++;

        secp256k1_gej_normalize(&K);
        if (solved) break;

        secp256k1_fe_get_b32(tmp, &K.x);
        resultmod = 0;
        for (uint32_t i=0;i<32;i++) {
            resultmod = ((resultmod<<8) + tmp[i]) % DPmodule;
        }

        /* TODO: Comparar el tame con la pub
        if (tame_wild==0) {

        } */

        if (resultmod == 0) {

            if (DEBUG) {
                FILE *fp;
                if (tame_wild==0) {
                    fp = fopen("tame.txt","a+");
                } else {
                    fp = fopen("wild.txt","a+");
                }

                for (uint32_t i=0;i<32;i++) {
                  fprintf(fp,"%02x", tmp[i]);
                }

                fprintf(fp, " %016lx%016lx\n", (uint64_t) (n>>64), (uint64_t) (n));
                fclose(fp);
            }

            if (tame_wild==0) {
                bloom_add(bloom_tame, tmp);
                memcpy(A[tame_count].x, tmp, 32);
                A[tame_count].n = n;
                tame_count++;

                if (bloom_test(bloom_wild, &tmp)) {
                    solved_thread = comparator(tmp, n, tame_wild); //0
                }

            } else {
                bloom_add(bloom_wild, tmp);
                memcpy(B[wild_count].x, tmp, 32);
                B[wild_count].n = n;
                wild_count++;

                if (bloom_test(bloom_tame, &tmp)) {
                    solved_thread = comparator(tmp, n, tame_wild); //1
                }

             }
        }

        if (solved || solved_thread) {
            solved = 1;
            break;
        }

        pw = 0;
        for (uint32_t i=0;i<32;i++) {
            pw = ((pw<<8) + tmp[i]) % JmaxofSp;
        }

        nowjumpsize = dS[pw];
        n += nowjumpsize;
        secp256k1_gej_add_ge_var(&K, &K, &P[pw], NULL);
    }
}


int main(int argc, char **argv) {

    unsigned char RANGE[130];
    unsigned char hi[17];
    unsigned char lo[17];
    uint128_t L, U, M, W1, Wsqrt;

    bool flag_pow2bits = 0;
    bool flag_keyspace = 0;

    int bitMin = 8;
    int bitMax = 120;

    printf("\033[0;31m################################################\n");
    printf("#\033[0;32m    Pollard-kangaroo PrivKey Recovery Tool    \033[0;31m#\n");
    printf("#\033[0;32m           ecdsa on curve secp256k1           \033[0;31m#\n");
    printf("#\033[0;32m                   multicore                  \033[0;31m#\n");
    printf("#\033[0;32m                    ver%4s                   \033[0;31m#\n", KANGAROO_VERSION);
    printf("################################################\033[0m\n");

    int c;
    while ((c = getopt(argc, argv, "hdt:b:r:")) != -1) {
        switch (c) {

            case 'b':
                CHECK_BITS = atoi(optarg);
                break;

            case 'r':
                strncpy(RANGE, optarg, 65);
                break;

            case 't':
                THREADS = atoi(optarg);
                break;

            case 'd':
                DEBUG++;
                break;

            case 'h':
                usage(argv[0]);
                return 0;

            default:
                return 1;
        }
    }

    if (!THREADS) THREADS = 1;

    if (CHECK_BITS) {
        if (CHECK_BITS<=1 || CHECK_BITS>NUMPUBKEYS || CHECK_BITS>127) {
            fprintf(stderr,"Number of bits not valid\n");
            exit(1);
        } else {
            flag_pow2bits = 1;
            printf("[BITS] %d\n", CHECK_BITS);
            L = (uint128_t) 1<<(CHECK_BITS-1);
            U = (uint128_t) 1<<(CHECK_BITS);
        }
    } else if (RANGE) {
        flag_keyspace = 1;
        printf("[RANGE] %s\n", RANGE);
        unsigned char init[65];
        unsigned char end[65];
        unsigned char* tok;
        char *ptr;
        int i, len;

        tok = strtok(RANGE, ":");
        if (tok==NULL) {
            fprintf(stderr,"Range not valid\n");
            usage(argv[0]);
            exit(1);
        }
        strncpy(init, tok, 32);

        len = strlen(init);
        if (len>=16) {
            init[33] = '\0';
            for (i=1; i<=len; i++) {
                init[32-i] = init[len-i];
            }
            for (i=0; i<32-len; i++) {
                init[i] = '0';
            }
            strncpy(hi, init, 16);
            hi[16] = '\0';
            strncpy(lo, init+16, 16);
            lo[16] = '\0';

            L = strtoull(lo, &ptr, 16);
            L += (uint128_t) strtoull(hi, &ptr, 16)<<64;
        } else {
            L = strtoull(init, &ptr, 16);
        }

        tok = strtok(NULL, ":");
        if (tok==NULL) {
            fprintf(stderr,"Range not valid\n");
            usage(argv[0]);
            exit(1);
        }
        strncpy(end, tok, 32);

        len = strlen(end);
        if (len>=16) {
           end[33] = '\0';
            for (i=1; i<=len; i++) {
                end[32-i] = end[len-i];
            }
            for (i=0; i<32-len; i++) {
                end[i] = '0';
            }
            strncpy(hi, end, 16);
            hi[16] = '\0';
            strncpy(lo, end+16, 16);
            lo[16] = '\0';

            U = strtoull(lo, &ptr, 16);
            U += (uint128_t) strtoull(hi, &ptr, 16)<<64;
        } else {
            U = strtoull(end, &ptr, 16);
        }

        printf("[L] %016lx%016lx\n", (uint64_t) (L>>64), (uint64_t) (L));
        printf("[U] %016lx%016lx\n", (uint64_t) (U>>64), (uint64_t) (U));
    }

    if (flag_pow2bits==0 && flag_keyspace==0) {
        fprintf(stderr,"Number of bits or range not defined\n");
        usage(argv[0]);
        exit(1);
    }

    if ((uint128_t) U <= (uint128_t) L) {
        fprintf(stderr,"[error] 0x%lx GreaterOrEqual 0x%lx\n", (uint64_t) L, (uint64_t) U); //TODO: 128bits
        usage(argv[0]);
        exit(1);
    }

    W1 = U - L;
    Wsqrt = round(sqrt(W1));
    M = L + (W1/2);

    printf("\033[1;35m[Wsqrt]\033[0m %lu\n", (uint64_t) Wsqrt); //TODO: 128bits

    unsigned int pow2L, pow2U, pow2W;
    if (flag_pow2bits) {
        pow2L = CHECK_BITS-1;
        pow2U = CHECK_BITS;
        pow2W = CHECK_BITS-1;
        //printf("\033[1;35m[range]\033[0m 0x%lx%016lx..0x%lx%016lx ; W = U - L = 0x%lx%016lx (~2^%d)\n", (uint64_t) (L>>64), (uint64_t) L, (uint64_t) (U>>64), (uint64_t) U, (uint64_t) (W1>>64), (uint64_t)W1, pow2W);
        printf("\033[1;35m[range]\033[0m 2^%d..2^%d ; W = U - L = 0x%lx%016lx (2^%d)\n", pow2L, pow2U, (uint64_t) (W1>>64), (uint64_t) W1, pow2W);

    } else if (flag_keyspace) {
        pow2L = log2(L)+0;
        pow2U = log2(U)+1;
        pow2W = log2(W1)+1;
        CHECK_BITS = pow2U;
        //printf("\033[1;35m[range]\033[0m 0x%lx%016lx..0x%lx%016lx ; W = U - L = 0x%lx%016lx (~2^%d)\n", (uint64_t) (L>>64), (uint64_t) L, (uint64_t) (U>>64), (uint64_t) U, (uint64_t) (W1>>64), (uint64_t)W1, pow2W);
        printf("\033[1;35m[range]\033[0m 2^%d..2^%d ; W = U - L = 0x%lx%016lx (2^%d)\n", pow2L, pow2U, (uint64_t) (W1>>64), (uint64_t) W1, pow2W); //DEBUG
    }

    if (pow2W < bitMin || pow2W > bitMax) {
        fprintf(stderr,"[error] W must be 2^%d..2^%d!\n", bitMin, bitMax);
        exit(1);
    }

    if (pow2W > 55) {
        printf("\033[1;31m[warn!]\033[0m W = 2^%d too big! long runtime expected\n", pow2W);
    }

    if (DEBUG)
        printf("\033[0;36m[DEBUG] level=%d\033[0m\n", DEBUG);

    bloom_tame = bloom_create(BLOOM_SIZE);
    bloom_wild = bloom_create(BLOOM_SIZE);

    A = malloc((uint64_t)(1<<CMPLEN) * sizeof(table_t));
    if (A==NULL) {
        fprintf(stderr,"Failed to allocate memory\n");
        exit(1);
    }
    B = malloc((uint64_t)(1<<CMPLEN) * sizeof(table_t));
    if (B==NULL) {
        fprintf(stderr,"Failed to allocate memory\n");
        exit(1);
    }


    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    for (int i = 0; i < NUMPUBKEYS; i++) {
        secp256k1_eckey_pubkey_parse(&pubkeys[i], rawpubkeys[i], 33);
    }

    // Precalculate 2**i table
    secp256k1_gej pt;
    secp256k1_gej_set_ge(&pt, &secp256k1_ge_const_g);

    for (int i = 0; i<128; i++) {
        secp256k1_ge_set_gej(&P[i], &pt);
        secp256k1_ge_set_gej(&P0[i], &pt);
        secp256k1_gej_double_var(&pt, &pt, NULL);

    }

    dS[0] = dS0[0] = 1;
    for (int i = 1; i<128; i++) {
        dS[i] = dS0[i] = 2 * dS[i-1];
    }


    printf("\033[0;36m[pubkey#%d]\033[0m ", CHECK_BITS);
    for (uint32_t i=0;i<33;i++)
      printf("%02x",rawpubkeys[CHECK_BITS - 1][i]);
    printf("\n");

    // PRINT X AND Y
    unsigned char tmp[33];
    secp256k1_fe_normalize_var(&pubkeys[CHECK_BITS - 1].x);
    secp256k1_fe_get_b32(tmp, &pubkeys[CHECK_BITS - 1].x);
    printf("\033[1;35m[Xcoordinate]\033[0m ");
    for (uint32_t i=0;i<32;i++)
      printf("%02x", tmp[i]);
    printf("\n");

    secp256k1_fe_normalize_var(&pubkeys[CHECK_BITS - 1].y);
    secp256k1_fe_get_b32(tmp, &pubkeys[CHECK_BITS - 1].y);
    printf("\033[1;35m[Ycoordinate]\033[0m ");
    for (uint32_t i=0;i<32;i++)
      printf("%02x", tmp[i]);
    printf("\n");

    unsigned int Nt, Nw;
    if (THREADS==1 || THREADS==2) {
        Nt = Nw = 1;
    } else if (THREADS>=4) {
        if (FLAG_PROFILE==1) {
            Nt = (THREADS/2)-1;
            Nw = (THREADS/2)+1;

            for (int k=0; k<128; k++) {
                secp256k1_ge_mul_k(&P[k], &P0[k], Nt*Nw);
                dS[k] = Nt*Nw*dS0[k];
            }
            printf("[+] recalc Sp-table of multiply UV\n");
        } else {
            Nt = Nw = THREADS/2;
        }
    } else {
        Nt = Nw = THREADS/2;
    }

    printf("\033[1;33m[Threads]\033[0m %d\n", THREADS);
    printf("\033[0;33m[U]\033[0m %d (0x%02x)\n", Nt, Nt);
    printf("\033[0;33m[V]\033[0m %d (0x%02x)\n", Nw, Nw);

    unsigned int kangoo_power = 3;

    // EMPTY TAME AND WILD FILES
    if (DEBUG) {
        FILE *fptame = fopen("tame.txt","w");
        fclose(fptame);
        FILE *fpwild = fopen("wild.txt","w");
        fclose(fpwild);
    }

    // discriminator for filter added new distinguished points (ram economy)
    int pow2dp;
    //pow2dp = ((pow2W/2)-2);
    pow2dp = ((pow2W/2)-10);

    if (pow2dp<1)
        pow2dp = 1;

    if (pow2dp>24)
        pow2dp = 24;

    DPmodule = (uint128_t) 1<<pow2dp;
    printf("\033[0;36m[DPmodule]\033[0m 2^%d 0x%lx\n", pow2dp, (uint64_t) DPmodule); //TODO: 128bits

    // === JUMPSIZE ===
    uint128_t midJsize;
    if (Nt == Nw == 1) {
        midJsize = round(1.0*Wsqrt/2);
    } else {
        if (FLAG_PROFILE==1) { // byPollard
            midJsize = round(1.0*(Nt+Nw)*Wsqrt/4);
        } else if (FLAG_PROFILE==2) { // NaiveSplitRange
            midJsize = round(sqrt(W1/Nt)/2);
        } else if (FLAG_PROFILE==3) { // byOorschot&Wiener
            midJsize = (Nt+Nw)*Wsqrt/4;
        } else {
            midJsize = Wsqrt/2;
        }
    }

    JmaxofSp = 1;
    JmaxofSp = getJmaxofSp(midJsize)+0;
    uint128_t sizeJmax = dS[JmaxofSp];

    printf("\033[0;36m[sizeJmax]\033[0m S[%lu] = %lu (0x%lx%016lx)\n", (uint64_t) JmaxofSp, (uint64_t) sizeJmax, (uint64_t) (sizeJmax>>64), (uint64_t) (sizeJmax));

    uint128_t t[Nt];
    secp256k1_gej T[Nt];
    uint128_t dt[Nt];
    for (int k=0; k<Nt; k++) {

        if (Nt==1 && Nw==1) {
            t[k] = M;
        } else if (FLAG_PROFILE==1) { // byPollard
            t[k] = M + (k-0)*Nw;
        } else if (FLAG_PROFILE==2) { // NaiveSplitRange
            t[k] = L + (W1/(2*Nt)) + ((k-0)*W1/Nw);
        } else if (FLAG_PROFILE==3) { // byOorschot&Wiener
            t[k] = L + (W1/(2*Nt)) + ((k-0)*W1/Nw);
        } else {
            t[k] = M + randint(1, W1/2);
        }

        // T odd recommended (for more efficiency)
        if (t[k]%2 != 0 && FLAG_PROFILE!=1) {
            t[k]++;
        }

        int pt_init = 0;
        for (uint64_t a = 0; a<128; a++) {
            if (t[k]>>a & 1 == 1) {
                if (pt_init) {
                    secp256k1_gej_add_ge_var(&T[k], &T[k], &P0[a], NULL);
                } else {
                    secp256k1_gej_set_ge(&T[k], &P0[a]);
                    pt_init = 1;
                }
            }
        }
        dt[k] = 0;
    }

    uint128_t w[Nw];
    secp256k1_gej W[Nw];
    uint128_t dw[Nw];
    for (int k=0; k<Nw; k++) {
        if (Nt==1 && Nw==1) {
            w[k] = 1;
        } else if (FLAG_PROFILE==1) { // byPollard
            w[k] = 1 + Nt*(k-0);
        } else if (FLAG_PROFILE==2) { // NaiveSplitRange
            w[k] = 1 + ((k-0)*W1/Nt);
        } else if (FLAG_PROFILE==3) { // byOorschot&Wiener
            w[k] = 1 + ((k-0)*W1/Nt);
        } else {
            w[k] = randint(1, W1/2);
        }

        secp256k1_gej_set_ge(&W[k], &pubkeys[CHECK_BITS - 1]);
        for (uint64_t a = 0; a<128; a++) {
            if (w[k]>>a & 1 == 1) {
                secp256k1_gej_add_ge_var(&W[k], &W[k], &P0[a], NULL); // WARNING
            }
        }
        dw[k] = 0;
    }

    time0 = time(0);

    // Init Tame threads
    pthread_t tid_tame;
    for (int z = 0; z < Nt; z++) {
        struct Pairs *pair;
        pair = malloc(sizeof(struct Pairs));
        (*pair).z = z;
        (*pair).t = z;
        (*pair).tame_wild = 0;
        (*pair).K = &T[z];
        (*pair).n = &t[z];

        pthread_create(&tid_tame, NULL, KANGAROO, pair);
        usleep(5000);
    }

    // Init Wild threads
    pthread_t tid_wild;
    for (int z = 0; z < Nw; z++) {
        struct Pairs *pair;
        pair = malloc(sizeof(struct Pairs));
        (*pair).z = z;
        (*pair).t = z+Nt;
        (*pair).tame_wild = 1;
        (*pair).K = &W[z];
        (*pair).n = &w[z];

        pthread_create(&tid_wild, NULL, KANGAROO, pair);
        usleep(5000);
    }

    time1 = time(0);

    while (1) {
        usleep(50000);
        if (solved) break;

        time2 = time(0);
        if (time2 - time1 >= 5) {
            uint128_t tmpstatus = 0;
            for (int j=0; j<Nt+Nw; j++) tmpstatus += status[j];

            printf("\33[2K\r");
            time_format((time2 - time0));
            printf("  %.2f Mh/s   %.2f Mh %.1f%% T:%lu W:%lu", (float) ((tmpstatus - status_old) / (1000000.0*(time2 - time1))), (float) tmpstatus/1000000.0,
            (1.0*tmpstatus/(2*Wsqrt))*100,
            tame_count, wild_count);
            if (tmpstatus<2*Wsqrt)
                time_format((time2-time0)*(1.0-((double) tmpstatus/(2*Wsqrt)))/((double) tmpstatus/(2*Wsqrt)));

            fflush(stdout);
            time1 = time2;
            status_old = tmpstatus;
        }
    }
    exit(1);
}
