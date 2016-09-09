#include "../include/WindowManager.h"
#include "../include/Window.h"
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <string>
#include <iostream>

using namespace graphics;

WindowManager::WindowManager() {
    std::cout << "Init WindowManager" << std::endl;
    glfwSetErrorCallback([] (int errorCode, const char* description) -> void {
        std::cout << description << std::endl;
    });

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
}

WindowManager& WindowManager::Instance() {
    static WindowManager instance;
    return instance;
}

Window WindowManager::createWindow(int width,
                                   int height,
                                   std::string title,
                                   bool isFullScreen,
                                   bool vsync) {
    return Window(width, height, title, isFullScreen, vsync);
}

WindowManager::~WindowManager() {
    std::cout << "Destroying WindowManager" << std::endl;
    glfwTerminate();
}