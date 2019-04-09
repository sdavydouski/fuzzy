#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec2 uv;

out vec4 out_Color;

uniform vec4 u_Color;

void main()
{
    float width = 0.1f;

    if (uv.x >= width && uv.x <= 1.f - width && uv.y >= width && uv.y <= 1.f - width)
    {
        discard;
    }
    else
    {
        out_Color = u_Color;
    }

    //out_Color = u_Color;
    //out_Color = vec4(0.f, 0.f, 1.f, 1.f);
}
