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
#include <algorithm>

#include "types.h"


constexpr vec3 normalizeRGB(s32 red, s32 green, s32 blue) {
    const f32 MAX = 255.f;
    return vec3(red / MAX, green / MAX, blue / MAX);
}

/*
 * Global constants
 */
const s32 WIDTH = 1280;
const s32 HEIGHT = 720;

constexpr vec3 backgroundColor = normalizeRGB(29, 33, 45);

b8 keys[512];
b8 processedKeys[512];

const f32 SPRITE_SIZE = 16.f * 4;
const u32 TILE_SIZE = 64;

vec2 topLeftPosition = vec2(250, 250);


/*
 * Function declarations
 */

// todo: inline?
string readTextFile(const string& path);
void processInput(f32 dt);
u32 createAndCompileShader(e32 shaderType, const string& path);
s32 getUniformLocation(u32 shaderProgram, const string& name);
void setShaderUniform(s32 location, b8 value);
void setShaderUniform(s32 location, s32 value);
void setShaderUniform(s32 location, const vec2& value);
void setShaderUniform(s32 location, const mat4& value);
f32 clamp(f32 value, f32 min, f32 max);

template<typename T>
u64 sizeInBytes(std::initializer_list<const std::vector<T>> vectors) {
    u64 size = 0;
    for (auto& vector: vectors) {
        size += vector.size() * sizeof(T);
    }
    return size;
}


struct animation {
    s32 x;
    s32 y;
    s32 frames;
    f32 delay;
    f32 xOffset;

    b8 operator==(const animation& other) const {
        return x == other.x && y == other.y;
    }

    b8 operator!=(const animation& other) const {
        return !(*this == other);
    }
};

b8 reversed = false;

struct sprite {
    std::vector<animation> animations;
    animation currentAnimation;
    vec2 position;
    vec2 velocity;
};

sprite bob;

