#version 330 core

#pragma optimize(off)
#pragma debug(on)

in vec2 uv;

out vec4 out_Color;

uniform vec2 u_UVOffset;
uniform sampler2D u_SpriteAtlas;

void main()
{
    out_Color = texture(u_SpriteAtlas, uv + u_UVOffset);
}
