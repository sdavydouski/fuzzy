#pragma once

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
    direction direction;

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
};

enum class entityType {
    PLAYER,
    EFFECT,
    REFLECTOR,
    LAMP,
    UNKNOWN
};

//todo: store in VBO only the ones that are actually used in shaders
//todo: rework concept of drawable entities (allow creation and removal)
struct drawableEntity {
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

    animation* currentAnimation;
    f32 xAnimationOffset;
    f32 frameTime;
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

