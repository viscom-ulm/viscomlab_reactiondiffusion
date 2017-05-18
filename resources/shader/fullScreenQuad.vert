#version 430 core

out vec2 texCoord;

const vec2 pos_data[3] = vec2[]
(
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main()
{
    texCoord = 0.5 * (vec2(1.0) + pos_data[ gl_VertexID ]);
    gl_Position = vec4( pos_data[ gl_VertexID ], 0.0, 1.0 );
}