// todo: world coordinate system
s32 main(s32 argc, char* argv[]) {

    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    srand((u32) glfwGetTimerValue());

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

    glfwSetKeyCallback(window, [](GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
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

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, s32 button, s32 action, s32 mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            bob.currentAnimation = bob.animations[3];
            bob.currentAnimation.xOffset = 0.f;
        }
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, s32 width, s32 height) {
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

    s32 textureWidth, textureHeight, textureChannels;
    u8* textureImage = stbi_load("textures/industrial_tileset.png",
        &textureWidth, &textureHeight, &textureChannels, 0);
    if (!textureImage) {
        std::cout << "Texture loading failed:" << std::endl << stbi_failure_reason() << std::endl;
    }
    assert(textureImage);

    u32 texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // note: default value for GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR
    // since we do not use mipmaps we must override this value
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureImage);

    stbi_image_free(textureImage);

    
    u32 vertexShader = createAndCompileShader(GL_VERTEX_SHADER, "shaders/basic.vert");
    u32 fragmentShader = createAndCompileShader(GL_FRAGMENT_SHADER, "shaders/basic.frag");

    u32 shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    s32 isShaderProgramLinked;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isShaderProgramLinked);

    if (!isShaderProgramLinked) {
        s32 LOG_LENGTH;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &LOG_LENGTH);

        std::vector<c8> errorLog(LOG_LENGTH);

        glGetProgramInfoLog(shaderProgram, LOG_LENGTH, nullptr, &errorLog[0]);
        std::cerr << "Shader program linkage failed:" << std::endl << &errorLog[0] << std::endl;
    }
    assert(isShaderProgramLinked);
    
    glUseProgram(shaderProgram);

    mat4 projection = glm::ortho(0.0f, (f32) WIDTH, (f32) HEIGHT, 0.0f);
    
    s32 projectionUniformLocation = getUniformLocation(shaderProgram, "projection");
    setShaderUniform(projectionUniformLocation, projection);
    s32 modelUniformLocation = getUniformLocation(shaderProgram, "model");
    s32 typeUniformLocation = getUniformLocation(shaderProgram, "type");
    s32 tileTypeUniformLocation = getUniformLocation(shaderProgram, "tileType");

    s32 spriteOffsetUniformLocation = getUniformLocation(shaderProgram, "spriteOffset");
    
    std::fstream spritesConfigIn("textures/sprites.json");
    json spritesConfig;
    spritesConfigIn >> spritesConfig;

    s32 tileWidth = spritesConfig["tileWidth"];
    s32 tileHeight = spritesConfig["tileHeight"];

    auto bobConfig = spritesConfig["sprites"][0];
    auto bobAnimations = bobConfig["animations"];

    bob = {};
    bob.position = topLeftPosition;
    bob.velocity = {0.f, 0.f};
    bob.animations = {
        { bobAnimations[0]["x"], bobAnimations[0]["y"], bobAnimations[0]["frames"], bobAnimations[0]["delay"], 0.f },
        { bobAnimations[1]["x"], bobAnimations[1]["y"], bobAnimations[1]["frames"], bobAnimations[1]["delay"], 0.f },
        { bobAnimations[2]["x"], bobAnimations[2]["y"], bobAnimations[2]["frames"], bobAnimations[2]["delay"], 0.f },
        { bobAnimations[3]["x"], bobAnimations[3]["y"], bobAnimations[3]["frames"], bobAnimations[3]["delay"], 0.f }
    };
    bob.currentAnimation = bob.animations[0];

    f32 spriteWidth = ((f32) tileWidth) / textureWidth;
    f32 spriteHeight = ((f32) tileHeight) / textureHeight;

    s32 spriteSizeUniformLocation = getUniformLocation(shaderProgram, "spriteSize");
    setShaderUniform(spriteSizeUniformLocation, vec2(spriteWidth, spriteHeight));

    s32 reversedUniformLocation = getUniformLocation(shaderProgram, "reversed");
    setShaderUniform(reversedUniformLocation, reversed);

    f32 vertices[] = {
        // Pos    // UV
        0.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, spriteHeight,
        1.f, 0.f, spriteWidth, 0.f,
        1.f, 1.f, spriteWidth, spriteHeight
    };

    std::fstream levelInfoIn("levels/level01.json");
    std::fstream tileSetInfoIn("levels/tileset.json");
    json levelInfo;
    json tilesetInfo;
    levelInfoIn >> levelInfo;
    tileSetInfoIn >> tilesetInfo;

    u32 columns = tilesetInfo["columns"];
    u32 levelWidth = levelInfo["width"];
    u32 levelHeight = levelInfo["height"];
    std::vector<s32> backgroundRawTiles = levelInfo["layers"][0]["data"];
    std::vector<s32> foregroundRawTiles = levelInfo["layers"][1]["data"];

    std::vector<vec2> xys;
    xys.reserve(levelWidth * levelHeight);
    for (u32 y = 0; y < levelHeight; ++y) {
        for (u32 x = 0; x < levelWidth; ++x) {
            xys.push_back(vec2(x * TILE_SIZE, y * TILE_SIZE));
        }
    }

    std::vector<vec2> backgroundUvs(backgroundRawTiles.size());
    // todo: rotating support
    std::transform(backgroundRawTiles.begin(), backgroundRawTiles.end(), backgroundUvs.begin(),
        [columns, spriteWidth, spriteHeight](s32 tile) {
            s32 uvX = tile > 0 ? ((tile - 1) % columns) : -1;
            s32 uvY = tile > 0 ? ((tile - 1) / columns) : -1;
            return vec2(uvX * spriteWidth, uvY * spriteHeight);
        });

    std::vector<vec2> foregroundUvs(foregroundRawTiles.size());
    std::transform(foregroundRawTiles.begin(), foregroundRawTiles.end(), foregroundUvs.begin(),
        [columns, spriteWidth, spriteHeight](s32 tile) {
            s32 uvX = tile > 0 ? ((tile - 1) % columns) : -1;
            s32 uvY = tile > 0 ? ((tile - 1) / columns) : -1;
            return vec2(uvX * spriteWidth, uvY * spriteHeight);
    });
    
    u32 VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeInBytes({xys, backgroundUvs, foregroundUvs}), nullptr, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeInBytes({xys}), xys.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeInBytes({xys}), 
        sizeInBytes({backgroundUvs}), backgroundUvs.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeInBytes({xys, backgroundUvs}), 
        sizeInBytes({foregroundUvs}), foregroundUvs.data());

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*) 0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*) sizeof(vertices));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*) (sizeof(vertices) + sizeInBytes({xys})));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*) (sizeof(vertices) + sizeInBytes({xys, backgroundUvs})));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    f64 lastTime = glfwGetTime();
    f64 currentTime;
    f64 delta = 0;
    
    f64 frameTime = 0.f;
    
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();

        glfwPollEvents();
        processInput((f32) delta);

        glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //--- drawing tilemap ---
        setShaderUniform(typeUniformLocation, 1);

        mat4 model = mat4(1.0f);
        model = glm::scale(model, vec3(SPRITE_SIZE));
        setShaderUniform(modelUniformLocation, model);
        
        setShaderUniform(tileTypeUniformLocation, 0);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, levelWidth * levelHeight);

        setShaderUniform(tileTypeUniformLocation, 1);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, levelWidth * levelHeight);

        //--- drawing bob ---
        setShaderUniform(typeUniformLocation, 2);

        model = mat4(1.0f);
        model = glm::translate(model, vec3(bob.position, 0.0f));
        model = glm::scale(model, vec3(SPRITE_SIZE));
        setShaderUniform(modelUniformLocation, model);

        f32 bobXOffset = (tileWidth * (f32) bob.currentAnimation.x) / textureWidth;
        f32 bobYOffset = (tileHeight * (f32) bob.currentAnimation.y) / textureHeight;

        if (frameTime >= bob.currentAnimation.delay) {
            bob.currentAnimation.xOffset += spriteWidth;
            if (bob.currentAnimation.xOffset >= ((bob.currentAnimation.frames * tileWidth) / (f32) textureWidth)) {
                bob.currentAnimation.xOffset = 0.f;
                if (bob.currentAnimation == bob.animations[3]) {
                    bob.currentAnimation = bob.animations[0];
                }
            }

            frameTime = 0.0f;
        }
        setShaderUniform(spriteOffsetUniformLocation, vec2(bobXOffset + bob.currentAnimation.xOffset, bobYOffset));
        setShaderUniform(reversedUniformLocation, reversed);
        
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
void processInput(f32 dt) {
    f32 acceleration = 0;
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
    if (keys[GLFW_KEY_LEFT] == GLFW_PRESS) {
        if (bob.currentAnimation != bob.animations[2]) {
            bob.currentAnimation = bob.animations[2];
            reversed = true;
        }
        acceleration = -800.f;
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
        acceleration = 800.f;
    }
    if (keys[GLFW_KEY_RIGHT] == GLFW_RELEASE && !processedKeys[GLFW_KEY_RIGHT]) {
        processedKeys[GLFW_KEY_RIGHT] = true;
        if (bob.currentAnimation != bob.animations[0]) {
            bob.currentAnimation= bob.animations[0];
            bob.currentAnimation.xOffset = 0.f;
            reversed = false;
        }
    }
    
    acceleration += -3.f * bob.velocity.x;
    bob.velocity.x += acceleration * dt;
    bob.position.x = clamp(0.5f * acceleration * dt * dt + bob.velocity.x * dt + bob.position.x, 0.f, WIDTH - SPRITE_SIZE);
}

