#version 400 core

in vec2 fTexCoordinates;

out vec4 outColor;

uniform sampler2D screenTexture;
uniform vec2 textureSize;
uniform vec2 screenSize;

void main()
{
    vec2 correctionFactor = screenSize / textureSize;
    outColor = texture(screenTexture, fTexCoordinates * correctionFactor);
    return;
}