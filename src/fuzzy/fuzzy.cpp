#include "../../generated/glad/src/glad.c"

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nlohmann\json.hpp>

// todo: just for key codes
#include <GLFW/glfw3.h>

#include <fstream>

#include "fuzzy_platform.h"
#include "tiled.cpp"
#include "fuzzy.h"

constexpr vec3 normalizeRGB(s32 red, s32 green, s32 blue) {
    const f32 MAX = 255.f;
    return vec3(red / MAX, green / MAX, blue / MAX);
}

u32 createAndCompileShader(game_memory* Memory, e32 shaderType, const string& path) {
    u32 shader = glCreateShader(shaderType);
    string shaderSource = Memory->PlatformAPI.ReadTextFile(path);
    const char* shaderSourcePtr = shaderSource.c_str();
    glShaderSource(shader, 1, &shaderSourcePtr, nullptr);
    glCompileShader(shader);

    s32 isShaderCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
    if (!isShaderCompiled) {
        s32 LOG_LENGTH;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &LOG_LENGTH);

        vector<char> errorLog(LOG_LENGTH);

        glGetShaderInfoLog(shader, LOG_LENGTH, nullptr, &errorLog[0]);
        Memory->PlatformAPI.PrintOutput("Shader compilation failed:\n" + string(&errorLog[0]) + "\n");

        glDeleteShader(shader);
    }
    assert(isShaderCompiled);

    return shader;
}

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

// todo: write more efficient functions
inline f32 clamp(f32 value, f32 min, f32 max) {
    if (value < min) return min;
    if (value > max) return max;

    return value;
}

//inline f32 abs(f32 value) {
//    if (value < 0.f) return -value;
//
//    return value;
//}

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
    for (auto& vector : vectors) {
        size += vector.size() * sizeof(T);
    }
    return size;
}

// todo: replace with smth more performant
inline drawableEntity* getEntityById(vector<drawableEntity>& entities, u32 id) {
    drawableEntity* result = nullptr;

    for (auto& entity : entities) {
        if (entity.id == id) {
            result = &entity;
            break;
        }
    }

    assert(result);

    return result;
}

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

