#version 400 core

uniform vec2 worldSize;

in vec2 position;

void main(void) {
    // Normalize to range [-1, 1]:
    vec2 pos = (2 * (position / worldSize)) - 1;

    gl_Position = vec4(pos, 0.0, 1.0);
}
