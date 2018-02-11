#version 330 core
in vec2 uv;
in vec2 uvOffset;
out vec4 color;

uniform int type;

uniform sampler2D spriteTexture;
uniform vec2 spriteSize;
uniform vec2 spriteOffset;
uniform bool reversed;

#define TILE_TYPE 1.f
#define SPRITE_TYPE 2.f

void main() {
    if (type == TILE_TYPE) {
        if (uvOffset.x != -1 && uvOffset.y != -1) {
            color = texture(spriteTexture, uv + uvOffset);
        } else {
            color = vec4(0.f);
        }
    } else if (type == SPRITE_TYPE) {
        float x = reversed ? 
            spriteSize.x - uv.x + spriteOffset.x :
            uv.x + spriteOffset.x;

        color = texture(spriteTexture, vec2(x, uv.y + spriteOffset.y));   
    }
}
