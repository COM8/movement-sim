#version 450 core

uniform vec2 worldSize;
uniform vec2 rectSize;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec4 gColor[];
out vec4 fColor;

// Normalize to range [-1, 1]:
vec2 normalize_position(vec2 pos)  {
    return ((pos / worldSize) * 2) - 1;
}

void build_rect(vec4 position, vec2 size) {
    size /= 2;
    gl_Position = vec4(normalize_position(position.xy + vec2(-size.x, -size.y)), 0.0, 1.0);
    EmitVertex();
    gl_Position = vec4(normalize_position(position.xy + vec2(size.x, -size.y)), 0.0, 1.0);
    EmitVertex();
    gl_Position = vec4(normalize_position(position.xy + vec2(-size.x, size.y)), 0.0, 1.0);
    EmitVertex();
    gl_Position = vec4(normalize_position(position.xy + vec2(size.x, size.y)), 0.0, 1.0);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    fColor = gColor[0];

    build_rect(gl_in[0].gl_Position, rectSize / 2);
}