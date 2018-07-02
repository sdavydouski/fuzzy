#include <Windows.h>
//#include <glad/glad.h>
#include "../generated/glad/src/glad.c"
#include <GLFW/glfw3.h>

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
#include <map>

#include "types.h"
#include "fuzzy.h"
#include "tiled.cpp"

#define length(arr) (sizeof(arr) / sizeof((arr)[0]))

#pragma warning(disable:4302)
#pragma warning(disable:4311)

#define offset(structType, structMember) ((u32)(&(((structType* )0)->structMember)))

enum class side {
    TOP, LEFT, BOTTOM, RIGHT
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

b32 keys[512];
b32 processedKeys[512];

vec2 scale = vec2(4.f);
// todo: could be different value than 16.f
const vec2 TILE_SIZE = {16.f * scale.x, 16.f * scale.y};

// top-left corner
vec2 camera = vec2(0.f);

struct animation {
    s32 x;
    s32 y;
    s32 frames;
    f32 delay;
    s32 size;

    b32 operator==(const animation& other) const {
        return x == other.x && y == other.y;
    }

    b32 operator!=(const animation& other) const {
        return !(*this == other);
    }
};

struct sprite {
    vector<animation> animations;
    animation currentAnimation;
    f32 xAnimationOffset;
    f32 frameTime;
    u32 flipped;

    vec2 position;
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

    vec2 position;
    aabb box;

    b32 shouldRender;
};

struct particle {
    vec2 position;
    vec2 size;
    vec2 velocity;
    vec2 acceleration;
    vec2 uv;
    f32 lifespan;
    f32 alpha;
};

struct particleEmitter {
    u32 maxParticlesCount;
    u32 lastUsedParticle;
    u32 newParticlesCount;
    f32 dt;
    vector<particle> particles;
    vec2 position;
    aabb box;
    vec2 velocity;