// basic Minkowski-based collision detection
vec2 sweptAABB(const vec2 point, const vec2 delta, const aabb& box, const vec2 padding) {
    vec2 time = vec2(1.f);

    f32 leftTime = 1.f;
    f32 rightTime = 1.f;
    f32 topTime = 1.f;
    f32 bottomTime = 1.f;

    vec2 position = box.position - padding;
    vec2 size = box.size + padding;

    if (delta.x != 0.f && position.y < point.y && point.y < position.y + size.y) {
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


constexpr vec3 backgroundColor = normalizeRGB(29, 33, 45);

vec2 scale = vec2(4.f);
// todo: could be different value than 16.f
const vec2 TILE_SIZE = { 16.f * scale.x, 16.f * scale.y };

// top-left corner
vec2 camera = vec2(0.f);

const f32 chargeVelocity = 10.f;

extern "C" EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
    assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state* GameState = (game_state*)Memory->PermanentStorage;

    s32 ScreenWidth = Params->ScreenWidth;
    s32 ScreenHeight = Params->ScreenHeight;

    sprite* Bob = &GameState->Bob;
    effect* Swoosh = &GameState->Swoosh;
    vector<entity>* Entities = &GameState->Entities;
    vector<drawableEntity>* DrawableEntities = &GameState->DrawableEntities;
    vector<particleEmitter>* ParticleEmitters = &GameState->ParticleEmitters;

    if (!Memory->IsInitalized) {
        if (!gladLoadGL()) {
            Memory->PlatformAPI.PrintOutput("Failed to initialize OpenGL context\n");
        }

        glEnable(GL_BLEND);

        Memory->PlatformAPI.PrintOutput((char*)glGetString(GL_VERSION));
        Memory->PlatformAPI.PrintOutput("\n");

        glViewport(0, 0, Params->ScreenWidth, Params->ScreenHeight);

        s32 textureChannels;
        u8* textureImage = stbi_load("textures/industrial_tileset.png",
            &GameState->TextureWidth, &GameState->TextureHeight, &textureChannels, 0);
        if (!textureImage) {
            Memory->PlatformAPI.PrintOutput("Texture loading failed:\n" + string(stbi_failure_reason()) + "\n");
        }
        assert(textureImage);

        u32 texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        // note: default value for GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR
        // since we do not use mipmaps we must override this value
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GameState->TextureWidth, GameState->TextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureImage);

        stbi_image_free(textureImage);


        u32 vertexShader = createAndCompileShader(Memory, GL_VERTEX_SHADER, "shaders/basic.vert");
        u32 fragmentShader = createAndCompileShader(Memory, GL_FRAGMENT_SHADER, "shaders/basic.frag");

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

            vector<char> errorLog(LOG_LENGTH);

            glGetProgramInfoLog(shaderProgram, LOG_LENGTH, nullptr, &errorLog[0]);
            Memory->PlatformAPI.PrintOutput("Shader program linkage failed:\n" + string(&errorLog[0]) + "\n");
        }
        assert(isShaderProgramLinked);

        glUseProgram(shaderProgram);

        mat4 projection = glm::ortho(0.0f, (f32)ScreenWidth, (f32)ScreenHeight, 0.0f);

        s32 projectionUniformLocation = getUniformLocation(shaderProgram, "projection");
        setShaderUniform(projectionUniformLocation, projection);
        GameState->ViewUniformLocation = getUniformLocation(shaderProgram, "view");
        GameState->ModelUniformLocation = getUniformLocation(shaderProgram, "model");
        GameState->TypeUniformLocation = getUniformLocation(shaderProgram, "type");
        s32 tileTypeUniformLocation = getUniformLocation(shaderProgram, "tileType");

        json spritesConfig = Memory->PlatformAPI.ReadJsonFile("textures/sprites.json");

        // todo: std::map isn't working
        tileset tileset = loadTileset(Memory->PlatformAPI.ReadJsonFile, "levels/tileset.json");
        GameState->TilesetColumns = tileset.columns;
        GameState->TilesetImageSize = tileset.imageSize;
        GameState->TilesetMargin = tileset.margin;
        GameState->TilesetSpacing = tileset.spacing;
        GameState->TilesetTileSize = tileset.tileSize;

        auto bobConfig = spritesConfig["sprites"][0];
        auto bobAnimations = bobConfig["animations"];

        *Bob = {};
        Bob->position = { 5 * TILE_SIZE.x, 0 * TILE_SIZE.y };
        Bob->box.position = { 5 * TILE_SIZE.x, 0 * TILE_SIZE.y };
        Bob->box.size = { 13.f * scale.x, 16.f * scale.y };
        Bob->velocity = { 0.f, 0.f };
        Bob->acceleration = { 0.f, 10.f };
        Bob->animations.reserve(bobAnimations.size());

        for (auto animation : bobAnimations) {
            Bob->animations.push_back({ animation["x"], animation["y"], animation["frames"], animation["delay"], animation["size"] });
        }

        Bob->currentAnimation = Bob->animations[0];
        Bob->xAnimationOffset = 0.f;
        Bob->frameTime = 0.f;

        auto effectsConfig = spritesConfig["sprites"][1];
        auto effectsAnimations = effectsConfig["animations"];

        *Swoosh = {};
        Swoosh->position = { 0.f, 0.f };     // todo: think about better ways
        Swoosh->box.position = { 0.f, 0.f };     // todo: think about better ways
        Swoosh->box.size = { 2 * TILE_SIZE.x, TILE_SIZE.y };
        Swoosh->shouldRender = false;

        for (auto animation : effectsAnimations) {
            Swoosh->animations.push_back({ animation["x"], animation["y"], animation["frames"], animation["delay"], animation["size"] });
        }

        Swoosh->currentAnimation = Swoosh->animations[0];
        Swoosh->xAnimationOffset = 0.f;
        Swoosh->frameTime = 0.f;

        auto lampConfig = spritesConfig["sprites"][2];
        auto lampAnimations = lampConfig["animations"];

        GameState->TurnOnAnimation = {};
        GameState->TurnOnAnimation.x = lampAnimations[0]["x"];
        GameState->TurnOnAnimation.y = lampAnimations[0]["y"];
        GameState->TurnOnAnimation.frames = lampAnimations[0]["frames"];
        GameState->TurnOnAnimation.delay = lampAnimations[0]["delay"];
        GameState->TurnOnAnimation.size = lampAnimations[0]["size"];

        auto platformConfig = spritesConfig["sprites"][3];
        auto platformAnimations = platformConfig["animations"];

        GameState->PlatformOnAnimation = {};
        GameState->PlatformOnAnimation.x = platformAnimations[0]["x"];
        GameState->PlatformOnAnimation.y = platformAnimations[0]["y"];
        GameState->PlatformOnAnimation.frames = platformAnimations[0]["frames"];
        GameState->PlatformOnAnimation.delay = platformAnimations[0]["delay"];
        GameState->PlatformOnAnimation.size = platformAnimations[0]["size"];
        string animationDirection = platformAnimations[0]["direction"];
        if (animationDirection == "right") {
            GameState->PlatformOnAnimation.direction = direction::RIGHT;
        }
        else if (animationDirection == "left") {
            GameState->PlatformOnAnimation.direction = direction::LEFT;
        }

        GameState->SpriteWidth = ((f32)GameState->TilesetTileSize.x) / GameState->TextureWidth;
        GameState->SpriteHeight = ((f32)GameState->TilesetTileSize.y) / GameState->TextureHeight;

        s32 spriteSizeUniformLocation = getUniformLocation(shaderProgram, "spriteSize");
        setShaderUniform(spriteSizeUniformLocation, vec2(GameState->SpriteWidth, GameState->SpriteHeight));

        f32 vertices[] = {
            // Pos    // UV
            0.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 1.f,
            1.f, 0.f, 1.f, 0.f,
            1.f, 1.f, 1.f, 1.f
        };

        GameState->Level = loadMap(Memory->PlatformAPI.ReadJsonFile, "levels/level01.json", tileset, scale);

        // todo: make it efficient
        for (auto& layer : GameState->Level.tileLayers) {
            for (auto& tile : layer.tiles) {
                GameState->Tiles.push_back(tile);
            }
        }

        u64 totalTileSizeInBytes = sizeInBytes({ GameState->Tiles });

        u32 VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &GameState->VBOTiles);
        glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOTiles);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + totalTileSizeInBytes, nullptr, GL_STATIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), totalTileSizeInBytes, GameState->Tiles.data());

        // vertices
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);
        glEnableVertexAttribArray(0);

        // tile position/uv
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(tile), (void*) sizeof(vertices));
        glEnableVertexAttribArray(1);
        glVertexAttribDivisor(1, 1);

        // tile flipped
        glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(tile), (void*)(sizeof(vertices) + offset(tile, flipped)));
        glEnableVertexAttribArray(2);
        glVertexAttribDivisor(2, 1);

        // todo: make it efficient
        for (auto& layer : GameState->Level.objectLayers) {
            for (auto& entity : layer.entities) {
                Entities->push_back(entity.second);
            }
            for (auto& drawableEntity : layer.drawableEntities) {
                DrawableEntities->push_back(drawableEntity.second);
            }
        }

        GameState->Player = {};
        GameState->Player.position = Bob->position;
        GameState->Player.box = Bob->box;
        GameState->Player.spriteScale = vec2(1.f);
        GameState->Player.offset = (u32)sizeInBytes({ *DrawableEntities });
        GameState->Player.shouldRender = 1;
        GameState->Player.collides = true;
        GameState->Player.type = entityType::PLAYER;
        DrawableEntities->push_back(GameState->Player);

        GameState->SwooshEffect = {};
        GameState->SwooshEffect.position = Swoosh->position;
        GameState->SwooshEffect.box = Swoosh->box;
        GameState->SwooshEffect.spriteScale = vec2(2.f, 1.f);
        GameState->SwooshEffect.offset = (u32)sizeInBytes({ *DrawableEntities });
        GameState->SwooshEffect.shouldRender = 0;
        GameState->SwooshEffect.collides = true;
        GameState->SwooshEffect.type = entityType::EFFECT;
        DrawableEntities->push_back(GameState->SwooshEffect);

        glGenBuffers(1, &GameState->VBOEntities);
        glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOEntities);
        glBufferData(GL_ARRAY_BUFFER, (u32)(sizeof(u32) + sizeof(drawableEntity)) * DrawableEntities->size(), DrawableEntities->data(), GL_STREAM_DRAW);

        // position
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*)offset(drawableEntity, position));
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);
        // aabb
        //glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*) offset(drawableEntity, box));
        //glEnableVertexAttribArray(4);
        //glVertexAttribDivisor(4, 1);
        // uv/rotation
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*)offset(drawableEntity, uv));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);
        // flipped
        glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(drawableEntity), (void*)offset(drawableEntity, flipped));
        glEnableVertexAttribArray(6);
        glVertexAttribDivisor(6, 1);
        // spriteScale
        glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(drawableEntity), (void*)offset(drawableEntity, spriteScale));
        glEnableVertexAttribArray(7);
        glVertexAttribDivisor(7, 1);
        // shouldRender
        glVertexAttribIPointer(8, 1, GL_UNSIGNED_INT, sizeof(drawableEntity), (void*)offset(drawableEntity, shouldRender));
        glEnableVertexAttribArray(8);
        glVertexAttribDivisor(8, 1);

        particleEmitter charge = {};
        charge.maxParticlesCount = 500;
        charge.newParticlesCount = 5;
        charge.dt = 0.01f;
        charge.position = { 4.5 * TILE_SIZE.x, 6.5 * TILE_SIZE.y };
        charge.box.position = { 4.5 * TILE_SIZE.x, 6.5 * TILE_SIZE.y };
        charge.box.size = { 0.1f * TILE_SIZE.x, 0.1f * TILE_SIZE.x };
        charge.velocity = { 0.f, 0.f };
        charge.reflectorIndex = -1;
        charge.timeLeft = 3.f;

        charge.particles.reserve(charge.maxParticlesCount);
        charge.particles.assign(charge.maxParticlesCount, particle());

        ParticleEmitters->push_back(charge);

        glGenBuffers(1, &GameState->VBOParticles);
        glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOParticles);
        // todo: manage it somehow
        glBufferData(GL_ARRAY_BUFFER, 50 * (charge.maxParticlesCount) * sizeof(particle),
            nullptr, GL_STREAM_DRAW);

        // particle's position/size
        glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)offset(particle, position));
        glEnableVertexAttribArray(9);
        glVertexAttribDivisor(9, 1);
        // particle's uv
        glVertexAttribPointer(10, 2, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)offset(particle, uv));
        glEnableVertexAttribArray(10);
        glVertexAttribDivisor(10, 1);
        // particle's alpha value
        glVertexAttribPointer(11, 1, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)offset(particle, alpha));
        glEnableVertexAttribArray(11);
        glVertexAttribDivisor(11, 1);

        GameState->UpdateRate = 0.01f;   // 10 ms
        GameState->Lag = 0.f;

        // todo: draw collision regions
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        GameState->ChargeSpawnCooldown = 0.f;

        glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);

        Memory->IsInitalized = true;
    }

    GameState->Lag += Params->Delta;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (GameState->Lag >= GameState->UpdateRate) {
        glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOEntities);

        f32 dt = 0.15f;

        Bob->acceleration.x = 0.f;
