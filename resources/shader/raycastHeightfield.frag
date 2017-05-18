#version 430 core

in vec2 texCoord;

uniform float simulationHeight;
uniform sampler2D environment;
uniform sampler2D backgroundTexture;
uniform image2D backPositionTexture;

layout(location = 0) out vec4 color;

void main()
{
    vec2 texCoordFront = texCoord;
    vec2 texCoordBack  = imageLoad(backPositionTexture, gl_FragCoord.xy).xy;
}
