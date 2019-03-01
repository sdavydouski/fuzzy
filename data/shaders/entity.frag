#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec2 uv;
in vec2 instanceUVOffset;

out vec4 out_Color;

uniform sampler2D u_TilesetImage;

void main()
{
    out_Color = texture(u_TilesetImage, uv + instanceUVOffset); //* vec4(0.5f, 1.f, 0.5f, 1.f);
}
