#pragma once

struct aabb {
    vec2 position;      // top-left
    vec2 size;
};

struct entity {
    aabb box;
};

enum class entityType {
    PLAYER,
    EFFECT,
    REFLECTOR,
    UNKNOWN
};

//todo: store in VBO only the ones that are actually used in shaders
struct drawableEntity {
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
};

