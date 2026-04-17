#include "dynamic_vbo.h"
#include "error.h"
#include "util.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

DynamicVbo* create_dynamic_vbo(Buffer* buffer, const usize z, const usize c) {
    const usize data_size = z * c;
    DynamicVbo* vbo = malloc(sizeof(DynamicVbo) + data_size);
    if (vbo == NULL) {
        return NULL;
    }

    *vbo = (DynamicVbo){
        .buffer = *buffer,
        .c = c,
        .z = z,
        .n = 0,
    };
    return vbo;
}

void destroy_dynamic_vbo(DynamicVbo* vbo) {
    vkDeviceWaitIdle(vbo->buffer.device);

    vkDestroyBuffer(vbo->buffer.device, vbo->buffer.buffer, NULL);
    vkFreeMemory(vbo->buffer.device, vbo->buffer.memory, NULL);
    free(vbo);
}

u8* dynamic_vbo_get_data(DynamicVbo* vbo) { return vbo->data; }

usize dynamic_vbo_frame_size(const DynamicVbo* vbo) {
    return sizeof(VkDrawIndirectCommand) + (vbo->z * vbo->c);
}

bool dynamic_vbo_write(DynamicVbo* vbo, const usize i) {
    // Get the mapped data.
    const usize offset = dynamic_vbo_frame_size(vbo) * i;
    const usize vertex_data_bytes = vbo->z * vbo->n;
    const usize data_size = sizeof(VkDrawIndirectCommand) + vertex_data_bytes;
    void* data = NULL;
    if (!query_vk_result(
            vkMapMemory(vbo->buffer.device, vbo->buffer.memory, offset, data_size, 0, &data))) {
        return false;
    }

    // Write command and vertex data.
    u8* head = (u8*)data;
    VkDrawIndirectCommand* command = (VkDrawIndirectCommand*)head;
    *command = (VkDrawIndirectCommand){
        .vertexCount = vbo->n,
        .instanceCount = 1,
        .firstVertex = 0,
        .firstInstance = 0,
    };
    memcpy(head + sizeof(VkDrawIndirectCommand), vbo->data, vertex_data_bytes);

    // Unmap.
    vkUnmapMemory(vbo->buffer.device, vbo->buffer.memory);

    return true;
}

void dynamic_vbo_push(DynamicVbo* vbo, const usize n, const void* p) {
    void* dst = (void*)(vbo->data + (vbo->n * vbo->z));
    const usize data_size = n * vbo->z;
    memcpy(dst, p, data_size);
    vbo->n += n;
}
