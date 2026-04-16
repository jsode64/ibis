use std::{os::raw::c_void, ptr, slice};

use crate::{
    Context, DynamicVbo, DynamicVboRaw, Result, Shader, VertexData, VkHandle, Window,
    error::vk_error,
};

unsafe extern "C" {
    /// Creates a renderer for the given context and render target.
    #[must_use]
    fn create_renderer(context: *const Context, window: *const Window) -> Renderer;

    /// Destroys the renderer.
    fn destroy_renderer(renderer: *mut Renderer);

    /// Submits drawing and presentation for the renderer.
    ///
    /// Returns `true` on success, `false` on failure.
    fn renderer_draw(renderer: *mut Renderer) -> bool;

    /// Replaces the renderer command stream storage and returns a writable pointer.
    #[must_use]
    fn renderer_commands_flush(renderer: *mut Renderer, n: usize) -> *mut u64;

    /// Reloads the command buffers. Returns `true` on success, `false` on failure.
    #[must_use]
    fn renderer_reload_commands(renderer: *mut Renderer) -> bool;
}

/// Command tags.
#[repr(u64)]
enum CommandOpcode {
    Draw = 1,

    DrawDynamicVbo = 2,
}

/// High-level command data.
pub enum Command {
    Draw {
        pipeline: VkHandle,
        num_vertices: usize,
        num_instances: usize,
    },

    DrawDynamicVbo {
        pipeline: VkHandle,
        buffer: *mut DynamicVboRaw,
    },
}

/// A Vulkan renderer.
#[repr(C)]
pub struct Renderer {
    /// The source context.
    context: *const Context,

    /// The render pass.
    render_pass: VkHandle,

    /// The swapchain.
    swapchain: Swapchain,

    /// The command pool.
    command_pool: VkHandle,

    /// The number of frames in flight.
    num_frames: usize,

    /// The index of the next frame to be rendered.
    frame_index: usize,

    /// Number of command buffers, one per swapchain image.
    num_command_buffers: usize,

    /// Recorded command buffers indexed by swapchain image index.
    command_buffers: *mut VkHandle,

    /// Number of `u64` words in `commands`.
    num_commands: usize,

    /// Encoded renderer commands.
    commands: *mut u64,

    /// The frame objects.
    frames: *mut Frame,
}

/// A Vulakn swapchain setup.
#[repr(C)]
struct Swapchain {
    /// The Vulkan swapchain handle.
    _swapchain: VkHandle,

    /// The number of swapchain images.
    _num_images: u32,

    /// The swapchain images.
    _swapchain_images: *mut SwapchainImage,

    /// The swapchain extent.
    _extent: [u32; 2],
}

/// A swapchain image
#[repr(C)]
struct SwapchainImage {
    /// The image.
    _image: VkHandle,

    /// The image view.
    _image_view: VkHandle,

    /// The framebuffer.
    _framebuffer: VkHandle,
}

/// Per-frame objects. Contains the frame's synchronization objects and command buffer.
#[repr(C)]
struct Frame {
    /// The "image available" semaphore.
    image_available: VkHandle,

    /// The "render finished" semaphore.
    render_finished: VkHandle,

    /// The "in flight" fence.
    in_flight: VkHandle,
}

impl Renderer {
    /// Creates a renderer for the context and window target.
    #[must_use]
    pub fn new(context: &Context, window: &Window) -> Result<Self> {
        let renderer = unsafe { create_renderer(ptr::from_ref(context), ptr::from_ref(window)) };

        let is_initialized = !renderer.context.is_null();

        is_initialized.then_some(renderer).ok_or_else(|| vk_error())
    }

    /// Submits rendering and presentation for the renderer.
    #[inline]
    #[must_use]
    pub fn draw(&mut self) -> Result<()> {
        let success = unsafe { renderer_draw(ptr::from_mut(self)) };

        success.then_some(()).ok_or_else(|| vk_error())
    }

    /// Replaces the renderer command stream from a typed command slice.
    #[must_use]
    pub fn set_commands(&mut self, commands: &[Command]) -> Result<()> {
        let mut words = Vec::<u64>::new();

        for command in commands {
            match command {
                &Command::Draw {
                    pipeline,
                    num_vertices,
                    num_instances,
                } => {
                    words.push(CommandOpcode::Draw as u64);
                    words.push(pipeline as _);
                    words.push(num_vertices as _);
                    words.push(num_instances as _);
                }
                &Command::DrawDynamicVbo { pipeline, buffer } => {
                    words.push(CommandOpcode::DrawDynamicVbo as u64);
                    words.push(pipeline as _);
                    words.push(buffer as _);
                }
            }
        }

        let dst = unsafe { renderer_commands_flush(ptr::from_mut(self), words.len()) };

        if !words.is_empty() {
            let dst_slice = unsafe { slice::from_raw_parts_mut(dst, words.len()) };
            dst_slice.copy_from_slice(&words);
        }

        let success = unsafe { renderer_reload_commands(ptr::from_mut(self)) };
        success.then_some(()).ok_or_else(|| vk_error())
    }
}

impl Command {
    #[must_use]
    pub fn draw<T: VertexData>(
        shader: &Shader<T>,
        num_vertices: usize,
        num_instances: usize,
    ) -> Self {
        Self::Draw {
            pipeline: shader.pipeline(),
            num_vertices,
            num_instances,
        }
    }

    #[must_use]
    pub fn draw_dynamic_vbo<T: VertexData>(
        shader: &Shader<T>,
        dynamic_vbo: &DynamicVbo<T>,
    ) -> Self {
        Self::DrawDynamicVbo {
            pipeline: shader.pipeline(),
            buffer: ptr::from_ref(dynamic_vbo.raw()) as *mut DynamicVboRaw,
        }
    }
}

impl Drop for Renderer {
    fn drop(&mut self) {
        unsafe {
            destroy_renderer(ptr::from_mut(self));
        }
    }
}
