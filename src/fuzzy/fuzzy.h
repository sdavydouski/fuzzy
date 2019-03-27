#pragma once

#include "fuzzy_types.h"
#include "fuzzy_memory.h"

#define ArrayLength(Array) (sizeof(Array) / sizeof((Array)[0]))

#pragma warning(disable:4302)
#pragma warning(disable:4311)

#define Offset(StructType, StructMember) ((u32)(&(((StructType *)0)->StructMember)))

// from https://stackoverflow.com/questions/664014
u32 Hash(u32 Value)
{
    u32 Hash = Value;

    Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
    Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
    Hash = (Hash >> 16) ^ Hash;

    return Hash;
}

// from https://stackoverflow.com/questions/7666509/
u32 Hash(char *Value)
{
    u32 Hash = 5381;
    s32 C;

    while (C = *Value++) {
        Hash = ((Hash << 5) + Hash) + C;
    }

    return Hash;
}

struct bitmap
{
    s32 Width;
    s32 Height;
    s32 Channels;
    void *Memory;
};

struct aabb
{
    // top-left
    vec2 Position;
    vec2 Size;
};

enum class direction
{
    TOP, LEFT, BOTTOM, RIGHT
};

struct animation
{
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

enum class entity_type
{
    UNKNOWN,
    PLAYER,
    REFLECTOR,
    LAMP,
    PLATFORM
};

struct animation_frame
{
    u32 Duration;
    u32 TileId;
};

struct tile_meta_info
{
    u32 Id;

    char *Type;

    u32 BoxCount;
    aabb *Boxes;

    u32 AnimationFrameCount;
    animation_frame *AnimationFrames;

    tile_meta_info *Next;
};

struct aabb_info
{
    aabb *Box;
    mat4 *Model;
};

struct entity
{
    u32 ID;

    vec2 Position;
    vec2 Velocity;
    vec2 Acceleration;

    vec2 Size;
    entity_type Type;

    u32 InstanceModelOffset;
    u32 BoxModelOffset;
    // todo: ???
    mat4 *InstanceModel;

    u32 BoxCount;
    aabb_info *Boxes;
   
    tile_meta_info *TileInfo;
};

//todo: store in VBO only the ones that are actually used in shaders
//todo: rework concept of drawable entities (allow creation and removal)
struct drawable_entity
{
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

    b32 IsColliding;

    u32 Offset;

    f32 StartAnimationDelay;
    f32 StartAnimationDelayTimer;
    animation *CurrentAnimation;
    f32 XAnimationOffset;
    f32 FrameTime;
};

struct tileset
{
    u32 Columns;
    u32 Margin;
    u32 Spacing;
    
    s32 TileWidthInPixels;
    s32 TileHeightInPixels;

    f32 TileWidthInMeters;
    f32 TileHeightInMeters;

    f32 TilesetWidthPixelsToMeters;
    f32 TilesetHeightPixelsToMeters;

    bitmap Image;

    u32 TileCount;
    tile_meta_info *Tiles;
};

inline tile_meta_info *
GetTileMetaInfo(tileset *Tileset, u32 Id) {
    tile_meta_info *Result = nullptr;

    u32 HashValue = Hash(Id) % Tileset->TileCount;
    assert(HashValue < Tileset->TileCount);

    tile_meta_info *Tile = Tileset->Tiles + HashValue;

    do {
        if (Tile->Id == Id) {
            Result = Tile;
            break;
        }

        Tile = Tile->Next;
    } while (Tile);

    return Result;
}

const u32 UNINITIALIZED_TILE_ID = 0;

inline tile_meta_info*
CreateTileMetaInfo(tileset *Tileset, u32 Id, memory_arena *Arena) {
    tile_meta_info *Result = nullptr;

    u32 HashValue = Hash(Id) % Tileset->TileCount;
    assert(HashValue < Tileset->TileCount);

    tile_meta_info *Tile = Tileset->Tiles + HashValue;

    do {
        if (Tile->Id == UNINITIALIZED_TILE_ID) {
            Result = Tile;
            Result->Id = Id;
            break;
        }

        if (Tile->Id != UNINITIALIZED_TILE_ID && !Tile->Next) {
            // todo: potentially dangerous operation
            Tile->Next = PushStruct<tile_meta_info>(Arena);
            Tile->Next->Id = UNINITIALIZED_TILE_ID;
        }

        Tile = Tile->Next;
    } while (Tile);

    return Result;
}

struct map_chunk
{
    s32 X;
    s32 Y;
    u32 Width;
    u32 Height;

