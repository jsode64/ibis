#pragma once

/// Returns the smaller of the two values.
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

/// Returns the larger of the two values.
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

/// Returns the value clamped between the ranges.
#define CLAMP(v, min, max) (MIN(MAX(v, min), max))
