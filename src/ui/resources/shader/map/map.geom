#version 400 core

uniform vec2 rectSize;

in vec2 gStartPos[];
in vec2 gEndPos[];

layout(points) in;
layout(line_strip, max_vertices = 2) out;

void main() {    
    gl_Position = gl_in[0].gl_Position; 
    EmitVertex();

    gl_Position = vec4(gEndPos[0].x, gEndPos[0].y, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}  