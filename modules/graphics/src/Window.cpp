#include "../include/Window.h"

Window::Window(int width, int height, std::string title, bool isFullScreen, bool vsync) {
    this->width = width;
    this->height = height;
    this->title = title;
    this->vsync = vsync;

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    if (isFullScreen) {
        this->window = glfwCreateWindow(width, height, title.c_str(), glfwGetPrimaryMonitor(), nullptr);
    } else {
        this->window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

        // Center window on screen
        auto vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowPos(this->window,
                         (vidMode->width - width) / 2,
                         (vidMode->height - height) / 2);
    }

    if (this->window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Create OpenGL context
    glfwMakeContextCurrent(this->window);

    // Enable v-sync
    if (vsync) {
        glfwSwapInterval(1);
    }
}

Window::~Window() {
    glfwDestroyWindow(this->window);
}

GLFWwindow* Window::get() {
    return this->window;
}

bool Window::isClosing() {
    return glfwWindowShouldClose(this->window) != 0;
}

void Window::setIsShoudClose(bool isShoudClose) {
    glfwSetWindowShouldClose(this->window, isShoudClose);
}

bool Window::isVSyncEnabled() {
    return this->vsync;
}

void Window::setVSync(bool vsync) {
    this->vsync = vsync;
    if (vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
}

void Window::setInputMode(int mode, int value) {
    glfwSetInputMode(this->window, mode, value);
}

void Window::update() {
    glfwSwapBuffers(this->window);
}
