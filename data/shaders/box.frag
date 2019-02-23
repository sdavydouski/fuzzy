#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec2 uv;

out vec4 out_Color;

void main()
{
    out_Color = vec4(1.f, 1.f, 0.f, 0.25f);
}
