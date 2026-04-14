#include "dynamic_vbo.h"
#include <string.h>
#include <vulkan/vulkan_core.h>

void destroy_dynamic_vbo(DynamicVbo* vbo) {
    vkDestroyBuffer(vbo->device, vbo->buffer, NULL);
    vkFreeMemory(vbo->device, vbo->memory, NULL);
}

void dynamic_vbo_push_vertices(DynamicVbo* vbo, void* vertices, usize n) {
    memcpy(vbo->data + (vbo->length * vbo->vertex_size), vertices, n * vbo->vertex_size);
    vbo->length += n;
    vbo->cmd->vertexCount = (u32)vbo->length;
    vbo->cmd->instanceCount = 1;
}

void dynamic_vbo_clear(DynamicVbo* vbo) {
    vbo->cmd->vertexCount = 0;
    vbo->cmd->instanceCount = 1;
    vbo->length = 0;
}
