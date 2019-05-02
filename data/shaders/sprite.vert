#version 330 core

#pragma optimize(off)
#pragma debug(on)

// <vec2 - position, vec2 - uv>
layout(location = 0) in vec4 in_Vertex;

out vec2 uv;

layout(std140) uniform transforms
{
    // view-projection matrix
    mat4 u_VP;
};

uniform mat4 u_Model;
uniform vec2 u_TileSize;

void main()
{
    // todo: precalculate this in vertex buffer?
    uv = in_Vertex.zw * u_TileSize;

    vec2 position = in_Vertex.xy;
    gl_Position = u_VP * u_Model * vec4(position, 0.f, 1.f);
}
