#version 400 core

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 position;

out vec4 gColor;

void main()
{
    gColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
