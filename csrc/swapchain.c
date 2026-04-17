#include "swapchain.h"
#include "error.h"
#include "util.h"
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

SwapchainInfo
get_swapchain_info(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const Window* window) {
    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

    // Get/calculate the extent.
    const bool extent_size_defined = capabilities.currentExtent.width != 0xFFFFFFFFu;
    const VkExtent2D extent = {
        .width = extent_size_defined ? capabilities.currentExtent.width
                                     : CLAMP(window->width,
                                             capabilities.minImageExtent.width,
                                             capabilities.maxImageExtent.width),
        .height = extent_size_defined ? capabilities.currentExtent.height
                                      : CLAMP(window->height,
                                              capabilities.minImageExtent.height,
                                              capabilities.maxImageExtent.height),
    };

    // Pick the best suited surface format.
    u32 num_surface_formats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, NULL);
    VkSurfaceFormatKHR* surface_formats = malloc(num_surface_formats * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &num_surface_formats, surface_formats);
    VkSurfaceFormatKHR surface_format = surface_formats[0];
    for (u32 i = 0; i < num_surface_formats; i++) {
        if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            surface_formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
            surface_format = surface_formats[i];
            break;
        }
    }
    free(surface_formats);

    // Pick the best suited present mode (mailbox if supported, FIFO otherwise).
    u32 num_present_modes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, NULL);
    VkPresentModeKHR* present_modes = malloc(num_present_modes * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &num_present_modes, present_modes);
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < num_present_modes; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = present_modes[i];
            break;
        }
    }
    free(present_modes);

    return (SwapchainInfo){
        .capabilities = capabilities,
        .extent = extent,
        .surface_format = surface_format,
        .present_mode = present_mode,
    };
}

Swapchain create_swapchain(const Context* context,
                           const VkRenderPass render_pass,
                           const SwapchainInfo* swapchain_info,
                           const VkSwapchainKHR previous,
                           const usize ideal_num_images) {
    Swapchain swapchain = NULL_SWAPCHAIN;
    VkImage* images = NULL;

    // Get the number of images.
    const u32 max_num_images = swapchain_info->capabilities.maxImageCount == 0
                                   ? UINT32_MAX
                                   : swapchain_info->capabilities.maxImageCount;
    const u32 num_images =
        CLAMP(ideal_num_images, swapchain_info->capabilities.minImageCount + 1, max_num_images);

    const u32 queue_family_indices[] = {
        context->graphics_queue_index,
        context->present_queue_index,
    };
    const VkSharingMode image_sharing_mode =
        context->graphics_queue_index == context->present_queue_index ? VK_SHARING_MODE_EXCLUSIVE
                                                                      : VK_SHARING_MODE_CONCURRENT;
    const VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = context->surface,
        .minImageCount = num_images,
        .imageFormat = swapchain_info->surface_format.format,
        .imageColorSpace = swapchain_info->surface_format.colorSpace,
        .imageExtent = swapchain_info->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = image_sharing_mode,
        .queueFamilyIndexCount = 2,
        .pQueueFamilyIndices = queue_family_indices,
        .preTransform = swapchain_info->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = swapchain_info->present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = previous,
    };

    if (!query_vk_result(
            vkCreateSwapchainKHR(context->device, &create_info, NULL, &swapchain.swapchain))) {
        goto FAIL;
    }

    // Get swapchain images.
    vkGetSwapchainImagesKHR(context->device, swapchain.swapchain, &swapchain.num_images, NULL);
    images = malloc(swapchain.num_images * sizeof(VkImage));
    vkGetSwapchainImagesKHR(context->device, swapchain.swapchain, &swapchain.num_images, images);

    // Create the rest of the structure (image view and framebuffer).
    swapchain.images = malloc(swapchain.num_images * sizeof(SwapchainImage));
    for (u32 i = 0; i < swapchain.num_images; i++) {
        VkComponentMapping component_mapping = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        };
        VkImageSubresourceRange subresource_range = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain_info->surface_format.format,
            .components = component_mapping,
            .subresourceRange = subresource_range,
        };
        VkImageView image_view = VK_NULL_HANDLE;
        if (!query_vk_result(
                vkCreateImageView(context->device, &image_view_create_info, NULL, &image_view))) {
            goto FAIL;
        }

        VkFramebufferCreateInfo framebuffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = render_pass,
            .attachmentCount = 1,
            .pAttachments = &image_view,
            .width = swapchain_info->extent.width,
            .height = swapchain_info->extent.height,
            .layers = 1,
        };
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (!query_vk_result(vkCreateFramebuffer(
                context->device, &framebuffer_create_info, NULL, &framebuffer))) {
            goto FAIL;
        }

        swapchain.images[i] = (SwapchainImage){
            .image = images[i],
            .image_view = image_view,
            .framebuffer = framebuffer,
        };
    }

    free(images);
    swapchain.extent = swapchain_info->extent;

    return swapchain;

    // Failure jump; cleans and returns a null swapchain.
FAIL:
    destroy_swapchain(&swapchain, context->device);
    if (images) {
        free(images);
    }
    return NULL_SWAPCHAIN;
}

void destroy_swapchain(Swapchain* swapchain, const VkDevice device) {
    // Destroy swapchain images.
    for (u32 i = 0; i < swapchain->num_images; i++) {
        vkDestroyFramebuffer(device, swapchain->images[i].framebuffer, NULL);
        vkDestroyImageView(device, swapchain->images[i].image_view, NULL);
    }
    if (swapchain->images) {
        free(swapchain->images);
    }

    vkDestroySwapchainKHR(device, swapchain->swapchain, NULL);
}
