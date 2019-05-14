#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec2 uv;

out vec4 out_Color;

uniform vec2 u_UVOffset;
uniform sampler2D u_FontTextureAtlas;
uniform vec4 u_TextColor;

void main()
{
    float ChannelValue = texture(u_FontTextureAtlas, uv + u_UVOffset).r;
    out_Color = vec4(1.f, 1.f, 1.f, ChannelValue) * u_TextColor;
    //out_Color = u_TextColor;
}
