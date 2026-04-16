#include "renderer.h"
#include "command.h"
#include "context.h"
#include "error.h"
#include "swapchain.h"
#include <stdint.h>
#include <stdlib.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define TIMEOUT_NANOS 2000000000

/// Returns whether the given `VkResult` is caused by an outdated swapchain.
///
/// @param result The `VkResult` to check.
/// @return Whether the result is caused by an outdated swapchain.
static bool is_vk_error_outdated_swapchain(VkResult result);

/// Creates a render pass for the window surface format.
///
/// @param device The logical device.
/// @param format The window surface format.
/// @return A new render pass, or `VK_NULL_HANDLE` on failure.
static VkRenderPass create_render_pass(const VkDevice device, VkFormat format);

/// Creates a command pool.
///
/// @param device The logical device.
/// @param queue_family_index The queue family index to target.
/// @return A new command pool, or `VK_NULL_HANDLE` on failure.
static VkCommandPool create_command_pool(const VkDevice device, u32 queue_family_index);

/// Creates N frames in an array.
///
/// @param device The logical device.
/// @param n The number of frames to create.
/// @return An array of N frames, or a null pointer on failure.
static Frame* create_frames(const VkDevice device, usize n);

/// Creates N command buffers in an array.
///
/// @param device The logical device.
/// @param command_pool The command pool.
/// @param n The number of command buffers to allocate.
/// @return An array of command buffers, or a null pointer on failure.
static VkCommandBuffer* create_command_buffers(const VkDevice device,
                                               const VkCommandPool command_pool, usize n);

/// Destroys the frame array.
/// This also frees the passed pointer.
///
/// @param frames The frames to destroy.
/// @param device The logical device.
/// @param n The number of frames in the array.
static void destroy_frames(Frame* frames, const VkDevice device, usize n);

#define N_FRAMES 3

Renderer create_renderer(const Context* context, const Window* window) {
    Renderer renderer = NULL_RENDERER;
    renderer.context = context;
    renderer.num_frames = N_FRAMES;
    renderer.frame_index = 0;

    const SwapchainInfo swapchain_info =
        get_swapchain_info(context->physical_device, context->surface, window);

    renderer.render_pass =
        create_render_pass(context->device, swapchain_info.surface_format.format);
    if (renderer.render_pass == VK_NULL_HANDLE) {
        goto FAIL;
    }

    renderer.swapchain =
        create_swapchain(context, renderer.render_pass, &swapchain_info, VK_NULL_HANDLE);
    if (renderer.swapchain.swapchain == VK_NULL_HANDLE) {
        goto FAIL;
    }

    renderer.command_pool = create_command_pool(context->device, context->graphics_queue_index);
    if (renderer.command_pool == VK_NULL_HANDLE) {
        goto FAIL;
    }

    renderer.frames = create_frames(context->device, renderer.num_frames);
    if (renderer.frames == NULL) {
        goto FAIL;
    }

    renderer.num_command_buffers = renderer.swapchain.num_images;
    renderer.command_buffers = create_command_buffers(context->device, renderer.command_pool,
                                                      renderer.num_command_buffers);
    if (renderer.command_buffers == NULL) {
        goto FAIL;
    }

    renderer_reload_commands(&renderer);

    return renderer;

    // Failure jump; cleans and returns a null renderer.
FAIL:
    destroy_renderer(&renderer);
    return NULL_RENDERER;
}

void destroy_renderer(Renderer* renderer) {
    vkDeviceWaitIdle(renderer->context->device);

    if (renderer->commands != NULL) {
        free(renderer->commands);
    }

    if (renderer->command_buffers != NULL) {
        free(renderer->command_buffers);
    }

    destroy_frames(renderer->frames, renderer->context->device, renderer->num_frames);
    vkDestroyCommandPool(renderer->context->device, renderer->command_pool, NULL);

    destroy_swapchain(&renderer->swapchain, renderer->context->device);
    if (renderer->render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(renderer->context->device, renderer->render_pass, NULL);
    }

    *renderer = NULL_RENDERER;
}

