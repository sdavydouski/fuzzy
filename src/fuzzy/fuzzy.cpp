#include "glad\glad.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nlohmann/json.hpp>

// todo: just for key codes
#include <GLFW/glfw3.h>

#include <fstream>

#include "fuzzy_types.h"
#include "fuzzy_platform.h"
#include "tiled.cpp"
#include "fuzzy.h"

vec3 normalizeRGB(s32 red, s32 green, s32 blue) {
    const f32 MAX = 255.f;
    return vec3(red / MAX, green / MAX, blue / MAX);
}

inline u32 createAndCompileShader(game_memory* Memory, game_state* GameState, GLenum shaderType, const string& path) {
    u32 shader = Memory->Renderer.glCreateShader(shaderType);
    string shaderSource = Memory->Platform.ReadTextFile(path);
    const char* shaderSourcePtr = shaderSource.c_str();
    Memory->Renderer.glShaderSource(shader, 1, &shaderSourcePtr, nullptr);
    Memory->Renderer.glCompileShader(shader);

    s32 isShaderCompiled;
    Memory->Renderer.glGetShaderiv(shader, GL_COMPILE_STATUS, &isShaderCompiled);
    if (!isShaderCompiled) {
        s32 LOG_LENGTH;
        Memory->Renderer.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &LOG_LENGTH);
        
        // todo: use transient arena?
        char* errorLog = PushArray<char>(&GameState->WorldArena, LOG_LENGTH);

        Memory->Renderer.glGetShaderInfoLog(shader, LOG_LENGTH, nullptr, &errorLog[0]);
        Memory->Platform.PrintOutput("Shader compilation failed:\n" + string(&errorLog[0]) + "\n");

        // todo: pop array

        Memory->Renderer.glDeleteShader(shader);
    }
    assert(isShaderCompiled);

    return shader;
}

inline s32 getUniformLocation(game_memory* Memory, u32 shaderProgram, const string& name) {
    s32 uniformLocation = Memory->Renderer.glGetUniformLocation(shaderProgram, name.c_str());
    //assert(uniformLocation != -1);
    return uniformLocation;
}

inline void setShaderUniform(game_memory* Memory, s32 location, s32 value) {
    Memory->Renderer.glUniform1i(location, value);
}

inline void setShaderUniform(game_memory* Memory, s32 location, const vec2& value) {
    Memory->Renderer.glUniform2f(location, value.x, value.y);
}

inline void setShaderUniform(game_memory* Memory, s32 location, const mat4& value) {
    Memory->Renderer.glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
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

// todo: replace with smth more performant
inline drawable_entity* GetEntityById(game_state* GameState, u32 Id) {
    drawable_entity* Result = nullptr;

    for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->DrawableEntitiesCount; ++DrawableEntityIndex) {
        drawable_entity* Entity = &GameState->DrawableEntities[DrawableEntityIndex];

        if (Entity->id == Id) {
            Result = Entity;
            break;
        }
    }

    assert(Result);

    return Result;
}

