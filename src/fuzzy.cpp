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

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdlib>
#include <string>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>


constexpr float color(float intensity) {
    return intensity / 255.f;
}

/*
 * Global constants
 */
const int WIDTH = 1280;
const int HEIGHT = 720;

float red = color(29);
float green = color(33);
float blue = color(45);

bool keys[512];
bool processedKeys[512];

const float SPRITE_WIDTH = 13.f * 4;
const float SPRITE_HEIGHT = 14.f * 4;

glm::vec2 topLeftPosition = glm::vec2(WIDTH / 2 - SPRITE_WIDTH, HEIGHT / 2 - SPRITE_HEIGHT);


/*
 * Function declarations
 */
std::string readTextFile(const std::string& path);
void processInput();
GLuint createAndCompileShader(GLenum shaderType, const std::string& path);



int main(int argc, char* argv[]) {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    srand((unsigned int) glfwGetTimerValue());

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << glGetString(GL_VERSION) << std::endl;

    glViewport(0, 0, WIDTH, HEIGHT);
    
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

    GLint spriteOffsetUniformLocation = glGetUniformLocation(shaderProgram, "spriteOffset");
    assert(spriteOffsetUniformLocation != -1);
    glUniform2f(spriteOffsetUniformLocation, 0.f, 0.f);

    int textureWidth, textureHeight, textureChannels;
    unsigned char* textureImage = stbi_load("textures/industrial_tileset.png", 
                                            &textureWidth, &textureHeight, &textureChannels, 0);
    if (!textureImage) {
        std::cout << "Texture loading failed:" << std::endl << stbi_failure_reason() << std::endl;
    }
    assert(textureImage);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // note: default value for GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR
    // since we do not use mipmaps we must override this value
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureImage);

    stbi_image_free(textureImage);

    struct frame {
        float x;
        float y;
        float width;
        float height;
        float xOffset;
        float yOffset;
    } spriteFrame;

    spriteFrame.x = 1.f / textureWidth;
    spriteFrame.y = 258.f / textureHeight;
    spriteFrame.width = 13.f / textureWidth;
    spriteFrame.height = 14.f / textureHeight;
    spriteFrame.xOffset = 0.f;
    spriteFrame.yOffset = 0.f;

    float vertices[] = {
        0.f, 0.f, spriteFrame.x, spriteFrame.y,
        0.f, SPRITE_HEIGHT, spriteFrame.x, spriteFrame.y + spriteFrame.height,
        SPRITE_WIDTH, 0.f, spriteFrame.x + spriteFrame.width, spriteFrame.y,
        SPRITE_WIDTH, SPRITE_HEIGHT, spriteFrame.x + spriteFrame.width, spriteFrame.y + spriteFrame.height,
    };

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    double lastTime = glfwGetTime();
    double currentTime;
    double delta;
    
    float xOffset = 3.f / textureWidth;
    float yOffset = 0.f / textureHeight;
    
    int counter = 0;

    while (!glfwWindowShouldClose(window)) {
        ++counter;
        currentTime = glfwGetTime();

        glfwPollEvents();

        processInput();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(topLeftPosition, 0.0f));
        glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, glm::value_ptr(model));
        
        if (counter > 15) {
            spriteFrame.xOffset += spriteFrame.width + xOffset;
            if (spriteFrame.xOffset > 45.f / 512.f) {
                spriteFrame.xOffset = 0.f;
            }
  
            counter = 0;
        }
        glUniform2f(spriteOffsetUniformLocation, spriteFrame.xOffset, 0.f);

        glClearColor(red, green, blue, 1.0f);
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


/*
 * Function definitions
 */
void processInput() {
    float step = 6.f;
    if (keys[GLFW_KEY_UP] == GLFW_PRESS) {
        if (topLeftPosition.y > 0.f) {
            topLeftPosition.y -= step;
        }
    }
    if (keys[GLFW_KEY_DOWN] == GLFW_PRESS) {
        if (topLeftPosition.y < HEIGHT - SPRITE_HEIGHT) {
            topLeftPosition.y += step;
        }
    }
    if (keys[GLFW_KEY_LEFT] == GLFW_PRESS) {
        if (topLeftPosition.x > 0.f) {
            topLeftPosition.x -= step;
        }
    }
    if (keys[GLFW_KEY_RIGHT] == GLFW_PRESS) {
        if (topLeftPosition.x < WIDTH - SPRITE_WIDTH) {
            topLeftPosition.x += step;
        }
    }
    if (keys[GLFW_KEY_SPACE] == GLFW_PRESS && !processedKeys[GLFW_KEY_SPACE]) {
        processedKeys[GLFW_KEY_SPACE] = true;
        red = (float) (rand()) / (float) RAND_MAX;
        green = (float) (rand()) / (float) RAND_MAX;
        blue = (float) (rand()) / (float) RAND_MAX;
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
