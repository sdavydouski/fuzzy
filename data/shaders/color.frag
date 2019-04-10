#version 330 core

#pragma optimize(off)
#pragma debug(on)

out vec4 out_Color;

uniform vec4 u_Color;

void main()
{
    out_Color = u_Color;
}
