#include "dynamic_vbo.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "error.h"

DynamicVbo* create_dynamic_vbo(
    const VkDevice device,
    const VkBuffer buffer,
    const VkDeviceMemory memory,
    const usize num_frames,
    const usize size,
    const usize capacity
) {
    const usize data_size = size * capacity;
    DynamicVbo* vbo = malloc(sizeof(DynamicVbo) + data_size);
    if (vbo == NULL) {
        return NULL;
    }

    *vbo = (DynamicVbo){
        .device = device,
        .buffer = buffer,
        .memory = memory,
        .num_frames = num_frames,
        .frame_index = 0,
        .capacity = capacity,
        .size = size,
        .length = 0,
    };
    return vbo;
}

void destroy_dynamic_vbo(DynamicVbo* vbo) {
    vkDestroyBuffer(vbo->device, vbo->buffer, NULL);
    vkFreeMemory(vbo->device, vbo->memory, NULL);
    free(vbo);
}

u8* get_dynamic_vbo_data(DynamicVbo* vbo) {
    return vbo->data;
}

usize get_dynamic_vbo_offset(const DynamicVbo* vbo) {
    return vbo->size * vbo->capacity * vbo->frame_index;
}

void redirect_dynamic_vbo(DynamicVbo* vbo) {
    vbo->frame_index = (vbo->frame_index + 1) % vbo->num_frames;
}

void write_dynamic_vbo(DynamicVbo* vbo) {
    // Point to the next frame.
    vbo->frame_index = (vbo->frame_index + 1) % vbo->num_frames;

    // Get the mapped data.
    const usize frame_size = sizeof(VkDrawIndirectCommand) + (vbo->size * vbo->capacity);
    const usize offset = frame_size * vbo->frame_index;
    const usize vertex_bytes = vbo->size * vbo->length;
    const usize data_size = sizeof(VkDrawIndirectCommand) + vertex_bytes;
    u8* head = NULL;
    if (!query_vk_result(vkMapMemory(vbo->device, vbo->memory, offset, data_size, 0, (void**)&head))) {
        return;
    }

    // Write command and vertex data.
    VkDrawIndirectCommand* command = (VkDrawIndirectCommand*)head;
    *command = (VkDrawIndirectCommand){
        .vertexCount = vbo->length,
        .instanceCount = 1,
        .firstVertex = 0,
        .firstInstance = 0,
    };
    void* data = (void*)(head + sizeof(VkDrawIndirectCommand));
    memcpy(data, vbo->data, vertex_bytes);
    
    vkUnmapMemory(vbo->device, vbo->memory);
}

void clear_dynamic_vbo(DynamicVbo* vbo) {
    vbo->length = 0;
}

void push_to_dynamic_vbo(DynamicVbo* vbo, const usize n, const void* p) {
    void* dst = (void*)(vbo->data + (vbo->length * vbo->size));
    const usize data_size = n * vbo->size;
    memcpy(dst, p, data_size);
    vbo->length += n;
}
