#version 430 core

in vec2 texCoord;
layout(location = 0) out vec4 texCoordBack;

void main()
{
    texCoordBack = vec4(texCoord, 0.0, 1.0);
}
