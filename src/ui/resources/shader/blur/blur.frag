#version 400 core

in vec2 fTexCoordinates;

uniform sampler2D inputTexture;

void main()
{
    gl_FragColor = texture(inputTexture, fTexCoordinates) * 0.98;
    return;
}