    u32 GIDCount;
    u32 *GIDs;
};

struct tile_layer
{
    s32 StartX;
    s32 StartY;

    u32 Width;
    u32 Height;

    u32 ChunkCount;
    map_chunk *Chunks;
};

struct map_object
{
    f32 X;
    f32 Y;
    f32 Width;
    f32 Height;

    f32 Rotation;
    entity_type Type;

    u32 ID;
    u32 GID;
};

struct object_layer
{
    u32 ObjectCount;
    map_object *Objects;
};

struct tileset_source
{
    u32 FirstGID;
    tileset Source;
};

struct tilemap
{
    u32 TileLayerCount;
    tile_layer *TileLayers;

    u32 ObjectLayerCount;
    object_layer *ObjectLayers;

    u32 TilesetCount;
    tileset_source *Tilesets;
};

struct sprite
{
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

    b32 ShouldRender;
};

struct particle
{
    vec2 Position;
    vec2 Size;
    vec2 Velocity;
    vec2 Acceleration;
    vec2 UV;
    f32 Lifespan;
    f32 Alpha;
};

struct particle_emitter
{
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

struct vertex_buffer_attribute
{
    u32 Index;
    s32 Size;
    s32 Type;
    b32 Normalized;
    u32 Stride;
    u32 Divisor;
    void *OffsetPointer;
};

struct vertex_buffer_attributes_layout
{
    u32 AttributeCount;
    vertex_buffer_attribute *Attributes;
};

struct vertex_sub_buffer
{
    u32 Offset;
    u32 Size;
    void *Data;
};

struct vertex_buffer_data_layout
{
    u32 SubBufferCount;
    vertex_sub_buffer *SubBuffers;
};

struct vertex_buffer
{
    u32 VAO;
    u32 VBO;
    u32 Size;
    u32 Usage;

    vertex_buffer_data_layout *DataLayout;
    vertex_buffer_attributes_layout *AttributesLayout;
};

struct game_state
{
    b32 IsInitialized;

    memory_arena WorldArena;

    //sprite Bob;
    //sprite Swoosh;
    //sprite Lamp;
    //sprite Platform;
    //sprite Enemy;

    //u32 EntitiesCount;
    //entity *Entities;
    //
    //u32 DrawableEntitiesCount;
    //drawable_entity *DrawableEntities;

    //u32 ParticleEmittersIndex;
    //u32 ParticleEmittersMaxCount;
    //particle_emitter *ParticleEmitters;

    //u32 TilesCount;
    //tile *Tiles;

    // top-left corner <-- is it?
    vec2 CameraPosition;
    f32 Zoom;

    tilemap Map;

    f32 Lag;
    f32 UpdateRate;

    u32 TotalBoxCount;
    u32 TotalTileCount;
    u32 TotalObjectCount;
    u32 TotalDrawableObjectCount;

    vertex_buffer BoxesVertexBuffer;
    vertex_buffer TilesVertexBuffer;
    vertex_buffer DrawableEntitiesVertexBuffer;

    u32 TilesShaderProgram;
    u32 BoxesShaderProgram;
    u32 DrawableEntitiesShaderProgram;

    mat4 Projection;
    mat4 VP;

    f32 ScreenWidthInMeters;
    f32 ScreenHeightInMeters;

    u32 UBO;

    entity *Player;
    aabb *Boxes;
};
