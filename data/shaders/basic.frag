#version 330 core
in vec2 uv;
in vec2 uvOffset;
out vec4 color;

uniform sampler2D spriteTexture;
uniform vec2 spriteSize;
uniform vec2 spriteOffset;
uniform bool reversed;

void main() {
    //float x = reversed ? 
    //    1 - uv.x + spriteOffset.x + spriteSize.x :
    //    uv.x + spriteOffset.x;

    //color = texture(spriteTexture, vec2(x, uv.y + spriteOffset.y));
    //color = vec4(1.f, 1.f, 1.f, 1.f);
    if (uvOffset.x != -1 && uvOffset.y != -1) {
        color = texture(spriteTexture, uv + uvOffset);
    } else {
        color = vec4(0.f);
    }
}
