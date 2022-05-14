#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 vColor[];
out vec3 fColor;

void build_rect(vec4 position) {
    gl_Position = position + vec4(-0.1, -0.1, 0.0, 0.0);
    EmitVertex();
    gl_Position = position + vec4(0.1, -0.1, 0.0, 0.0);
    EmitVertex();
    gl_Position = position + vec4(-0.1, 0.1, 0.0, 0.0);
    EmitVertex();
    gl_Position = position + vec4(0.1, 0.1, 0.0, 0.0);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    fColor = vColor[0];
    build_rect(gl_in[0].gl_Position);
}