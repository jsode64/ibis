#pragma once

#include "context.h"
#include "dynamic_vbo.h"
#include "renderer.h"
#include "ubo.h"
#include <vulkan/vulkan.h>

/// A GPU allocator.
typedef struct GpuAllocator {
    /// The source device. Must outlive the allocator.
    VkDevice device;

    /// The physical device's memory properties.
    VkPhysicalDeviceMemoryProperties memory_properties;
} GpuAllocator;

/// Creates a GPU allocator for the context.
GpuAllocator create_gpu_allocator(const Context* context);

/// Creates a new dynamic VBO with the given target renderer, vertex size, and capacity.
DynamicVbo*
allocate_dynamic_vbo(const GpuAllocator* allocator, const Renderer* renderer, usize z, usize c);

/// Creates a new UBO with the given target renderer and vertex size.
Ubo* allocate_ubo(const GpuAllocator allocator, const Renderer* renderer, usize z);
