#pragma once

#include "fuzzy_types.h"
#include "fuzzy_memory.h"
#include "fuzzy_tiled.h"
#include "fuzzy_graphics.h"
#include "fuzzy_animations.h"
#include "fuzzy_containers.h"

#define ArrayLength(Array) (sizeof(Array) / sizeof((Array)[0]))

#pragma warning(disable:4302)
#pragma warning(disable:4311)

#define StructOffset(StructType, StructMember) ((u64)(&(((StructType *)0)->StructMember)))

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

enum entity_state
{
    ENTITY_STATE_IDLE,
    ENTITY_STATE_RUN,
    ENTITY_STATE_JUMP,
    ENTITY_STATE_FALL,
    ENTITY_STATE_SQUASH,
    ENTITY_STATE_DUCK,
    ENTITY_STATE_ATTACK
};

struct entity_state_stack
{

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
    entity_state PrevState;
    entity_state State;

    entity_render_info *RenderInfo;

    u32 BoxCount;
    aabb_info *Boxes;

    animation *CurrentAnimation;
};

struct game_state
{
    b32 IsInitialized;

    memory_arena WorldArena;

    // bottom-left corner <-- is it?
    vec2 Camera;
    f32 Zoom;

    tilemap Map;

    f32 Lag;
    f32 UpdateRate;

    u32 TotalBoxCount;
    u32 TotalTileCount;
    u32 TotalObjectCount;

    u32 TotalDrawableObjectCount;
    entity *DrawableEntities;

    vertex_buffer BoxesVertexBuffer;
    vertex_buffer TilesVertexBuffer;
    vertex_buffer DrawableEntitiesVertexBuffer;
    vertex_buffer BorderVertexBuffer;

    shader_program TilesShaderProgram;
    shader_program BoxesShaderProgram;
    shader_program DrawableEntitiesShaderProgram;
    shader_program DrawableEntitiesBorderShaderProgram;
    shader_program BorderShaderProgram;

    mat4 Projection;
    mat4 VP;

    f32 ScreenWidthInMeters;
    f32 ScreenHeightInMeters;

    u32 UBO;

    entity *Player;
    aabb *Boxes;

    hash_table<animation> Animations;

    // todo: merge with TotalDrawableObjectCount?
    u32 EntityRenderInfoCount;
    // todo: maybe store in in entity directly?
    entity_render_info *EntityRenderInfos;
};
