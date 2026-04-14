#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Primitives:

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ptrdiff_t isize;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

typedef uint32_t b32;

typedef float f32;
typedef double f64;

typedef const char* cstr;

// Types defined in Rust:

/// A version consisting of variant, major, minor, and patch.
///
/// Defined in `src/version.rs`.
typedef struct Version {
    u16 major;
    u16 minor;
    u16 patch;
} Version;
