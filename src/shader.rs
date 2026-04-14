use std::{fs, io, marker::PhantomData, ptr};

use crate::{
    Error, Renderer, Result, VertexAttribute, VertexBinding, VertexData, VkHandle, error::vk_error,
};

unsafe extern "C" {
    /// Creates a shader layout from a shader layout builder.
    #[must_use]
    fn create_shader_layout(
        builder: *const ShaderLayoutBuilder,
        renderer: *const Renderer,
    ) -> ShaderLayout;

    /// Destroys the shader layout.
    fn destroy_shader_layout(layout: *mut ShaderLayout);

    /// Creates a shader from a shader builder.
    #[must_use]
    fn create_shader(
        builder: *const RawShaderBuilder,
        layout: *const ShaderLayout,
        renderer: *const Renderer,
    ) -> RawShader;

    /// Destroys the shader.
    fn destroy_shader(shader: *mut RawShader);
}

/// A builder for a shader layout.
#[repr(C)]
pub struct ShaderLayoutBuilder {
    _reserved: u8,
}
/// A shader layout.
#[repr(C)]
pub struct ShaderLayout {
    /// The source device. Must outlive the shader layout.
    device: VkHandle,

    pipeline_layout: VkHandle,
}

/// A builder for a shader.
pub struct ShaderBuilder<T: VertexData = ()> {
    /// Vertex shader code.
    vertex_code: Vec<u32>,

    /// Fragment shader code.
    fragment_code: Vec<u32>,

    _t: PhantomData<T>,
}

/// A raw builder for a shader.
#[repr(C)]
struct RawShaderBuilder {
    /// The vertex shader code length in bytes.
    vertex_length: usize,

    /// The vertex shader code.
    vertex_code: *const u32,

    /// The fragment shader code length in bytes.
    fragment_length: usize,

    /// The fragment shader code.
    fragment_code: *const u32,

    /// The number of vertex bindings.
    num_bindings: usize,

    /// Vertex binding descriptions.
    bindings: *const VertexBinding,

    /// The number of vertex attributes.
    num_attributes: usize,

    /// Vertex attribute descriptions.
    attributes: *const VertexAttribute,
}

/// A shader.
#[repr(C)]
struct RawShader {
    /// The source device. Must outlive the shader.
    device: VkHandle,

    pipeline: VkHandle,
}

/// A shader.
pub struct Shader<T: VertexData = ()> {
    raw: RawShader,

    _t: PhantomData<T>,
}

impl ShaderLayoutBuilder {
    /// Returns a default shader layout builder.
    #[inline]
    #[must_use]
    pub const fn new() -> Self {
        Self { _reserved: 0 }
    }

    /// Builds the shader layout.
    #[inline]
    #[must_use]
    pub fn build(self, renderer: &Renderer) -> Result<ShaderLayout> {
        ShaderLayout::new(self, renderer)
    }
}

impl ShaderLayout {
    /// Returns a default shader layout builder.
    ///
    /// See [`ShaderLayoutBuilder::new`]
    #[inline]
    #[must_use]
    pub const fn builder() -> ShaderLayoutBuilder {
        ShaderLayoutBuilder::new()
    }

    /// Returns a new shader layout.
    #[inline]
    #[must_use]
    pub fn new(builder: ShaderLayoutBuilder, renderer: &Renderer) -> Result<Self> {
        let shader_layout =
            unsafe { create_shader_layout(ptr::from_ref(&builder), ptr::from_ref(renderer)) };

        let is_initialized = !shader_layout.device.is_null();

        is_initialized
            .then_some(shader_layout)
            .ok_or_else(|| vk_error())
    }
}

impl Drop for ShaderLayout {
    fn drop(&mut self) {
        unsafe {
            destroy_shader_layout(ptr::from_mut(self));
        }
    }
}

impl<T: VertexData> ShaderBuilder<T> {
    /// Returns a new shader builder.
    #[inline]
    #[must_use]
    pub const fn new() -> Self {
        Self {
            vertex_code: Vec::new(),
            fragment_code: Vec::new(),
            _t: PhantomData,
        }
    }

    /// Builds the shader.
    #[inline]
    #[must_use]
    pub fn build(self, layout: &ShaderLayout, renderer: &Renderer) -> Result<Shader<T>> {
        Shader::new(self, layout, renderer)
    }

    /// Sets the vertex shader path.
    #[inline]
    #[must_use]
    pub fn vertex_shader_path(mut self, path: &str) -> Result<Self> {
        self.vertex_code = Self::read_spirv(path).map_err(|e| Error::IoError(e))?;
        Ok(self)
    }

    /// Sets the fragment shader path.
    #[inline]
    #[must_use]
    pub fn fragment_shader_path(mut self, path: &str) -> Result<Self> {
        self.fragment_code = Self::read_spirv(path).map_err(|e| Error::IoError(e))?;
        Ok(self)
    }

    /// Reads the given file as SPIR-V.
    #[must_use]
    fn read_spirv(path: &str) -> io::Result<Vec<u32>> {
        let bytes = fs::read(path)?;

        // SPIRV is 4 byte chunks.
        if bytes.len() % 4 != 0 {
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "SPIR-V file was not 4-byte chunks",
            ));
        }

        Ok(bytes
            .chunks_exact(4)
            .map(|chunk| u32::from_le_bytes(chunk.try_into().unwrap()))
            .collect())
    }

    /// The shader builder as a raw shader builder.
    #[inline]
    #[must_use]
    fn as_raw(&self) -> RawShaderBuilder {
        let bindings = T::bindings();
        let attributes = T::attributes();

        RawShaderBuilder {
            vertex_length: self.vertex_code.len() * std::mem::size_of::<u32>(),
            vertex_code: self.vertex_code.as_ptr(),
            fragment_length: self.fragment_code.len() * std::mem::size_of::<u32>(),
            fragment_code: self.fragment_code.as_ptr(),
            num_bindings: bindings.len(),
            bindings: bindings.as_ptr(),
            num_attributes: attributes.len(),
            attributes: attributes.as_ptr(),
        }
    }
}

impl<T: VertexData> Shader<T> {
    /// Returns a default shader builder.
    #[inline]
    #[must_use]
    pub const fn builder() -> ShaderBuilder<T> {
        ShaderBuilder::new()
    }

    /// Returns a new shader.
    #[inline]
    #[must_use]
    pub fn new(
        builder: ShaderBuilder<T>,
        layout: &ShaderLayout,
        renderer: &Renderer,
    ) -> Result<Self> {
        let builder = builder.as_raw();
        let shader = unsafe {
            create_shader(
                ptr::from_ref(&builder),
                ptr::from_ref(layout),
                ptr::from_ref(renderer),
            )
        };

        let is_initialized = !shader.device.is_null();

        is_initialized
            .then(|| Self {
                raw: shader,
                _t: PhantomData,
            })
            .ok_or_else(|| vk_error())
    }

    /// Returns the shader's `VkPipeline` handle.
    #[inline]
    #[must_use]
    pub(crate) fn pipeline(&self) -> VkHandle {
        self.raw.pipeline
    }
}

impl<T: VertexData> Drop for Shader<T> {
    fn drop(&mut self) {
        let view = ptr::from_mut(&mut self.raw);
        unsafe {
            destroy_shader(view);
        }
    }
}
