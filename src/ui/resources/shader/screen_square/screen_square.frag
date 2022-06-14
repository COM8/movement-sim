#version 400 core

in vec2 fTexCoordinates;

out vec4 outColor;

uniform sampler2D screenTexture;
uniform vec2 textureSize;
uniform vec2 screenSize;

void main()
{
    float textureAspectRatio = textureSize.x / textureSize.y;
    float screenAspectRatio = screenSize.x / screenSize.y;
    float textureScreenRatio = textureAspectRatio / screenAspectRatio;
    float scaleX = 1.0;
    float scaleY = 1.0;

    if(screenAspectRatio < 1.0) {
        scaleY = textureScreenRatio;
    }
    else {
        scaleX = 1.0 / textureScreenRatio;
    }

    outColor = texture(screenTexture, fTexCoordinates * vec2(scaleX, scaleY));
    return;
}