#pragma once

#include <vulkan/vulkan.h>
#include "def.h"

/// Renderer command buffer command types.
typedef enum Command {
    /// Parameters: shader, vertex count, instance count.
    CMD_DRAW = 1,

    /// Parameters: shader, dynamic VBO.
    CMD_DRAW_DYNAMIC_VBO,
} Command;

/// Fills the command buffer with the encoded commands.
///
/// @param command_buffer The command buffer to fill.
/// @param data A pointer to the encoded commands.
/// @param n The number of words in the encoded commands.
/// @return `true` on success, `false` on failure.
bool fill_command_buffer(VkCommandBuffer command_buffer, const u64* data, usize n);
