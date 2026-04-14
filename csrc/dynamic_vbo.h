#pragma once

#include "def.h"
#include <vulkan/vulkan.h>

/// A dynamic vertex buffer with a draw indirect command.
typedef struct DynamicVbo {
    /// The source device. Must outlive the dynamic vertex buffer.
    const VkDevice device;

    VkBuffer buffer;

    VkDeviceMemory memory;

    /// The draw indirect command info.
    VkDrawIndirectCommand* cmd;

    /// The size of each vertex..
    usize vertex_size;

    /// The capacity of the vector.
    usize capacity;

    /// The current number of stored vertices.
    usize length;

    /// The head of the vertex data.
    u8* data;
} DynamicVbo;

static const DynamicVbo NULL_DYNAMIC_VBO = {
    .device = VK_NULL_HANDLE,
    .buffer = VK_NULL_HANDLE,
    .memory = VK_NULL_HANDLE,
    .cmd = NULL,
    .vertex_size = 0,
    .capacity = 0,
    .length = 0,
    .data = NULL,
};

/// Destroys the dynamic vertex buffer.
///
/// @param vbo The vertex buffer.
void destroy_dynamic_vbo(DynamicVbo* vbo);

/// Pushes the vertices into the buffer.
///
/// @param vbo The vertex buffer.
/// @param vertices A pointer to the vertices.
/// @param n The number of vertices in the data.
void dynamic_vbo_push_vertices(DynamicVbo* vbo, void* vertices, usize n);

/// Clears the vertex buffer.
///
/// @param vbo The vertex buffer.
void dynamic_vbo_clear(DynamicVbo* vbo);
