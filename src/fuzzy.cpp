#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>

const int WIDTH = 1280;
const int HEIGHT = 720;

float blueOffset = 0;
float greenOffset = 0;

bool keys[512];

void processInput();

int main(int argc, char* argv[])
{
    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Fuzzy", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);

    glfwSetWindowPos(window, (vidmode->width - WIDTH) / 2, (vidmode->height - HEIGHT) / 2);

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (action == GLFW_PRESS) {
            keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            keys[key] = false;
        }
    });

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        processInput();

        glClearColor(0.5f, greenOffset, blueOffset, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}

void processInput() {
    float step = 0.0005f;
    if (keys[GLFW_KEY_UP] == GLFW_PRESS) {
        if (blueOffset < 1.0f) {
            blueOffset += step;
        }
    }
    if (keys[GLFW_KEY_DOWN] == GLFW_PRESS) {
        if (blueOffset > 0.f) {
            blueOffset -= step;
        }
    }
    if (keys[GLFW_KEY_RIGHT] == GLFW_PRESS) {
        if (greenOffset < 1.0f) {
            greenOffset += step;
        }
    }
    if (keys[GLFW_KEY_LEFT] == GLFW_PRESS) {
        if (greenOffset > 0.f) {
            greenOffset -= step;
        }
    }
}
