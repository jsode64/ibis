use std::error::Error;
use std::mem;

use ibis::{
    Command, Context, DynamicVbo, GpuAllocator, Renderer, Shader, ShaderLayout, Vec2, Vec3,
    Version, VertexAttribute, VertexBinding, VertexData, VertexInputRate, Window,
};

#[repr(C)]
#[derive(Clone, Copy, Debug)]
struct Vertex {
    position: Vec2<f32>,

    color: Vec3<f32>,
}

impl VertexData for Vertex {
    fn bindings() -> &'static [VertexBinding] {
        const BINDINGS: [VertexBinding; 1] = [VertexBinding {
            binding: 0,
            stride: mem::size_of::<Vertex>() as u32,
            input_rate: VertexInputRate::Vertex,
        }];

        &BINDINGS
    }

    fn attributes() -> &'static [VertexAttribute] {
        // VkFormat values:
        // VK_FORMAT_R32G32_SFLOAT = 103
        // VK_FORMAT_R32G32B32_SFLOAT = 106
        const ATTRIBUTES: [VertexAttribute; 2] = [
            VertexAttribute {
                location: 0,
                binding: 0,
                format: 103,
                offset: 0,
            },
            VertexAttribute {
                location: 1,
                binding: 0,
                format: 106,
                offset: mem::size_of::<Vec2<f32>>() as u32,
            },
        ];

        &ATTRIBUTES
    }
}

fn main() -> Result<(), impl Error> {
    let mut window = Window::new(c"Hello", 1600, 900)?;
    let context = Context::builder()
        .app_name(c"Hello")
        .app_version(Version::new(1, 0, 0))
        .vk_validation(true)
        .build(&window)?;
    let mut renderer = Renderer::new(&context, &window)?;
    let gpu_allocator = GpuAllocator::new(&context);
    let mut buffer: DynamicVbo<Vertex> = DynamicVbo::new(&gpu_allocator, 4096)?;
    let layout = ShaderLayout::builder().build(&renderer)?;
    let shader: Shader<Vertex> = Shader::builder()
        .vertex_shader_path("shaders/vertex.spv")?
        .fragment_shader_path("shaders/fragment.spv")?
        .build(&layout, &renderer)?;

    buffer.push(&Vertex {
        position: Vec2::new(0.0, 0.0),
        color: Vec3::new(1.0, 0.0, 0.0),
    });
    buffer.push(&Vertex {
        position: Vec2::new(1.0, 1.0),
        color: Vec3::new(0.0, 0.0, 1.0),
    });
    buffer.push(&Vertex {
        position: Vec2::new(1.0, 0.0),
        color: Vec3::new(1.0, 1.0, 0.0),
    });
    renderer.set_commands(&[Command::draw_dynamic_vbo(&shader, &buffer)])?;

    println!("Ok");

    while !window.should_close() {
        window.update();
        renderer.draw()?;
    }

    Ok::<(), ibis::Error>(())
}
