#version 330 core

uniform vec2 worldSize;
uniform vec2 rectSize;
uniform float zoomFactor;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 gColor[];
out vec3 fColor;

in vec2 gPosition[];
out vec2 fPosition;

void build_rect(vec4 position, vec2 size) {
    size /= 2;
    gl_Position = position + vec4(- size.x, -size.y, 0.0, 0.0);
    EmitVertex();
    gl_Position = position + vec4(size.x, -size.y, 0.0, 0.0);
    EmitVertex();
    gl_Position = position + vec4(-size.x, size.y, 0.0, 0.0);
    EmitVertex();
    gl_Position = position + vec4(size.x, size.y, 0.0, 0.0);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    fColor = gColor[0];
    fPosition = gPosition[0];
    vec2 size = 2 * (rectSize / worldSize);

    // Apply zoom factor:
    size *= zoomFactor;
    build_rect(gl_in[0].gl_Position, size);
}