#pragma once

#include "context.h"
#include "dynamic_vbo.h"
#include <vulkan/vulkan.h>

/// A GPU allocator.
typedef struct GpuAllocator {
    /// The source device. Must outlive the allocator.
    VkDevice device;

    /// The physical device's memory properties.
    VkPhysicalDeviceMemoryProperties memory_properties;
} GpuAllocator;

/// Creates a GPU allocator for the context.
///
/// @param context The source context. Must outlive the allocator.
/// @return A new GPU allocator.
GpuAllocator create_gpu_allocator(const Context* context);

/// Creates a new dynamic vertex buffer.
///
/// @param allocator The GPU allocator.
/// @param num_frames The number of frames in the buffer.
/// @param z The size each vertex.
/// @param n The capacity of the vertex buffer.
/// @return A pointer to a new dynamic vertex buffer, or a null pointer on failure.
DynamicVbo* allocate_dynamic_vbo(const GpuAllocator* allocator, usize num_frames, usize z, usize n);
