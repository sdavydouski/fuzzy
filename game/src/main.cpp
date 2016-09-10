#define GLEW_STATIC
#include <GL/glew.h>
#include <math.h>
#include "Window.h"
#include "WindowManager.h"
#include <iostream>
#include "glutils/ShaderProgram.h";

using namespace graphics;

auto& windowManager = WindowManager::Instance();

int main(int argc, char* argv[]) {
    windowManager.startUp();

    Window window(1600, 900, "Fuzzy");
    window.setKeyCallback([](GLFWwindow* window,
                             int key,
                             int scancode,
                             int action,
                             int mode) -> void {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
    });

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    while(!window.isClosing()) {
        glfwPollEvents();

        glClearColor(0.2f, (float) (sin(glfwGetTime()) / 2) + 0.5f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        window.swapBuffers();
    }

    window.destroy();
    windowManager.shutDown();

    return 0;
}
