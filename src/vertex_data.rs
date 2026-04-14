use std::mem;

/// Vertex input rate for a binding.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum VertexInputRate {
    Vertex = 0,

    Instance = 1,
}

/// Vertex buffer binding description.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(C)]
pub struct VertexBinding {
    pub binding: u32,

    pub stride: u32,

    pub input_rate: VertexInputRate,
}

/// Vertex attribute description consumed by the shader.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(C)]
pub struct VertexAttribute {
    pub location: u32,

    pub binding: u32,

    pub format: u32,

    pub offset: u32,
}

/// Vertex layout metadata for a vertex type.
pub trait VertexData: Sized {
    /// Binding descriptions used by this vertex type.
    fn bindings() -> &'static [VertexBinding];

    /// Attribute descriptions used by this vertex type.
    fn attributes() -> &'static [VertexAttribute];

    /// Byte stride for one vertex element.
    #[inline]
    fn stride() -> u32 {
        mem::size_of::<Self>() as u32
    }

    /// Convenience helper for the common single-binding case.
    #[inline]
    fn single_binding(binding: u32, input_rate: VertexInputRate) -> VertexBinding {
        VertexBinding {
            binding,
            stride: Self::stride(),
            input_rate,
        }
    }
}

impl VertexData for () {
    #[inline]
    fn bindings() -> &'static [VertexBinding] {
        &[]
    }

    #[inline]
    fn attributes() -> &'static [VertexAttribute] {
        &[]
    }
}
