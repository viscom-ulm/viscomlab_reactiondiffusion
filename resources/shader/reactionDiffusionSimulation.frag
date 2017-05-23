#version 430 core

// input attributes
in vec2 texCoord;

// output attributes
layout(location = 0) out vec4 AB_next;
layout(location = 1) out vec4 result;

// uniforms
uniform sampler2D texture_0;

//uniform vec2 inv_tex_dim;

uniform float diffusion_rate_A = 1.0;
uniform float diffusion_rate_B = 0.5;
uniform float feed_rate = 0.055;
uniform float kill_rate = 0.062;
uniform float dt = 1.0;

uniform float seed_point_radius = 0.001;
uniform uint num_seed_points = 0;
const uint max_seed_points = 10;
uniform vec2 seed_points[max_seed_points];
uniform bool use_manhatten_distance = false;

vec2 laplaceAB() // vec2 laplaceAB(vec2 inv_tex_dim)
{
    // 0.0500    0.2000    0.0500
    // 0.2000   -1.0000    0.2000
    // 0.0500    0.2000    0.0500

    //const float inv_tex_dim_x = inv_tex_dim.x;
    //const float inv_tex_dim_y = inv_tex_dim.y;
    //return 0.05 * texture(texture_0, texCoord + vec2(-inv_tex_dim_x, inv_tex_dim_y)).rg // upper line
    //     + 0.20 * texture(texture_0, texCoord + vec2(0.0, inv_tex_dim_y)).rg
    //     + 0.05 * texture(texture_0, texCoord + vec2(inv_tex_dim_x, inv_tex_dim_y)).rg
    //     + 0.20 * texture(texture_0, texCoord + vec2(-inv_tex_dim_x, 0.0)).rg // middle line
    //     -        texture(texture_0, texCoord).rg
    //     + 0.20 * texture(texture_0, texCoord + vec2(inv_tex_dim_x, 0.0)).rg
    //     + 0.05 * texture(texture_0, texCoord + vec2(-inv_tex_dim_x, -inv_tex_dim_y)).rg // lower line
    //     + 0.20 * texture(texture_0, texCoord + vec2(0.0, -inv_tex_dim_y)).rg
    //     + 0.05 * texture(texture_0, texCoord + vec2(inv_tex_dim_x, -inv_tex_dim_y)).rg;

    return 0.05 * textureOffset(texture_0, texCoord, ivec2(-1,  1)).rg // upper line
         + 0.20 * textureOffset(texture_0, texCoord, ivec2( 0,  1)).rg
         + 0.05 * textureOffset(texture_0, texCoord, ivec2( 1,  1)).rg
         + 0.20 * textureOffset(texture_0, texCoord, ivec2(-1,  0)).rg // middle line
         -        textureOffset(texture_0, texCoord, ivec2( 0,  0)).rg
         + 0.20 * textureOffset(texture_0, texCoord, ivec2( 1,  0)).rg
         + 0.05 * textureOffset(texture_0, texCoord, ivec2(-1, -1)).rg // lower line
         + 0.20 * textureOffset(texture_0, texCoord, ivec2( 0, -1)).rg
         + 0.05 * textureOffset(texture_0, texCoord, ivec2( 1, -1)).rg;
}

void main()
{
    //const vec2 inv_tex_dim = 1.0 / textureSize(texture_0, 0).xy;
    const vec2 AB = texture(texture_0, texCoord).rg;
    const float A = AB.r;
    float B = AB.g; // TODO: add const here when brush shader is available

    for (int i = 0; i < num_seed_points; ++i) {
        const vec2 seed_point = abs(texCoord - seed_points[i]);
        if (use_manhatten_distance) {
            const float d = seed_point.x + seed_point.y;
            if (d < seed_point_radius) {
                B = 1.0;
            }
        } else {
            const float d = dot(seed_point, seed_point);
            const float r = seed_point_radius * seed_point_radius;
            if (d < r && d > 0.9 * r) {
                B = 1.0;
            }
        }
    }

    const vec2 laplace_AB = laplaceAB();
    const float laplace_A = laplace_AB.r;
    const float laplace_B = laplace_AB.g;

    const float ABB = A * B * B;
    const float A_next = A + (diffusion_rate_A * laplace_A - ABB + feed_rate * (1 - A)) * dt;
    const float B_next = B + (diffusion_rate_B * laplace_B + ABB - (kill_rate + feed_rate) * B) * dt;

    const float result_value = 1.0 - clamp(A_next - B_next, 0.0, 1.0);
    //const float result_value = clamp(A_next - B_next, 0.0, 1.0);
    result = vec4(result_value, result_value, result_value, 1.0);
    AB_next = vec4(clamp(A_next, 0.0, 1.0), clamp(B_next, 0.0, 1.0), 1.0, 1.0);
}
