use std::{ffi::CStr, ptr};

use crate::{Result, Version, VkHandle, Window, error::vk_error};

unsafe extern "C" {
    fn create_context(builder: *const ContextBuilder, window: *const Window) -> Context;

    fn destroy_context(context: *mut Context);
}

#[repr(C)]
pub struct ContextBuilder {
    /// The application's name.
    app_name: *const i8,

    /// The application's version.
    app_version: Version,

    /// Whether Vulkan validation and debug messaging is enabled.
    vk_validation: bool,
}

#[repr(C)]
pub struct Context {
    /// The Vulkan instance.
    instance: VkHandle,

    /// The (optional) debug messenger.
    debug_messenger: VkHandle,

    /// The target surface.
    surface: VkHandle,

    /// The physical device.
    physical_device: VkHandle,

    /// The compute queue index.
    compute_queue_index: u32,

    /// The graphics queue index.
    graphics_queue_index: u32,

    /// The present queue index.
    present_queue_index: u32,

    /// The logical device.
    device: VkHandle,

    /// The compute queue.
    compute_queue: VkHandle,

    /// The graphics queue.
    graphics_queue: VkHandle,

    /// The present queue.
    present_queue: VkHandle,
}

impl ContextBuilder {
    /// Returns a default context builder.
    ///
    /// The default parameters are:
    /// - `app_name`: A null C-string
    /// - `app_version`: `Version::new(0, 0, 0)` (v0.0.0)
    /// - `vk_validation`: `cfg!(debug_assertions)`
    pub const fn new() -> Self {
        Self {
            app_name: ptr::null(),
            app_version: Version::new(0, 0, 0),
            vk_validation: cfg!(debug_assertions),
        }
    }

    /// Builds and returns a context.
    #[must_use]
    pub fn build(self, window: &Window) -> Result<Context> {
        Context::new(self, window)
    }

    /// Sets the application name.
    ///
    /// By default this is set to a null C-string.
    #[inline]
    #[must_use]
    pub const fn app_name(mut self, name: &CStr) -> Self {
        self.app_name = name.as_ptr();
        self
    }

    /// Sets the application version.
    ///
    /// By default this is set to `Version::new(, 0, 0)` (v0.0.0).
    #[inline]
    #[must_use]
    pub const fn app_version(mut self, version: Version) -> Self {
        self.app_version = version;
        self
    }

    /// Sets whether Vulkan validation and debug messaging is enabled.
    ///
    /// This requires the system to have the `VK_debug_utils` extension
    /// and `VK_KHR_validation` layers for Vulkan. If either is not present,
    /// debug messaging will not be done.
    ///
    /// By default this is set to `cfg!(debug_assertions)`.
    #[inline]
    #[must_use]
    pub const fn vk_validation(mut self, b: bool) -> Self {
        self.vk_validation = b;
        self
    }
}

impl Default for ContextBuilder {
    /// Returns a default context builder.
    ///
    /// See [`ContextBuilder::new`].
    #[inline]
    fn default() -> Self {
        Self {
            app_name: Default::default(),
            app_version: Default::default(),
            vk_validation: cfg!(debug_assertions),
        }
    }
}

impl Context {
    /// Returns a default context builder.
    ///
    /// See [`ContextBuilder::new`].
    #[inline]
    #[must_use]
    pub const fn builder() -> ContextBuilder {
        ContextBuilder::new()
    }

    /// Creates a context from the builder and window.
    pub fn new(builder: ContextBuilder, window: &Window) -> Result<Context> {
        let context = unsafe { create_context(ptr::from_ref(&builder), ptr::from_ref(window)) };
        let is_initialized = !context.instance.is_null()
            && !context.surface.is_null()
            && !context.physical_device.is_null()
            && !context.device.is_null();

        // Tell if debug messaging was requested but couldn't be set up.
        if is_initialized && context.debug_messenger.is_null() && builder.vk_validation {
            eprintln!("Vulkan validation was requested but not supported");
        }

        is_initialized.then_some(context).ok_or_else(|| vk_error())
    }
}

impl Drop for Context {
    fn drop(&mut self) {
        unsafe {
            destroy_context(ptr::from_mut(self));
        }
    }
}
