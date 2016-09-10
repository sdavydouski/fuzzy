#include "../include/WindowManager.h"
#include "../include/Window.h"
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <string>
#include <iostream>

using namespace graphics;

WindowManager::WindowManager() {
    //empty constructor
}

WindowManager& WindowManager::Instance() {
    static WindowManager instance;
    return instance;
}

void WindowManager::startUp() {
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

void WindowManager::shutDown() {
    glfwTerminate();
}

WindowManager::~WindowManager() {
    //empty destructor
}