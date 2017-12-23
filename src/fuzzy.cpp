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

#include <json.hpp>

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


constexpr glm::vec3 normalizeRGB(int red, int green, int blue) {
    const float MAX = 255.f;
    return glm::vec3(red / MAX, green / MAX, blue / MAX);
}

/*
 * Global constants
 */
const int WIDTH = 1280;
const int HEIGHT = 720;

constexpr glm::vec3 backgroundColor = normalizeRGB(29, 33, 45);

bool keys[512];
bool processedKeys[512];

const float SPRITE_SIZE = 16.f * 4;

glm::vec2 topLeftPosition = glm::vec2(WIDTH / 2 - SPRITE_SIZE, HEIGHT / 2 - SPRITE_SIZE);


/*
 * Function declarations
 */
std::string readTextFile(const std::string& path);
void processInput();
GLuint createAndCompileShader(GLenum shaderType, const std::string& path);


// for convenience
using json = nlohmann::json;


struct animation {
    int x;
    int y;
    int frames;
    float delay;
    float xOffset;

    bool operator==(const animation& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const animation& other) const {
        return !(*this == other);
    }
};

bool reversed = false;

struct sprite {
    std::vector<animation> animations;
    animation currentAnimation;
};

sprite bob;

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

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            bob.currentAnimation = bob.animations[3];
            bob.currentAnimation.xOffset = 0.f;
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
    
    std::fstream spritesConfigIn("textures/sprites.json");
    json spritesConfig;
    spritesConfigIn >> spritesConfig;

    int tileWidth = spritesConfig["tileWidth"];
    int tileHeight = spritesConfig["tileHeight"];

    auto bobConfig = spritesConfig["sprites"][0];
    auto bobAnimations = bobConfig["animations"];

    bob = {};
    bob.animations = {
        { bobAnimations[0]["x"], bobAnimations[0]["y"], bobAnimations[0]["frames"], bobAnimations[0]["delay"], 0.f },
        { bobAnimations[1]["x"], bobAnimations[1]["y"], bobAnimations[1]["frames"], bobAnimations[1]["delay"], 0.f },
        { bobAnimations[2]["x"], bobAnimations[2]["y"], bobAnimations[2]["frames"], bobAnimations[2]["delay"], 0.f },
        { bobAnimations[3]["x"], bobAnimations[3]["y"], bobAnimations[3]["frames"], bobAnimations[3]["delay"], 0.f }
    };
    bob.currentAnimation = bob.animations[0];

    float spriteWidth = ((float) tileWidth) / textureWidth;
    float spriteHeight = ((float) tileHeight) / textureHeight;

    GLint spriteSizeUniformLocation = glGetUniformLocation(shaderProgram, "spriteSize");
    assert(spriteSizeUniformLocation != -1);
    glUniform2f(spriteSizeUniformLocation, spriteWidth, spriteHeight);

    GLint reversedUniformLocation = glGetUniformLocation(shaderProgram, "reversed");
    assert(reversedUniformLocation != -1);
    glUniform1i(reversedUniformLocation, reversed);

    float vertices[] = {
        // Pos    // UV
        0.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, spriteHeight,
        1.f, 0.f, spriteWidth, 0.f,
        1.f, 1.f, spriteWidth, spriteHeight
    };

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    double lastTime = glfwGetTime();
    double currentTime;
    double delta;
    
    double frameTime = 0.f;

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();

        glfwPollEvents();

        processInput();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(topLeftPosition, 0.0f));
        model = glm::scale(model, glm::vec3(SPRITE_SIZE));
        glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, glm::value_ptr(model));

        float bobXOffset = (tileWidth * (float)bob.currentAnimation.x) / textureWidth;
        float bobYOffset = (tileHeight * (float)bob.currentAnimation.y) / textureHeight;

        if (frameTime >= bob.currentAnimation.delay) {
            bob.currentAnimation.xOffset += spriteWidth;
            if (bob.currentAnimation.xOffset >= ((bob.currentAnimation.frames * tileWidth) / (float) textureWidth)) {
                bob.currentAnimation.xOffset = 0.f;
                if (bob.currentAnimation == bob.animations[3]) {
                    bob.currentAnimation = bob.animations[0];
                }
            }

            frameTime = 0.0f;
        }
        glUniform2f(spriteOffsetUniformLocation, bobXOffset + bob.currentAnimation.xOffset, bobYOffset);

        glUniform1i(reversedUniformLocation, reversed);

        glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);

        delta = currentTime - lastTime;
        lastTime = currentTime;

        frameTime += delta;

        //std::cout << delta * 1000.f << " ms" << std::endl;
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}


/*
 * Function definitions
 */
void processInput() {
    float step = 4.f;
    //if (keys[GLFW_KEY_UP] == GLFW_PRESS) {
    //    if (topLeftPosition.y > 0.f) {
    //        topLeftPosition.y -= step;
    //    }
    //}
    //if (keys[GLFW_KEY_DOWN] == GLFW_PRESS) {
    //    if (topLeftPosition.y < HEIGHT - SPRITE_SIZE) {
    //        topLeftPosition.y += step;
    //    }
    //}
    // todo: jump
    if (keys[GLFW_KEY_LEFT] == GLFW_PRESS) {
        if (bob.currentAnimation != bob.animations[2]) {
            bob.currentAnimation = bob.animations[2];
            reversed = true;
        }
        if (topLeftPosition.x > 0.f) {
            topLeftPosition.x -= step;
        }
    }

    if (keys[GLFW_KEY_LEFT] == GLFW_RELEASE && !processedKeys[GLFW_KEY_LEFT]) {
        processedKeys[GLFW_KEY_LEFT] = true;
        if (bob.currentAnimation != bob.animations[0]) {
            bob.currentAnimation = bob.animations[0];
            bob.currentAnimation.xOffset = 0.f;
            reversed = true;
        }
    }

    if (keys[GLFW_KEY_RIGHT] == GLFW_PRESS) {
        if (bob.currentAnimation != bob.animations[2]) {
            bob.currentAnimation = bob.animations[2];
            reversed = false;
        }
        if (topLeftPosition.x < WIDTH - SPRITE_SIZE) {
            topLeftPosition.x += step;
        }
    }
    if (keys[GLFW_KEY_RIGHT] == GLFW_RELEASE && !processedKeys[GLFW_KEY_RIGHT]) {
        processedKeys[GLFW_KEY_RIGHT] = true;
        if (bob.currentAnimation != bob.animations[0]) {
            bob.currentAnimation= bob.animations[0];
            bob.currentAnimation.xOffset = 0.f;
            reversed = false;
        }
    }
//    if (keys[GLFW_KEY_SPACE] == GLFW_PRESS && !processedKeys[GLFW_KEY_SPACE]) {
//        processedKeys[GLFW_KEY_SPACE] = true;
//        red = (float) (rand()) / (float) RAND_MAX;
//        green = (float) (rand()) / (float) RAND_MAX;
//        blue = (float) (rand()) / (float) RAND_MAX;
//    }
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
