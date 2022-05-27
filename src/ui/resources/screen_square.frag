#version 330 core

in vec2 texCoordinates;

out vec4 outColor;

uniform sampler2D screenTexture;

void main()
{
    outColor = texture(screenTexture, texCoordinates);
}