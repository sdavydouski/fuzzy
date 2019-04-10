#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec2 uv;

out vec4 out_Color;

uniform vec4 u_Color;
uniform float u_BorderWidth;
uniform float u_WidthOverHeight;

void main()
{
    float MinX = u_BorderWidth;
    float MaxX = 1.f - u_BorderWidth;
    float MinY = u_BorderWidth * u_WidthOverHeight;
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
