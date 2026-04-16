use core::slice;
use std::{
    ffi::c_void,
    marker::PhantomData,
    ops::{Deref, DerefMut},
    ptr,
};

use crate::{GpuAllocator, VertexData, VkHandle};

unsafe extern "C" {
    /// Creates a new dynamic vertex buffer for the given data.
    #[must_use]
    fn allocate_dynamic_vbo(
        allocator: *const GpuAllocator,
        num_frames: usize,
        z: usize,
        n: usize,
    ) -> *mut DynamicVboRaw;

    /// Destroys the dynamic vertex buffer.
    fn destroy_dynamic_vbo(vbo: *mut DynamicVboRaw);

    /// Returns a pointer to the dynamic vertex buffer's data.
    fn get_dynamic_vbo_data(vbo: *mut DynamicVboRaw) -> *mut u8;

    /// Writes the dynamic vertex buffer's data to its next frame.
    fn write_dynamic_vbo(vbo: *mut DynamicVboRaw);

    /// Pushes vertices into the dynamic vertex buffer's data.
    fn push_to_dynamic_vbo(vbo: *mut DynamicVboRaw, n: usize, vertices: *const c_void);

    /// Clears the dynamic vertex buffer's data.
    fn clear_dynamic_vbo(vbo: *mut DynamicVboRaw);
}

/// Fixed C-layout header of `DynamicVbo`.
///
/// The flexible array tail `data[]` is not represented as a field here.
#[repr(C)]
pub(crate) struct DynamicVboRaw {
    /// The source device. Must outlive the dynamic vertex buffer.
    device: VkHandle,

    /// The Vulkan buffer object.
    buffer: VkHandle,

    /// The device memory allocation.
    memory: VkHandle,

    /// The number of frames.
    num_frames: usize,

    /// The index of the next frame to write to.
    frame_index: usize,

    /// The data capacity.
    capacity: usize,

    /// The size of each vertex.
    size: usize,

    /// The current number of stored vertices.
    length: usize,
}

/// A Dynamic VBO.
pub struct DynamicVbo<T: VertexData> {
    raw: *mut DynamicVboRaw,

    _t: PhantomData<T>,
}

impl<T: VertexData> DynamicVbo<T> {
    #[inline]
    #[must_use]
    pub(crate) fn new(raw: *mut DynamicVboRaw) -> Self {
        Self {
            raw,
            _t: PhantomData,
        }
    }

    /// Writes the dynamic vertex buffer's data to its next frame.
    pub fn write(&mut self) {
        unsafe {
            write_dynamic_vbo(self.raw);
        }
    }

    /// Pushes the vertex into the buffer.
    ///
    /// Returns whether there was room for the vertex in the buffer.
    /// If there wasn't the vertex isn't pushed.
    pub fn push(&mut self, vertex: &T) -> bool {
        if self.raw().length < self.raw().capacity {
            unsafe {
                push_to_dynamic_vbo(self.raw, 1, ptr::from_ref(vertex) as *const c_void);
            }
            true
        } else {
            false
        }
    }

    /// Pushes the vertices into the buffer.
    ///
    /// Returns whether there was enough room in the buffer for all vertices.
    /// If there wasn't, none get pushed.
    pub fn push_slice(&mut self, vertices: &[T]) -> bool {
        if self.raw().length + vertices.len() <= self.raw().capacity {
            unsafe {
                push_to_dynamic_vbo(self.raw, vertices.len(), vertices.as_ptr() as *const c_void);
            }
            true
        } else {
            false
        }
    }

    /// Clears the vertex buffer.
    pub fn clear(&mut self) {
        unsafe {
            clear_dynamic_vbo(self.raw);
        }
    }

    /// Returns a reference to the internal raw dynamic vertex buffer.
    #[inline]
    #[must_use]
    pub(crate) fn raw(&self) -> &DynamicVboRaw {
        unsafe { &*self.raw }
    }

    /// Returns a mutable reference to the internal raw dynamic vertex buffer.
    #[inline]
    #[must_use]
    pub(crate) fn raw_mut(&mut self) -> &mut DynamicVboRaw {
        unsafe { &mut *self.raw }
    }

    /// Returns a pointer to the vertex data.
    #[inline]
    #[must_use]
    pub(crate) fn data(&self) -> *const T {
        unsafe { get_dynamic_vbo_data(self.raw) as *const T }
    }

    /// Returns a mutable pointer to the vertex data.
    #[inline]
    #[must_use]
    pub(crate) fn data_mut(&self) -> *mut T {
        unsafe { get_dynamic_vbo_data(self.raw) as *mut T }
    }
}

impl<T: VertexData> Deref for DynamicVbo<T> {
    type Target = [T];

    fn deref(&self) -> &Self::Target {
        unsafe { slice::from_raw_parts(self.data(), self.raw().length) }
    }
}

impl<T: VertexData> DerefMut for DynamicVbo<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { slice::from_raw_parts_mut(self.data_mut(), self.raw().length) }
    }
}

impl<T: VertexData> Drop for DynamicVbo<T> {
    fn drop(&mut self) {
        unsafe {
            if !self.raw.is_null() {
                destroy_dynamic_vbo(self.raw);
            }
        }
    }
}
