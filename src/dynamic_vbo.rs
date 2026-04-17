use core::slice;
use std::{
    ffi::c_void,
    marker::PhantomData,
    ops::{Deref, DerefMut},
    ptr,
};

use crate::{Buffer, RenderBeginInfo, Result, VertexData, error::vk_error};

unsafe extern "C" {
    /// Destroys the dynamic VBO.
    fn destroy_dynamic_vbo(vbo: *mut DynamicVboRaw);

    /// Returns a pointer to the dynamic vertex VBO's vertex data.
    #[must_use]
    fn dynamic_vbo_get_data(vbo: *mut DynamicVboRaw) -> *mut u8;

    /// Writes the dynamic VBO's data to its frame at the given index.
    #[must_use]
    fn dynamic_vbo_write(vbo: *mut DynamicVboRaw, i: usize) -> bool;

    /// Pushes the vertices to the dynamic VBO's vertex data.
    fn dynamic_vbo_push(vbo: *mut DynamicVboRaw, n: usize, vertices: *const c_void);
}

/// Fixed C-layout header of `DynamicVbo`.
///
/// The flexible array tail `data[]` is not represented as a field here.
#[repr(C)]
pub(crate) struct DynamicVboRaw {
    /// The buffer.
    buffer: Buffer,

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
    #[must_use]
    pub fn write(&mut self, begin_info: &RenderBeginInfo) -> Result<()> {
        let success = unsafe { dynamic_vbo_write(self.raw, begin_info.image_index()) };

        success.then_some(()).ok_or_else(|| vk_error())
    }

    /// Pushes the vertex into the buffer.
    ///
    /// Returns whether there was room for the vertex in the buffer.
    pub fn push(&mut self, vertex: &T) -> bool {
        // Don't push if there isn't enough room.
        if self.raw().length >= self.raw().capacity {
            return false;
        }

        // Push the vertices.
        unsafe {
            dynamic_vbo_push(self.raw, 1, ptr::from_ref(vertex) as *const c_void);
        }
        true
    }

    /// Pushes the vertices into the buffer.
    ///
    /// Returns whether there was enough room in the buffer for all vertices.
    /// If there wasn't enough room for all of them, none are pushed.
    pub fn push_slice(&mut self, vertices: &[T]) -> bool {
        // Don't push if there isn't enough room.
        if self.raw().length + vertices.len() > self.raw().capacity {
            return false;
        }

        // Push the vertices.
        unsafe {
            dynamic_vbo_push(self.raw, vertices.len(), vertices.as_ptr() as *const c_void);
        }
        true
    }

    /// Clears the vertex buffer.
    #[inline]
    pub fn clear(&mut self) {
        self.raw_mut().length = 0;
    }

    /// Trims the vertex buffer so its length is at most the given value.
    #[inline]
    pub fn trim(&mut self, n: usize) {
        let n = n.min(self.raw().length);
        self.raw_mut().length = n;
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
        unsafe { dynamic_vbo_get_data(self.raw) as *const T }
    }

    /// Returns a mutable pointer to the vertex data.
    #[inline]
    #[must_use]
    pub(crate) fn data_mut(&self) -> *mut T {
        unsafe { dynamic_vbo_get_data(self.raw) as *mut T }
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
            destroy_dynamic_vbo(self.raw);
        }
    }
}
