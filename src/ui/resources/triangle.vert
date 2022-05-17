#version 330 core

uniform vec2 worldSize;

in vec3 color;
in vec2 position;

out vec3 vColor;
out vec2 vPosition;

void main()
{
    vColor = color;
    vPosition = position;

    // Normalize to range [-1, 1]:
    vec2 pos = (2 * (position / worldSize)) - 1;

    gl_Position = vec4(pos, 0.0, 1.0);
}