u32 createAndCompileShader(e32 shaderType, const string& path) {
    u32 shader = glCreateShader(shaderType);
    string shaderSource = readTextFile(path);
    const c8* shaderSourcePtr = shaderSource.c_str();
    glShaderSource(shader, 1, &shaderSourcePtr, nullptr);
    glCompileShader(shader);

    s32 isShaderCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
    if (!isShaderCompiled) {
        s32 LOG_LENGTH;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &LOG_LENGTH);

        std::vector<c8> errorLog(LOG_LENGTH);

        glGetShaderInfoLog(shader, LOG_LENGTH, nullptr, &errorLog[0]);
        std::cerr << "Shader compilation failed:" << std::endl << &errorLog[0] << std::endl;
    }
    assert(isShaderCompiled);

    return shader;
}

string readTextFile(const string& path) {
    std::ifstream in(path);

    assert(in.good());

    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

s32 getUniformLocation(u32 shaderProgram, const string& name) {
    s32 uniformLocation = glGetUniformLocation(shaderProgram, name.c_str());
    assert(uniformLocation != -1);
    return uniformLocation;
}

void setShaderUniform(s32 location, b8 value) {
    glUniform1i(location, value);
}

void setShaderUniform(s32 location, s32 value) {
    glUniform1i(location, value);
}

void setShaderUniform(s32 location, const vec2& value) {
    glUniform2f(location, value.x, value.y);
}

void setShaderUniform(s32 location, const mat4& value) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

f32 clamp(f32 value, f32 min, f32 max) {
    if (value < min) return min;
    if (value > max) return max;

    return value;
}
