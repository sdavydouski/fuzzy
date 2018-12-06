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

enum class entity_type {
    PLAYER,
    EFFECT,
    REFLECTOR,
    LAMP,
    PLATFORM,
    UNKNOWN
};

//todo: store in VBO only the ones that are actually used in shaders
//todo: rework concept of drawable entities (allow creation and removal)
struct drawable_entity {
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
    entity_type type;

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

struct tile_layer {
    u32 TilesCount;
    tile* Tiles;
};

struct object_layer {
    u32 EntitiesCount;
    entity* Entities;

    u32 DrawableEntitiesCount;
    drawable_entity* DrawableEntities;
};

struct tiled_map {
    u32 Width;
    u32 Height;

    u32 TileLayersCount;
    tile_layer* TileLayers;

    u32 ObjectLayersCount;
    object_layer* ObjectLayers;
};

struct tile_spec {
    entity_type type;
    aabb box;
};

struct tileset {
    u32 columns;
    u32 margin;
    u32 spacing;
    vec2 tileSize;
    vec2 imageSize;
    map<u32, tile_spec> tiles;
};

struct sprite {
    u32 AnimationsCount;
    animation* Animations;

    animation CurrentAnimation;

    f32 XAnimationOffset;
    f32 FrameTime;
    u32 Flipped;

    vec2 Position;
    aabb Box;
    vec2 Velocity;
    vec2 Acceleration;
};

struct effect {
    u32 AnimationsCount;
    animation* Animations;

    animation CurrentAnimation;
    f32 XAnimationOffset;
    f32 FrameTime;
    u32 Flipped;

    vec2 Position;
    aabb Box;

    b32 ShouldRender;
};

struct particle {
    vec2 Position;
    vec2 Size;
    vec2 Velocity;
    vec2 Acceleration;
    vec2 UV;
    f32 Lifespan;
    f32 Alpha;
};

struct particle_emitter {
    u32 LastUsedParticle;
    u32 NewParticlesCount;
    f32 Dt;

    u32 ParticlesCount;
    particle* Particles;
    
    vec2 Position;
    aabb Box;
    vec2 Velocity;

    b32 StopProcessingCollision;
    s32 ReflectorIndex;
    b32 IsFading;
    f32 TimeLeft;
};

struct game_state {
    memory_arena WorldArena;

    sprite Bob;
    effect Swoosh;
    
    u32 EntitiesCount;
    entity* Entities;
    
    u32 DrawableEntitiesCount;
    drawable_entity* DrawableEntities;

    u32 ParticleEmittersIndex;
    u32 ParticleEmittersMaxCount;
    particle_emitter* ParticleEmitters;

    u32 TilesCount;
    tile* Tiles;

    // top-left corner
    vec2 Camera;

    drawable_entity Player;
    drawable_entity SwooshEffect;

    s32 TextureWidth;
    s32 TextureHeight;

    f32 SpriteWidth;
    f32 SpriteHeight;

    tiled_map Level;
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
