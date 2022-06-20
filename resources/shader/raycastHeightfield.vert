#version 430 core

uniform mat4 viewProjectionMatrix;
uniform vec2 quadSize;
uniform float distance;

out vec2 texCoord;

const vec2 pos_data[4] = vec2[]
(
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);

void main()
{
    texCoord = 0.5 * (vec2(1.0) + pos_data[ gl_VertexID ]);
    vec4 position = vec4( quadSize * pos_data[ gl_VertexID ], distance, 1.0 );
    gl_Position = viewProjectionMatrix * (position);
}
