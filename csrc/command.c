#include "command.h"
#include "dynamic_vbo.h"
#include <vulkan/vulkan_core.h>

bool fill_command_buffer(VkCommandBuffer const command_buffer,
                         const usize i,
                         const usize n,
                         const u64* data) {
    const u64* next = data;
    while (data + n > next) {
        if (*data == CMD_DRAW) {
            const VkPipeline pipeline = (VkPipeline)next[1];
            const u32 num_vertices = (u32)next[2];
            const u32 num_instances = (u32)next[3];
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdDraw(command_buffer, num_vertices, num_instances, 0, 0);
            next += 4;
        } else if (*data == CMD_DRAW_DYNAMIC_VBO) {
            const VkPipeline pipeline = (VkPipeline)next[1];
            DynamicVbo* vbo = (DynamicVbo*)next[2];
            const VkDeviceSize command_offset = dynamic_vbo_frame_size(vbo) * i;
            const VkDeviceSize vertex_offset = command_offset + sizeof(VkDrawIndirectCommand);
            vkCmdBindVertexBuffers(command_buffer, 0, 1, &vbo->buffer.buffer, &vertex_offset);
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdDrawIndirect(command_buffer, vbo->buffer.buffer, command_offset, 1, 0);
            next += 3;
        } else {
            return false;
        }
    }

    return true;
}
