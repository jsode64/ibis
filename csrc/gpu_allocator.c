#include "gpu_allocator.h"

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
///
/// @param allocator The GPU allocator.
/// @param filter The memory type bit filter.
/// @param properties The required memory properties.
/// @param memory_type_index Output memory type index.
/// @return Whether a matching memory type index was found.
static bool find_memory_type_index(GpuAllocator* allocator, u32 filter,
                                   VkMemoryPropertyFlags properties, u32* memory_type_index);

/// Creates and allocates a Vulkan buffer.
///
/// @param allocator The GPU allocator.
/// @param size The target buffer size in bytes.
/// @param usage_flags Vulkan usage flags for the buffer.
/// @param memory_flags Vulkan memory property flags for the allocation.
/// @return A buffer allocation, or `NULL_BUFFER_ALLOCATION` on failure.
static BufferAllocation allocate_buffer(GpuAllocator* allocator, usize size,
                                        VkBufferUsageFlags usage_flags,
                                        VkMemoryPropertyFlags memory_flags);

GpuAllocator create_gpu_allocator(const Context* context) {
    GpuAllocator allocator = NULL_GPU_ALLOCATOR;

    allocator.device = context->device;

    vkGetPhysicalDeviceMemoryProperties(context->physical_device, &allocator.memory_properties);

    return allocator;
}

DynamicVbo allocate_dynamic_vbo(GpuAllocator* allocator, usize z, usize n) {
    if (allocator == NULL || allocator->device == VK_NULL_HANDLE || z == 0 || n == 0) {
        return NULL_DYNAMIC_VBO;
    }

    usize size = (z * n) + sizeof(VkDrawIndirectCommand);
    BufferAllocation allocation = allocate_buffer(
        allocator, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkBuffer buffer = allocation.buffer;
    VkDeviceMemory memory = allocation.memory;
    if (buffer == VK_NULL_HANDLE || memory == VK_NULL_HANDLE) {
        goto FAIL;
    }

    void* raw_data = NULL;
    if (!query_vk_result(vkMapMemory(allocator->device, memory, 0, size, 0, &raw_data))) {
        goto FAIL;
    }

    // Initialize the draw indirect command.
    VkDrawIndirectCommand* cmd = (VkDrawIndirectCommand*)raw_data;
    cmd->vertexCount = 0;
    cmd->instanceCount = 1;
    cmd->firstVertex = 0;
    cmd->firstInstance = 0;

    return (DynamicVbo){
        .device = allocator->device,
        .buffer = buffer,
        .memory = memory,
        .cmd = cmd,
        .vertex_size = z,
        .capacity = n,
        .length = 0,
        .data = (u8*)raw_data + sizeof(VkDrawIndirectCommand),
    };

    // Failure jump; cleans and returns a null dynamic VBO.
FAIL:
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(allocator->device, buffer, NULL);
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(allocator->device, memory, NULL);
    }
    return NULL_DYNAMIC_VBO;
}

bool find_memory_type_index(GpuAllocator* allocator, u32 filter, VkMemoryPropertyFlags properties,
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

BufferAllocation allocate_buffer(GpuAllocator* allocator, usize size,
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
