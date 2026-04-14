#pragma once

#include <vulkan/vulkan.h>
#include "command.h"
#include "context.h"
#include "def.h"
#include "dynamic_vbo.h"
#include "swapchain.h"
#include "window.h"

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

    VkRenderPass render_pass;

    Swapchain swapchain;

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

static const Renderer NULL_RENDERER = {
    .context = NULL,
    .render_pass = VK_NULL_HANDLE,
    .swapchain = NULL_SWAPCHAIN,
    .command_pool = VK_NULL_HANDLE,
    .num_frames = 0,
    .frame_index = 0,
    .num_command_buffers = 0,
    .command_buffers = NULL,
    .num_commands = 0,
    .commands = NULL,
    .frames = NULL,
};

/// Creates a renderer for the context and render target.
///
/// @param context The source context. This must outlive the renderer.
/// @param window The render target.
/// @return A new renderer. On failure, it will contain only null handles.
Renderer create_renderer(const Context* context, const Window* window);

/// Destroys the renderer.
///
/// @param renderer The renderer to destroy.
void destroy_renderer(Renderer* renderer);

/// Queues drawing and presentation for the renderer.
///
/// @param renderer The renderer.
/// @return `true` on success, `false` on failure.
bool renderer_draw(Renderer* renderer);

/// Frees all previous commands and reallocates N for refilling.
/// If N is zero, no reallocation will happen and no commands will be stored.
///
/// @param renderer The renderer.
/// @param n The number of words to allocate.
/// @return A pointer to the new data for filling, or a null pointer on failure.
u64* renderer_commands_flush(Renderer* renderer, usize n);

/// Reloads the command buffers.
///
/// @param renderer The renderer.
/// @return `true` on success, `false` on failure.
bool renderer_reload_commands(Renderer* renderer);