#version 330 core
in vec2 uv;
in vec2 uvOffset;
out vec4 color;

uniform int type;

uniform sampler2D spriteTexture;

#define TILE_TYPE 1.f
#define SPRITE_TYPE 2.f
#define ENTITY_TYPE 3.f

void main() {
    if (type == TILE_TYPE) {
        if (uvOffset.x >= 0.f && uvOffset.y >= 0.f) {
            color = texture(spriteTexture, uv + uvOffset);
        } else {
            color = vec4(0.f);
        }
    } else if (type == SPRITE_TYPE || type == ENTITY_TYPE) {
        color = texture(spriteTexture, uv + uvOffset);   
    }
}
