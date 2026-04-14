use std::ffi::c_void;

/// A GLFW handle.
pub type GlfwHandle = *mut c_void;

/// A Vulkan handle.
pub type VkHandle = *mut c_void;
