#pragma once

#include "def.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/// A window.
typedef struct Window {
    /// The GLFW window handle.
    GLFWwindow* window;

    /// The window's width.
    u32 width;

    /// The window's height.
    u32 height;

    /// Whether the window should close.
    bool should_close;
} Window;

/// Creates a window with the given title, width, and height.
Window create_window(cstr title, u32 width, u32 height);

/// Destroys the window.
void destroy_window(Window* window);

/// Updates the window.
void update_window(Window* window);
