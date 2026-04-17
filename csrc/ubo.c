#include "ubo.h"
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "buffer.h"
#include "error.h"

Ubo* create_ubo(Buffer* buffer, const usize n, const usize z) {
    const usize data_size = z * n;
    Ubo* ubo = malloc(sizeof(Ubo) + data_size);
    if (!ubo) {
        return NULL;
    }

    *ubo = (Ubo){
        .buffer = *buffer,
        .size = z,
    };

    return ubo;
}

void destroy_ubo(Ubo* ubo) {
    vkDeviceWaitIdle(ubo->buffer.device);

    destroy_buffer(&ubo->buffer);
    free(ubo);
}

u8* ubo_get_data(Ubo* ubo) {
    return ubo->data;
}

bool ubo_write(Ubo* ubo, usize i) {
    // Get the mapped data.
    const usize offset = ubo->size * i;
    void* data = NULL;
    if (!query_vk_result(vkMapMemory(ubo->buffer.device, ubo->buffer.memory, offset, ubo->size, 0, &data))) {
        return false;
    }

    // Write data.
    memcpy(data, ubo->data, ubo->size);

    // Unmap.
    vkUnmapMemory(ubo->buffer.device, ubo->buffer.memory);

    return true;
}
