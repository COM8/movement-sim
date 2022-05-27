#version 400 core

in vec2 fTexCoordinates;

out vec4 outColor;

uniform sampler2D screenTexture;

void main()
{
    outColor = texture(screenTexture, fTexCoordinates);
}