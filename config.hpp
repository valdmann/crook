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

#endif
