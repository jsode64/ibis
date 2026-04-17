#include "buffer.h"
#include <vulkan/vulkan_core.h>

void destroy_buffer(Buffer* buffer) {
    vkDeviceWaitIdle(buffer->device);

    vkDestroyBuffer(buffer->device, buffer->buffer, NULL);
    vkFreeMemory(buffer->device, buffer->memory, NULL);
}
