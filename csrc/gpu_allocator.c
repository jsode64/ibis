#include "gpu_allocator.h"

#include "buffer.h"
#include "error.h"
#include <vulkan/vulkan_core.h>

/// A null GPU allocator.
static const GpuAllocator NULL_GPU_ALLOCATOR = {
    .device = VK_NULL_HANDLE,
    .memory_properties = {},
};

/// Return value for `allocate_buffer`.
typedef struct BufferAllocation {
    VkBuffer buffer;
    VkDeviceMemory memory;
} BufferAllocation;

static const BufferAllocation NULL_BUFFER_ALLOCATION = {
    .buffer = VK_NULL_HANDLE,
    .memory = VK_NULL_HANDLE,
};

/// Returns a matching memory type index for the flags.
static bool find_memory_type_index(const GpuAllocator* allocator, u32 filter,
                                   VkMemoryPropertyFlags properties, u32* memory_type_index);

/// Creates and allocates a Vulkan buffer.
static BufferAllocation allocate_buffer(const GpuAllocator* allocator, usize size,
                                        VkBufferUsageFlags usage_flags,
                                        VkMemoryPropertyFlags memory_flags);

GpuAllocator create_gpu_allocator(const Context* context) {
    GpuAllocator allocator = NULL_GPU_ALLOCATOR;

    allocator.device = context->device;

    vkGetPhysicalDeviceMemoryProperties(context->physical_device, &allocator.memory_properties);

    return allocator;
}

DynamicVbo* allocate_dynamic_vbo(
    const GpuAllocator* allocator,
    const Renderer* renderer,
    const usize z,
    const usize c
) {
    // Create buffer and memory allocation.
    const usize frame_size = (z * c) + sizeof(VkDrawIndirectCommand);
    const usize size = frame_size * renderer->swapchain.num_images;
    BufferAllocation allocation = allocate_buffer(
        allocator,
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (allocation.buffer == VK_NULL_HANDLE || allocation.memory == VK_NULL_HANDLE) {
        goto FAIL;
    }

    // Create the dynamic VBO.
    Buffer buffer = {
        .device = allocator->device,
        .buffer = allocation.buffer,
        .memory = allocation.memory
    };
    DynamicVbo* vbo = create_dynamic_vbo(
        &buffer,
        z,
        c
    );
    if (vbo == NULL) {
        goto FAIL;
    }

    return vbo;

    // Failure jump; cleans and returns a null dynamic VBO.
FAIL:
    vkDestroyBuffer(allocator->device, allocation.buffer, NULL);
    vkFreeMemory(allocator->device, allocation.memory, NULL);
    return NULL;
}

Ubo* allocate_ubo(const GpuAllocator allocator, const Renderer* renderer, usize z);

bool find_memory_type_index(const GpuAllocator* allocator, u32 filter, VkMemoryPropertyFlags properties,
                            u32* memory_type_index) {
    for (u32 i = 0; i < allocator->memory_properties.memoryTypeCount; i++) {
        const bool bit_matches = (filter & (1u << i)) != 0;
        const bool flags_match =
            (allocator->memory_properties.memoryTypes[i].propertyFlags & properties) == properties;
        if (bit_matches && flags_match) {
            *memory_type_index = i;
            return true;
        }
    }

    return false;
}

BufferAllocation allocate_buffer(const GpuAllocator* allocator, usize size,
                                 VkBufferUsageFlags usage_flags,
                                 VkMemoryPropertyFlags memory_flags) {
    BufferAllocation allocation = NULL_BUFFER_ALLOCATION;
    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = size,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
    };
    if (!query_vk_result(
            vkCreateBuffer(allocator->device, &create_info, NULL, &allocation.buffer))) {
        return NULL_BUFFER_ALLOCATION;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(allocator->device, allocation.buffer, &memory_requirements);

    u32 memory_type_index = 0;
    if (!find_memory_type_index(allocator, memory_requirements.memoryTypeBits, memory_flags,
                                &memory_type_index)) {
        set_vk_error(VK_ERROR_INITIALIZATION_FAILED);
        vkDestroyBuffer(allocator->device, allocation.buffer, NULL);
        return NULL_BUFFER_ALLOCATION;
    }

    // Allocate memory.
    VkMemoryAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = memory_type_index,
    };
    if (!query_vk_result(
            vkAllocateMemory(allocator->device, &allocate_info, NULL, &allocation.memory))) {
        vkDestroyBuffer(allocator->device, allocation.buffer, NULL);
        return NULL_BUFFER_ALLOCATION;
    }

    // Bind buffer to memory.
    if (!query_vk_result(
            vkBindBufferMemory(allocator->device, allocation.buffer, allocation.memory, 0))) {
        vkFreeMemory(allocator->device, allocation.memory, NULL);
        vkDestroyBuffer(allocator->device, allocation.buffer, NULL);
        return NULL_BUFFER_ALLOCATION;
    }

    return allocation;
}
