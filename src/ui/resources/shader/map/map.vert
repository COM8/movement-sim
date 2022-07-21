#version 450 core

uniform vec2 worldSize;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 position;

out vec4 fColor;

void main(void) {
    // Normalize to range [-1, 1]:
    vec2 pos = ((position / worldSize) * 2) - 1;

    gl_Position = vec4(pos, 0.0, 1.0);
    fColor = color;
}
