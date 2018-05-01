#version 330 core
// <vec2 - position, vec2 - uv>
layout(location = 0) in vec4 sprite;
// <vec2 - offset, float - rotation in background tile, float - rotation in foreground tile>
// todo: think about better ways
layout(location = 1) in vec4 xyr;
layout(location = 2) in vec2 backgroundUv;
layout(location = 3) in vec2 foregroundUv;

// todo: combine this differently
layout(location = 4) in vec4 aabb;
layout(location = 5) in vec3 uvr;
layout(location = 6) in uint flipped;
layout(location = 7) in vec2 spriteScale;
layout(location = 8) in uint shouldRender;

layout(location = 9) in vec4 particleBox;
layout(location = 10) in vec2 particleUv;
layout(location = 11) in float particleAlpha;

out vec2 uv;
out vec2 uvOffset;
out float alpha;

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
#define PARTICLE_TYPE 4.f

uint FLIPPED_HORIZONTALLY_FLAG = 0x80000000u;
uint FLIPPED_VERTICALLY_FLAG = 0x40000000u;
uint FLIPPED_DIAGONALLY_FLAG = 0x20000000u;

vec2 swapXY(vec2 vec) {
    float temp = vec.x;
    vec.x = vec.y;
    vec.y = temp;
    
    return vec;
}

#define PI 3.14159265

float toRadians(float angleInDegrees) {
    return angleInDegrees * PI / 180.f;    
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
        alpha = 1.f;

        gl_Position = projection * view * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(xyr.xy, 0.f, 0.f));
    } else if (type == SPRITE_TYPE || type == ENTITY_TYPE) {
        if (shouldRender != 1u) return;
        
        uvOffset = uvr.xy;
        alpha = 1.f;
        
        bool flippedHorizontally = bool(flipped & FLIPPED_HORIZONTALLY_FLAG);
        bool flippedVertically = bool(flipped & FLIPPED_VERTICALLY_FLAG);
        bool flippedDiagonally = bool(flipped & FLIPPED_DIAGONALLY_FLAG);
        
        if (flippedHorizontally) {
            uv.x = spriteSize.x - uv.x;
        }
        if (flippedVertically) {
            uv.y = spriteSize.y - uv.y;
        }
        if (flippedDiagonally) {
            uv = swapXY(uv);
        }

        uv *= spriteScale;

        vec2 position = aabb.xy;
        vec2 size = aabb.zw;

        float rotate = uvr.z;

        mat4 translationMatrix = mat4(
            1.f, 0.f, 0.f, 0.f,                 // first column (not row!)
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            position.x, position.y, 0.f, 1.f
        );
        
        mat4 preRotationTranslationMatrix = mat4(
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            64.f * spriteScale.x / 2.f, 64.f * spriteScale.y / 2.f, 0.f, 1.f
        );

        mat4 postRotationTranslationMatrix = mat4(
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            -64.f * spriteScale.x / 2.f, -64.f * spriteScale.y / 2.f, 0.f, 1.f
        );
        
        mat4 rotationMatrix = preRotationTranslationMatrix * mat4(
            cos(toRadians(rotate)), sin(toRadians(rotate)), 0.f, 0.f,
            -sin(toRadians(rotate)), cos(toRadians(rotate)), 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
        ) * postRotationTranslationMatrix;

        mat4 scalingMatrix = mat4(
           1.f * 64.f * spriteScale.x, 0.f, 0.f, 0.f,     
           0.f, 1.f * 64.f * spriteScale.y, 0.f, 0.f,
           0.f, 0.f, 1.f * 64.f, 0.f,
           0.f, 0.f, 0.f, 1.f
        );

        mat4 model = translationMatrix * rotationMatrix * scalingMatrix;

        gl_Position = projection * view * model * vec4(sprite.xy, 0.f, 1.0f);
    } else if (type == PARTICLE_TYPE) {
        uvOffset = particleUv;
        alpha = particleAlpha;

        vec2 particlePosition = particleBox.xy;
        vec2 particleSize = particleBox.zw;  // scale

        mat4 scalingMatrix = mat4(
           1.f * 16.f * particleSize.x, 0.f, 0.f, 0.f,     
           0.f, 1.f * 16.f * particleSize.y, 0.f, 0.f,
           0.f, 0.f, 1.f * 16.f, 0.f,
           0.f, 0.f, 0.f, 1.f
        );
        
        mat4 model = scalingMatrix;

        gl_Position = projection * view * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(particlePosition, 0.f, 0.f));
    }
}
