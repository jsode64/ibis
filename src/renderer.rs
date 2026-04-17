use std::{ptr, slice};

use crate::{
    Context, DynamicVbo, DynamicVboRaw, Result, Shader, VertexData, VkHandle, error::vk_error,
};

unsafe extern "C" {
    /// Creates a renderer for the given context and render target.
    #[must_use]
    fn create_renderer(builder: *const RendererBuilder, context: *const Context) -> Renderer;

    /// Destroys the renderer.
    fn destroy_renderer(renderer: *mut Renderer);

    /// Gets and returns information for the renderer's next draw.
    fn renderer_begin(renderer: *mut Renderer) -> RenderBeginInfo;

    /// Submits drawing and presentation for the renderer.
    ///
    /// Returns `true` on success, `false` on failure.
    fn renderer_draw(renderer: *mut Renderer, begin_info: *const RenderBeginInfo) -> bool;

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

/// A swapchain image.
#[repr(C)]
struct SwapchainImage {
    /// The image.
    _image: VkHandle,

    /// The image view.
    _image_view: VkHandle,

    /// The framebuffer.
    _framebuffer: VkHandle,
}

/// A builder for a renderer.
#[repr(C)]
pub struct RendererBuilder {
    /// The number of frames in flight to use.
    num_frames: usize,

    /// The ideal number of swapchain images.
    num_images: usize,

    /// The maximum number of uniform buffers to be created.
    max_num_uniform_buffers: usize,
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
/// A Vulkan renderer.
#[repr(C)]
pub struct Renderer {
    /// The source context.
    context: *const Context,

    /// The render pass.
    render_pass: VkHandle,

    /// The swapchain.
    swapchain: Swapchain,

    /// The descriptor pool.
    descriptor_pool: VkHandle,

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

/// Information for a renderer's next draw.
#[repr(C)]
pub struct RenderBeginInfo {
    /// The index of the target frame.
    frame_index: usize,

    /// The index of the target swapchain image.
    image_index: usize,

    /// Whether the swapchain was out of date.
    is_swapchain_outdated: bool,

    /// Whether the begin info is valid/no errors occured.
    is_valid: bool,
}

impl RendererBuilder {
    /// Builds the renderer.
    #[inline]
    #[must_use]
    pub fn build(self, context: &Context) -> Result<Renderer> {
        Renderer::new(self, context)
    }

    /// Sets the number of frames in flight.
    #[inline]
    #[must_use]
    pub const fn num_frames(mut self, n: usize) -> Self {
        self.num_frames = n;
        self
    }

    /// Sets the ideal number of swapchain images.
    #[inline]
    #[must_use]
    pub const fn num_images(mut self, n: usize) -> Self {
        self.num_images = n;
        self
    }

    /// Sets the maximum number of uniform buffers that can be created.
    #[inline]
    #[must_use]
    pub const fn max_num_uniform_buffers(mut self, n: usize) -> Self {
        self.max_num_uniform_buffers = n;
        self
    }
}

impl Renderer {
    /// Returns a default renderer builder.
    #[inline]
    #[must_use]
    pub fn builder() -> RendererBuilder {
        RendererBuilder {
            num_frames: 2,
            num_images: 3,
            max_num_uniform_buffers: 0,
        }
    }

    /// Creates a renderer for the given context.
    #[must_use]
    pub(crate) fn new(builder: RendererBuilder, context: &Context) -> Result<Self> {
        let renderer = unsafe { create_renderer(ptr::from_ref(&builder), ptr::from_ref(context)) };

        let is_initialized = !renderer.context.is_null();

        is_initialized.then_some(renderer).ok_or_else(|| vk_error())
    }

    /// Returns information for the next draw.
    #[inline]
    #[must_use]
    pub fn begin(&mut self) -> Result<RenderBeginInfo> {
        let begin_info = unsafe { renderer_begin(ptr::from_mut(self)) };

        begin_info
            .is_valid
            .then_some(begin_info)
            .ok_or_else(|| vk_error())
    }

    /// Draws and presents the next render.
    #[inline]
    #[must_use]
    pub fn draw(&mut self, begin_info: &RenderBeginInfo) -> Result<()> {
        let success = unsafe { renderer_draw(ptr::from_mut(self), ptr::from_ref(begin_info)) };

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

impl RenderBeginInfo {
    /// Returns the index of the next target frame.
    #[inline]
    #[must_use]
    pub(crate) fn frame_index(&self) -> usize {
        self.frame_index
    }

    /// Returns the index of the next target swapchain image.
    #[inline]
    #[must_use]
    pub(crate) fn image_index(&self) -> usize {
        self.image_index
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
