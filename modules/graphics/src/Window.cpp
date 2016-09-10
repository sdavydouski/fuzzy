#include "../include/Window.h"
#include <stdexcept>
#include <string>

using namespace graphics;

Window::Window(int width,
               int height,
               std::string title,
               bool isFullScreen,
               bool vsync) :
        _width(width), _height(height), _title(title), _vsync(vsync) {
    if (isFullScreen) {
        this->_window = glfwCreateWindow(width, height, title.c_str(), glfwGetPrimaryMonitor(), nullptr);
    } else {
        this->_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

        // Center window on screen
        auto vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowPos(this->_window,
                         (vidMode->width - width) / 2,
                         (vidMode->height - height) / 2);
    }

    if (this->_window == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Create OpenGL context
    glfwMakeContextCurrent(this->_window);

    // Enable v-sync
    if (vsync) {
        glfwSwapInterval(1);
    }

    // Define the viewport dimensions
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(this->_window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);
}

void Window::setKeyCallback(GLFWkeyfun callback) {
    glfwSetKeyCallback(this->_window, callback);
}

void Window::setMouseCallback(GLFWcursorposfun callback) {
    glfwSetCursorPosCallback(this->_window, callback);
}

void Window::setScrollCallback(GLFWscrollfun callback) {
    glfwSetScrollCallback(this->_window, callback);
}

bool Window::isClosing() {
    return (bool) glfwWindowShouldClose(this->_window);
}

void Window::setIsShoudClose(bool isShouldClose) {
    glfwSetWindowShouldClose(this->_window, isShouldClose);
}

bool Window::isVSyncEnabled() {
    return this->_vsync;
}

void Window::setVSync(bool vsync) {
    this->_vsync = vsync;
    if (vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
}

void Window::makeContextCurrent() {
    glfwMakeContextCurrent(this->_window);
}

void Window::setInputMode(int mode, int value) {
    glfwSetInputMode(this->_window, mode, value);
}

void Window::swapBuffers() {
    glfwSwapBuffers(this->_window);
}

void Window::destroy() {
    glfwDestroyWindow(this->_window);
}

Window::~Window() {
    //empty destructor
}
