#pragma once

#include "command.h"
#include "context.h"
#include "def.h"
#include "dynamic_vbo.h"
#include "swapchain.h"
#include "window.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

/// A builder for a renderer.
typedef struct RendererBuilder {
    /// The number of frames in flight.
    usize num_frames;

    /// The ideal number of swapchain images.
    usize num_images;

    /// The maximum number of uniform buffers that will be made.
    usize max_num_uniform_buffers;
} RendererBuilder;

/// Per-frame synchronization objects.
typedef struct Frame {
    VkSemaphore image_available;

    VkSemaphore render_finished;

    VkFence in_flight;
} Frame;

/// A Vulkan renderer.
typedef struct Renderer {
    /// The source context. Must outlive the renderer.
    const Context* context;

    /// The render pass.
    VkRenderPass render_pass;

    /// The swapchain object.
    Swapchain swapchain;

    /// The descriptor pool.
    VkDescriptorPool descriptor_pool;

    /// The command pool.
    VkCommandPool command_pool;

    /// The number of frames in flight.
    usize num_frames;

    /// The index of the next frame to be rendered.
    usize frame_index;

    /// Number of command buffers, one per swapchain image.
    usize num_command_buffers;

    /// Recorded command buffers indexed by swapchain image index.
    VkCommandBuffer* command_buffers;

    /// Number of words in `commands`.
    usize num_commands;

    /// Encoded renderer commands.
    u64* commands;

    Frame* frames;
} Renderer;

/// Information for a renderer's next draw.
typedef struct RenderBeginInfo {
    /// The index of the target frame.
    usize frame_index;

    /// The index of the target swapchain image.
    usize image_index;

    /// Whether the swapchain was out of date.
    bool is_swapchain_outdated;

    /// Whether the begin info is valid/no errors occured.
    bool is_valid;
} RenderBeginInfo;

/// Creates a renderer for the given context.
Renderer create_renderer(const RendererBuilder* builder, const Context* context);

/// Destroys the renderer.
///
/// @param renderer The renderer to destroy.
void destroy_renderer(Renderer* renderer);

/// Frees all previous commands and reallocates N for refilling.
/// If N is zero, no reallocation will happen and no commands will be stored.
u64* renderer_commands_flush(Renderer* renderer, usize n);

/// Gets information for the next render.
RenderBeginInfo renderer_begin(Renderer* renderer);

/// Queues drawing and presentation for the renderer.
bool renderer_draw(Renderer* renderer, const RenderBeginInfo* begin_info);

/// Recreates the renderer's swapchain.
bool renderer_recreate_swapchain(Renderer* renderer);

/// Reloads the command buffers.
bool renderer_reload_commands(Renderer* renderer);