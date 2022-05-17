#version 330 core

uniform vec4 viewPort;

in vec3 fColor;
in vec2 fPosition;

out vec4 outColor;

void main()
{
    // Discard invisible stuff:
    if(fPosition.x < viewPort.x || fPosition.x > viewPort.y || fPosition.y < viewPort.z || fPosition.y > viewPort.w) {
        discard;
    }
    outColor = vec4(fColor, 1.0);
}
