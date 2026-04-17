#pragma once

#include "buffer.h"
#include "def.h"

/// A uniform buffer object.
typedef struct Ubo {
    /// The buffer.
    Buffer buffer;

    /// The size of the buffer's data.
    usize size;

    /// The buffer data.
    u8 data[];
} Ubo;

/// Creates a UBO with the given buffer, number of frames, and data size.
Ubo* create_ubo(Buffer* buffer, usize n, usize z);

/// Destroys the UBO.
void destroy_ubo(Ubo* ubo);

/// Returns a pointer to the UBO's data.
u8* ubo_get_data(Ubo* ubo);

/// Writes the UBO's data to the given frame index.
bool ubo_write(Ubo* ubo, usize i);