    b32 stopProcessingCollision;
    s32 reflectorIndex;
    b32 isFading;
    f32 timeLeft;
};

u32 findFirstUnusedParticle(const particleEmitter& emitter) {
    for (u32 i = emitter.lastUsedParticle; i < emitter.maxParticlesCount; ++i) {
        if (emitter.particles[i].lifespan <= 0.f) {
            return i;
        }
    }

    for (u32 i = 0; i < emitter.lastUsedParticle; ++i) {
        if (emitter.particles[i].lifespan <= 0.f) {
            return i;
        }
    }

    return 0;       // all particles are taken, override the first one
}

string readTextFile(const string& path);
void processInput();
u32 createAndCompileShader(e32 shaderType, const string& path);
vec2 sweptAABB(const vec2 point, const vec2 delta, const aabb& box, vec2 padding = vec2(0.f));

inline s32 getUniformLocation(u32 shaderProgram, const string& name) {
    s32 uniformLocation = glGetUniformLocation(shaderProgram, name.c_str());
    //assert(uniformLocation != -1);
    return uniformLocation;
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

inline f32 randomInRange(f32 min, f32 max) {
    f32 result = min + (f32)(rand()) / ((f32)(RAND_MAX / (max - min)));
    return result;
}

inline b32 intersectAABB(const aabb& box1, const aabb& box2) {
    // Separating Axis Theorem
    b32 xCollision = box1.position.x + box1.size.x > box2.position.x && box1.position.x < box2.position.x + box2.size.x;
    b32 yCollision = box1.position.y + box1.size.y > box2.position.y && box1.position.y < box2.position.y + box2.size.y;

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
vector<drawableEntity> drawableEntities;
vector<particleEmitter> particleEmitters;

const f32 chargeVelocity = 10.f;

s32 main(s32 argc, c8* argv[]) {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    srand((u32) glfwGetTimerValue());

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

    tileset tileset = loadTileset("levels/tileset.json");

    auto bobConfig = spritesConfig["sprites"][0];
    auto bobAnimations = bobConfig["animations"];

    bob = {};
    bob.position = {5 * TILE_SIZE.x, 0 * TILE_SIZE.y};
    bob.box.position = {5 * TILE_SIZE.x, 0 * TILE_SIZE.y};
    bob.box.size = {13.f * scale.x, 16.f * scale.y};
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
    swoosh.position = { 0.f, 0.f };     // todo: think about better ways
    swoosh.box.position = { 0.f, 0.f };     // todo: think about better ways
    swoosh.box.size = { 2 * TILE_SIZE.x, TILE_SIZE.y };
    swoosh.shouldRender = false;

    for (auto animation : effectsAnimations) {
        swoosh.animations.push_back({ animation["x"], animation["y"], animation["frames"], animation["delay"], animation["size"] });
    }

    swoosh.currentAnimation = swoosh.animations[0];
    swoosh.xAnimationOffset = 0.f;
    swoosh.frameTime = 0.f;

    f32 spriteWidth = ((f32) tileset.tileSize.x) / textureWidth;
    f32 spriteHeight = ((f32) tileset.tileSize.y) / textureHeight;
    
    s32 spriteSizeUniformLocation = getUniformLocation(shaderProgram, "spriteSize");
    setShaderUniform(spriteSizeUniformLocation, vec2(spriteWidth, spriteHeight));

    f32 vertices[] = {
        // Pos    // UV
        0.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 1.f,
        1.f, 0.f, 1.f, 0.f,
        1.f, 1.f, 1.f, 1.f
    };

    tiledMap level = loadMap("levels/level01.json", tileset, scale);

    // todo: make it efficient
    vector<tile> tiles;
    for (auto& layer : level.tileLayers) {
        for (auto& tile : layer.tiles) {
            tiles.push_back(tile);
        }
    }

    u64 totalTileSizeInBytes = sizeInBytes({tiles});

    u32 VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    u32 VBOTiles;
    glGenBuffers(1, &VBOTiles);
    glBindBuffer(GL_ARRAY_BUFFER, VBOTiles);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + totalTileSizeInBytes, nullptr, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), totalTileSizeInBytes, tiles.data());

    // vertices
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*) 0);
    glEnableVertexAttribArray(0);

    // tile position/uv
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(tile), (void*) sizeof(vertices));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // tile flipped
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(tile), (void*) (sizeof(vertices) + offset(tile, flipped)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // todo: make it efficient
    vector<entity> entities;
    for (auto& layer : level.objectLayers) {
        for (auto& entity : layer.entities) {
            entities.push_back(entity);
        }
        for (auto& drawableEntity : layer.drawableEntities) {
            drawableEntities.push_back(drawableEntity);
        }
    }

    drawableEntity player = {};
    player.position = bob.position;
    player.box = bob.box;
    player.spriteScale = vec2(1.f);
    player.offset = (u32) sizeInBytes({drawableEntities});
    player.shouldRender = 1;
    player.collides = true;
    player.type = entityType::PLAYER;
    drawableEntities.push_back(player);

    drawableEntity swooshEffect = {};
    swooshEffect.position = swoosh.position;
    swooshEffect.box = swoosh.box;
    swooshEffect.spriteScale = vec2(2.f, 1.f);
    swooshEffect.offset = (u32) sizeInBytes({drawableEntities});
    swooshEffect.shouldRender = 0;
    swooshEffect.collides = true;
    swooshEffect.type = entityType::EFFECT;
    drawableEntities.push_back(swooshEffect);

    u32 VBOEntities;
    glGenBuffers(1, &VBOEntities);
    glBindBuffer(GL_ARRAY_BUFFER, VBOEntities);
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes({drawableEntities}), drawableEntities.data(), GL_STREAM_DRAW);
    
    // position
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*) offset(drawableEntity, position));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    // aabb
    //glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*) offset(drawableEntity, box));
    //glEnableVertexAttribArray(4);
    //glVertexAttribDivisor(4, 1);
    // uv/rotation
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*) offset(drawableEntity, uv));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
    // flipped
    glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(drawableEntity), (void*) offset(drawableEntity, flipped));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);
    // spriteScale
    glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*) offset(drawableEntity, spriteScale));
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);
    // shouldRender
    glVertexAttribIPointer(8, 1, GL_UNSIGNED_INT, sizeof(drawableEntity), (void*) offset(drawableEntity, shouldRender));
    glEnableVertexAttribArray(8);
    glVertexAttribDivisor(8, 1);

    particleEmitter charge = {};
    charge.maxParticlesCount = 500;
    charge.newParticlesCount = 5;
    charge.dt = 0.01f;
    charge.position = { 4.5 * TILE_SIZE.x, 6.5 * TILE_SIZE.y };
    charge.box.position = { 4.5 * TILE_SIZE.x, 6.5 * TILE_SIZE.y };
    charge.box.size = {0.1f * TILE_SIZE.x, 0.1f * TILE_SIZE.x};
    charge.velocity = {0.f, 0.f};
    charge.reflectorIndex = -1;
    charge.timeLeft = 3.f;
    
    charge.particles.reserve(charge.maxParticlesCount);
    charge.particles.assign(charge.maxParticlesCount, particle());

    particleEmitters.push_back(charge);
    
    u32 VBOParticles;
    glGenBuffers(1, &VBOParticles);
    glBindBuffer(GL_ARRAY_BUFFER, VBOParticles);
    // todo: manage it somehow
    glBufferData(GL_ARRAY_BUFFER, 50 * (charge.maxParticlesCount) * sizeof(particle), 
        nullptr, GL_STREAM_DRAW);
    
    // particle's position/size
    glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, sizeof(particle), (void*) offset(particle, position));
    glEnableVertexAttribArray(9);
    glVertexAttribDivisor(9, 1);
    // particle's uv
    glVertexAttribPointer(10, 2, GL_FLOAT, GL_FALSE, sizeof(particle), (void*) offset(particle, uv));
    glEnableVertexAttribArray(10);
    glVertexAttribDivisor(10, 1);
    // particle's alpha value
    glVertexAttribPointer(11, 1, GL_FLOAT, GL_FALSE, sizeof(particle), (void*) offset(particle, alpha));
    glEnableVertexAttribArray(11);
    glVertexAttribDivisor(11, 1);

    f64 lastTime = glfwGetTime();
    f64 currentTime;
    f64 delta = 0;

    f32 updateRate = 0.01f;   // 10 ms
    f32 lag = 0.f;
    
    // todo: draw collision regions
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    f32 chargeSpawnCooldown = 0.f;

    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();
        delta = currentTime - lastTime;

        lastTime = currentTime;
        lag += (f32) delta;

        glfwPollEvents();
        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        while (lag >= updateRate) {
            glBindBuffer(GL_ARRAY_BUFFER, VBOEntities);

            f32 dt = 0.15f;
            
            bob.acceleration.x = 0.f;
            processInput();
            
            // friction imitation
            // todo: take scale into account!
            bob.acceleration.x += -0.5f * bob.velocity.x;
            bob.velocity.x += bob.acceleration.x * dt;

            bob.acceleration.y += -0.01f * bob.velocity.y;
            bob.velocity.y += bob.acceleration.y * dt;

            vec2 move = 0.5f * bob.acceleration * dt * dt + bob.velocity * dt;
            
            vec2 oldPosition = bob.position;
            vec2 time = vec2(1.f);

            for (auto& entity : entities) {
                vec2 t = sweptAABB(oldPosition, move, entity.box, bob.box.size);
                
                if (t.x >= 0.f && t.x < time.x) time.x = t.x;
                if (t.y >= 0.f && t.y < time.y) time.y = t.y;
            }

            for (u32 i = 0; i < drawableEntities.size(); ++i) {
                drawableEntity& entity = drawableEntities[i];

                if (entity.type == entityType::REFLECTOR) {
                    if (!entity.collides) break;

                    vec2 t = sweptAABB(oldPosition, move, entity.box, bob.box.size);

                    if (t.x >= 0.f && t.x < time.x) time.x = t.x;
                    if (t.y >= 0.f && t.y < time.y) time.y = t.y;

                    if (entity.type == entityType::REFLECTOR) {
                        b32 swooshCollide = intersectAABB(swoosh.box, entity.box);
                        if (!entity.underEffect && swooshCollide) {
                            entity.underEffect = true;
                            entity.isRotating = true;
                        }

                        if (entity.isRotating) {
                            entity.rotation += 5.f;
                            glBufferSubData(GL_ARRAY_BUFFER, entity.offset + offset(drawableEntity, rotation), sizeof(u32), &entity.rotation);

                            if (0.f < entity.rotation && entity.rotation <= 90.f) {
                                if (entity.rotation == 90.f) {
                                    entity.isRotating = false;
                                    break;
                                }
                            }
                            if (90.f < entity.rotation && entity.rotation <= 180.f) {
                                if (entity.rotation == 180.f) {
                                    entity.isRotating = false;
                                    break;
                                }
                            }
                            if (180.f < entity.rotation && entity.rotation <= 270.f) {
                                if (entity.rotation == 270.f) {
                                    entity.isRotating = false;
                                    break;
                                }
                            }
                            if (270.f < entity.rotation && entity.rotation <= 360.f) {
                                if (entity.rotation == 360.f) {
                                    entity.isRotating = false;
                                    entity.rotation = 0.f;
                                    break;
                                }
                            }
                        }
                    }
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

            bob.position.x = oldPosition.x + updatedMove.x;
            bob.position.y = oldPosition.y + updatedMove.y;

            bob.position.x = clamp(bob.position.x, 0.f, (f32) TILE_SIZE.x * level.width - TILE_SIZE.x);
            bob.position.y = clamp(bob.position.y, 0.f, (f32) TILE_SIZE.y * level.height - TILE_SIZE.y);

            bob.box.position = bob.position;

            bob.acceleration.y = 10.f;

            vec2 idleArea = {2 * TILE_SIZE.x, 1 * TILE_SIZE.y};

            if (updatedMove.x > 0.f) {
                if (bob.position.x + TILE_SIZE.x > camera.x + SCREEN_WIDTH / 2 + idleArea.x) {
                    camera.x += updatedMove.x;
                }
            }
            else if (updatedMove.x < 0.f) {
                if (bob.position.x < camera.x + SCREEN_WIDTH / 2 - idleArea.x) {
                    camera.x += updatedMove.x;
                }
            }

            if (updatedMove.y > 0.f) {
                if (bob.position.y + TILE_SIZE.y > camera.y + SCREEN_HEIGHT / 2 + idleArea.y) {
                    camera.y += updatedMove.y;
                }
            }
            else if (updatedMove.y < 0.f) {
                if (bob.position.y < camera.y + SCREEN_HEIGHT / 2 - idleArea.y) {
                    camera.y += updatedMove.y;
                }
            }

            camera.x = camera.x > 0.f ? camera.x : 0.f;
            camera.y = camera.y > 0.f ? camera.y : 0.f;

            if (TILE_SIZE.x * level.width - SCREEN_WIDTH >= 0) {
                camera.x = clamp(camera.x, 0.f, (f32) TILE_SIZE.x * level.width - SCREEN_WIDTH);
            }
            if (TILE_SIZE.y * level.height - SCREEN_HEIGHT >= 0) {
                camera.y = clamp(camera.y, 0.f, (f32) TILE_SIZE.y * level.height - SCREEN_HEIGHT);
            }

            glBindBuffer(GL_ARRAY_BUFFER, VBOParticles);

            u32 chargesCount = (u32) particleEmitters.size();

            for (u32 i = 0; i < chargesCount; ++i) {
                particleEmitter& charge = particleEmitters[i];

                vec2 oldChargePosition = charge.box.position;
                vec2 chargeMove = charge.velocity * dt;
                vec2 chargeTime = vec2(1.f);

                for (u32 j = 0; j < drawableEntities.size(); ++j) {
                    drawableEntity& entity = drawableEntities[j];

                    if (entity.type == entityType::REFLECTOR) {
                        aabb reflectorBox = entity.box;

                        vec2 t = sweptAABB(oldChargePosition, chargeMove, reflectorBox, charge.box.size);

                        // if not colliding
                        if (!((0.f <= t.x && t.x < 1.f) || (0.f <= t.y && t.y < 1.f))) {
                            if (!intersectAABB(charge.box, reflectorBox)) {
                                if (charge.reflectorIndex == (s32) j) {
                                    charge.stopProcessingCollision = false;
                                    charge.reflectorIndex = -1;
                                }
                                continue;
                            }
                        }

                        // if collides check direction of the charge
                        // check reflector's angle
                        // dimiss charge if it's coming from the wrong side
                        // proceed with new collision rule otherwise.

                        if (charge.stopProcessingCollision && charge.reflectorIndex == (s32) j) continue;

                        aabb testBox = {};

                        if (chargeMove.x > 0.f) {
                            if (entity.rotation == 180.f || entity.rotation == 270.f) {
                                testBox.position.x = reflectorBox.position.x + reflectorBox.size.x / 2.f + charge.box.size.x / 2.f;
                                testBox.position.y = reflectorBox.position.y;
                                testBox.size.x = reflectorBox.size.x / 2.f - charge.box.size.x / 2.f;
                                testBox.size.y = reflectorBox.size.y;

                                vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, charge.box.size);
                                
                                if (0.f <= t.x && t.x < 1.f) {
                                    chargeTime.x = t.x;

                                    charge.velocity.x = 0.f;
                                    charge.velocity.y = entity.rotation == 180.f ? chargeVelocity : -chargeVelocity;
                                    charge.stopProcessingCollision = true;
                                    charge.reflectorIndex = (s32) j;
                                }
                            } else {
                                // collided with outer border: stop processing
                                chargeTime = t;
                                charge.isFading = true;
                            }
                        } else if (chargeMove.x < 0.f) {
                            if (entity.rotation == 0.f || entity.rotation == 90.f) {
                                testBox.position.x = reflectorBox.position.x;
                                testBox.position.y = reflectorBox.position.y;
                                testBox.size.x = reflectorBox.size.x / 2.f - charge.box.size.x / 2.f;
                                testBox.size.y = reflectorBox.size.y;

                                vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, charge.box.size);
                                
                                if (0.f <= t.x && t.x < 1.f) {
                                    chargeTime.x = t.x;

                                    charge.velocity.x = 0.f;
                                    charge.velocity.y = entity.rotation == 0.f ? -chargeVelocity : chargeVelocity;
                                    charge.stopProcessingCollision = true;
                                    charge.reflectorIndex = (s32) j;
                                }
                            } else {
                                chargeTime = t;
                                charge.isFading = true;
                            }
                        } else if (chargeMove.y > 0.f) {
                            if (entity.rotation == 0.f || entity.rotation == 270.f) {
                                testBox.position.x = reflectorBox.position.x;
                                testBox.position.y = reflectorBox.position.y + reflectorBox.size.y / 2.f + charge.box.size.y / 2.f;
                                testBox.size.x = reflectorBox.size.x;
                                testBox.size.y = reflectorBox.size.y / 2.f - charge.box.size.y / 2.f;

                                vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, charge.box.size);
                                
                                if (0.f <= t.y && t.y < 1.f) {
                                    chargeTime.y = t.y;

                                    charge.velocity.x = entity.rotation == 0.f ? chargeVelocity : -chargeVelocity;
                                    charge.velocity.y = 0.f;
                                    charge.stopProcessingCollision = true;
                                    charge.reflectorIndex = (s32) j;
                                }
                            } else {
                                chargeTime = t;
                                charge.isFading = true;
                            }
                        } else if (chargeMove.y < 0.f) {
                            if (entity.rotation == 90.f || entity.rotation == 180.f) {
                                testBox.position.x = reflectorBox.position.x;
                                testBox.position.y = reflectorBox.position.y;
                                testBox.size.x = reflectorBox.size.x;
                                testBox.size.y = reflectorBox.size.y / 2.f - charge.box.size.y / 2.f;

                                vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, charge.box.size);
                                
                                if (0.f <= t.y && t.y < 1.f) {
                                    chargeTime.y = t.y;

                                    charge.velocity.x = entity.rotation == 90.f ? chargeVelocity : -chargeVelocity;
                                    charge.velocity.y = 0.f;
                                    charge.stopProcessingCollision = true;
                                    charge.reflectorIndex = (s32) j;
                                }
                            } else {
                                chargeTime = t;
                                charge.isFading = true;
                            }
                        }
                    }

                    if (entity.type == entityType::LAMP) {
                        vec2 t = sweptAABB(oldChargePosition, chargeMove, entity.box, charge.box.size);

                        if ((0.f <= t.x && t.x < 1.f) || (0.f <= t.y && t.y < 1.f)) {
                            std::cout << "collides!" << std::endl;
                            // todo: trigger lamp animation, etc.
                        }
                    }
                }

                chargeMove *= chargeTime;
                charge.box.position += chargeMove;
                charge.position += chargeMove;

                if (charge.box.position.x <= 0.f || charge.box.position.x >= (f32) TILE_SIZE.x * level.width) {
                    charge.velocity.x = -charge.velocity.x;
                }
                if (charge.box.position.y <= 0.f || charge.box.position.y >= (f32) TILE_SIZE.y * level.height) {
                    charge.velocity.y = -charge.velocity.y;
                }

                b32 chargeCollide = intersectAABB(swoosh.box, particleEmitters[0].box);
                if (chargeCollide && swoosh.shouldRender && chargeSpawnCooldown > 1.f) {
                    chargeSpawnCooldown = 0.f;
                    
                    particleEmitter newCharge = particleEmitters[0];        // copy
                    newCharge.velocity.x = bob.flipped ? -chargeVelocity: chargeVelocity;

                    particleEmitters.push_back(newCharge);
                }
            }

            for (u32 i = 0; i < particleEmitters.size(); ++i) {
                if (particleEmitters[i].isFading) {
                    particleEmitters[i].timeLeft -= (f32) delta;
                }

                if (particleEmitters[i].timeLeft <= 0.f) {
                    particleEmitters.erase(particleEmitters.begin() + i);
                }
            }

            u64 particlesSize = 0;
            // todo: use transform feedback instead?
            for (u32 i = 0; i < particleEmitters.size(); ++i) {
                particleEmitter& emitter = particleEmitters[i];

                 if (emitter.timeLeft <= 0.f) {
                     particlesSize += sizeInBytes({emitter.particles});
                     continue;
                 };
                
                 for (u32 j = 0; j < emitter.newParticlesCount; ++j) {
                     u32 unusedParticleIndex = findFirstUnusedParticle(emitter);
                     emitter.lastUsedParticle = unusedParticleIndex;
                
                     particle& particle = emitter.particles[unusedParticleIndex];
                
                     // respawing particle
                     f32 randomX = randomInRange(-1.f * scale.x, 1.f * scale.x);
                     f32 randomY = randomInRange(-1.f * scale.y, 1.f * scale.y);
                
                     particle.lifespan = 1.f;
                     particle.position.x = emitter.position.x + randomX;
                     particle.position.y = emitter.position.y + randomY;
                     particle.size = { 0.2f * TILE_SIZE.x, 0.2f * TILE_SIZE.y };
                     particle.velocity = { 0.f, 0.f };
                     particle.acceleration = { randomX * 10.f, 10.f };
                     particle.uv = vec2((13 * (tileset.tileSize.y + tileset.spacing) + tileset.margin) / (f32)textureHeight,
                         (16 * (tileset.tileSize.y + tileset.spacing) + tileset.margin) / (f32)textureHeight);
                     particle.alpha = 1.f;
                 }
                
                 for (u32 j = 0; j < emitter.maxParticlesCount; ++j) {
                     particle& p = emitter.particles[j];
                     f32 dt = emitter.dt;
                
                     if (p.lifespan > 0.f) {
                         p.lifespan -= (f32)dt;
                         p.velocity = p.acceleration * dt;
                         p.position.x += randomInRange(-1.f, 1.f);
                         p.position.y += randomInRange(-1.f, 1.f);
                         p.alpha -= (f32)dt * 1.f;
                         p.size -= (f32)dt * 1.f;
                     }
                 }
                
                 glBufferSubData(GL_ARRAY_BUFFER, particlesSize, sizeInBytes({ emitter.particles }),
                     emitter.particles.data());
                
                 particlesSize += sizeInBytes({emitter.particles});
            }

            lag -= updateRate;
        }

        chargeSpawnCooldown += (f32) delta;
        
        mat4 view = mat4(1.0f);
        view = glm::translate(view, vec3(-camera, 0.f));
        setShaderUniform(viewUniformLocation, view);

        glClear(GL_COLOR_BUFFER_BIT);

        //--- drawing tilemap ---
        glBindBuffer(GL_ARRAY_BUFFER, VBOTiles);
        setShaderUniform(typeUniformLocation, 1);

        mat4 model = mat4(1.0f);
        model = glm::scale(model, vec3(TILE_SIZE, 1.f));
        setShaderUniform(modelUniformLocation, model);
        
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (u32) tiles.size());

        //--- bob ---
        f32 bobXOffset = (f32) (bob.currentAnimation.x * (tileset.tileSize.x + tileset.spacing) + tileset.margin) / textureWidth;
        f32 bobYOffset = (f32) (bob.currentAnimation.y * (tileset.tileSize.y + tileset.spacing) + tileset.margin) / textureHeight;

        if (bob.frameTime >= bob.currentAnimation.delay) {
            bob.xAnimationOffset += (spriteWidth + (f32) tileset.spacing / textureWidth) * bob.currentAnimation.size;
            if (bob.xAnimationOffset >= ((bob.currentAnimation.frames * tileset.tileSize.x * bob.currentAnimation.size) / (f32) textureWidth)) {
                bob.xAnimationOffset = 0.f;
                if (bob.currentAnimation == bob.animations[1] || bob.currentAnimation == bob.animations[3]) {
                    bob.currentAnimation = bob.animations[0];
                }
            }

            bob.frameTime = 0.0f;
        }
        bob.frameTime += (f32) delta;

        model = mat4(1.0f);
        model = glm::scale(model, vec3(TILE_SIZE, 1.f));
        setShaderUniform(modelUniformLocation, model);

        f32 effectXOffset = (f32)(swoosh.currentAnimation.x * (tileset.tileSize.x + tileset.spacing) + tileset.margin) / textureWidth;
        f32 effectYOffset = (f32)(swoosh.currentAnimation.y * (tileset.tileSize.y + tileset.spacing) + tileset.margin) / textureHeight;

        if (swoosh.frameTime >= swoosh.currentAnimation.delay) {
            swoosh.xAnimationOffset += (spriteWidth + (f32)tileset.spacing / textureWidth) * swoosh.currentAnimation.size;
            if (swoosh.xAnimationOffset >= ((swoosh.currentAnimation.frames * tileset.tileSize.x * swoosh.currentAnimation.size) / (f32)textureWidth)) {
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

        player.uv = vec2(bobXOffset + bob.xAnimationOffset, bobYOffset);
        player.position = bob.position;
        player.box.position = bob.box.position;
        player.flipped = bob.flipped;

        swooshEffect.uv = vec2(effectXOffset + swoosh.xAnimationOffset, effectYOffset);
        swooshEffect.position = swoosh.position;
        swooshEffect.box.position = swoosh.box.position;
        swooshEffect.flipped = bob.flipped;
        swooshEffect.shouldRender = swoosh.shouldRender ? 1 : 0;

        glBufferSubData(GL_ARRAY_BUFFER, player.offset, 2 * sizeof(f32), &player.position);
        glBufferSubData(GL_ARRAY_BUFFER, swooshEffect.offset, 2 * sizeof(f32), &swooshEffect.position);
        glBufferSubData(GL_ARRAY_BUFFER, player.offset + offset(drawableEntity, uv), 2 * sizeof(f32), &player.uv);
        glBufferSubData(GL_ARRAY_BUFFER, swooshEffect.offset + offset(drawableEntity, uv), 2 * sizeof(f32), &swooshEffect.uv);
        glBufferSubData(GL_ARRAY_BUFFER, player.offset + offset(drawableEntity, flipped), sizeof(u32), &player.flipped);
        glBufferSubData(GL_ARRAY_BUFFER, swooshEffect.offset + offset(drawableEntity, flipped), sizeof(u32), &swooshEffect.flipped);
        glBufferSubData(GL_ARRAY_BUFFER, swooshEffect.offset + offset(drawableEntity, shouldRender), sizeof(u32), &swooshEffect.shouldRender);
        
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (s32) drawableEntities.size());

        //--- drawing particles ---
        glBindBuffer(GL_ARRAY_BUFFER, VBOParticles);
        setShaderUniform(typeUniformLocation, 4);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        // todo: offsets when delete in the middle?
        s32 totalParticlesCount = 0;
        for (u32 i = 0; i < particleEmitters.size(); ++i) {
            totalParticlesCount += (s32) particleEmitters[i].particles.size();
        }

        // todo: draw only the ones which lifespan is greater than zero
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, totalParticlesCount);
        
        glFinish();
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

        // todo: make it better
        for (u32 i = 0; i < drawableEntities.size(); ++i) {
            drawableEntity& entity = drawableEntities[i];
            if (entity.type == entityType::REFLECTOR) {
                entity.underEffect = false;
            }
        }
        
        if (swoosh.flipped & FLIPPED_HORIZONTALLY_FLAG) {
            swoosh.position = { bob.box.position.x - 2 * TILE_SIZE.x, bob.box.position.y };
            swoosh.box.position = { bob.box.position.x - 2 * TILE_SIZE.x, bob.box.position.y };
        } else {
            swoosh.position = { bob.box.position.x + TILE_SIZE.x, bob.box.position.y };
            swoosh.box.position = { bob.box.position.x + TILE_SIZE.x, bob.box.position.y };
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

    f32 leftTime = 1.f;
    f32 rightTime = 1.f;
    f32 topTime = 1.f;
    f32 bottomTime = 1.f;
    
    vec2 position = box.position - padding;
    vec2 size= box.size + padding;

    if (delta.x != 0.f && position.y <= point.y && point.y <= position.y + size.y) {
        leftTime = (position.x - point.x) / delta.x;
        if (leftTime < time.x) {
            time.x = leftTime;
        }
        
        rightTime = (position.x + size.x - point.x) / delta.x;
        if (rightTime < time.x) {
            time.x = rightTime;
        }
    }

    if (delta.y != 0.f && position.x < point.x && point.x < position.x + size.x) {
        topTime = (position.y - point.y) / delta.y;
        if (topTime < time.y) {
            time.y = topTime;
        }
        
        bottomTime = (position.y + size.y - point.y) / delta.y;
        if (bottomTime < time.y) {
            time.y = bottomTime;
        }
    }

    return time;
}
