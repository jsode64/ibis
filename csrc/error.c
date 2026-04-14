#include "error.h"
#include <GLFW/glfw3.h>

static VkResult g_vk_error = VK_SUCCESS;

bool query_vk_result(VkResult result) {
    if (result == VK_SUCCESS) {
        return true;
    } else {
        g_vk_error = result;
        return false;
    }
}

void set_vk_error(VkResult error) { g_vk_error = error; }

i32 get_glfw_error() { return glfwGetError(NULL); }

u32 get_vk_error() {
    const u32 error = g_vk_error;
    g_vk_error = VK_SUCCESS;

    return error;
}