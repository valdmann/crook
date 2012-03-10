#ifndef CONFIG_HPP
#define CONFIG_HPP

// disable asserts for a moderate speed-up:
#define NDEBUG

#include <cassert>
#include <cstdio>
#include <stdint.h>

// since crook is single-threaded it won't need thread-safe I/O; with
// GNU libc (and others?) the _unlocked variants are much faster.
#ifdef __GLIBC__
#define putc putc_unlocked
#define getc getc_unlocked
#endif

// same sort of thing for Windows:
#ifdef _MSC_VER
#define putc _fputc_nolock
#define getc _fgetc_nolock
#endif

using namespace std;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

// Fractional numbers are represented in fixed-point form and the
// following naming scheme is used for relevant constants:
//   _BITS  - total bits for both the whole and the fractional parts together
//   _SCALE - unit, i.e. the representation for the number 1
//            if omitted then it's just 1 (i.e. there's no fractional part)
//   _LIMIT - one larger than the largest number that can be represented
//            if omitted then equal to _SCALE (i.e. there's no whole part)

// probabilities in the range coder, see "rc.hpp".
const U32 ARI_P_BITS  = 12;
const U32 ARI_P_SCALE = 1 << ARI_P_BITS;

// used for the reciprocal table, see "divide.hpp".
const U32 DIVISOR_BITS     = 10;
const U32 DIVISOR_LIMIT    = 1 << DIVISOR_BITS;
const U32 RECIPROCAL_BITS  = 15;
const U32 RECIPROCAL_LIMIT = 1 << RECIPROCAL_BITS;

// probabilities and counts in the model, see "model.hpp".
const U32 PPM_P_BITS  = 22;
const U32 PPM_C_BITS  = 10;
const U32 PPM_P_SCALE = 1 << PPM_P_BITS;
const U32 PPM_C_LIMIT = 1 << PPM_C_BITS;
const U32 PPM_C_SCALE = 32;
const U32 PPM_P_MASK  = (PPM_P_SCALE  - 1) << PPM_C_BITS;
const U32 PPM_C_MASK  = PPM_C_LIMIT - 1;
const U32 PPM_P_START = PPM_P_SCALE / 2;
const U32 PPM_C_START = PPM_C_SCALE * 12;     // these were hand-tuned
const U32 PPM_C_INH   = PPM_C_SCALE * 3 / 2;  // on enwik7
const U32 PPM_C_INC   = PPM_C_SCALE;

// global command line options, defined in "crook.cpp".
extern int command;     // 'c' or 'd'
extern int memoryLimit; // memory limit in MiB
extern int orderLimit;  //  order limit in bytes

#endif
