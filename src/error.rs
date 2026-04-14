use thiserror::Error;

unsafe extern "C" {
    /// Returns the last GLFW error or 0 (`GLFW_NO_ERROR`) if there was none.
    #[must_use]
    fn get_glfw_error() -> i32;

    /// Returns the last Vulkan error or 0 (`VK_SUCCESS`) if there was none.
    #[must_use]
    fn get_vk_error() -> u32;
}

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug, Error)]
pub enum Error {
    /// An `std::io` error.
    #[error("IO error: \"{0}\"")]
    IoError(std::io::Error),

    /// A GLFW error.
    #[error("GLFW gave error cose {0}")]
    GlfwError(i32),

    /// A Vulkan error.
    #[error("Vulkan function returned error code {0}")]
    VkError(u32),
}

/// Returns the last GLFW error or 0 (`GLFW_NO_ERROR`) if there was none.
pub(crate) fn glfw_error() -> Error {
    Error::GlfwError(unsafe { get_glfw_error() })
}

/// Returns the last Vulkan error or 0 (`VK_SUCCESS`) if there was none.
pub(crate) fn vk_error() -> Error {
    Error::VkError(unsafe { get_vk_error() })
}
