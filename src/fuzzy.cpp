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

#include <nlohmann\json.hpp>

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

#include "types.h"

struct aabb {
    vec2 position;      // top-left
    vec2 size;
};

constexpr vec3 normalizeRGB(s32 red, s32 green, s32 blue) {
    const f32 MAX = 255.f;
    return vec3(red / MAX, green / MAX, blue / MAX);
}

/*
 * Global constants
 */
const s32 SCREEN_WIDTH = 1280;
const s32 SCREEN_HEIGHT = 720;

constexpr vec3 backgroundColor = normalizeRGB(29, 33, 45);

b8 keys[512];
b8 processedKeys[512];

const u32 SCALE = 4;

const f32 SPRITE_SIZE = 16.f * SCALE;
const f32 TILE_SIZE = 16.f * SCALE;

// top-left corner
vec2 camera = vec2(0.f);

const u32 FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
const u32 FLIPPED_VERTICALLY_FLAG = 0x40000000;
const u32 FLIPPED_DIAGONALLY_FLAG = 0x20000000;

struct animation {
    s32 x;
    s32 y;
    s32 frames;
    f32 delay;
    s32 size;

    b8 operator==(const animation& other) const {
        return x == other.x && y == other.y;
    }

    b8 operator!=(const animation& other) const {
        return !(*this == other);
    }
};

struct entity {
    aabb box;
};

struct drawableEntity {
    aabb box;

    vec2 uv;
    f32 rotation;
    
    u32 flipped;
    
    vec2 spriteScale;
    u32 shouldRender;

    u32 offset;
};

struct sprite {
    vector<animation> animations;
    animation currentAnimation;
    f32 xAnimationOffset;
    f32 frameTime;
    u32 flipped;

    aabb box;
    vec2 velocity;
    vec2 acceleration;
};

struct effect {
    vector<animation> animations;
    animation currentAnimation;
    f32 xAnimationOffset;
    f32 frameTime;
    u32 flipped;

    aabb box;

    b8 shouldRender;
};

string readTextFile(const string& path);
void processInput();
u32 createAndCompileShader(e32 shaderType, const string& path);
vec2 sweptAABB(const vec2 point, const vec2 delta, const aabb& box, vec2 padding = vec2(0.f));

inline s32 getUniformLocation(u32 shaderProgram, const string& name) {
    s32 uniformLocation = glGetUniformLocation(shaderProgram, name.c_str());
    //assert(uniformLocation != -1);
    return uniformLocation;
}

inline void setShaderUniform(s32 location, b8 value) {
    glUniform1i(location, value);
}
inline void setShaderUniform(s32 location, s32 value) {
    glUniform1i(location, value);
}

inline void setShaderUniform(s32 location, const vec2& value) {
    glUniform2f(location, value.x, value.y);
}

inline void setShaderUniform(s32 location, const mat4& value) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

inline f32 clamp(f32 value, f32 min, f32 max) {
    if (value < min) return min;
    if (value > max) return max;

    return value;
}

inline b8 intersectAABB(const aabb& box1, const aabb& box2) {
    // Separating Axis Theorem
    b8 xCollision = box1.position.x + box1.size.x > box2.position.x && box1.position.x < box2.position.x + box2.size.x;
    b8 yCollision = box1.position.y + box1.size.y > box2.position.y && box1.position.y < box2.position.y + box2.size.y;

    return xCollision && yCollision;
}

// todo: different Ts
template<typename T>
u64 sizeInBytes(std::initializer_list<const vector<T>> vectors) {
    u64 size = 0;
    for (auto& vector: vectors) {
        size += vector.size() * sizeof(T);
    }
    return size;
}

sprite bob;
effect swoosh;

