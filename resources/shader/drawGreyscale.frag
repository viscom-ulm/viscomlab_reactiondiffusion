#version 430 core

in vec4 gl_FragCoord;

in vec2 texCoord;

uniform vec2 quadSize;
uniform float distance;
uniform sampler2D heightTexture;

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(texture(heightTexture, texCoord).rrr, 1.0);
}
