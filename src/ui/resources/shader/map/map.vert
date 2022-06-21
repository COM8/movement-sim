#version 400 core

uniform vec2 worldSize;

layout(location = 0) in vec2 position;

void main(void) {
    // Normalize to range [-1, 0]:
    vec2 pos = (position / worldSize) - 1;

    gl_Position = vec4(pos, 0.0, 1.0);
}
