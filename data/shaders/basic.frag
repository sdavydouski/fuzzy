#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec2 uv;
in vec2 uvOffset;
in float alpha;

out vec4 color;

uniform int type;

uniform sampler2D spriteTexture;

#define TILE_TYPE 1.f
#define SPRITE_TYPE 2.f
#define ENTITY_TYPE 3.f
#define PARTICLE_TYPE 4.f

void main() {
    color = vec4(1.f);
    //if (type == TILE_TYPE) {
    //    // todo: do i need this condition?
    //    if (uvOffset.x >= 0.f && uvOffset.y >= 0.f) {
    //        color = texture(spriteTexture, uv + uvOffset);
    //    } else {
    //        color = vec4(0.f);
    //    }
    //} else if (type == SPRITE_TYPE || type == ENTITY_TYPE) {
    //    color = texture(spriteTexture, uv + uvOffset);
    //    //color = vec4(1.f);
    //} else if (type == PARTICLE_TYPE) {
    //    color = texture(spriteTexture, uv + uvOffset);
    //    color.a = alpha;
    //}
}
