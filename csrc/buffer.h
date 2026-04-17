#pragma once

#include <vulkan/vulkan.h>

/// A Vulkan buffer.
typedef struct Buffer {
    /// The source device (must outlive the buffer).
    VkDevice device;

    /// The buffer handle.
    VkBuffer buffer;

    /// The allocated device memory.
    VkDeviceMemory memory;
} Buffer;

/// Destroys the buffer.
void destroy_buffer(Buffer* buffer);
