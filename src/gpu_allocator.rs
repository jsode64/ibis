use std::ptr;

use crate::{Context, DynamicVbo, Renderer, Result, VertexData, VkHandle, error::vk_error};

unsafe extern "C" {
    /// Creates a GPU allocator for the context.
    #[must_use]
    fn create_gpu_allocator(context: *const Context) -> GpuAllocator;

    /// Creates a new dynamic VBO with the given target renderer, vertex size, and capacity.
    #[must_use]
    fn allocate_dynamic_vbo(
        allocator: *const GpuAllocator,
        renderer: *const Renderer,
        z: usize,
        n: usize,
    ) -> *mut crate::DynamicVboRaw;
}

/// An allocator for GPU memory.
#[repr(C)]
pub struct GpuAllocator {
    /// The source device. Must outlive the allocator.
    device: VkHandle,

    /// The physical device's memory properties.
    memory_properties: [u8; 520],
}

impl GpuAllocator {
    /// Creates a GPU allocator for the context.
    #[inline]
    #[must_use]
    pub fn new(context: &Context) -> Self {
        unsafe { create_gpu_allocator(ptr::from_ref(context)) }
    }

    /// Allocates a dynamic VBO with the given renderer target and capacity.
    #[inline]
    #[must_use]
    pub fn allocate_dynamic_vbo<T: VertexData>(
        &self,
        renderer: &Renderer,
        capacity: usize,
    ) -> Result<DynamicVbo<T>> {
        let raw = unsafe {
            allocate_dynamic_vbo(
                ptr::from_ref(self),
                ptr::from_ref(renderer),
                std::mem::size_of::<T>(),
                capacity,
            )
        };

        (!raw.is_null())
            .then(|| DynamicVbo::new(raw))
            .ok_or_else(|| vk_error())
    }
}
