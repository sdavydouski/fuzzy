// GLAD will not include windows.h for APIENTRY if it was previously defined
#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// confirm that neither GLAD nor GLFW didn't include windows.h
#ifdef _WINDOWS_
#error windows.h was included!
#endif

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
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Fuzzy", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
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

    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    double lastTime = glfwGetTime();
    double currentTime;
    double delta;

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();

        glfwPollEvents();

        processInput();

        glClearColor(0.5f, greenOffset, blueOffset, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);

        delta = currentTime - lastTime;
        lastTime = currentTime;

        //std::cout << delta * 1000.f << " ms" << std::endl;
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
