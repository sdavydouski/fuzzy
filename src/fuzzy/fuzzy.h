#pragma once

#include "fuzzy_memory.h"

#define length(arr) (sizeof(arr) / sizeof((arr)[0]))

#pragma warning(disable:4302)
#pragma warning(disable:4311)

#define offset(structType, structMember) ((u32)(&(((structType* )0)->structMember)))

struct aabb {
    vec2 position;      // top-left
    vec2 size;
};

enum class direction {
    TOP, LEFT, BOTTOM, RIGHT
};

struct animation {
    s32 x;
    s32 y;
    s32 frames;
    f32 delay;
    s32 size;
    direction direction = direction::RIGHT;

    b32 operator==(const animation& other) const {
        return x == other.x && y == other.y;
    }

    b32 operator!=(const animation& other) const {
        return !(*this == other);
    }
};

struct entity {
    // todo: do not use size from aabb struct; use separate vec2 size; (maybe even store it as scale number)
    vec2 position;
    aabb box;
    u32 id;
};

enum class entityType {
    PLAYER,
    EFFECT,
    REFLECTOR,
    LAMP,
    PLATFORM,
    UNKNOWN
};

//todo: store in VBO only the ones that are actually used in shaders
//todo: rework concept of drawable entities (allow creation and removal)
struct drawableEntity {
    u32 id;
    vec2 position;
    aabb box;

    vec2 uv;
    f32 rotation;

    u32 flipped;

    vec2 spriteScale;
    // todo: manage it somehow
    u32 shouldRender;
    b32 collides;
    b32 underEffect;
    b32 isRotating;
    entityType type;

    b32 isColliding;

    u32 offset;

    f32 startAnimationDelay;
    f32 startAnimationDelayTimer;
    animation* currentAnimation;
    f32 xAnimationOffset;
    f32 frameTime;
};

struct tile {
    vec2 position;
    vec2 uv;
    u32 flipped;
};

struct tileLayer {
    vector<tile> tiles;
};

struct objectLayer {
    map<u32, entity> entities;
    map<u32, drawableEntity> drawableEntities;
};

struct tiledMap {
    u32 width;
    u32 height;

    vector<tileLayer> tileLayers;
    vector<objectLayer> objectLayers;
};

struct tileSpec {
    entityType type;
    aabb box;
};

struct tileset {
    u32 columns;
    u32 margin;
    u32 spacing;
    vec2 tileSize;
    vec2 imageSize;
    map<u32, tileSpec> tiles;
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

struct game_state {
    memory_arena WorldArena;

    sprite Bob;
    effect Swoosh;
    vector<entity> Entities;
    vector<drawableEntity> DrawableEntities;
    vector<particleEmitter> ParticleEmitters;
    vector<tile> Tiles;

    // top-left corner
    vec2 Camera;

    drawableEntity Player;
    drawableEntity SwooshEffect;

    s32 TextureWidth;
    s32 TextureHeight;

    f32 SpriteWidth;
    f32 SpriteHeight;

    tiledMap Level;
    // todo: replace with tileset
    u32 TilesetColumns;
    u32 TilesetMargin;
    u32 TilesetSpacing;
    vec2 TilesetTileSize;
    vec2 TilesetImageSize;
    //tileset Tileset;

    animation TurnOnAnimation;
    animation PlatformOnAnimation;

    f32 Lag;
    f32 UpdateRate;
    f32 ChargeSpawnCooldown;

    u32 VBOTiles;
    u32 VBOEntities;
    u32 VBOParticles;

    s32 ModelUniformLocation;
    s32 ViewUniformLocation;
    s32 TypeUniformLocation;
};
