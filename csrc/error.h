#pragma once

#include "def.h"
#include <vulkan/vulkan.h>

/// Queries the `vk::Result`. If it's an error, stores it. If not, does nothing.
///
/// @param result The result being queried.
/// @return Whether the result was a success.
bool query_vk_result(VkResult result);

/// Sets the stored Vulkan error.
///
/// @param error The error to store.
void set_vk_error(VkResult error);

/// Returns the last GLFW error and removes it.
///
/// @return A GLFW error or 0 for no error.
i32 get_glfw_error();

/// Returns the last Vulkan error and removes it.
///
/// @return A `vk::Result` error or 0 (`VK_SUCCESS`) for no error.
u32 get_vk_error();
