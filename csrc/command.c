#include "command.h"
#include <vulkan/vulkan_core.h>
#include "dynamic_vbo.h"

bool fill_command_buffer(VkCommandBuffer command_buffer, const u64* data, const usize n) {
    usize i = 0;
    while (i < n) {
        if (data[i] == CMD_DRAW) {
            const VkPipeline pipeline = (VkPipeline)data[i + 1];
            const u32 num_vertices = (u32)data[i + 2];
            const u32 num_instances = (u32)data[i + 3];
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdDraw(command_buffer, num_vertices, num_instances, 0, 0);
            i += 4;
        } else if (data[i] == CMD_DRAW_DYNAMIC_VBO) {
            const VkPipeline pipeline = (VkPipeline)data[i + 1];
            DynamicVbo* vbo = (DynamicVbo*)data[i + 2];
            const VkDeviceSize command_offset = get_dynamic_vbo_offset(vbo);
            const VkDeviceSize vertex_offset = command_offset + sizeof(VkDrawIndirectCommand);
            vkCmdBindVertexBuffers(command_buffer, 0, 1, &vbo->buffer, &vertex_offset);
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdDrawIndirect(command_buffer, vbo->buffer, command_offset, 1, 0);
            i += 3;
        } else {
            return false;
        }
    }

    return true;
}
