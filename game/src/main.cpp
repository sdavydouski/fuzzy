#include <GL/glew.h>
#include "../../externals/glfw/deps/linmath.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <math.h>
#include "Window.h"
#include "WindowManager.h"
#include "glutils/ShaderProgram.h"
#include "glutils/Shader.h"
#include "g3d/Model.h"
#include "Texture.h"
#include "Camera.h"

using namespace graphics;

auto& windowManager = WindowManager::Instance();
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Moves/alters the camera positions based on user input
void Do_Movement() {
    // Camera controls
    if(keys[GLFW_KEY_W]) {
        camera.ProcessKeyboard(camera.FORWARD, deltaTime);
    }
    if(keys[GLFW_KEY_S]) {
        camera.ProcessKeyboard(camera.BACKWARD, deltaTime);
    }
    if(keys[GLFW_KEY_A]) {
        camera.ProcessKeyboard(camera.LEFT, deltaTime);
    }
    if(keys[GLFW_KEY_D]) {
        camera.ProcessKeyboard(camera.RIGHT, deltaTime);
    }
}

int main(int argc, char* argv[]) {
    int screenWidth = 1600;
    int screenHeight = 900;

    windowManager.startUp();

    Window window(screenWidth, screenHeight, "Fuzzy");
    window.setKeyCallback([](GLFWwindow* window,
                             int key,
                             int scancode,
                             int action,
                             int mode) -> void {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        if (key >= 0 && key < 1024) {
            if (action == GLFW_PRESS) {
                keys[key] = true;
            }
            else if (action == GLFW_RELEASE) {
                keys[key] = false;
            }
        }
    });
    window.setMouseCallback([](GLFWwindow* window, double xpos, double ypos) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        GLfloat xoffset = xpos - lastX;
        GLfloat yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    });
    window.setScrollCallback([](GLFWwindow* window, double xoffset, double yoffset) {
        camera.ProcessMouseScroll(yoffset);
    });

    // Options
    window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    ShaderProgram shaderProgram(
        Shader::load(Shader::VERTEX, "resources/shaders/basic/shader.vert"),
        Shader::load(Shader::FRAGMENT, "resources/shaders/basic/shader.frag")
    );

    Model nanosuit("resources/models/nanosuit/nanosuit.obj");

    // Draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while(!window.isClosing()) {
        // Set frame time
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check and call events
        glfwPollEvents();
        Do_Movement();

        // Clear the colorbuffer
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        shaderProgram.use();

        // Create camera transformation
        glm::mat4 view;
        view = camera.GetViewMatrix();
        glm::mat4 projection;
        projection = glm::perspective(camera.Zoom, (float)screenWidth/(float)screenHeight, 0.1f, 1000.0f);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);

        // Draw the loaded model
        glm::mat4 model;
        // Translate it down a bit so it's at the center of the scene
        model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f));
        // It's a bit too big for our scene, so scale it down
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        shaderProgram.setUniform("model", model);

        // finally...
        nanosuit.Draw(shaderProgram);

        window.swapBuffers();
    }

    //todo: delete all model buffers (vao, vbo, ebo)

    window.destroy();
    windowManager.shutDown();

    return 0;
}
