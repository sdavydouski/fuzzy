#version 330 core

in vec2 uv;

out vec4 color;

uniform bool useTexture;
uniform sampler2D texture;

void main() {
    if (useTexture) {
        color = texture(texture, uv);
    } else {
        color = vec4(1.0f);
    }
}
