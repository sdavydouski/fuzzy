#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uvIn;

out vec2 uv;

uniform mat4 model;
uniform mat4 projection;

void main() {
    uv = uvIn;
    gl_Position = projection * model * vec4(position, 0.0f, 1.0f);
}
