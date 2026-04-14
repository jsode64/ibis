use std::ptr;

use crate::{Context, VkHandle};

unsafe extern "C" {
    /// Creates a GPU allocator for the context.
    #[must_use]
    fn create_gpu_allocator(context: *const Context) -> GpuAllocator;
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
}
