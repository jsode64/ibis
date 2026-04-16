use std::ptr;

use crate::{Context, DynamicVbo, Result, VertexData, VkHandle, error::vk_error};

unsafe extern "C" {
    /// Creates a GPU allocator for the context.
    #[must_use]
    fn create_gpu_allocator(context: *const Context) -> GpuAllocator;

    /// Allocates a dynamic vertex buffer with frame slices.
    #[must_use]
    fn allocate_dynamic_vbo(
        allocator: *const GpuAllocator,
        num_frames: usize,
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

    /// Allocates a typed dynamic vertex buffer.
    #[inline]
    #[must_use]
    pub fn allocate_dynamic_vbo<T: VertexData>(
        &self,
        num_frames: usize,
        capacity: usize,
    ) -> Result<DynamicVbo<T>> {
        let raw = unsafe {
            allocate_dynamic_vbo(
                ptr::from_ref(self),
                num_frames,
                std::mem::size_of::<T>(),
                capacity,
            )
        };

        (!raw.is_null())
            .then(|| DynamicVbo::new(raw))
            .ok_or_else(|| vk_error())
    }
}
