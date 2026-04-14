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

/// Creates a new dynamic vertex buffer for the given data.
///
/// The returned buffer will have an indirect command at its start.
///
/// @param allocator The GPU allocator.
/// @param z The size each vertex.
/// @param n The capacity of the vertex buffer.
/// @return A new dynamic vertex buffer, or `NULL_DYNAMIC_VBO` on failure.
DynamicVbo allocate_dynamic_vbo(GpuAllocator* allocator, usize z, usize n);
