#pragma once

#include "fuzzy_types.h"
#include "fuzzy_memory.h"
#include "fuzzy_tiled.h"
#include "fuzzy_renderer.h"
#include "fuzzy_animations.h"
#include "fuzzy_containers.h"
#include "assets.h"

enum event_type
{
    EVENT_TYPE_NONE,
    EVENT_TYPE_PLAYER_DIVE_HIT,

    EVENT_TYPE_COUNT
};

struct event_data
{

};

struct event
{
    event_type Type;
    event_data *Data;
};

struct aabb_info
{
    aabb *Box;
    mat4 *Model;
};

struct entity_render_info
{
    u64 Offset;

    mat4 InstanceModel;
    vec2 InstanceUVOffset01;
    u32 Flipped;

    // todo:
    u32 BoxModelOffset;
};

struct particle_render_info
{
    u64 Offset;

    mat4 Model;
    //vec2 uvOffset01;
    vec4 Color;
};

enum entity_state
{
    ENTITY_STATE_IDLE,
    ENTITY_STATE_RUN,
    ENTITY_STATE_JUMP,
    ENTITY_STATE_DIVE,
    ENTITY_STATE_FALL,
    ENTITY_STATE_SQUASH,
    ENTITY_STATE_DUCK,
    ENTITY_STATE_ATTACK
};

struct entity
{
    u32 ID;

    vec2 Position;
    vec2 Velocity;
    vec2 Acceleration;

    vec2 Size;
    entity_type Type;
    // todo: pushdown automata?
    stack<entity_state> StatesStack; 

    entity_render_info *RenderInfo;

    u32 BoxCount;
    aabb_info *Boxes;

    animation *CurrentAnimation;
};

struct particle
{
    vec2 Position;
    vec2 Velocity;
    vec2 Acceleration;
    vec4 Color;
    vec4 dColor;
    vec2 Size;
    vec2 dSize;

    particle_render_info *RenderInfo;
};

struct game_state
{
    b32 IsInitialized;

    memory_arena WorldArena;

    // bottom-left corner <-- is it?
    vec2 Camera;
    f32 Zoom;

    tilemap Map;

    f64 Time;
    f32 Lag;
    f32 UpdateRate;

    u32 TotalBoxCount;
    u32 TotalTileCount;
    u32 TotalObjectCount;

    u32 TotalDrawableObjectCount;
    entity *DrawableEntities;

    vertex_buffer TilesVertexBuffer;
    vertex_buffer BoxesVertexBuffer;
    vertex_buffer DrawableEntitiesVertexBuffer;
    vertex_buffer ParticlesVertexBuffer;
    vertex_buffer QuadVertexBuffer;
    vertex_buffer PlayerDiveParticlesVertexBuffer;

    shader_program TilesShaderProgram;
    shader_program BoxesShaderProgram;
    shader_program DrawableEntitiesShaderProgram;
    shader_program DrawableEntitiesBorderShaderProgram;
    shader_program RectangleOutlineShaderProgram;
    shader_program RectangleShaderProgram;
    shader_program ParticlesShaderProgram;
    shader_program SpriteShaderProgram;
    shader_program TextShaderProgram;

    mat4 Projection;
    mat4 VP;

    f32 ScreenWidthInWorldUnits;
    f32 ScreenHeightInWorldUnits;

    u32 UBO;

    entity *Player;
    aabb *Boxes;

    hash_table<animation> Animations;

    // todo: merge with TotalDrawableObjectCount?
    u32 EntityRenderInfoCount;
    // todo: maybe store in in entity directly?
    entity_render_info *EntityRenderInfos;
    particle_render_info *ParticleRenderInfos;
    particle_render_info *PlayerDiveParticleRenderInfos;

    u32 NextParticle;
    particle Particles[1024];

    u32 NextPlayerDiveParticle;
    particle PlayerDiveParticles[256];

    random_sequence Entropy;

    u32 QuadVerticesSize;

    u32 TilesetTexture;
    u32 FontTextureAtlas;

    u32 FontAssetCount;
    font_asset *FontAssets;

    font_asset *CurrentFont;

    f32 PixelsToWorldUnits;
    f32 WorldUnitsToPixels;

    queue<event> EventQueue;

    vec3 BackgroundColor;
};
