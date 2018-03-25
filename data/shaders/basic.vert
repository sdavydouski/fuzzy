#version 330 core
// <vec2 - position, vec2 - uv>
layout(location = 0) in vec4 sprite;
// <vec2 - offset, float - rotation in background tile, float - rotation in foreground tile>
// todo: think about better ways
layout(location = 1) in vec4 xyr;
layout(location = 2) in vec2 backgroundUv;
layout(location = 3) in vec2 foregroundUv;

layout(location = 4) in vec4 aabb;
layout(location = 5) in vec3 uvr;

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
#define ENTITY_TYPE 3.f

vec2 swapXY(vec2 vec) {
    float temp = vec.x;
    vec.x = vec.y;
    vec.y = temp;
    
    return vec;
}

void main() {
    uv = spriteSize * sprite.zw;
    
    if (type == TILE_TYPE) {
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

        gl_Position = projection * view * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(xyr.xy, 0.f, 0.f));
    } else if (type == SPRITE_TYPE || type == ENTITY_TYPE) {
        uvOffset = uvr.xy;
        float rotate = uvr.z;
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

        vec2 position = aabb.xy;
        vec2 size = aabb.zw;
        // scale and translation
        mat4 model = mat4(
           1.f * 64.f, 0.f, 0.f, 0.f,     // first column (not row!)
           0.f, 1.f * 64.f, 0.f, 0.f,
           0.f, 0.f, 1.f * 64.f, 0.f,
           position.x, position.y, 0.f, 1.f
        );
        gl_Position = projection * view * model * vec4(sprite.xy, 0.f, 1.0f);
    }
}
