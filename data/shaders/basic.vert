#version 330 core
// <vec2 - position, vec2 - uv>
layout(location = 0) in vec4 sprite;
layout(location = 1) in vec4 tile;

out vec2 uv;
out vec2 uvOffset;

uniform mat4 model;
uniform mat4 projection;

void main() {
    //uv = uvIn;
    uv = sprite.zw;
    uvOffset = tile.zw;
    gl_Position = projection * (model * vec4(sprite.xy, 0.f, 1.0f)  + vec4(tile.xy, 0.f, 0.f));
}