u64* renderer_commands_flush(Renderer* renderer, const usize n) {
    if (renderer->commands != NULL) {
        free(renderer->commands);
        renderer->commands = NULL;
        renderer->num_commands = 0;
    }

    if (n == 0) {
        return NULL;
    }

    // Allocate new data and return it for filling.
    u64* data = malloc(n * sizeof(u64));
    renderer->commands = data;
    renderer->num_commands = n;
    return data;
}

RenderBeginInfo renderer_begin(Renderer* renderer) {
    // Get and wait for the next frame.
    const usize frame_index = renderer->frame_index;
    const Frame* frame = &renderer->frames[frame_index];
    renderer->frame_index = (renderer->frame_index + 1) % renderer->num_frames;
    if (!query_vk_result(vkWaitForFences(renderer->context->device, 1, &frame->in_flight, VK_TRUE, 2000000000))) {
        return NULL_RENDER_BEGIN_INFO;
    }

    // Acquire the next swapchain image index.
    u32 image_index = 0;
    VkAcquireNextImageInfoKHR acquire_info = {
        .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        .pNext = NULL,
        .swapchain = renderer->swapchain.swapchain,
        .timeout = TIMEOUT_NANOS,
        .semaphore = frame->image_available,
        .fence = VK_NULL_HANDLE,
        .deviceMask = 1,
    };
    VkResult acquire_image_result = vkAcquireNextImage2KHR(
        renderer->context->device,
        &acquire_info,
        &image_index
    );

    return (RenderBeginInfo){
        .frame_index = frame_index,
        .image_index = image_index,
        .is_swapchain_outdated = is_vk_error_outdated_swapchain(acquire_image_result),
        .is_valid = true,
    };
}

bool renderer_draw(Renderer* renderer, const RenderBeginInfo* begin_info) {
    // If the swapchain is outdated, recreate it and skip rendering.
    if (begin_info->is_swapchain_outdated) {
        return renderer_recreate_swapchain(renderer);
    }

    // Reset fence.
    const Frame* frame = &renderer->frames[begin_info->frame_index];
    vkResetFences(renderer->context->device, 1, &frame->in_flight);

    // Submit commands.
    const VkCommandBuffer command_buffer = renderer->command_buffers[begin_info->image_index];
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame->image_available,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &frame->render_finished,
    };
    if (!query_vk_result(
            vkQueueSubmit(renderer->context->graphics_queue, 1, &submit_info, frame->in_flight))) {
        return false;
    }

    // Present.
    const u32 image_index = (u32)begin_info->image_index;
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame->render_finished,
        .swapchainCount = 1,
        .pSwapchains = &renderer->swapchain.swapchain,
        .pImageIndices = &image_index,
        .pResults = NULL,
    };
    const VkResult present_result = vkQueuePresentKHR(renderer->context->present_queue, &present_info);

    // Recreate swapchain if outdated.
    if (is_vk_error_outdated_swapchain(present_result)) {
        return renderer_recreate_swapchain(renderer);
    }

    return true;
}

bool renderer_reload_commands(Renderer* renderer) {
    vkDeviceWaitIdle(renderer->context->device);

    for (usize i = 0; i < renderer->num_command_buffers; i++) {
        const VkCommandBuffer command_buffer = renderer->command_buffers[i];
        if (!query_vk_result(vkResetCommandBuffer(command_buffer, 0))) {
            return false;
        }

        // Begin the command buffer.
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = 0,
            .pInheritanceInfo = NULL,
        };
        if (!query_vk_result(vkBeginCommandBuffer(command_buffer, &begin_info))) {
            return false;
        }

        // Send dynamic states.
        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = renderer->swapchain.extent.width,
            .height = renderer->swapchain.extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        VkRect2D scissor = {
            .offset =
                {
                    .x = 0,
                    .y = 0,
                },
            .extent = renderer->swapchain.extent,
        };
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        // Begin the render pass.
        VkClearValue clear_value = {
            .color =
                {
                    .float32 = {0.0f, 0.0f, 0.0f, 1.0f},
                },
        };
        VkRenderPassBeginInfo render_pass_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = NULL,
            .renderPass = renderer->render_pass,
            .framebuffer = renderer->swapchain.images[i].framebuffer,
            .renderArea = scissor,
            .clearValueCount = 1,
            .pClearValues = &clear_value,
        };
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // Add encoded commands.
        if (!fill_command_buffer(command_buffer, i, renderer->num_commands, renderer->commands)) {
            return false;
        }

        // End render pass and command buffer.
        vkCmdEndRenderPass(command_buffer);
        if (!query_vk_result(vkEndCommandBuffer(command_buffer))) {
            return false;
        }
    }

    return true;
}

