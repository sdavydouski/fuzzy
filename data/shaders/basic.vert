#version 330 core
// <vec2 - position, vec2 - uv>
layout(location = 0) in vec4 sprite;
// <vec2 - offset, float - rotation in background tile, float - rotation in foreground tile>
// todo: think about better ways
layout(location = 1) in vec4 xyr;
layout(location = 2) in vec2 backgroundUv;
layout(location = 3) in vec2 foregroundUv;

out vec2 uv;
out vec2 uvOffset;

uniform int type;
// background or foreground tile
uniform int tileType;

uniform vec2 spriteSize;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define ROTATE_90 1.f
#define ROTATE_180 2.f
#define ROTATE_270 3.f

#define TILE_TYPE 1.f
#define SPRITE_TYPE 2.f

vec2 swapXY(vec2 vec) {
    float temp = vec.x;
    vec.x = vec.y;
    vec.y = temp;
    
    return vec;
}

void main() {
    uv = spriteSize * sprite.zw;
    
    float rotate = tileType == 0 ? xyr.z : xyr.w;    

    // handle rotating
    if (rotate == ROTATE_90) {
        uv.y = spriteSize.y - uv.y;
        uv = swapXY(uv);
    } else if (rotate == ROTATE_180) {
        uv.x = spriteSize.x - uv.x;
        uv.y = spriteSize.y - uv.y;
    } else if (rotate == ROTATE_270) {
        uv.x = spriteSize.x - uv.x;
        uv = swapXY(uv);
    }
    
    uvOffset = tileType == 0 ? backgroundUv : foregroundUv;

    if (type == TILE_TYPE) {
        gl_Position = projection * view * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(xyr.xy, 0.f, 0.f));
    } else if (type == SPRITE_TYPE) {
        gl_Position = projection * view * model * vec4(sprite.xy, 0.f, 1.0f);
    }
}
