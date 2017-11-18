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

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdlib>
#include <string>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

const int WIDTH = 1280;
const int HEIGHT = 720;

float redOffset = 0.5f;
float greenOffset = 0.6f;
float blueOffset = 0.92f;

bool keys[512];
bool processedKeys[512];

const float SIZE = 50.f;

glm::vec2 topLeftPosition = glm::vec2(WIDTH / 2 - SIZE, HEIGHT / 2 - SIZE);

std::string readTextFile(const std::string& path);
void processInput();

GLuint createAndCompileShader(GLenum shaderType, const std::string& path);

int main(int argc, char* argv[])
{
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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
            processedKeys[key] = false;
        }
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    glViewport(0, 0, WIDTH, HEIGHT);
    
    float vertices[] = {
        0.f, 0.f,
        0.f, SIZE,
        SIZE, 0.f,
        SIZE, SIZE
    };

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    GLuint vertexShader = createAndCompileShader(GL_VERTEX_SHADER, "shaders/basic.vert");
    GLuint fragmentShader = createAndCompileShader(GL_FRAGMENT_SHADER, "shaders/basic.frag");

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint isShaderProgramLinked;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isShaderProgramLinked);

    if (!isShaderProgramLinked) {
        GLint LOG_LENGTH;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &LOG_LENGTH);

        std::vector<GLchar> errorLog(LOG_LENGTH);

        glGetProgramInfoLog(shaderProgram, LOG_LENGTH, nullptr, &errorLog[0]);
        std::cerr << "Shader program linkage failed:" << std::endl << &errorLog[0] << std::endl;
    }
    assert(isShaderProgramLinked);
    
    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float) WIDTH, (float) HEIGHT, 0.0f);
    
    GLint projectionUniformLocation = glGetUniformLocation(shaderProgram, "projection");
    assert(projectionUniformLocation != -1);
    glUniformMatrix4fv(projectionUniformLocation, 1, GL_FALSE, glm::value_ptr(projection));

    GLint modelUniformLocation = glGetUniformLocation(shaderProgram, "model");
    assert(modelUniformLocation != -1);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    double lastTime = glfwGetTime();
    double currentTime;
    double delta;

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();

        glfwPollEvents();

        processInput();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(topLeftPosition, 0.0f));
        glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, glm::value_ptr(model));

        glClearColor(redOffset, greenOffset, blueOffset, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);

        delta = currentTime - lastTime;
        lastTime = currentTime;

//        std::cout << delta * 1000.f << " ms" << std::endl;
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}

void processInput() {
    float step = 8.f;
    if (keys[GLFW_KEY_UP] == GLFW_PRESS) {
        if (topLeftPosition.y > 0.f) {
            topLeftPosition.y -= step;
        }
    }
    if (keys[GLFW_KEY_DOWN] == GLFW_PRESS) {
        if (topLeftPosition.y < HEIGHT - SIZE) {
            topLeftPosition.y += step;
        }
    }
    if (keys[GLFW_KEY_LEFT] == GLFW_PRESS) {
        if (topLeftPosition.x > 0.f) {
            topLeftPosition.x -= step;
        }
    }
    if (keys[GLFW_KEY_RIGHT] == GLFW_PRESS) {
        if (topLeftPosition.x < WIDTH - SIZE) {
            topLeftPosition.x += step;
        }
    }
    if (keys[GLFW_KEY_SPACE] == GLFW_PRESS && !processedKeys[GLFW_KEY_SPACE]) {
        processedKeys[GLFW_KEY_SPACE] = true;
        redOffset = (float) (rand()) / (float) RAND_MAX;
        greenOffset = (float) (rand()) / (float) RAND_MAX;
        blueOffset = (float) (rand()) / (float) RAND_MAX;
    }
}

GLuint createAndCompileShader(GLenum shaderType, const std::string& path) {
    GLuint shader = glCreateShader(shaderType);
    std::string shaderSource = readTextFile(path);
    const GLchar* shaderSourcePtr = shaderSource.c_str();
    glShaderSource(shader, 1, &shaderSourcePtr, nullptr);
    glCompileShader(shader);

    GLint isShaderCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
    if (!isShaderCompiled) {
        GLint LOG_LENGTH;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &LOG_LENGTH);

        std::vector<GLchar> errorLog(LOG_LENGTH);

        glGetShaderInfoLog(shader, LOG_LENGTH, nullptr, &errorLog[0]);
        std::cerr << "Shader compilation failed:" << std::endl << &errorLog[0] << std::endl;
    }
    assert(isShaderCompiled);

    return shader;
}

std::string readTextFile(const std::string& path) {
    std::ifstream in(path);

    assert(in.good());

    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
