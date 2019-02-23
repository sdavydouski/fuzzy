#version 330 core

#pragma optimize(off)
#pragma debug(on)

// <vec2 - position, vec2 - uv>
//layout(location = 0) in vec4 in_Vertex;
layout(location = 0) in vec4 in_Vertex;
layout(location = 1) in mat4 in_InstanceModel;

out vec2 uv;
out float ratio;

// view-projection matrix
uniform mat4 u_VP;

void main()
{
    uv = in_Vertex.zw;

    vec2 position = in_Vertex.xy;
    gl_Position = u_VP * in_InstanceModel * vec4(position, 0.f, 1.f);
}
