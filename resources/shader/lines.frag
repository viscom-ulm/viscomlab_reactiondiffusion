#version 330 core
#extension GL_ARB_explicit_uniform_location : enable

layout(location = 1) uniform vec3 colorIn;
layout(location = 0) out vec4 color;

void main()
{
    color = vec4(colorIn, 1.0f);
}
