#pragma once

// todo: move it somewhere else
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

enum entity_type
{
    ENTITY_UNKNOWN,
    ENTITY_PLAYER,
    ENTITY_SIREN
};

struct tile_animation_frame
{
    u32 Duration;
    u32 TileId;
};

struct tile_custom_property
{
    char *Name;
    char *Type;
    void *Value;
};

struct tile_meta_info
{
    u32 Id;

    char *Type;

    u32 BoxCount;
    aabb *Boxes;

    u32 AnimationFrameCount;
    tile_animation_frame *AnimationFrames;

    u32 CustomPropertiesCount;
    tile_custom_property *CustomProperties;

    tile_meta_info *Next;
};

struct tileset
{
    u32 Columns;
    u32 Margin;
    u32 Spacing;

    s32 TileWidthInPixels;
    s32 TileHeightInPixels;

    f32 TileWidthInWorldUnits;
    f32 TileHeightInWorldUnits;

    f32 TilesetWidthPixelsToWorldUnits;
    f32 TilesetHeightPixelsToWorldUnits;

    bitmap Image;

    hash_table<tile_meta_info> Tiles;
};

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

    b32 Visible;
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

    b32 Visible;
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