#pragma region process input
        if (Params->Keys[GLFW_KEY_LEFT] == GLFW_PRESS) {
            if (
                Bob->currentAnimation != Bob->animations[2] &&
                Bob->currentAnimation != Bob->animations[1] &&
                Bob->currentAnimation != Bob->animations[3]
                ) {
                Bob->currentAnimation = Bob->animations[2];
            }
            Bob->acceleration.x = -12.f;
            Bob->flipped |= FLIPPED_HORIZONTALLY_FLAG;
        }

        if (Params->Keys[GLFW_KEY_RIGHT] == GLFW_PRESS) {
            if (
                Bob->currentAnimation != Bob->animations[2] &&
                Bob->currentAnimation != Bob->animations[1] &&
                Bob->currentAnimation != Bob->animations[3]
                ) {
                Bob->currentAnimation = Bob->animations[2];
            }
            Bob->acceleration.x = 12.f;
            Bob->flipped &= 0;
        }

        if (Params->Keys[GLFW_KEY_LEFT] == GLFW_RELEASE && !Params->ProcessedKeys[GLFW_KEY_LEFT]) {
            Params->ProcessedKeys[GLFW_KEY_LEFT] = true;
            if (Bob->currentAnimation != Bob->animations[0]) {
                Bob->currentAnimation = Bob->animations[0];
                Bob->xAnimationOffset = 0.f;
                Bob->flipped |= FLIPPED_HORIZONTALLY_FLAG;
            }
        }

        if (Params->Keys[GLFW_KEY_RIGHT] == GLFW_RELEASE && !Params->ProcessedKeys[GLFW_KEY_RIGHT]) {
            Params->ProcessedKeys[GLFW_KEY_RIGHT] = true;
            if (Bob->currentAnimation != Bob->animations[0]) {
                Bob->currentAnimation = Bob->animations[0];
                Bob->xAnimationOffset = 0.f;
                Bob->flipped &= 0;
            }
        }

        if (Params->Keys[GLFW_KEY_SPACE] == GLFW_PRESS && !Params->ProcessedKeys[GLFW_KEY_SPACE]) {
            Params->ProcessedKeys[GLFW_KEY_SPACE] = true;
            Bob->acceleration.y = -350.f;
            Bob->velocity.y = 0.f;
        }

        if (Params->Keys[GLFW_KEY_S] == GLFW_PRESS && !Params->ProcessedKeys[GLFW_KEY_S]) {
            Params->ProcessedKeys[GLFW_KEY_S] = true;
            Bob->currentAnimation = Bob->animations[3];
            Bob->xAnimationOffset = 0.f;

            Swoosh->shouldRender = true;
            Swoosh->xAnimationOffset = 0.f;
            Swoosh->flipped = Bob->flipped;

            // todo: make it better
            for (u32 i = 0; i < DrawableEntities->size(); ++i) {
                drawableEntity& entity = (*DrawableEntities)[i];
                if (entity.type == entityType::REFLECTOR) {
                    entity.underEffect = false;
                }
            }

            if (Swoosh->flipped & FLIPPED_HORIZONTALLY_FLAG) {
                Swoosh->position = { Bob->box.position.x - 2 * TILE_SIZE.x, Bob->box.position.y };
                Swoosh->box.position = { Bob->box.position.x - 2 * TILE_SIZE.x, Bob->box.position.y };
            }
            else {
                Swoosh->position = { Bob->box.position.x + TILE_SIZE.x, Bob->box.position.y };
                Swoosh->box.position = { Bob->box.position.x + TILE_SIZE.x, Bob->box.position.y };
            }
        }
