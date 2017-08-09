#version 330 core
#extension GL_ARB_explicit_uniform_location : enable

layout(location = 0) in vec3 position;

layout(location = 0) uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(position, 1.0f);
}
