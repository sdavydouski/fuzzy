#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec4 Color;
//in vec2 uv;
//in vec2 instanceUVOffset;

out vec4 out_Color;

//uniform sampler2D u_TilesetImage;

void main()
{
    //out_Color = texture(u_TilesetImage, uv + instanceUVOffset);
    out_Color = Color;
}
