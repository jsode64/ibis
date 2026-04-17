#pragma once

#include "buffer.h"
#include "def.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

/// A dynamically updated vertex buffer.
typedef struct DynamicVbo {
    /// The buffer.
    Buffer buffer;

    /// The data capacity.
    usize c;

    /// The size of each element.
    usize z;

    /// The current length of the data.
    usize n;

    /// A pointer to the head of the vertex data.
    u8 data[];
} DynamicVbo;

/// Creates a new dynamic VBO with the given buffer, vertex size, and capacity.
DynamicVbo* create_dynamic_vbo(Buffer* buffer, usize z, usize c);

/// Destroys the dynamic VBO.
void destroy_dynamic_vbo(DynamicVbo* vbo);

/// Returns a pointer to the dynamic vertex VBO's vertex data.
u8* dynamic_vbo_get_data(DynamicVbo* vbo);

/// Returns the size of one of the dynamic VBO's frames.
usize dynamic_vbo_frame_size(const DynamicVbo* vbo);

/// Writes the dynamic VBO's data to its frame at the given index.
bool dynamic_vbo_write(DynamicVbo* vbo, usize i);

/// Pushes the vertices to the dynamic VBO's vertex data.
void dynamic_vbo_push(DynamicVbo* vbo, usize n, const void* p);
