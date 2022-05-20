#version 330 core

uniform vec2 worldSize;
uniform float zoomFactor;

in vec3 color;
in vec2 position;

out vec3 gColor;
out vec2 gPosition;

void main()
{
    gColor = color;
    gPosition = position * zoomFactor;

    // Normalize to range [-1, 1] and apply zoom factor:
    vec2 pos = (2 * (position / (worldSize * zoomFactor))) - 1;

    gl_Position = vec4(pos, 0.0, 1.0);
}
