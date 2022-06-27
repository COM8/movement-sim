#version 400 core

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 position;

out vec3 gColor;

void main()
{
    gColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