s32 main(s32 argc, char* argv[]) {

    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fuzzy", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);

    glfwSetWindowPos(window, (vidmode->width - SCREEN_WIDTH) / 2, (vidmode->height - SCREEN_HEIGHT) / 2);

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

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

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

        vector<c8> errorLog(LOG_LENGTH);

        glGetProgramInfoLog(shaderProgram, LOG_LENGTH, nullptr, &errorLog[0]);
        std::cerr << "Shader program linkage failed:" << std::endl << &errorLog[0] << std::endl;
    }
    assert(isShaderProgramLinked);
    
    glUseProgram(shaderProgram);

    mat4 projection = glm::ortho(0.0f, (f32) SCREEN_WIDTH, (f32) SCREEN_HEIGHT, 0.0f);
    
    s32 projectionUniformLocation = getUniformLocation(shaderProgram, "projection");
    setShaderUniform(projectionUniformLocation, projection);
    s32 viewUniformLocation = getUniformLocation(shaderProgram, "view");
    s32 modelUniformLocation = getUniformLocation(shaderProgram, "model");
    s32 typeUniformLocation = getUniformLocation(shaderProgram, "type");
    s32 tileTypeUniformLocation = getUniformLocation(shaderProgram, "tileType");

    std::fstream spritesConfigIn("textures/sprites.json");
    json spritesConfig;
    spritesConfigIn >> spritesConfig;

    s32 tileWidth = spritesConfig["tileWidth"];
    s32 tileHeight = spritesConfig["tileHeight"];

    auto bobConfig = spritesConfig["sprites"][0];
    auto bobAnimations = bobConfig["animations"];

    bob = {};
    bob.box.position = {5 * TILE_SIZE, 0 * TILE_SIZE};
    bob.box.size = {13.f * SCALE, 16.f * SCALE};
    bob.velocity = {0.f, 0.f};
    bob.acceleration = {0.f, 10.f};
    bob.animations.reserve(bobAnimations.size());

    for (auto animation : bobAnimations) {
        bob.animations.push_back({animation["x"], animation["y"], animation["frames"], animation["delay"], animation["size"]});
    }

    bob.currentAnimation = bob.animations[0];
    bob.xAnimationOffset = 0.f;
    bob.frameTime = 0.f;
    
    auto effectsConfig = spritesConfig["sprites"][1];
    auto effectsAnimations = effectsConfig["animations"];

    swoosh = {};
    swoosh.box.position = { 0.f, 0.f };     // todo: think about better ways
    swoosh.box.size = { 2 * SPRITE_SIZE, SPRITE_SIZE };
    swoosh.shouldRender = false;

    for (auto animation : effectsAnimations) {
        swoosh.animations.push_back({ animation["x"], animation["y"], animation["frames"], animation["delay"], animation["size"] });
    }

    swoosh.currentAnimation = swoosh.animations[0];
    swoosh.xAnimationOffset = 0.f;
    swoosh.frameTime = 0.f;

    f32 spriteWidth = ((f32) tileWidth) / textureWidth;
    f32 spriteHeight = ((f32) tileHeight) / textureHeight;
    
    s32 spriteSizeUniformLocation = getUniformLocation(shaderProgram, "spriteSize");
    setShaderUniform(spriteSizeUniformLocation, vec2(spriteWidth, spriteHeight));

    f32 vertices[] = {
        // Pos    // UV
        0.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 1.f,
        1.f, 0.f, 1.f, 0.f,
        1.f, 1.f, 1.f, 1.f
    };

    std::fstream levelInfoIn("levels/level01.json");
    std::fstream tileSetInfoIn("levels/tileset.json");
    json levelInfo;
    json tilesetInfo;
    levelInfoIn >> levelInfo;
    tileSetInfoIn >> tilesetInfo;

    u32 columns = tilesetInfo["columns"];
    s32 margin = tilesetInfo["margin"];
    s32 spacing = tilesetInfo["spacing"];
    u32 levelWidth = levelInfo["width"];
    u32 levelHeight = levelInfo["height"];

    vector<s32> backgroundRawTiles = levelInfo["layers"][0]["data"];
    vector<s32> foregroundRawTiles = levelInfo["layers"][1]["data"];

    vector<vec4> xyr;
    xyr.reserve(levelWidth * levelHeight);
    for (u32 y = 0; y < levelHeight; ++y) {
        for (u32 x = 0; x < levelWidth; ++x) {
            xyr.push_back(vec4(x * TILE_SIZE, y * TILE_SIZE, 0.f, 0.f));
        }
    }
    
    vector<vec2> backgroundUvs;
    backgroundUvs.reserve(levelWidth * levelHeight);
    for (u32 i = 0; i < levelWidth * levelHeight; ++i) {
        s32 tile = backgroundRawTiles[i];

        bool flippedHorizontally = tile & FLIPPED_HORIZONTALLY_FLAG;
        bool flippedVertically = tile & FLIPPED_VERTICALLY_FLAG;
        bool flippedDiagonally = tile & FLIPPED_DIAGONALLY_FLAG;

        // Clear the flags
        tile &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

        f32 rotate = 0.f;
        if (flippedVertically && flippedDiagonally) {
            // 90 degrees
            rotate = 1.f;
        } else if (flippedHorizontally && flippedVertically) {
            // 180 degrees
            rotate = 2.f;
        } else if (flippedHorizontally && flippedDiagonally) {
            // 270 degrees
            rotate = 3.f;
        }

        xyr[i].z = rotate;
        
        // todo: completely arbitrary negative value
        // handle sparseness???
        s32 uvX = tile > 0 ? ((tile - 1) % columns) : -10;
        s32 uvY = tile > 0 ? ((tile - 1) / columns) : -10;

        backgroundUvs.push_back(vec2((uvX * (tileWidth + spacing) + margin) / (f32) textureWidth, 
                                     (uvY * (tileHeight + spacing) + margin) / (f32) textureHeight));
    }

    vector<vec2> foregroundUvs;
    foregroundUvs.reserve(levelWidth * levelHeight);
    for (u32 i = 0; i < levelWidth * levelHeight; ++i) {
        s32 tile = foregroundRawTiles[i];

        bool flippedHorizontally = tile & FLIPPED_HORIZONTALLY_FLAG;
        bool flippedVertically = tile & FLIPPED_VERTICALLY_FLAG;
        bool flippedDiagonally = tile & FLIPPED_DIAGONALLY_FLAG;

        // Clear the flags
        tile &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

        f32 rotate = 0.f;
        if (flippedVertically && flippedDiagonally) {
            // 90 degrees
            rotate = 1.f;
        } else if (flippedHorizontally && flippedVertically) {
            // 180 degrees
            rotate = 2.f;
        } else if (flippedHorizontally && flippedDiagonally) {
            // 270 degrees
            rotate = 3.f;
        }

        xyr[i].w = rotate;

        s32 uvX = tile > 0 ? ((tile - 1) % columns) : -10;
        s32 uvY = tile > 0 ? ((tile - 1) / columns) : -10;

        foregroundUvs.push_back(vec2((uvX * (tileWidth + spacing) + margin) / (f32)textureWidth,
                                     (uvY * (tileHeight + spacing) + margin) / (f32)textureHeight));
    }

    auto rawEntities = levelInfo["layers"][2]["objects"];
    vector<entity> entities;
    entities.reserve(rawEntities.size());

    vector<drawableEntity> drawableEntities;
    
    u32 index = 0;
    for (u32 i = 0; i < rawEntities.size(); i++) {
        auto rawEntity = rawEntities[i];
        
        if (rawEntity.find("gid") != rawEntity.end()) {
            drawableEntity entity = {};
            // tile objects have their position at bottom-left
            // see: https://github.com/bjorn/tiled/issues/91
            entity.box.position = { (f32)rawEntity["x"] * SCALE, ((f32)rawEntity["y"] - tileHeight) * SCALE };

            u32 gid = rawEntity["gid"];

            bool flippedHorizontally = gid & FLIPPED_HORIZONTALLY_FLAG;
            bool flippedVertically = gid & FLIPPED_VERTICALLY_FLAG;
            // todo: objects handle rotation differently from the tiles
            bool flippedDiagonally = gid & FLIPPED_DIAGONALLY_FLAG;

            gid &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

            f32 rotation = 0.f;
            if (flippedVertically && flippedDiagonally) {
                // 90 degrees
                rotation = 1.f;
            }
            else if (flippedHorizontally && flippedVertically) {
                // 180 degrees
                rotation = 2.f;
            }
            else if (flippedHorizontally && flippedDiagonally) {
                // 270 degrees
                rotation = 3.f;
            }

            entity.rotation = rotation;

            s32 uvX = (gid - 1) % columns;
            s32 uvY = (gid - 1) / columns;

            entity.uv = vec2((uvX * (tileWidth + spacing) + margin) / (f32) textureWidth,
                             (uvY * (tileHeight + spacing) + margin) / (f32) textureHeight);

            entity.box.size = { (f32)rawEntity["width"] * SCALE, (f32)rawEntity["height"] * SCALE };
            entity.offset = index * sizeof(drawableEntity);
            entity.spriteScale = vec2(1.f);
            entity.shouldRender = 1;
            ++index;
            
            drawableEntities.push_back(entity);
        } else {
            entity entity = {};
            entity.box.position = {(f32) rawEntity["x"] * SCALE, (f32) rawEntity["y"] * SCALE};
            entity.box.size = { (f32)rawEntity["width"] * SCALE, (f32)rawEntity["height"] * SCALE };

            entities.push_back(entity);
        }
    }
    
    drawableEntity player = {};
    player.box = bob.box;
    player.spriteScale = vec2(1.f);
    player.offset = (u32) sizeInBytes({drawableEntities});
    player.shouldRender = 1;
    drawableEntities.push_back(player);

    drawableEntity swooshEffect = {};
    swooshEffect.box = swoosh.box;
    swooshEffect.spriteScale = vec2(2.f, 1.f);
    swooshEffect.offset = (u32) sizeInBytes({drawableEntities});
    swooshEffect.shouldRender = 0;
    drawableEntities.push_back(swooshEffect);
    
    u32 VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    u32 VBOTiles;
    glGenBuffers(1, &VBOTiles);
    glBindBuffer(GL_ARRAY_BUFFER, VBOTiles);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeInBytes({xyr}) + sizeInBytes({backgroundUvs, foregroundUvs}), 
        nullptr, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeInBytes({xyr}), xyr.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeInBytes({xyr}),
        sizeInBytes({backgroundUvs}), backgroundUvs.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeInBytes({ xyr }) + sizeInBytes({backgroundUvs}),
        sizeInBytes({foregroundUvs}), foregroundUvs.data());

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*) 0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*) sizeof(vertices));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*) (sizeof(vertices) + sizeInBytes({xyr})));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), 
        (void*) (sizeof(vertices) + sizeInBytes({xyr}) + sizeInBytes({backgroundUvs})));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    
    u32 VBOEntities;
    glGenBuffers(1, &VBOEntities);
    glBindBuffer(GL_ARRAY_BUFFER, VBOEntities);
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes({drawableEntities}), drawableEntities.data(), GL_DYNAMIC_DRAW);
    
    // aabb
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*) 0);
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    // uv/rotation
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*) (4 * sizeof(f32)));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
    // flipped
    glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(drawableEntity), (void*)(7 * sizeof(f32)));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);
    // spriteScale
    glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*)(7 * sizeof(f32) + sizeof(u32)));
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);
    // shouldRender
    glVertexAttribIPointer(8, 1, GL_UNSIGNED_INT, sizeof(drawableEntity), (void*)(9 * sizeof(f32) + sizeof(u32)));
    glEnableVertexAttribArray(8);
    glVertexAttribDivisor(8, 1);


    f64 lastTime = glfwGetTime();
    f64 currentTime;
    f64 delta = 0;

    f32 updateRate = 0.01f;   // 10 ms
    f32 lag = 0.f;
    
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();
        delta = currentTime - lastTime;
        lastTime = currentTime;
        lag += (f32) delta;

        glfwPollEvents();

        while (lag >= updateRate) {
            f32 dt = 0.15f;
            
            bob.acceleration.x = 0.f;
            processInput();
            
            // friction imitation
            bob.acceleration.x += -0.5f * bob.velocity.x;
            bob.velocity.x += bob.acceleration.x * dt;

            bob.acceleration.y += -0.01f * bob.velocity.y;
            bob.velocity.y += bob.acceleration.y * dt;

            vec2 move = vec2(
                0.5f * bob.acceleration.x * dt * dt + bob.velocity.x * dt,
                0.5f * bob.acceleration.y * dt * dt + bob.velocity.y * dt
            );
            
            vec2 oldPosition = bob.box.position;
            vec2 time = vec2(1.f);

            for (auto entity : entities) {
                vec2 t = sweptAABB(oldPosition, move, entity.box, bob.box.size);
                
                if (t.x >= 0.f && t.x < time.x) time.x = t.x;
                if (t.y >= 0.f && t.y < time.y) time.y = t.y;
            }
            for (auto entity : drawableEntities) {
                if (entity.uv == drawableEntities.back().uv) break;     // if player - break
                vec2 t = sweptAABB(oldPosition, move, entity.box, bob.box.size);

                if (t.x >= 0.f && t.x < time.x) time.x = t.x;
                if (t.y >= 0.f && t.y < time.y) time.y = t.y;

                b8 swooshCollide = intersectAABB(swoosh.box, entity.box);
                if (swooshCollide) {
                    auto t = 0;
                    t++;
                }
            }
            
            if (time.x < 1.f) {
                bob.velocity.x = 0.f;
            }
            if (time.y < 1.f) {
                bob.velocity.y = 0.f;

                if (time.y > 0.f && move.y > 0.f && bob.currentAnimation != bob.animations[1]) {
                    bob.currentAnimation = bob.animations[1];
                    bob.xAnimationOffset = 0.f;
                }
            }
            if (time.y == 1.f) {
                if (bob.velocity.y > 0.f) {
                    if (bob.currentAnimation != bob.animations[3]) {
                        bob.currentAnimation = bob.animations[5];
                        bob.xAnimationOffset = 0.f;
                    }
                } else {
                    if (bob.currentAnimation != bob.animations[3]) {
                        bob.currentAnimation = bob.animations[4];
                        bob.xAnimationOffset = 0.f;
                    }
                }
            }

            vec2 updatedMove = move * time;

            bob.box.position.x = oldPosition.x + updatedMove.x;
            bob.box.position.y = oldPosition.y + updatedMove.y;

            bob.box.position.x = clamp(bob.box.position.x, 0.f, (f32) TILE_SIZE * levelWidth - SPRITE_SIZE);
            bob.box.position.y = clamp(bob.box.position.y, 0.f, (f32) TILE_SIZE * levelHeight - SPRITE_SIZE);

            bob.acceleration.y = 10.f;

            vec2 idleArea = {100.f, 50.f};

            if (updatedMove.x > 0.f) {
                if (bob.box.position.x + SPRITE_SIZE > camera.x + SCREEN_WIDTH / 2 + idleArea.x) {
                    camera.x += updatedMove.x;
                }
            }
            else if (updatedMove.x < 0.f) {
                if (bob.box.position.x < camera.x + SCREEN_WIDTH / 2 - idleArea.x) {
                    camera.x += updatedMove.x;
                }
            }

            if (updatedMove.y > 0.f) {
                if (bob.box.position.y + SPRITE_SIZE > camera.y + SCREEN_HEIGHT / 2 + idleArea.y) {
                    camera.y += updatedMove.y;
                }
            }
            else if (updatedMove.y < 0.f) {
                if (bob.box.position.y < camera.y + SCREEN_HEIGHT / 2 - idleArea.y) {
                    camera.y += updatedMove.y;
                }
            }

            camera.x = clamp(camera.x, 0.f, (f32) TILE_SIZE * levelWidth - SCREEN_WIDTH);
            camera.y = clamp(camera.y, 0.f, (f32) TILE_SIZE * levelHeight - SCREEN_HEIGHT);

            lag -= updateRate;
        }


        mat4 view = mat4(1.0f);
        view = glm::translate(view, vec3(-camera, 0.f));
        setShaderUniform(viewUniformLocation, view);

        glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //--- drawing tilemap ---
        glBindBuffer(GL_ARRAY_BUFFER, VBOTiles);
        setShaderUniform(typeUniformLocation, 1);

        mat4 model = mat4(1.0f);
        model = glm::scale(model, vec3(SPRITE_SIZE));
        setShaderUniform(modelUniformLocation, model);
        
        setShaderUniform(tileTypeUniformLocation, 0);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, levelWidth * levelHeight);

        setShaderUniform(tileTypeUniformLocation, 1);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, levelWidth * levelHeight);

        //--- bob ---
        f32 bobXOffset = (f32) (bob.currentAnimation.x * (tileWidth + spacing) + margin) / textureWidth;
        f32 bobYOffset = (f32) (bob.currentAnimation.y * (tileHeight + spacing) + margin) / textureHeight;

        if (bob.frameTime >= bob.currentAnimation.delay) {
            bob.xAnimationOffset += (spriteWidth + (f32) spacing / textureWidth) * bob.currentAnimation.size;
            if (bob.xAnimationOffset >= ((bob.currentAnimation.frames * tileWidth * bob.currentAnimation.size) / (f32) textureWidth)) {
                bob.xAnimationOffset = 0.f;
                if (bob.currentAnimation == bob.animations[1] || bob.currentAnimation == bob.animations[3]) {
                    bob.currentAnimation = bob.animations[0];
                }
            }

            bob.frameTime = 0.0f;
        }
        bob.frameTime += (f32) delta;

        //--- effect ---
        model = mat4(1.0f);
        model = glm::translate(model, vec3(swoosh.box.position, 0.0f));
        model = glm::scale(model, vec3(2 * SPRITE_SIZE, SPRITE_SIZE, SPRITE_SIZE));
        setShaderUniform(modelUniformLocation, model);

        f32 effectXOffset = (f32)(swoosh.currentAnimation.x * (tileWidth + spacing) + margin) / textureWidth;
        f32 effectYOffset = (f32)(swoosh.currentAnimation.y * (tileHeight + spacing) + margin) / textureHeight;

        if (swoosh.frameTime >= swoosh.currentAnimation.delay) {
            swoosh.xAnimationOffset += (spriteWidth + (f32)spacing / textureWidth) * swoosh.currentAnimation.size;
            if (swoosh.xAnimationOffset >= ((swoosh.currentAnimation.frames * tileWidth * swoosh.currentAnimation.size) / (f32)textureWidth)) {
                swoosh.xAnimationOffset = 0.f;
                swoosh.shouldRender = false;
            }

            swoosh.frameTime = 0.0f;
        }

        if (swoosh.shouldRender) {
            swoosh.frameTime += (f32)delta;
        }

        //--- drawing entities ---
        glBindBuffer(GL_ARRAY_BUFFER, VBOEntities);
        setShaderUniform(typeUniformLocation, 3);
        
        drawableEntity player = drawableEntities[2];
        player.uv = vec2(bobXOffset + bob.xAnimationOffset, bobYOffset);
        player.box.position = bob.box.position;
        player.flipped = bob.flipped;

        drawableEntity swooshEffect = drawableEntities[3];
        swooshEffect.uv = vec2(effectXOffset + swoosh.xAnimationOffset, effectYOffset);
        swooshEffect.box.position = swoosh.box.position;
        swooshEffect.flipped = bob.flipped;
        swooshEffect.shouldRender = swoosh.shouldRender ? 1 : 0;

        glBufferSubData(GL_ARRAY_BUFFER, player.offset, 2 * sizeof(f32), &player.box.position);
        glBufferSubData(GL_ARRAY_BUFFER, swooshEffect.offset, 2 * sizeof(f32), &swooshEffect.box.position);
        glBufferSubData(GL_ARRAY_BUFFER, player.offset + sizeof(aabb), 2 * sizeof(f32), &player.uv);
        glBufferSubData(GL_ARRAY_BUFFER, swooshEffect.offset + sizeof(aabb), 2 * sizeof(f32), &swooshEffect.uv);
        glBufferSubData(GL_ARRAY_BUFFER, player.offset + sizeof(aabb) + 3 * sizeof(f32), sizeof(u32), &player.flipped);
        glBufferSubData(GL_ARRAY_BUFFER, swooshEffect.offset + sizeof(aabb) + 3 * sizeof(f32), sizeof(u32), &swooshEffect.flipped);
        glBufferSubData(GL_ARRAY_BUFFER, swooshEffect.offset + sizeof(aabb) + 5 * sizeof(f32) + sizeof(u32), sizeof(u32), &swooshEffect.shouldRender);
        //sdf
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (s32)drawableEntities.size());


        glfwSwapBuffers(window);

        //std::cout << delta * 1000.f << " ms" << std::endl;
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}


