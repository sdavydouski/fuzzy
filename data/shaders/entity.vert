#version 330 core

#pragma optimize(off)
#pragma debug(on)

// <vec2 - position, vec2 - uv>
layout(location = 0) in vec4 in_Vertex;
layout(location = 1) in mat4 in_InstanceModel;
layout(location = 5) in vec2 in_InstanceUVOffset;

out vec2 uv;
out vec2 instanceUVOffset;

layout(std140) uniform transforms
{
    // view-projection matrix
    mat4 u_VP;
};

uniform vec2 u_TileSize;

void main()
{
    // getting correct uv from texture atlas
    uv = in_Vertex.zw * u_TileSize;
    instanceUVOffset = in_InstanceUVOffset;

    vec2 position = in_Vertex.xy;
    gl_Position = u_VP * in_InstanceModel * vec4(position, 0.f, 1.f);
}
