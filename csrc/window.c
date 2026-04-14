#include "window.h"
#include <GLFW/glfw3.h>

static const Window NULL_WINDOW = {
    .window = NULL,
    .should_close = true,
};

Window create_window(cstr title, u32 width, u32 height) {
    Window window = NULL_WINDOW;

    if (glfwInit() != GLFW_TRUE) {
        goto FAIL;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window.window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window.window == NULL) {
        goto FAIL;
    }

    window.width = width;
    window.height = height;
    window.should_close = false;
    return window;

    // Failure jump; cleans and returns a null window.
FAIL:
    destroy_window(&window);
    return NULL_WINDOW;
}

void destroy_window(Window* window) {
    if (window != NULL && window->window != NULL) {
        glfwDestroyWindow(window->window);
    }
    glfwTerminate();
}

void update_window(Window* window) {
    // Poll events/closing.
    glfwPollEvents();
    window->should_close = glfwWindowShouldClose(window->window);

    // Update dimensions.
    i32 width;
    i32 height;
    glfwGetFramebufferSize(window->window, &width, &height);
    window->width = width;
    window->height = height;
}
