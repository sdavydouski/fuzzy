#version 330 core
in vec2 uv;
out vec4 color;

uniform sampler2D spriteTexture;
uniform vec2 spriteOffset;

void main() {
    color = texture(spriteTexture, vec2(uv.x + spriteOffset.x, uv.y + spriteOffset.y));
}
