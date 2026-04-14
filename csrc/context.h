#pragma once

#include "def.h"
#include "window.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

/// A builder for `Context`.
typedef struct ContextBuilder {
    cstr app_name;

    Version app_version;

    bool do_debug_messaging;
} ContextBuilder;

/// A Vulkan context.
typedef struct Context {
    VkInstance instance;

    VkDebugUtilsMessengerEXT debug_messenger;

    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device;

    u32 compute_queue_index;

    u32 graphics_queue_index;

    u32 present_queue_index;

    VkDevice device;

    VkQueue compute_queue;

    VkQueue graphics_queue;

    VkQueue present_queue;
} Context;

static const Context NULL_CONTEXT = {
    .instance = VK_NULL_HANDLE,
    .physical_device = VK_NULL_HANDLE,
    .device = VK_NULL_HANDLE,
};

/// Creates and returns a context from the given builder.
///
/// @param builder The context builder.
/// @return A new context. On failure, it will contain only null handles.
Context create_context(const ContextBuilder* builder, const Window* window);

/// Destroys the context.
///
/// @param context The context to be destroyed.
void destroy_context(Context* context);
