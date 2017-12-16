#version 330 core
in vec2 uv;
out vec4 color;

uniform sampler2D spriteTexture;
uniform vec2 spriteSize;
uniform vec2 spriteOffset;
uniform bool reversed;

void main() {
    float x = reversed ? 
        1 - uv.x + spriteOffset.x + spriteSize.x :
        uv.x + spriteOffset.x;

    color = texture(spriteTexture, vec2(x, uv.y + spriteOffset.y));
}
