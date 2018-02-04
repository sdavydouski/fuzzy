#version 330 core
// <vec2 - position, vec2 - uv>
layout(location = 0) in vec4 sprite;
layout(location = 1) in vec4 backgroundTile;
layout(location = 2) in vec4 foregroundTile;

out vec2 uv;
out vec2 uvOffset;

uniform int type;
uniform int tileType;

uniform mat4 model;
uniform mat4 projection;

void main() {
    uv = sprite.zw;
    
    vec4 tile = tileType == 0 ? backgroundTile : foregroundTile;

    uvOffset = tile.zw;
    
    if (type == 1) {
        // tile
        gl_Position = projection * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(tile.xy, 0.f, 0.f));
    } else if (type == 2) {
        // sprite
        gl_Position = projection * model * vec4(sprite.xy, 0.f, 1.0f);
    }
}
