#version 460 core

in vec2 fTexCoordinates;

uniform sampler2D inputTexture;
layout(std140, binding = 0) uniform blurArrayBlock {
    vec2 offsets[9];
};

out vec4 outColor;

void main()
{
    outColor = vec4(0);
    for(int i = 0; i < 9; i++) {
        vec2 pos = fTexCoordinates + offsets[i];
        outColor += texture(inputTexture, pos);
    }
    outColor /= 9.0;
    outColor *= vec4(1, 1, 1, 0.995);
    return;
}