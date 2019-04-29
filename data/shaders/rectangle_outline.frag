#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec2 uv;

out vec4 out_Color;

uniform vec4 u_Color;
uniform float u_Thickness;
uniform float u_WidthOverHeight;

void main()
{
    float MinX = u_Thickness;
    float MaxX = 1.f - u_Thickness;
    float MinY = u_Thickness * u_WidthOverHeight;
    float MaxY = 1.f - MinY;

    if (uv.x > MinX && uv.x < MaxX && uv.y > MinY && uv.y < MaxY)
    {
        discard;
    }
    else
    {
        out_Color = u_Color;
    }
}