bool renderer_recreate_swapchain(Renderer* renderer) {
    vkDeviceWaitIdle(renderer->context->device);

    SwapchainInfo swapchain_info = get_swapchain_info(
        renderer->context->physical_device, renderer->context->surface, renderer->context->window);
    Swapchain new_swapchain = create_swapchain(renderer->context, renderer->render_pass,
                                               &swapchain_info, renderer->swapchain.swapchain);
    if (new_swapchain.swapchain == VK_NULL_HANDLE) {
        return false;
    }
    destroy_swapchain(&renderer->swapchain, renderer->context->device);
    renderer->swapchain = new_swapchain;

    if (!renderer_reload_commands(renderer)) {
        return false;
    }

    return true;
}

bool is_vk_error_outdated_swapchain(VkResult result) {
    return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR;
}

VkRenderPass create_render_pass(const VkDevice device, const VkFormat format) {
    const VkAttachmentDescription attachment_description = {
        .flags = 0,
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    const VkAttachmentReference attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass_description = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_reference,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = NULL,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
    };
    const VkSubpassDependency subpass_dependancy = {
        .srcSubpass = UINT32_MAX,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    const VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &attachment_description,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = 1,
        .pDependencies = &subpass_dependancy,
    };

    VkRenderPass render_pass = VK_NULL_HANDLE;
    if (query_vk_result(vkCreateRenderPass(device, &create_info, NULL, &render_pass))) {
        return render_pass;
    } else {
        return VK_NULL_HANDLE;
    }
}

VkCommandPool create_command_pool(const VkDevice device, const u32 queue_family_index) {
    const VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_index,
    };

    VkCommandPool command_pool = VK_NULL_HANDLE;
    if (query_vk_result(vkCreateCommandPool(device, &create_info, NULL, &command_pool))) {
        return command_pool;
    } else {
        return VK_NULL_HANDLE;
    }
}

Frame* create_frames(const VkDevice device, const usize n) {
    // Create frames.
    Frame* frames = calloc(n, sizeof(Frame));
    if (frames == NULL) {
        return NULL;
    }

    for (usize i = 0; i < n; i++) {
        const VkSemaphoreCreateInfo semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
        };
        VkSemaphore image_available = VK_NULL_HANDLE;
        VkSemaphore render_finished = VK_NULL_HANDLE;
        if (!query_vk_result(
                vkCreateSemaphore(device, &semaphore_create_info, NULL, &image_available)) ||
            !query_vk_result(
                vkCreateSemaphore(device, &semaphore_create_info, NULL, &render_finished))) {
            goto FAIL;
        }

        const VkFenceCreateInfo fence_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VkFence in_flight = VK_NULL_HANDLE;
        if (!query_vk_result(vkCreateFence(device, &fence_create_info, NULL, &in_flight))) {
            goto FAIL;
        }

        frames[i] = (Frame){
            .image_available = image_available,
            .render_finished = render_finished,
            .in_flight = in_flight,
        };
    }

    return frames;

    // Failure jump; cleans and returns a null pointer.
FAIL:
    destroy_frames(frames, device, n);
    return NULL;
}

VkCommandBuffer* create_command_buffers(const VkDevice device, const VkCommandPool command_pool,
                                        const usize n) {
    const VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = n,
    };
    VkCommandBuffer* command_buffers = malloc(n * sizeof(VkCommandBuffer));
    if (command_buffers == NULL) {
        return NULL;
    }
    if (!query_vk_result(vkAllocateCommandBuffers(device, &allocate_info, command_buffers))) {
        free(command_buffers);
        return NULL;
    }

    return command_buffers;
}

void destroy_frames(Frame* frames, const VkDevice device, const usize n) {
    if (frames != NULL) {
        for (usize i = 0; i < n; i++) {
            vkDestroyFence(device, frames[i].in_flight, NULL);
            vkDestroySemaphore(device, frames[i].render_finished, NULL);
            vkDestroySemaphore(device, frames[i].image_available, NULL);
        }
        free(frames);
    }
}