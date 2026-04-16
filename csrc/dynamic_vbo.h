#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "def.h"

/// A dynamically updated vertex buffer.
typedef struct DynamicVbo {
    /// The source device. Must outlive the dynamic vertex buffer.
    VkDevice device;

    /// The Vulkan buffer object.
    VkBuffer buffer;

    /// The allocated device memory.
    VkDeviceMemory memory;

    /// The number of frames.
    usize num_frames;

    /// The index of the next frame to write to.
    usize frame_index;

    /// The data capacity.
    usize capacity;

    /// The size of each element.
    usize size;

    /// The current length of the data.
    usize length;

    /// A pointer to the head of the vertex data.
    u8 data[];
} DynamicVbo;

/// Creates a new dynamic vertex buffer.
///
/// @param device The source device.
/// @param buffer The vertex buffer.
/// @param memory The allocated memory for the buffer.
/// @param num_frames The number of frames in the buffer.
/// @param size The size of each element in the vertex buffer.
/// @param capacity The buffer's capacity.
/// @return A pointer to a new dynamic VBO, or a null pointer on failure.
DynamicVbo* create_dynamic_vbo(
    const VkDevice device,
    const VkBuffer buffer,
    const VkDeviceMemory memory,
    usize num_frames,
    usize size,
    usize capacity
);

/// Destroys the dynamic vertex buffer.
void destroy_dynamic_vbo(DynamicVbo* vbo);

/// Returns a pointer to the dynamic vertex buffer's vertex data.
u8* get_dynamic_vbo_data(DynamicVbo* vbo);

/// Returns the offset of the dynamic vertex buffer's next frame.
usize get_dynamic_vbo_offset(const DynamicVbo* vbo);

/// Points the dynamic vertex buffer to start writing to its next frame.
void redirect_dynamic_vbo(DynamicVbo* vbo);

/// Writes the dynamic vertex buffer's data to its buffer.
void write_dynamic_vbo(DynamicVbo* vbo);

/// Clears the dynamic vertex buffer's data, removing all elements.
void clear_dynamic_vbo(DynamicVbo* vbo);

/// Writes the vertices into the vertex data.
///
/// @param n The number of vertices to write.
/// @param p A pointer to the vertices.
void push_to_dynamic_vbo(DynamicVbo* vbo, usize n, const void* p);