u32 FindFirstUnusedParticle(particle_emitter* Emitter) {
    for (u32 i = Emitter->LastUsedParticle; i < Emitter->ParticlesCount; ++i) {
        if (Emitter->Particles[i].Lifespan <= 0.f) {
            return i;
        }
    }

    for (u32 i = 0; i < Emitter->LastUsedParticle; ++i) {
        if (Emitter->Particles[i].Lifespan <= 0.f) {
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

vec3 backgroundColor = normalizeRGB(29, 33, 45);

vec2 Scale = vec2(4.f);
// todo: could be different value than 16.f
const vec2 TILE_SIZE = { 16.f * Scale.x, 16.f * Scale.y };

const f32 chargeVelocity = 10.f;

void processInput(game_state* GameState, game_input* Input) {
    sprite* Bob = &GameState->Bob;
    effect* Swoosh = &GameState->Swoosh;

    if (Input->Keys[GLFW_KEY_LEFT] == GLFW_PRESS) {
        if (
            Bob->CurrentAnimation != Bob->Animations[2] &&
            Bob->CurrentAnimation != Bob->Animations[1] &&
            Bob->CurrentAnimation != Bob->Animations[3]
            ) {
            Bob->CurrentAnimation = Bob->Animations[2];
        }
        Bob->Acceleration.x = -12.f;
        Bob->Flipped |= FLIPPED_HORIZONTALLY_FLAG;
    }

    if (Input->Keys[GLFW_KEY_RIGHT] == GLFW_PRESS) {
        if (
            Bob->CurrentAnimation != Bob->Animations[2] &&
            Bob->CurrentAnimation != Bob->Animations[1] &&
            Bob->CurrentAnimation != Bob->Animations[3]
            ) {
            Bob->CurrentAnimation = Bob->Animations[2];
        }
        Bob->Acceleration.x = 12.f;
        Bob->Flipped &= 0;
    }

    if (Input->Keys[GLFW_KEY_LEFT] == GLFW_RELEASE && !Input->ProcessedKeys[GLFW_KEY_LEFT]) {
        Input->ProcessedKeys[GLFW_KEY_LEFT] = true;
        if (Bob->CurrentAnimation != Bob->Animations[0]) {
            Bob->CurrentAnimation = Bob->Animations[0];
            Bob->XAnimationOffset = 0.f;
            Bob->Flipped |= FLIPPED_HORIZONTALLY_FLAG;
        }
    }

    if (Input->Keys[GLFW_KEY_RIGHT] == GLFW_RELEASE && !Input->ProcessedKeys[GLFW_KEY_RIGHT]) {
        Input->ProcessedKeys[GLFW_KEY_RIGHT] = true;
        if (Bob->CurrentAnimation != Bob->Animations[0]) {
            Bob->CurrentAnimation = Bob->Animations[0];
            Bob->XAnimationOffset = 0.f;
            Bob->Flipped &= 0;
        }
    }

    if (Input->Keys[GLFW_KEY_SPACE] == GLFW_PRESS && !Input->ProcessedKeys[GLFW_KEY_SPACE]) {
        Input->ProcessedKeys[GLFW_KEY_SPACE] = true;
        Bob->Acceleration.y = -350.f;
        Bob->Velocity.y = 0.f;
    }

    if (Input->Keys[GLFW_KEY_S] == GLFW_PRESS && !Input->ProcessedKeys[GLFW_KEY_S]) {
        Input->ProcessedKeys[GLFW_KEY_S] = true;
        Bob->CurrentAnimation = Bob->Animations[3];
        Bob->XAnimationOffset = 0.f;

        Swoosh->ShouldRender = true;
        Swoosh->XAnimationOffset = 0.f;
        Swoosh->Flipped = Bob->Flipped;

        // todo: make it better
        for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->DrawableEntitiesCount; ++DrawableEntityIndex) {
            drawable_entity* Entity = &GameState->DrawableEntities[DrawableEntityIndex];
            if (Entity->type == entity_type::REFLECTOR) {
                Entity->underEffect = false;
            }
        }

        if (Swoosh->Flipped & FLIPPED_HORIZONTALLY_FLAG) {
            Swoosh->Position = { Bob->Box.position.x - 2 * TILE_SIZE.x, Bob->Box.position.y };
            Swoosh->Box.position = { Bob->Box.position.x - 2 * TILE_SIZE.x, Bob->Box.position.y };
        }
        else {
            Swoosh->Position = { Bob->Box.position.x + TILE_SIZE.x, Bob->Box.position.y };
            Swoosh->Box.position = { Bob->Box.position.x + TILE_SIZE.x, Bob->Box.position.y };
        }
    }
}

extern "C" EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
    assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state* GameState = (game_state*)Memory->PermanentStorage;

    s32 ScreenWidth = Params->ScreenWidth;
    s32 ScreenHeight = Params->ScreenHeight;

    sprite* Bob = &GameState->Bob;
    effect* Swoosh = &GameState->Swoosh;

    platform_api* Platform = &Memory->Platform;
    renderer_api* Renderer = &Memory->Renderer;

    //GameState->TestMap["purpuse"] = 42;

    if (!Memory->IsInitalized) {
        InitializeMemoryArena(
            &GameState->WorldArena, 
            Memory->PermanentStorageSize - sizeof(game_state), 
            (u8*)Memory->PermanentStorage + sizeof(game_state)
        );

        s32 textureChannels;
        u8* textureImage = Platform->ReadImageFile("textures/industrial_tileset.png",
            &GameState->TextureWidth, &GameState->TextureHeight, &textureChannels, 0);
        if (!textureImage) {
            Platform->PrintOutput("Texture loading failed:\n");
        }
        assert(textureImage);

        u32 texture;
        Renderer->glGenTextures(1, &texture);
        Renderer->glBindTexture(GL_TEXTURE_2D, texture);

        // note: default value for GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR
        // since we do not use mipmaps we must override this value
        Renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        Renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        Renderer->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GameState->TextureWidth, GameState->TextureHeight, 
            0, GL_RGBA, GL_UNSIGNED_BYTE, textureImage);

        Platform->FreeImageFile(textureImage);

        u32 vertexShader = createAndCompileShader(Memory, GameState, GL_VERTEX_SHADER, "shaders/basic.vert");
        u32 fragmentShader = createAndCompileShader(Memory, GameState, GL_FRAGMENT_SHADER, "shaders/basic.frag");

        u32 shaderProgram = Renderer->glCreateProgram();
        Renderer->glAttachShader(shaderProgram, vertexShader);
        Renderer->glAttachShader(shaderProgram, fragmentShader);
        Renderer->glLinkProgram(shaderProgram);
        Renderer->glDeleteShader(vertexShader);
        Renderer->glDeleteShader(fragmentShader);

        s32 isShaderProgramLinked;
        Renderer->glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isShaderProgramLinked);

        if (!isShaderProgramLinked) {
            s32 LOG_LENGTH;
            Renderer->glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &LOG_LENGTH);

            vector<char> errorLog(LOG_LENGTH);

            Renderer->glGetProgramInfoLog(shaderProgram, LOG_LENGTH, nullptr, &errorLog[0]);
            Platform->PrintOutput("Shader program linkage failed:\n" + string(&errorLog[0]) + "\n");
        }
        assert(isShaderProgramLinked);

        Renderer->glUseProgram(shaderProgram);

        mat4 projection = glm::ortho(0.0f, (f32)ScreenWidth, (f32)ScreenHeight, 0.0f);

        s32 projectionUniformLocation = getUniformLocation(Memory, shaderProgram, "projection");
        setShaderUniform(Memory, projectionUniformLocation, projection);
        GameState->ViewUniformLocation = getUniformLocation(Memory, shaderProgram, "view");
        GameState->ModelUniformLocation = getUniformLocation(Memory, shaderProgram, "model");
        GameState->TypeUniformLocation = getUniformLocation(Memory, shaderProgram, "type");
        s32 tileTypeUniformLocation = getUniformLocation(Memory, shaderProgram, "tileType");

        json spritesConfig = Platform->ReadJsonFile("textures/sprites.json");

        // todo: std::map isn't working
        tileset Tileset = loadTileset(Platform->ReadJsonFile, "levels/tileset.json");
        GameState->TilesetColumns = Tileset.columns;
        GameState->TilesetImageSize = Tileset.imageSize;
        GameState->TilesetMargin = Tileset.margin;
        GameState->TilesetSpacing = Tileset.spacing;
        GameState->TilesetTileSize = Tileset.tileSize;

        auto bobConfig = spritesConfig["sprites"][0];
        auto bobAnimations = bobConfig["animations"];

        *Bob = {};
        Bob->Position = { 5 * TILE_SIZE.x, 0 * TILE_SIZE.y };
        Bob->Box.position = { 5 * TILE_SIZE.x, 0 * TILE_SIZE.y };
        Bob->Box.size = { 13.f * Scale.x, 16.f * Scale.y };
        Bob->Velocity = { 0.f, 0.f };
        Bob->Acceleration = { 0.f, 10.f };
        Bob->AnimationsCount = (u32)bobAnimations.size();
        Bob->Animations = PushArray<animation>(&GameState->WorldArena, Bob->AnimationsCount);

        u32 BobAnimationsIndex = 0;
        for (auto animation : bobAnimations) {
            Bob->Animations[BobAnimationsIndex++] = {
                animation["x"], animation["y"], animation["frames"], animation["delay"], animation["size"]
            };
        }

        Bob->CurrentAnimation = Bob->Animations[0];
        Bob->XAnimationOffset = 0.f;
        Bob->FrameTime = 0.f;

        auto effectsConfig = spritesConfig["sprites"][1];
        auto effectsAnimations = effectsConfig["animations"];

        *Swoosh = {};
        Swoosh->Position = { 0.f, 0.f };     // todo: think about better ways
        Swoosh->Box.position = { 0.f, 0.f };     // todo: think about better ways
        Swoosh->Box.size = { 2 * TILE_SIZE.x, TILE_SIZE.y };
        Swoosh->ShouldRender = false;

        Swoosh->AnimationsCount = (u32)effectsAnimations.size();
        Swoosh->Animations = PushArray<animation>(&GameState->WorldArena, Swoosh->AnimationsCount);

        u32 SwooshAnimationIndex = 0;
        for (auto& animation : effectsAnimations) {
            Swoosh->Animations[SwooshAnimationIndex++] = {
                animation["x"], animation["y"], animation["frames"], animation["delay"], animation["size"]
            };
        }

        Swoosh->CurrentAnimation = Swoosh->Animations[0];
        Swoosh->XAnimationOffset = 0.f;
        Swoosh->FrameTime = 0.f;

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

        s32 spriteSizeUniformLocation = getUniformLocation(Memory, shaderProgram, "spriteSize");
        setShaderUniform(Memory, spriteSizeUniformLocation, vec2(GameState->SpriteWidth, GameState->SpriteHeight));

        f32 vertices[] = {
            // Pos    // UV
            0.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 1.f,
            1.f, 0.f, 1.f, 0.f,
            1.f, 1.f, 1.f, 1.f
        };

        GameState->Level = LoadMap(&GameState->WorldArena, Platform->ReadJsonFile, "levels/level01.json", Tileset, Scale);

        // todo: make it efficient
        for (u32 TileLayersIndex = 0; TileLayersIndex < GameState->Level.TileLayersCount; ++TileLayersIndex) {
            GameState->TilesCount += GameState->Level.TileLayers[TileLayersIndex].TilesCount;
        }

        GameState->Tiles = PushArray<tile>(&GameState->WorldArena, GameState->TilesCount);

        u32 TotalTilesIndex = 0;
        for (u32 TileLayersIndex = 0; TileLayersIndex < GameState->Level.TileLayersCount; ++TileLayersIndex) {
            for (u32 TilesIndex = 0; TilesIndex < GameState->Level.TileLayers[TileLayersIndex].TilesCount; ++TilesIndex) {
                GameState->Tiles[TotalTilesIndex++] = GameState->Level.TileLayers[TileLayersIndex].Tiles[TilesIndex];
            }
        }

        u64 TotalTileSizeInBytes = GameState->TilesCount * sizeof(tile);

        u32 VAO;
        Renderer->glGenVertexArrays(1, &VAO);
        Renderer->glBindVertexArray(VAO);

        Renderer->glGenBuffers(1, &GameState->VBOTiles);
        Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOTiles);
        Renderer->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + TotalTileSizeInBytes, nullptr, GL_STATIC_DRAW);

        Renderer->glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        Renderer->glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), TotalTileSizeInBytes, GameState->Tiles);

        // vertices
        Renderer->glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);
        Renderer->glEnableVertexAttribArray(0);

        // tile position/uv
        Renderer->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(tile), (void*) sizeof(vertices));
        Renderer->glEnableVertexAttribArray(1);
        Renderer->glVertexAttribDivisor(1, 1);

        // tile flipped
        Renderer->glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(tile), (void*)(sizeof(vertices) + offset(tile, flipped)));
        Renderer->glEnableVertexAttribArray(2);
        Renderer->glVertexAttribDivisor(2, 1);

        // todo: make it efficient
        for (u32 ObjectLayersIndex = 0; ObjectLayersIndex < GameState->Level.ObjectLayersCount; ++ObjectLayersIndex) {
            GameState->EntitiesCount += GameState->Level.ObjectLayers[ObjectLayersIndex].EntitiesCount;
            GameState->DrawableEntitiesCount += GameState->Level.ObjectLayers[ObjectLayersIndex].DrawableEntitiesCount;
        }
        
        GameState->Entities = PushArray<entity>(&GameState->WorldArena, GameState->EntitiesCount);

        // + player and swoosh effect
        GameState->DrawableEntitiesCount += 2;
        GameState->DrawableEntities = PushArray<drawable_entity>(&GameState->WorldArena, GameState->DrawableEntitiesCount);

        u32 TotalEntityIndex = 0;
        u32 TotalDrawableEntityIndex = 0;
        for (u32 ObjectLayersIndex = 0; ObjectLayersIndex < GameState->Level.ObjectLayersCount; ++ObjectLayersIndex) {
            for (u32 EntityIndex = 0; EntityIndex < GameState->Level.ObjectLayers[ObjectLayersIndex].EntitiesCount; ++EntityIndex) {
                GameState->Entities[TotalEntityIndex++] = GameState->Level.ObjectLayers[ObjectLayersIndex].Entities[EntityIndex];
            }
            for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->Level.ObjectLayers[ObjectLayersIndex].DrawableEntitiesCount; ++DrawableEntityIndex) {
                GameState->DrawableEntities[TotalDrawableEntityIndex++] = GameState->Level.ObjectLayers[ObjectLayersIndex].DrawableEntities[DrawableEntityIndex];
            }
        }

        GameState->Player = {};
        GameState->Player.position = Bob->Position;
        GameState->Player.box = Bob->Box;
        GameState->Player.spriteScale = vec2(1.f);
        GameState->Player.offset = (GameState->DrawableEntitiesCount - 2) * sizeof(drawable_entity);
        GameState->Player.shouldRender = 1;
        GameState->Player.collides = true;
        GameState->Player.type = entity_type::PLAYER;
        GameState->DrawableEntities[GameState->DrawableEntitiesCount - 2] = GameState->Player;

        GameState->SwooshEffect = {};
        GameState->SwooshEffect.position = Swoosh->Position;
        GameState->SwooshEffect.box = Swoosh->Box;
        GameState->SwooshEffect.spriteScale = vec2(2.f, 1.f);
        GameState->SwooshEffect.offset = (GameState->DrawableEntitiesCount - 1) * sizeof(drawable_entity);
        GameState->SwooshEffect.shouldRender = 0;
        GameState->SwooshEffect.collides = true;
        GameState->SwooshEffect.type = entity_type::EFFECT;
        GameState->DrawableEntities[GameState->DrawableEntitiesCount - 1] = GameState->SwooshEffect;

        Renderer->glGenBuffers(1, &GameState->VBOEntities);
        Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOEntities);
        Renderer->glBufferData(GL_ARRAY_BUFFER, (u32)(sizeof(u32) + sizeof(drawable_entity)) * GameState->DrawableEntitiesCount, GameState->DrawableEntities, GL_STREAM_DRAW);

        // position
        Renderer->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(drawable_entity), (void*)offset(drawable_entity, position));
        Renderer->glEnableVertexAttribArray(3);
        Renderer->glVertexAttribDivisor(3, 1);
        // aabb
        //glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(drawable_entity), (void*) offset(drawable_entity, box));
        //glEnableVertexAttribArray(4);
        //glVertexAttribDivisor(4, 1);
        // uv/rotation
        Renderer->glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(drawable_entity), (void*)offset(drawable_entity, uv));
        Renderer->glEnableVertexAttribArray(5);
        Renderer->glVertexAttribDivisor(5, 1);
        // flipped
        Renderer->glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(drawable_entity), (void*)offset(drawable_entity, flipped));
        Renderer->glEnableVertexAttribArray(6);
        Renderer->glVertexAttribDivisor(6, 1);
        // spriteScale
        Renderer->glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(drawable_entity), (void*)offset(drawable_entity, spriteScale));
        Renderer->glEnableVertexAttribArray(7);
        Renderer->glVertexAttribDivisor(7, 1);
        // shouldRender
        Renderer->glVertexAttribIPointer(8, 1, GL_UNSIGNED_INT, sizeof(drawable_entity), (void*)offset(drawable_entity, shouldRender));
        Renderer->glEnableVertexAttribArray(8);
        Renderer->glVertexAttribDivisor(8, 1);

        GameState->ParticleEmittersIndex = 0;
        GameState->ParticleEmittersMaxCount = 50;
        GameState->ParticleEmitters = PushArray<particle_emitter>(&GameState->WorldArena, GameState->ParticleEmittersMaxCount);

        particle_emitter Charge = {};
        Charge.ParticlesCount = 500;
        Charge.NewParticlesCount = 5;
        Charge.Dt = 0.01f;
        Charge.Position = { 4.5 * TILE_SIZE.x, 6.5 * TILE_SIZE.y };
        Charge.Box.position = { 4.5 * TILE_SIZE.x, 6.5 * TILE_SIZE.y };
        Charge.Box.size = { 0.1f * TILE_SIZE.x, 0.1f * TILE_SIZE.x };
        Charge.Velocity = { 0.f, 0.f };
        Charge.ReflectorIndex = -1;
        Charge.TimeLeft = 3.f;
        Charge.Particles = PushArray<particle>(&GameState->WorldArena, Charge.ParticlesCount);

        GameState->ParticleEmitters[GameState->ParticleEmittersIndex++] = Charge;

        Renderer->glGenBuffers(1, &GameState->VBOParticles);
        Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOParticles);
        // todo: manage it somehow
        Renderer->glBufferData(GL_ARRAY_BUFFER, GameState->ParticleEmittersMaxCount * (Charge.ParticlesCount) * sizeof(particle),
            nullptr, GL_STREAM_DRAW);

        // particle's position/size
        Renderer->glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)offset(particle, Position));
        Renderer->glEnableVertexAttribArray(9);
        Renderer->glVertexAttribDivisor(9, 1);
        // particle's uv
        Renderer->glVertexAttribPointer(10, 2, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)offset(particle, UV));
        Renderer->glEnableVertexAttribArray(10);
        Renderer->glVertexAttribDivisor(10, 1);
        // particle's alpha value
        Renderer->glVertexAttribPointer(11, 1, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)offset(particle, Alpha));
        Renderer->glEnableVertexAttribArray(11);
        Renderer->glVertexAttribDivisor(11, 1);

        GameState->UpdateRate = 0.01f;   // 10 ms
        GameState->Lag = 0.f;

        // todo: draw collision regions
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        GameState->ChargeSpawnCooldown = 0.f;

        Renderer->glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);

        Memory->IsInitalized = true;
    }

    GameState->Lag += Params->Delta;

    Renderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (GameState->Lag >= GameState->UpdateRate) {
        Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOEntities);

        f32 dt = 0.15f;

        Bob->Acceleration.x = 0.f;
        processInput(GameState, &Params->Input);

        // friction imitation
        // todo: take scale into account!
        Bob->Acceleration.x += -0.5f * Bob->Velocity.x;
        Bob->Velocity.x += Bob->Acceleration.x * dt;

        Bob->Acceleration.y += -0.01f * Bob->Velocity.y;
        Bob->Velocity.y += Bob->Acceleration.y * dt;

        vec2 move = 0.5f * Bob->Acceleration * dt * dt + Bob->Velocity * dt;

        vec2 oldPosition = Bob->Position;
        vec2 time = vec2(1.f);

        for (u32 EntityIndex = 0; EntityIndex < GameState->EntitiesCount; ++EntityIndex) {
            vec2 t = sweptAABB(oldPosition, move, GameState->Entities[EntityIndex].box, Bob->Box.size);

            if (t.x >= 0.f && t.x < time.x) time.x = t.x;
            if (t.y >= 0.f && t.y < time.y) time.y = t.y;
        }

        for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->DrawableEntitiesCount; ++DrawableEntityIndex) {
            drawable_entity* Entity = &GameState->DrawableEntities[DrawableEntityIndex];

            if (Entity->type == entity_type::REFLECTOR || Entity->type == entity_type::PLATFORM) {
                if (!Entity->collides) break;

                vec2 t = sweptAABB(oldPosition, move, Entity->box, Bob->Box.size);

                if (t.x >= 0.f && t.x < time.x) time.x = t.x;
                if (t.y >= 0.f && t.y < time.y) time.y = t.y;

                if (Entity->type == entity_type::REFLECTOR) {
                    b32 swooshCollide = intersectAABB(Swoosh->Box, Entity->box);
                    if (!Entity->underEffect && swooshCollide) {
                        Entity->underEffect = true;
                        Entity->isRotating = true;
                    }

                    if (Entity->isRotating) {
                        Entity->rotation += 5.f;
                        Renderer->glBufferSubData(GL_ARRAY_BUFFER, Entity->offset + offset(drawable_entity, rotation), sizeof(u32), &Entity->rotation);

                        if (0.f < Entity->rotation && Entity->rotation <= 90.f) {
                            if (Entity->rotation == 90.f) {
                                Entity->isRotating = false;
                                break;
                            }
                        }
                        if (90.f < Entity->rotation && Entity->rotation <= 180.f) {
                            if (Entity->rotation == 180.f) {
                                Entity->isRotating = false;
                                break;
                            }
                        }
                        if (180.f < Entity->rotation && Entity->rotation <= 270.f) {
                            if (Entity->rotation == 270.f) {
                                Entity->isRotating = false;
                                break;
                            }
                        }
                        if (270.f < Entity->rotation && Entity->rotation <= 360.f) {
                            if (Entity->rotation == 360.f) {
                                Entity->isRotating = false;
                                Entity->rotation = 0.f;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (time.x < 1.f) {
            Bob->Velocity.x = 0.f;
        }
        if (time.y < 1.f) {
            Bob->Velocity.y = 0.f;

            if (time.y > 0.f && move.y > 0.f && Bob->CurrentAnimation != Bob->Animations[1]) {
                Bob->CurrentAnimation = Bob->Animations[1];
                Bob->XAnimationOffset = 0.f;
            }
        }
        if (time.y == 1.f) {
            if (Bob->Velocity.y > 0.f) {
                if (Bob->CurrentAnimation != Bob->Animations[3]) {
                    Bob->CurrentAnimation = Bob->Animations[5];
                    Bob->XAnimationOffset = 0.f;
                }
            }
            else {
                if (Bob->CurrentAnimation != Bob->Animations[3]) {
                    Bob->CurrentAnimation = Bob->Animations[4];
                    Bob->XAnimationOffset = 0.f;
                }
            }
        }

        vec2 updatedMove = move * time;

        Bob->Position.x = oldPosition.x + updatedMove.x;
        Bob->Position.y = oldPosition.y + updatedMove.y;

        Bob->Position.x = clamp(Bob->Position.x, 0.f, (f32)TILE_SIZE.x * GameState->Level.Width - TILE_SIZE.x);
        Bob->Position.y = clamp(Bob->Position.y, 0.f, (f32)TILE_SIZE.y * GameState->Level.Height - TILE_SIZE.y);

        Bob->Box.position = Bob->Position;

        Bob->Acceleration.y = 10.f;

        vec2 idleArea = { 2 * TILE_SIZE.x, 1 * TILE_SIZE.y };

        if (updatedMove.x > 0.f) {
            if (Bob->Position.x + TILE_SIZE.x > GameState->Camera.x + ScreenWidth / 2 + idleArea.x) {
                GameState->Camera.x += updatedMove.x;
            }
        }
        else if (updatedMove.x < 0.f) {
            if (Bob->Position.x < GameState->Camera.x + ScreenWidth / 2 - idleArea.x) {
                GameState->Camera.x += updatedMove.x;
            }
        }

        if (updatedMove.y > 0.f) {
            if (Bob->Position.y + TILE_SIZE.y > GameState->Camera.y + ScreenHeight / 2 + idleArea.y) {
                GameState->Camera.y += updatedMove.y;
            }
        }
        else if (updatedMove.y < 0.f) {
            if (Bob->Position.y < GameState->Camera.y + ScreenHeight / 2 - idleArea.y) {
                GameState->Camera.y += updatedMove.y;
            }
        }

        GameState->Camera.x = GameState->Camera.x > 0.f ? GameState->Camera.x : 0.f;
        GameState->Camera.y = GameState->Camera.y > 0.f ? GameState->Camera.y : 0.f;

        if (TILE_SIZE.x * GameState->Level.Width - ScreenWidth >= 0) {
            GameState->Camera.x = clamp(GameState->Camera.x, 0.f, (f32)TILE_SIZE.x * GameState->Level.Width - ScreenWidth);
        }
        if (TILE_SIZE.y * GameState->Level.Height - ScreenHeight >= 0) {
            GameState->Camera.y = clamp(GameState->Camera.y, 0.f, (f32)TILE_SIZE.y * GameState->Level.Height - ScreenHeight);
        }

        Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOParticles);

        for (u32 i = 0; i < GameState->ParticleEmittersIndex; ++i) {
            particle_emitter* Charge = &GameState->ParticleEmitters[i];

            vec2 oldChargePosition = Charge->Box.position;
            vec2 chargeMove = Charge->Velocity * dt;
            vec2 chargeTime = vec2(1.f);

            for (u32 j = 0; j < GameState->DrawableEntitiesCount; ++j) {
                drawable_entity* Entity = &GameState->DrawableEntities[j];

                if (Entity->type == entity_type::REFLECTOR) {
                    aabb reflectorBox = Entity->box;

                    vec2 t = sweptAABB(oldChargePosition, chargeMove, reflectorBox, Charge->Box.size);

                    // if not colliding
                    if (!((0.f <= t.x && t.x < 1.f) || (0.f <= t.y && t.y < 1.f))) {
                        if (!intersectAABB(Charge->Box, reflectorBox)) {
                            if (Charge->ReflectorIndex == (s32)j) {
                                Charge->StopProcessingCollision = false;
                                Charge->ReflectorIndex = -1;
                            }
                            continue;
                        }
                    }

                    // if collides check direction of the charge
                    // check reflector's angle
                    // dimiss charge if it's coming from the wrong side
                    // proceed with new collision rule otherwise.

                    if (Charge->StopProcessingCollision && Charge->ReflectorIndex == (s32)j) continue;

                    aabb testBox = {};

                    if (chargeMove.x > 0.f) {
                        if (Entity->rotation == 180.f || Entity->rotation == 270.f) {
                            testBox.position.x = reflectorBox.position.x + reflectorBox.size.x / 2.f + Charge->Box.size.x / 2.f;
                            testBox.position.y = reflectorBox.position.y;
                            testBox.size.x = reflectorBox.size.x / 2.f - Charge->Box.size.x / 2.f;
                            testBox.size.y = reflectorBox.size.y;

                            vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, Charge->Box.size);

                            if (0.f <= t.x && t.x < 1.f) {
                                chargeTime.x = t.x;

                                Charge->Velocity.x = 0.f;
                                Charge->Velocity.y = Entity->rotation == 180.f ? chargeVelocity : -chargeVelocity;
                                Charge->StopProcessingCollision = true;
                                Charge->ReflectorIndex = (s32)j;
                            }
                        }
                        else {
                            // collided with outer border: stop processing
                            chargeTime = t;
                            Charge->IsFading = true;
                        }
                    }
                    else if (chargeMove.x < 0.f) {
                        if (Entity->rotation == 0.f || Entity->rotation == 90.f) {
                            testBox.position.x = reflectorBox.position.x;
                            testBox.position.y = reflectorBox.position.y;
                            testBox.size.x = reflectorBox.size.x / 2.f - Charge->Box.size.x / 2.f;
                            testBox.size.y = reflectorBox.size.y;

                            vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, Charge->Box.size);

                            if (0.f <= t.x && t.x < 1.f) {
                                chargeTime.x = t.x;

                                Charge->Velocity.x = 0.f;
                                Charge->Velocity.y = Entity->rotation == 0.f ? -chargeVelocity : chargeVelocity;
                                Charge->StopProcessingCollision = true;
                                Charge->ReflectorIndex = (s32)j;
                            }
                        }
                        else {
                            chargeTime = t;
                            Charge->IsFading = true;
                        }
                    }
                    else if (chargeMove.y > 0.f) {
                        if (Entity->rotation == 0.f || Entity->rotation == 270.f) {
                            testBox.position.x = reflectorBox.position.x;
                            testBox.position.y = reflectorBox.position.y + reflectorBox.size.y / 2.f + Charge->Box.size.y / 2.f;
                            testBox.size.x = reflectorBox.size.x;
                            testBox.size.y = reflectorBox.size.y / 2.f - Charge->Box.size.y / 2.f;

                            vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, Charge->Box.size);

                            if (0.f <= t.y && t.y < 1.f) {
                                chargeTime.y = t.y;

                                Charge->Velocity.x = Entity->rotation == 0.f ? chargeVelocity : -chargeVelocity;
                                Charge->Velocity.y = 0.f;
                                Charge->StopProcessingCollision = true;
                                Charge->ReflectorIndex = (s32)j;
                            }
                        }
                        else {
                            chargeTime = t;
                            Charge->IsFading = true;
                        }
                    }
                    else if (chargeMove.y < 0.f) {
                        if (Entity->rotation == 90.f || Entity->rotation == 180.f) {
                            testBox.position.x = reflectorBox.position.x;
                            testBox.position.y = reflectorBox.position.y;
                            testBox.size.x = reflectorBox.size.x;
                            testBox.size.y = reflectorBox.size.y / 2.f - Charge->Box.size.y / 2.f;

                            vec2 t = sweptAABB(oldChargePosition, chargeMove, testBox, Charge->Box.size);

                            if (0.f <= t.y && t.y < 1.f) {
                                chargeTime.y = t.y;

                                Charge->Velocity.x = Entity->rotation == 90.f ? chargeVelocity : -chargeVelocity;
                                Charge->Velocity.y = 0.f;
                                Charge->StopProcessingCollision = true;
                                Charge->ReflectorIndex = (s32)j;
                            }
                        }
                        else {
                            chargeTime = t;
                            Charge->IsFading = true;
                        }
                    }
                }

                if (Entity->id == 52) {
                    vec2 t = sweptAABB(oldChargePosition, chargeMove, Entity->box, Charge->Box.size);

                    if ((0.f <= t.x && t.x < 1.f) || (0.f <= t.y && t.y < 1.f)) {
                        Entity->currentAnimation = &GameState->TurnOnAnimation;
                        chargeTime = t;
                        Charge->IsFading = true;
                        Charge->TimeLeft = 0.f;

                        drawable_entity* platform1 = GetEntityById(GameState, 57);
                        drawable_entity* platform2 = GetEntityById(GameState, 60);
                        drawable_entity* platform3 = GetEntityById(GameState, 61);

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
            Charge->Box.position += chargeMove;
            Charge->Position += chargeMove;

            if (Charge->Box.position.x <= 0.f || Charge->Box.position.x >= (f32)TILE_SIZE.x * GameState->Level.Width) {
                Charge->Velocity.x = -Charge->Velocity.x;
            }
            if (Charge->Box.position.y <= 0.f || Charge->Box.position.y >= (f32)TILE_SIZE.y * GameState->Level.Height) {
                Charge->Velocity.y = -Charge->Velocity.y;
            }

            b32 chargeCollide = intersectAABB(Swoosh->Box, GameState->ParticleEmitters[0].Box);
            if (chargeCollide && Swoosh->ShouldRender && GameState->ChargeSpawnCooldown > 1.f) {
                GameState->ChargeSpawnCooldown = 0.f;

                particle_emitter NewCharge = GameState->ParticleEmitters[0];        // copy
                NewCharge.Particles = PushArray<particle>(&GameState->WorldArena, NewCharge.ParticlesCount);
                NewCharge.Velocity.x = Bob->Flipped ? -chargeVelocity : chargeVelocity;

                GameState->ParticleEmitters[GameState->ParticleEmittersIndex++] = NewCharge;
            }
        }

        for (u32 ParticleEmitterIndex = 0; ParticleEmitterIndex < GameState->ParticleEmittersIndex; ++ParticleEmitterIndex) {
            particle_emitter* ParticleEmitter = &GameState->ParticleEmitters[ParticleEmitterIndex];

            if (ParticleEmitter->IsFading) {
                ParticleEmitter->TimeLeft -= Params->Delta;
            }

            if (ParticleEmitter->TimeLeft <= 0.f) {
                // todo: read access violation
                //particle_emitters->erase(particle_emitters->begin() + i);
                --GameState->ParticleEmittersIndex;
            }
        }

        u64 particlesSize = 0;
        // todo: use transform feedback instead?
        for (u32 ParticleEmitterIndex = 0; ParticleEmitterIndex < GameState->ParticleEmittersIndex; ++ParticleEmitterIndex) {
            particle_emitter* ParticleEmitter = &GameState->ParticleEmitters[ParticleEmitterIndex];

            if (ParticleEmitter->TimeLeft <= 0.f) {
                particlesSize += ParticleEmitter->ParticlesCount * sizeof(particle);
                continue;
            };

            for (u32 j = 0; j < ParticleEmitter->NewParticlesCount; ++j) {
                u32 unusedParticleIndex = FindFirstUnusedParticle(ParticleEmitter);
                ParticleEmitter->LastUsedParticle = unusedParticleIndex;

                particle* Particle = &ParticleEmitter->Particles[unusedParticleIndex];

                // respawing particle
                f32 randomX = randomInRange(-1.f * Scale.x, 1.f * Scale.x);
                f32 randomY = randomInRange(-1.f * Scale.y, 1.f * Scale.y);

                Particle->Lifespan = 1.f;
                Particle->Position.x = ParticleEmitter->Position.x + randomX;
                Particle->Position.y = ParticleEmitter->Position.y + randomY;
                Particle->Size = { 0.2f * TILE_SIZE.x, 0.2f * TILE_SIZE.y };
                Particle->Velocity = { 0.f, 0.f };
                Particle->Acceleration = { randomX * 10.f, 10.f };
                Particle->UV = vec2((13 * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / (f32)GameState->TextureHeight,
                    (16 * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / (f32)GameState->TextureHeight);
                Particle->Alpha = 1.f;
            }

            for (u32 j = 0; j < ParticleEmitter->ParticlesCount; ++j) {
                particle* P = &ParticleEmitter->Particles[j];
                f32 dt = ParticleEmitter->Dt;

                if (P->Lifespan > 0.f) {
                    P->Lifespan -= (f32)dt;
                    P->Velocity = P->Acceleration * dt;
                    P->Position.x += randomInRange(-1.f, 1.f);
                    P->Position.y += randomInRange(-1.f, 1.f);
                    P->Alpha -= (f32)dt * 1.f;
                    P->Size -= (f32)dt * 1.f;
                }
            }

            Renderer->glBufferSubData(GL_ARRAY_BUFFER, particlesSize, ParticleEmitter->ParticlesCount * sizeof(particle),
                ParticleEmitter->Particles);

            particlesSize += ParticleEmitter->ParticlesCount * sizeof(particle);
        }

        GameState->Lag -= GameState->UpdateRate;
    }

    GameState->ChargeSpawnCooldown += Params->Delta;

    mat4 view = mat4(1.0f);
    view = glm::translate(view, vec3(-GameState->Camera, 0.f));
    setShaderUniform(Memory, GameState->ViewUniformLocation, view);

    Renderer->glClear(GL_COLOR_BUFFER_BIT);

    //--- drawing tilemap ---
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOTiles);
    setShaderUniform(Memory, GameState->TypeUniformLocation, 1);

    mat4 model = mat4(1.0f);
    model = glm::scale(model, vec3(TILE_SIZE, 1.f));
    setShaderUniform(Memory, GameState->ModelUniformLocation, model);

    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GameState->TilesCount);

    //--- bob ---
    f32 bobXOffset = (f32)(Bob->CurrentAnimation.x * (GameState->TilesetTileSize.x + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureWidth;
    f32 bobYOffset = (f32)(Bob->CurrentAnimation.y * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureHeight;

    if (Bob->FrameTime >= Bob->CurrentAnimation.delay) {
        Bob->XAnimationOffset += (GameState->SpriteWidth + (f32)GameState->TilesetSpacing / GameState->TextureWidth) * Bob->CurrentAnimation.size;
        if (Bob->XAnimationOffset >= ((Bob->CurrentAnimation.frames * GameState->TilesetTileSize.x * Bob->CurrentAnimation.size) / (f32)GameState->TextureWidth)) {
            Bob->XAnimationOffset = 0.f;
            if (Bob->CurrentAnimation == Bob->Animations[1] || Bob->CurrentAnimation == Bob->Animations[3]) {
                Bob->CurrentAnimation = Bob->Animations[0];
            }
        }

        Bob->FrameTime = 0.0f;
    }
    Bob->FrameTime += Params->Delta;

    model = mat4(1.0f);
    model = glm::scale(model, vec3(TILE_SIZE, 1.f));
    setShaderUniform(Memory, GameState->ModelUniformLocation, model);

    f32 effectXOffset = (f32)(Swoosh->CurrentAnimation.x * (GameState->TilesetTileSize.x + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureWidth;
    f32 effectYOffset = (f32)(Swoosh->CurrentAnimation.y * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureHeight;

    if (Swoosh->FrameTime >= Swoosh->CurrentAnimation.delay) {
        Swoosh->XAnimationOffset += (GameState->SpriteWidth + (f32)GameState->TilesetSpacing / GameState->TextureWidth) * Swoosh->CurrentAnimation.size;
        if (Swoosh->XAnimationOffset >= ((Swoosh->CurrentAnimation.frames * GameState->TilesetTileSize.x * Swoosh->CurrentAnimation.size) / (f32)GameState->TextureWidth)) {
            Swoosh->XAnimationOffset = 0.f;
            Swoosh->ShouldRender = false;
        }

        Swoosh->FrameTime = 0.0f;
    }

    if (Swoosh->ShouldRender) {
        Swoosh->FrameTime += Params->Delta;
    }

    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOEntities);
    // handling animations on all entities
    for (u32 DrawableEntityIndex = 0; DrawableEntityIndex < GameState->DrawableEntitiesCount; ++DrawableEntityIndex) {
        drawable_entity* Entity = &GameState->DrawableEntities[DrawableEntityIndex];

        if (Entity->currentAnimation) {
            if (Entity->startAnimationDelayTimer < Entity->startAnimationDelay) {
                Entity->startAnimationDelayTimer += Params->Delta;
                break;
            }

            f32 entityXOffset = (f32)(Entity->currentAnimation->x * (GameState->TilesetTileSize.x + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureWidth;
            f32 entityYOffset = (f32)(Entity->currentAnimation->y * (GameState->TilesetTileSize.y + GameState->TilesetSpacing) + GameState->TilesetMargin) / GameState->TextureHeight;

            if (Entity->frameTime >= Entity->currentAnimation->delay) {
                if (Entity->currentAnimation->direction == direction::RIGHT) {
                    Entity->xAnimationOffset += (GameState->SpriteWidth + (f32)GameState->TilesetSpacing / GameState->TextureWidth) * Entity->currentAnimation->size;
                }
                else if (Entity->currentAnimation->direction == direction::LEFT) {
                    Entity->xAnimationOffset -= (GameState->SpriteWidth + (f32)GameState->TilesetSpacing / GameState->TextureWidth) * Entity->currentAnimation->size;
                }

                Entity->uv = vec2(entityXOffset + Entity->xAnimationOffset, entityYOffset);
                Renderer->glBufferSubData(GL_ARRAY_BUFFER, Entity->offset + offset(drawable_entity, uv), sizeof(vec2), &Entity->uv);

                if (abs(Entity->xAnimationOffset) >= ((Entity->currentAnimation->frames * GameState->TilesetTileSize.x * Entity->currentAnimation->size) / (f32)GameState->TextureWidth)) {
                    Entity->xAnimationOffset = 0.f;

                    Entity->startAnimationDelayTimer = 0.f;
                    Entity->startAnimationDelay = 0.f;
                    Entity->currentAnimation = nullptr;
                }

                Entity->frameTime = 0.0f;
            }
            Entity->frameTime += Params->Delta;
        }
    }

    //--- drawing entities ---
    setShaderUniform(Memory, GameState->TypeUniformLocation, 3);

    GameState->Player.uv = vec2(bobXOffset + Bob->XAnimationOffset, bobYOffset);
    GameState->Player.position = Bob->Position;
    GameState->Player.box.position = Bob->Box.position;
    GameState->Player.flipped = Bob->Flipped;

    GameState->SwooshEffect.uv = vec2(effectXOffset + Swoosh->XAnimationOffset, effectYOffset);
    GameState->SwooshEffect.position = Swoosh->Position;
    GameState->SwooshEffect.box.position = Swoosh->Box.position;
    GameState->SwooshEffect.flipped = Bob->Flipped;
    GameState->SwooshEffect.shouldRender = Swoosh->ShouldRender ? 1 : 0;

    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.offset + offset(drawable_entity, position), 2 * sizeof(f32), &GameState->Player.position);
    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.offset + offset(drawable_entity, position), 2 * sizeof(f32), &GameState->SwooshEffect.position);
    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.offset + offset(drawable_entity, uv), 2 * sizeof(f32), &GameState->Player.uv);
    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.offset + offset(drawable_entity, uv), 2 * sizeof(f32), &GameState->SwooshEffect.uv);
    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->Player.offset + offset(drawable_entity, flipped), sizeof(u32), &GameState->Player.flipped);
    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.offset + offset(drawable_entity, flipped), sizeof(u32), &GameState->SwooshEffect.flipped);
    Renderer->glBufferSubData(GL_ARRAY_BUFFER, GameState->SwooshEffect.offset + offset(drawable_entity, shouldRender), sizeof(u32), &GameState->SwooshEffect.shouldRender);

    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (s32)GameState->DrawableEntitiesCount);

    //--- drawing particles ---
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, GameState->VBOParticles);
    setShaderUniform(Memory, GameState->TypeUniformLocation, 4);

    Renderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // todo: offsets when delete in the middle?
    s32 totalParticlesCount = 0;
    for (u32 ParticleEmitterIndex = 0; ParticleEmitterIndex < GameState->ParticleEmittersIndex; ++ParticleEmitterIndex) {
        totalParticlesCount += GameState->ParticleEmitters[ParticleEmitterIndex].ParticlesCount;
    }

    // todo: draw only the ones which lifespan is greater than zero
    Renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, totalParticlesCount);

    //std::cout << delta * 1000.f << " ms" << std::endl;
}