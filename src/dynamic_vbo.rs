use std::{ffi::c_void, marker::PhantomData, ptr};

use crate::{GpuAllocator, Result, VertexData, VkHandle, error::vk_error};

unsafe extern "C" {
    /// Creates a new dynamic vertex buffer for the given data.
    #[must_use]
    fn allocate_dynamic_vbo(allocator: *const GpuAllocator, z: usize, n: usize) -> DynamicVboRaw;

    /// Destroys the dynamic vertex buffer.
    fn destroy_dynamic_vbo(vbo: *mut DynamicVboRaw);

    /// Pushes the vertices into the buffer.
    fn dynamic_vbo_push_vertices(vbo: *mut DynamicVboRaw, vertices: *const c_void, n: usize);

    /// Clears the vertex buffer.
    fn dynamic_vbo_clear(vbo: *mut DynamicVboRaw);
}

/// A raw dynamic VBO with no vertex type.
#[repr(C)]
pub struct DynamicVboRaw {
    /// The source device. Must outlive the dynamic vertex buffer.
    device: VkHandle,

    buffer: VkHandle,

    memory: VkHandle,

    /// The draw indirect command info.
    cmd: *mut c_void,

    /// The size of each vertex.
    vertex_size: usize,

    /// The capacity of the vector.
    capacity: usize,

    /// The current number of stored vertices.
    length: usize,

    /// The head of the vertex data.
    data: *mut u8,
}

/// A Dynamic VBO.
pub struct DynamicVbo<T: VertexData> {
    raw: DynamicVboRaw,

    _t: PhantomData<T>,
}

impl<T: VertexData> DynamicVbo<T> {
    /// Allocates a dynamic VBO with the given capacity from the GPU allocator.
    #[inline]
    #[must_use]
    pub fn new(allocator: &GpuAllocator, capacity: usize) -> Result<Self> {
        let raw: DynamicVboRaw =
            unsafe { allocate_dynamic_vbo(ptr::from_ref(allocator), size_of::<T>(), capacity) };

        let is_initialized = !raw.device.is_null();

        is_initialized
            .then(|| Self {
                raw,
                _t: PhantomData,
            })
            .ok_or_else(|| vk_error())
    }

    /// Pushes the vertex into the buffer.
    ///
    /// Returns whether there was room for the vertex in the buffer.
    /// If there wasn't the vertex isn't pushed.
    pub fn push(&mut self, vertex: &T) -> bool {
        if self.raw.length < self.raw.capacity {
            unsafe {
                dynamic_vbo_push_vertices(
                    ptr::from_mut(&mut self.raw),
                    ptr::from_ref(vertex) as *const c_void,
                    1,
                );
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
        if self.raw.length + vertices.len() <= self.raw.capacity {
            unsafe {
                dynamic_vbo_push_vertices(
                    ptr::from_mut(&mut self.raw),
                    vertices.as_ptr() as *const c_void,
                    vertices.len(),
                );
            };
            true
        } else {
            false
        }
    }

    /// Clears the vertex buffer.
    pub fn clear(&mut self) {
        unsafe {
            dynamic_vbo_clear(ptr::from_mut(&mut self.raw));
        }
    }

    /// Returns the dynamic VBO's `VkBuffer` handle.
    #[inline]
    #[must_use]
    pub(crate) fn buffer(&self) -> VkHandle {
        self.raw.buffer
    }
}
impl<T: VertexData> Drop for DynamicVbo<T> {
    fn drop(&mut self) {
        unsafe {
            destroy_dynamic_vbo(ptr::from_mut(&mut self.raw));
        }
    }
}
