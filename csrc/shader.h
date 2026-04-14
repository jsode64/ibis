#pragma once

#include "def.h"
#include "renderer.h"
#include <vulkan/vulkan.h>

/// A builder for a shader layout.
typedef struct ShaderLayoutBuilder {
    /// Placeholder for future options.
    u8 _reserved;
} ShaderLayoutBuilder;

/// A shader layout.
typedef struct ShaderLayout {
    /// The source device. Must outlive the shader layout.
    const VkDevice device;

    VkPipelineLayout pipeline_layout;
} ShaderLayout;

/// A builder for a shader.
typedef struct ShaderBuilder {
    usize vertex_length;

    const u32* vertex_code;

    usize fragment_length;

    const u32* fragment_code;

    usize num_bindings;

    const VkVertexInputBindingDescription* bindings;

    usize num_attributes;

    const VkVertexInputAttributeDescription* attributes;
} ShaderBuilder;

/// A shader.
typedef struct Shader {
    /// The source device. Must outlive the shader.
    const VkDevice device;

    VkPipeline pipeline;
} Shader;

static const ShaderLayout NULL_SHADER_LAYOUT = {
    .device = VK_NULL_HANDLE,
    .pipeline_layout = VK_NULL_HANDLE,
};

static const Shader NULL_SHADER = {
    .device = VK_NULL_HANDLE,
    .pipeline = VK_NULL_HANDLE,
};

/// Creates a shader layout from a shader layout builder.
///
/// @param builder The shader layout builder.
/// @param renderer The target renderer.
/// @return A shader layout, or a null shader layout on failure.
ShaderLayout create_shader_layout(const ShaderLayoutBuilder* builder, const Renderer* renderer);

/// Destroys the shader layout.
void destroy_shader_layout(ShaderLayout* layout);

/// Creates a shader from a shader builder.
///
/// @param builder The shader builder.
/// @param layout The shader layout.
/// @param renderer The target renderer.
/// @return A shader, or a null shader on failure.
Shader create_shader(const ShaderBuilder* builder, const ShaderLayout* layout,
                     const Renderer* renderer);

/// Destroys the shader.
void destroy_shader(Shader* shader);