#pragma endregion

        // friction imitation
        // todo: take scale into account!
        Bob->acceleration.x += -0.5f * Bob->velocity.x;
        Bob->velocity.x += Bob->acceleration.x * dt;

        Bob->acceleration.y += -0.01f * Bob->velocity.y;
        Bob->velocity.y += Bob->acceleration.y * dt;

        vec2 move = 0.5f * Bob->acceleration * dt * dt + Bob->velocity * dt;

        vec2 oldPosition = Bob->position;
        vec2 time = vec2(1.f);

        for (auto& entity : *Entities) {
            vec2 t = sweptAABB(oldPosition, move, entity.box, Bob->box.size);

            if (t.x >= 0.f && t.x < time.x) time.x = t.x;
            if (t.y >= 0.f && t.y < time.y) time.y = t.y;
        }

        for (u32 i = 0; i < DrawableEntities->size(); ++i) {
            drawableEntity& entity = (*DrawableEntities)[i];

            if (entity.type == entityType::REFLECTOR || entity.type == entityType::PLATFORM) {
                if (!entity.collides) break;

                vec2 t = sweptAABB(oldPosition, move, entity.box, Bob->box.size);

                if (t.x >= 0.f && t.x < time.x) time.x = t.x;
                if (t.y >= 0.f && t.y < time.y) time.y = t.y;

                if (entity.type == entityType::REFLECTOR) {
                    b32 swooshCollide = intersectAABB(Swoosh->box, entity.box);
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
            Bob->velocity.x = 0.f;
        }
        if (time.y < 1.f) {
            Bob->velocity.y = 0.f;

            if (time.y > 0.f && move.y > 0.f && Bob->currentAnimation != Bob->animations[1]) {
                Bob->currentAnimation = Bob->animations[1];
                Bob->xAnimationOffset = 0.f;
            }
        }
        if (time.y == 1.f) {
            if (Bob->velocity.y > 0.f) {
                if (Bob->currentAnimation != Bob->animations[3]) {
                    Bob->currentAnimation = Bob->animations[5];
                    Bob->xAnimationOffset = 0.f;
                }
            }
            else {
                if (Bob->currentAnimation != Bob->animations[3]) {
                    Bob->currentAnimation = Bob->animations[4];
                    Bob->xAnimationOffset = 0.f;
                }
            }
        }

        vec2 updatedMove = move * time;

        Bob->position.x = oldPosition.x + updatedMove.x;
        Bob->position.y = oldPosition.y + updatedMove.y;

        Bob->position.x = clamp(Bob->position.x, 0.f, (f32)TILE_SIZE.x * GameState->Level.width - TILE_SIZE.x);
        Bob->position.y = clamp(Bob->position.y, 0.f, (f32)TILE_SIZE.y * GameState->Level.height - TILE_SIZE.y);

        Bob->box.position = Bob->position;

        Bob->acceleration.y = 10.f;

        vec2 idleArea = { 2 * TILE_SIZE.x, 1 * TILE_SIZE.y };

        if (updatedMove.x > 0.f) {
            if (Bob->position.x + TILE_SIZE.x > camera.x + ScreenWidth / 2 + idleArea.x) {
                camera.x += updatedMove.x;
            }
        }
        else if (updatedMove.x < 0.f) {
            if (Bob->position.x < camera.x + ScreenWidth / 2 - idleArea.x) {
                camera.x += updatedMove.x;
            }
        }

        if (updatedMove.y > 0.f) {
            if (Bob->position.y + TILE_SIZE.y > camera.y + ScreenHeight / 2 + idleArea.y) {
                camera.y += updatedMove.y;
            }
        }
        else if (updatedMove.y < 0.f) {
            if (Bob->position.y < camera.y + ScreenHeight / 2 - idleArea.y) {
                camera.y += updatedMove.y;
            }
        }

        camera.x = camera.x > 0.f ? camera.x : 0.f;
        camera.y = camera.y > 0.f ? camera.y : 0.f;

        if (TILE_SIZE.x * GameState->Level.width - ScreenWidth >= 0) {
            camera.x = clamp(camera.x, 0.f, (f32)TILE_SIZE.x * GameState->Level.width - ScreenWidth);
        }
        if (TILE_SIZE.y * GameState->Level.height - ScreenHeight >= 0) {
            camera.y = clamp(camera.y, 0.f, (f32)TILE_SIZE.y * GameState->Level.height - ScreenHeight);
        }

        glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOParticles);

        u32 chargesCount = (u32)ParticleEmitters->size();

        for (u32 i = 0; i < chargesCount; ++i) {
            particleEmitter& charge = (*ParticleEmitters)[i];

            vec2 oldChargePosition = charge.box.position;
            vec2 chargeMove = charge.velocity * dt;
            vec2 chargeTime = vec2(1.f);

            for (u32 j = 0; j < DrawableEntities->size(); ++j) {
                drawableEntity& entity = (*DrawableEntities)[j];

                if (entity.type == entityType::REFLECTOR) {
                    aabb reflectorBox = entity.box;

                    vec2 t = sweptAABB(oldChargePosition, chargeMove, reflectorBox, charge.box.size);

                    // if not colliding
                    if (!((0.f <= t.x && t.x < 1.f) || (0.f <= t.y && t.y < 1.f))) {
                        if (!intersectAABB(charge.box, reflectorBox)) {
                            if (charge.reflectorIndex == (s32)j) {
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

                    if (charge.stopProcessingCollision && charge.reflectorIndex == (s32)j) continue;

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
                                charge.reflectorIndex = (s32)j;
                            }
                        }
                        else {
                            // collided with outer border: stop processing
                            chargeTime = t;
                            charge.isFading = true;
                        }
                    }
                    else if (chargeMove.x < 0.f) {
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
                                charge.reflectorIndex = (s32)j;
                            }
                        }
                        else {
                            chargeTime = t;
                            charge.isFading = true;
                        }
                    }
                    else if (chargeMove.y > 0.f) {
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
                                charge.reflectorIndex = (s32)j;
                            }
                        }
                        else {
                            chargeTime = t;
                            charge.isFading = true;
                        }
                    }
                    else if (chargeMove.y < 0.f) {
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
                                charge.reflectorIndex = (s32)j;
                            }
                        }
                        else {
                            chargeTime = t;
                            charge.isFading = true;
                        }
                    }
                }

                if (entity.id == 52) {
                    vec2 t = sweptAABB(oldChargePosition, chargeMove, entity.box, charge.box.size);

                    if ((0.f <= t.x && t.x < 1.f) || (0.f <= t.y && t.y < 1.f)) {
                        entity.currentAnimation = &GameState->TurnOnAnimation;
                        chargeTime = t;
                        charge.isFading = true;
                        charge.timeLeft = 0.f;

                        drawableEntity* platform1 = getEntityById(GameState->DrawableEntities, 57);
                        drawableEntity* platform2 = getEntityById(GameState->DrawableEntities, 60);
                        drawableEntity* platform3 = getEntityById(GameState->DrawableEntities, 61);

                        // todo: i need smth like setTimeout here
                        platform1->currentAnimation = &GameState->PlatformOnAnimation;
                        platform1->startAnimationDelay = 0.1f;
                        platform2->currentAnimation = &GameState->PlatformOnAnimation;
                        platform2->startAnimationDelay = 0.2f;
                        platform3->currentAnimation = &GameState->PlatformOnAnimation;
                        platform3->startAnimationDelay = 0.3f;
                    }
                }
            }

            chargeMove *= chargeTime;
            charge.box.position += chargeMove;
            charge.position += chargeMove;

            if (charge.box.position.x <= 0.f || charge.box.position.x >= (f32)TILE_SIZE.x * GameState->Level.width) {
                charge.velocity.x = -charge.velocity.x;
            }
            if (charge.box.position.y <= 0.f || charge.box.position.y >= (f32)TILE_SIZE.y * GameState->Level.height) {
                charge.velocity.y = -charge.velocity.y;
            }

            b32 chargeCollide = intersectAABB(Swoosh->box, (*ParticleEmitters)[0].box);
            if (chargeCollide && Swoosh->shouldRender && GameState->ChargeSpawnCooldown > 1.f) {
                GameState->ChargeSpawnCooldown = 0.f;

                particleEmitter newCharge = (*ParticleEmitters)[0];        // copy
                newCharge.velocity.x = Bob->flipped ? -chargeVelocity : chargeVelocity;

                ParticleEmitters->push_back(newCharge);
            }
        }

        for (u32 i = 0; i < ParticleEmitters->size(); ++i) {
            if ((*ParticleEmitters)[i].isFading) {
                (*ParticleEmitters)[i].timeLeft -= Params->Delta;
            }

            if ((*ParticleEmitters)[i].timeLeft <= 0.f) {
                // todo: read access violation
                //ParticleEmitters->erase(ParticleEmitters->begin() + i);
            }
        }

        u64 particlesSize = 0;
        // todo: use transform feedback instead?
        for (u32 i = 0; i < ParticleEmitters->size(); ++i) {
            particleEmitter& emitter = (*ParticleEmitters)[i];

            if (emitter.timeLeft <= 0.f) {
                particlesSize += sizeInBytes({ emitter.particles });
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
                particle.uv = vec2((13 * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / (f32)GameState->TextureHeight,
                    (16 * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / (f32)GameState->TextureHeight);
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

            particlesSize += sizeInBytes({ emitter.particles });
        }

        GameState->Lag -= GameState->UpdateRate;
    }

    GameState->ChargeSpawnCooldown += Params->Delta;

    mat4 view = mat4(1.0f);
    view = glm::translate(view, vec3(-camera, 0.f));
    setShaderUniform(GameState->ViewUniformLocation, view);

    glClear(GL_COLOR_BUFFER_BIT);

    //--- drawing tilemap ---
    glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOTiles);
    setShaderUniform(GameState->TypeUniformLocation, 1);

    mat4 model = mat4(1.0f);
    model = glm::scale(model, vec3(TILE_SIZE, 1.f));
    setShaderUniform(GameState->ModelUniformLocation, model);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (u32)GameState->Tiles.size());

    //--- bob ---
    f32 bobXOffset = (f32)(Bob->currentAnimation.x * (GameState->TilesetTileSize.x + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureWidth;
    f32 bobYOffset = (f32)(Bob->currentAnimation.y * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureHeight;

    if (Bob->frameTime >= Bob->currentAnimation.delay) {
        Bob->xAnimationOffset += (GameState->SpriteWidth + (f32)GameState->TilesetSpacing / GameState->TextureWidth) * Bob->currentAnimation.size;
        if (Bob->xAnimationOffset >= ((Bob->currentAnimation.frames * GameState->TilesetTileSize.x * Bob->currentAnimation.size) / (f32)GameState->TextureWidth)) {
            Bob->xAnimationOffset = 0.f;
            if (Bob->currentAnimation == Bob->animations[1] || Bob->currentAnimation == Bob->animations[3]) {
                Bob->currentAnimation = Bob->animations[0];
            }
        }

        Bob->frameTime = 0.0f;
    }
    Bob->frameTime += Params->Delta;

    model = mat4(1.0f);
    model = glm::scale(model, vec3(TILE_SIZE, 1.f));
    setShaderUniform(GameState->ModelUniformLocation, model);

    f32 effectXOffset = (f32)(Swoosh->currentAnimation.x * (GameState->TilesetTileSize.x + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureWidth;
    f32 effectYOffset = (f32)(Swoosh->currentAnimation.y * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureHeight;

    if (Swoosh->frameTime >= Swoosh->currentAnimation.delay) {
        Swoosh->xAnimationOffset += (GameState->SpriteWidth + (f32)GameState->TilesetSpacing / GameState->TextureWidth) * Swoosh->currentAnimation.size;
        if (Swoosh->xAnimationOffset >= ((Swoosh->currentAnimation.frames * GameState->TilesetTileSize.x * Swoosh->currentAnimation.size) / (f32)GameState->TextureWidth)) {
            Swoosh->xAnimationOffset = 0.f;
            Swoosh->shouldRender = false;
        }

        Swoosh->frameTime = 0.0f;
    }

    if (Swoosh->shouldRender) {
        Swoosh->frameTime += Params->Delta;
    }

    glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOEntities);
    // handling animations on all entities
    for (u32 i = 0; i < DrawableEntities->size(); ++i) {
        auto& entity = (*DrawableEntities)[i];

        if (entity.currentAnimation) {
            if (entity.startAnimationDelayTimer < entity.startAnimationDelay) {
                entity.startAnimationDelayTimer += Params->Delta;
                break;
            }

            f32 entityXOffset = (f32)(entity.currentAnimation->x * (GameState->TilesetTileSize.x + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureWidth;
            f32 entityYOffset = (f32)(entity.currentAnimation->y * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureHeight;

            if (entity.frameTime >= entity.currentAnimation->delay) {
                if (entity.currentAnimation->direction == direction::RIGHT) {
                    entity.xAnimationOffset += (GameState->SpriteWidth + (f32)GameState->TilesetSpacing / GameState->TextureWidth) * entity.currentAnimation->size;
                }
                else if (entity.currentAnimation->direction == direction::LEFT) {
                    entity.xAnimationOffset -= (GameState->SpriteWidth + (f32)GameState->TilesetSpacing / GameState->TextureWidth) * entity.currentAnimation->size;
                }

                entity.uv = vec2(entityXOffset + entity.xAnimationOffset, entityYOffset);
                glBufferSubData(GL_ARRAY_BUFFER, entity.offset + offset(drawableEntity, uv), sizeof(vec2), &entity.uv);

                if (abs(entity.xAnimationOffset) >= ((entity.currentAnimation->frames * GameState->TilesetTileSize.x * entity.currentAnimation->size) / (f32)GameState->TextureWidth)) {
                    entity.xAnimationOffset = 0.f;

                    entity.startAnimationDelayTimer = 0.f;
                    entity.startAnimationDelay = 0.f;
                    entity.currentAnimation = nullptr;
                }

                entity.frameTime = 0.0f;
            }
            entity.frameTime += Params->Delta;
        }
    }

    //--- drawing entities ---
    setShaderUniform(GameState->TypeUniformLocation, 3);

    GameState->Player.uv = vec2(bobXOffset + Bob->xAnimationOffset, bobYOffset);
    GameState->Player.position = Bob->position;
    GameState->Player.box.position = Bob->box.position;
    GameState->Player.flipped = Bob->flipped;

    GameState->SwooshEffect.uv = vec2(effectXOffset + Swoosh->xAnimationOffset, effectYOffset);
    GameState->SwooshEffect.position = Swoosh->position;
    GameState->SwooshEffect.box.position = Swoosh->box.position;
    GameState->SwooshEffect.flipped = Bob->flipped;
    GameState->SwooshEffect.shouldRender = Swoosh->shouldRender ? 1 : 0;

    glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.offset + offset(drawableEntity, position), 2 * sizeof(f32), &GameState->Player.position);
    glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.offset + offset(drawableEntity, position), 2 * sizeof(f32), &GameState->SwooshEffect.position);
    glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.offset + offset(drawableEntity, uv), 2 * sizeof(f32), &GameState->Player.uv);
    glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.offset + offset(drawableEntity, uv), 2 * sizeof(f32), &GameState->SwooshEffect.uv);
    glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.offset + offset(drawableEntity, flipped), sizeof(u32), &GameState->Player.flipped);
    glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.offset + offset(drawableEntity, flipped), sizeof(u32), &GameState->SwooshEffect.flipped);
    glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.offset + offset(drawableEntity, shouldRender), sizeof(u32), &GameState->SwooshEffect.shouldRender);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (s32)DrawableEntities->size());

    //--- drawing particles ---
    glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOParticles);
    setShaderUniform(GameState->TypeUniformLocation, 4);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // todo: offsets when delete in the middle?
    s32 totalParticlesCount = 0;
    for (u32 i = 0; i < ParticleEmitters->size(); ++i) {
        totalParticlesCount += (s32)(*ParticleEmitters)[i].particles.size();
    }

    // todo: draw only the ones which lifespan is greater than zero
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, totalParticlesCount);

    glFinish();

    //std::cout << delta * 1000.f << " ms" << std::endl;
}