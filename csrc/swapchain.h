#pragma once

#include "context.h"
#include "window.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

/// A platform's swapchain information.
typedef struct SwapchainInfo {
    /// The surface's swapchain capabilities.
    VkSurfaceCapabilitiesKHR capabilities;

    /// The surface's extent.
    VkExtent2D extent;

    /// The best suited surface format.
    VkSurfaceFormatKHR surface_format;

    /// The best suited present mode.
    VkPresentModeKHR present_mode;
} SwapchainInfo;

typedef struct SwapchainImage {
    VkImage image;

    VkImageView image_view;

    VkFramebuffer framebuffer;
} SwapchainImage;

typedef struct Swapchain {
    VkSwapchainKHR swapchain;

    u32 num_images;

    SwapchainImage* images;

    /// The swapchain's extent.
    VkExtent2D extent;
} Swapchain;

static const Swapchain NULL_SWAPCHAIN = {
    .swapchain = VK_NULL_HANDLE,
    .num_images = 0,
    .images = NULL,
    .extent =
        {
            .width = 0,
            .height = 0,
        },
};

/// Gets the swapchain information for the physical device and window surface.
///
/// @param physical_device The source physical device.
/// @param surface The source window surface.
/// @param window The target window.
/// @return Swapchain information for the given parameters.
SwapchainInfo
get_swapchain_info(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const Window* window);

/// Creates a swapchain for the given context.
Swapchain create_swapchain(const Context* context,
                           const VkRenderPass render_pass,
                           const SwapchainInfo* swapchain_info,
                           const VkSwapchainKHR previous,
                           usize num_images);

/// Destroys the swapchain.
void destroy_swapchain(Swapchain* swapchain, VkDevice device);
