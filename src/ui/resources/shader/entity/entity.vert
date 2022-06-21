#version 400 core

uniform vec2 worldSize;

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 position;

out vec3 gColor;

void main()
{
    gColor = color;

    // Normalize to range [-1, 0]:
    vec2 pos = (position / worldSize) - 1;

    gl_Position = vec4(pos, 0.0, 1.0);
}