void processInput() {
    if (keys[GLFW_KEY_LEFT] == GLFW_PRESS) {
        if (
            bob.currentAnimation != bob.animations[2] && 
            bob.currentAnimation != bob.animations[1] && 
            bob.currentAnimation != bob.animations[3]
        ) {
            bob.currentAnimation = bob.animations[2];
        }
        bob.acceleration.x = -12.f;
        bob.flipped |= FLIPPED_HORIZONTALLY_FLAG;
    }

    if (keys[GLFW_KEY_RIGHT] == GLFW_PRESS) {
        if (
            bob.currentAnimation != bob.animations[2] && 
            bob.currentAnimation != bob.animations[1] && 
            bob.currentAnimation != bob.animations[3]
        ) {
            bob.currentAnimation = bob.animations[2];
        }
        bob.acceleration.x = 12.f;
        bob.flipped &= 0;
    }

    if (keys[GLFW_KEY_LEFT] == GLFW_RELEASE && !processedKeys[GLFW_KEY_LEFT]) {
        processedKeys[GLFW_KEY_LEFT] = true;
        if (bob.currentAnimation != bob.animations[0]) {
            bob.currentAnimation = bob.animations[0];
            bob.xAnimationOffset = 0.f;
            bob.flipped |= FLIPPED_HORIZONTALLY_FLAG;
        }
    }

    if (keys[GLFW_KEY_RIGHT] == GLFW_RELEASE && !processedKeys[GLFW_KEY_RIGHT]) {
        processedKeys[GLFW_KEY_RIGHT] = true;
        if (bob.currentAnimation != bob.animations[0]) {
            bob.currentAnimation= bob.animations[0];
            bob.xAnimationOffset = 0.f;
            bob.flipped &= 0;
        }
    }

    if (keys[GLFW_KEY_SPACE] == GLFW_PRESS && !processedKeys[GLFW_KEY_SPACE]) {
        processedKeys[GLFW_KEY_SPACE] = true;
        bob.acceleration.y = -350.f;
        bob.velocity.y = 0.f;
    }

    if (keys[GLFW_KEY_S] == GLFW_PRESS && !processedKeys[GLFW_KEY_S]) {
        processedKeys[GLFW_KEY_S] = true;
        bob.currentAnimation = bob.animations[3];
        bob.xAnimationOffset = 0.f;

        swoosh.shouldRender = true;
        swoosh.xAnimationOffset = 0.f;
        swoosh.flipped = bob.flipped;
        if (swoosh.flipped & FLIPPED_HORIZONTALLY_FLAG) {
            swoosh.box.position = { bob.box.position.x - 2 * SPRITE_SIZE, bob.box.position.y };
        } else {
            swoosh.box.position = { bob.box.position.x + SPRITE_SIZE, bob.box.position.y };
        }
    }
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

        vector<c8> errorLog(LOG_LENGTH);

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

// basic Minkowski-based collision detection
vec2 sweptAABB(const vec2 point, const vec2 delta, const aabb& box, const vec2 padding) {
    vec2 time = vec2(1.f);

    f32 nearTimeX = 1.f;
    f32 farTimeX = 1.f;
    f32 nearTimeY = 1.f;
    f32 farTimeY = 1.f;
    
    vec2 position = box.position - padding;
    vec2 size= box.size + padding;

    if (delta.x != 0.f && position.y <= point.y && point.y <= position.y + size.y) {
        nearTimeX = (position.x - point.x) / delta.x;
        if (nearTimeX < time.x) {
            time.x = nearTimeX;
        }
        
        farTimeX = (position.x + size.x - point.x) / delta.x;
        if (farTimeX < time.x) {
            time.x = farTimeX;
        }
    }

    if (delta.y != 0.f && position.x < point.x && point.x < position.x + size.x) {
        nearTimeY = (position.y - point.y) / delta.y;
        if (nearTimeY < time.y) {
            time.y = nearTimeY;
        }
        
        farTimeY = (position.y + size.y - point.y) / delta.y;
        if (farTimeY < time.y) {
            time.y = farTimeY;
        }
    }

    return time;
}
