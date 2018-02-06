#version 330 core
// <vec2 - position, vec2 - uv>
layout(location = 0) in vec4 sprite;
layout(location = 1) in vec2 xy;
layout(location = 2) in vec2 backgroundUv;
layout(location = 3) in vec2 foregroundUv;

out vec2 uv;
out vec2 uvOffset;

uniform int type;
uniform int tileType;

uniform mat4 model;
uniform mat4 projection;

void main() {
    uv = sprite.zw;
    
    uvOffset = tileType == 0 ? backgroundUv : foregroundUv;

    if (type == 1) {
        // tile
        gl_Position = projection * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(xy, 0.f, 0.f));
    } else if (type == 2) {
        // sprite
        gl_Position = projection * model * vec4(sprite.xy, 0.f, 1.0f);
    }
}
