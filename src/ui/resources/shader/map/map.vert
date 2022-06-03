#version 400 core

uniform vec2 worldSize;

in vec2 startPos;
in vec2 endPos;

out vec2 gStartPos;
out vec2 gEndPos;

void main()
{
    // Normalize to range [-1, 1]:
    gStartPos = startPos / worldSize;
    gEndPos = endPos / worldSize;
    gl_Position = vec4(gStartPos, 0.0, 1.0);
}
