#define GLEW_STATIC
#include <GL/glew.h>
#include "Window.h"
#include <math.h>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

const int WIDTH = 1600, HEIGHT = 900;

int main(int argc, char* argv[]) {
    std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;

    if (glfwInit() != GL_TRUE) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    Window window(WIDTH, HEIGHT, "Fuzzy", false, true);
    glfwSetKeyCallback(window.get(), key_callback);

    // Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
    glewExperimental = GL_TRUE;
    // Initialize GLEW to setup the OpenGL Function pointers
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window.get(), &width, &height);
    glViewport(0, 0, width, height);

    while(!window.isClosing()) {
        glfwPollEvents();

        glClearColor(0.2f, (sin(glfwGetTime()) / 2) + 0.5, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        window.update();
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    std::cout << key << std::endl;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}
