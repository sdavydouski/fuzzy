#pragma once

#include "fuzzy_memory.h"

#define ArrayLength(Array) (sizeof(Array) / sizeof((Array)[0]))

#pragma warning(disable:4302)
#pragma warning(disable:4311)

#define Offset(StructType, StructMember) ((u32)(&(((StructType* )0)->StructMember)))

struct aabb {
    vec2 Position;      // top-left
    vec2 Size;
};

enum class direction {
    TOP, LEFT, BOTTOM, RIGHT
};

struct animation {
    s32 X;
    s32 Y;
    s32 Frames;
    f32 Delay;
    s32 Size;
    direction Direction = direction::RIGHT;

    b32 operator==(const animation& Other) const {
        return X == Other.X && Y == Other.Y;
    }

    b32 operator!=(const animation& Other) const {
        return !(*this == Other);
    }
};

struct entity {
    // todo: do not use size from aabb struct; use separate vec2 size; (maybe even store it as scale number)
    vec2 Position;
    aabb Box;
    u32 Id;
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
    u32 Id;
    vec2 Position;
    aabb Box;

    vec2 UV;
    f32 Rotation;

    u32 Flipped;

    vec2 SpriteScale;
    // todo: manage it somehow
    u32 ShouldRender;
    b32 Collides;
    b32 UnderEffect;
    b32 IsRotating;
    entity_type Type;

    b32 IsColliding;

    u32 Offset;

    f32 StartAnimationDelay;
    f32 StartAnimationDelayTimer;
    animation *CurrentAnimation;
    f32 XAnimationOffset;
    f32 FrameTime;
};

struct tile {
    vec2 Position;
    vec2 UV;
    u32 Flipped;
};

struct tile_layer {
    u32 TilesCount;
    tile *Tiles;
};

struct object_layer {
    u32 EntitiesCount;
    entity *Entities;

    u32 DrawableEntitiesCount;
    drawable_entity *DrawableEntities;
};

struct tiled_map {
    u32 Width;
    u32 Height;

    u32 TileLayersCount;
    tile_layer *TileLayers;

    u32 ObjectLayersCount;
    object_layer *ObjectLayers;
};

struct tile_spec {
    u32 Gid;
    //entity_type Type;
    aabb Box;

    tile_spec* Next;
};

struct tileset {
    u32 Columns;
    u32 Margin;
    u32 Spacing;
    vec2 TileSize;
    vec2 ImageSize;

    u32 TileSpecsCount;
    tile_spec* TilesHashTable;
};

// from https://stackoverflow.com/questions/664014
u32 Hash(u32 Value) {
    u32 Hash = Value;

    Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
    Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
    Hash = (Hash >> 16) ^ Hash;

    return Hash;
}

tile_spec *GetOrCreateTileSpec(tileset* Tileset, u32 Gid, memory_arena* Arena) {
    tile_spec *Result = 0;

    u32 HashValue = Hash(Gid) % Tileset->TileSpecsCount;
    assert(HashValue < Tileset->TileSpecsCount);

    tile_spec *TileSpec = Tileset->TilesHashTable + HashValue;

    do {
        // get existing
        if (TileSpec->Gid == Gid) {
            Result = TileSpec;
            break;
        }

        if (Arena) {
            if (TileSpec->Gid != 0 && !TileSpec->Next) {
                TileSpec->Next = PushStruct<tile_spec>(Arena);
                TileSpec->Next->Gid = 0;
                TileSpec = TileSpec->Next;
            }

            // init new
            if (TileSpec->Gid == 0) {
                Result = TileSpec;
                Result->Gid = Gid;
                break;
            }
        }
        
        TileSpec = TileSpec->Next;
    } while (TileSpec);

    return Result;
}

struct sprite {
    u32 AnimationsCount;
    animation *Animations;

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
    animation *Animations;

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
    particle *Particles;
    
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
    entity *Entities;
    
    u32 DrawableEntitiesCount;
    drawable_entity *DrawableEntities;

    u32 ParticleEmittersIndex;
    u32 ParticleEmittersMaxCount;
    particle_emitter *ParticleEmitters;

    u32 TilesCount;
    tile *Tiles;

    // top-left corner
    vec2 Camera;

    drawable_entity Player;
    drawable_entity SwooshEffect;

    s32 TextureWidth;
    s32 TextureHeight;

    f32 SpriteWidth;
    f32 SpriteHeight;

    tiled_map Level;
    tileset Tileset;

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
