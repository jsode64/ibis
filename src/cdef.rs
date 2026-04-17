use std::ffi::c_void;

/// A GLFW handle.
pub type GlfwHandle = *mut c_void;

/// A Vulkan handle.
pub type VkHandle = *mut c_void;

/// A buffer object.
#[repr(C)]
pub struct Buffer {
    /// The device handle (must outlive the buffer).
    device: VkHandle,

    /// The buffer handle.
    buffer: VkHandle,

    /// The memory allocation.
    memory: VkHandle,
}
