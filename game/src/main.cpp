#include <GL/glew.h>
#include "Window.h"
#include "WindowManager.h"
#include "glutils/ShaderProgram.h"
#include "glutils/Shader.h"
#include "Texture.h"
#include <iostream>

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

    Shader vertexShader = Shader::load(Shader::VERTEX, "resources/shaders/basic/shader.vert");
    Shader fragmentShader = Shader::load(Shader::FRAGMENT, "resources/shaders/basic/shader.frag");

    ShaderProgram shaderProgram(vertexShader, fragmentShader);

    GLfloat vertices[] = {
            // Positions          // Colors           // Texture Coords
            0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // Top Right
            0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // Bottom Right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // Bottom Left
            -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // Top Left
    };
    GLuint indices[] = {
            0, 1, 3,  // First Triangle
            1, 2, 3   // Second Triangle
    };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // TexCoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // Unbind VAO

    Texture texture1 = Texture::load("resources/textures/boxes/wooden_container.jpg");
    Texture texture2 = Texture::load("resources/textures/boxes/awesomeface.png");

    while(!window.isClosing()) {
        glfwPollEvents();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Bind Textures using texture units
        texture1.bind(0);
        shaderProgram.setUniform("ourTexture1", 0);
        texture2.bind(1);
        shaderProgram.setUniform("ourTexture2", 1);

        // Activate shader
        shaderProgram.use();

        // Draw container
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        window.swapBuffers();
    }

    // Properly de-allocate all resources once they've outlived their purpose
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    window.destroy();
    windowManager.shutDown();

    return 0;
}
