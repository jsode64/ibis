use std::{ffi::CStr, ptr};

use crate::{GlfwHandle, Result, error::glfw_error};

unsafe extern "C" {
    /// Creates a window with the given title, width, and height.
    #[must_use]
    fn create_window(title: *const i8, width: u32, height: u32) -> Window;

    /// Destroys the window.
    fn destroy_window(window: *mut Window);

    /// Updates the window.
    fn update_window(window: *mut Window);
}

/// A window.
#[repr(C)]
pub struct Window {
    /// The GLFW window handle.
    window: GlfwHandle,

    /// The window's width.
    width: u32,

    /// The window's height.
    height: u32,

    /// Whether the window should close.
    should_close: bool,
}

impl Window {
    /// Creates a window with the given title, width, and height.
    #[inline]
    #[must_use]
    pub fn new(title: &CStr, width: u32, height: u32) -> Result<Self> {
        let window = unsafe { create_window(title.as_ptr(), width, height) };
        let is_initialized = !window.window.is_null() && !window.should_close;

        is_initialized.then_some(window).ok_or_else(|| glfw_error())
    }

    /// The window's width.
    #[inline]
    #[must_use]
    pub const fn width(&self) -> u32 {
        self.width
    }

    /// The window's height.
    #[inline]
    #[must_use]
    pub const fn height(&self) -> u32 {
        self.height
    }

    /// Whether the window should close.
    #[inline]
    #[must_use]
    pub fn should_close(&self) -> bool {
        self.should_close
    }

    /// Updates the window.
    #[inline]
    pub fn update(&mut self) {
        unsafe {
            update_window(ptr::from_mut(self));
        }
    }
}

impl Drop for Window {
    fn drop(&mut self) {
        unsafe {
            destroy_window(ptr::from_mut(self));
        }
    }
}
