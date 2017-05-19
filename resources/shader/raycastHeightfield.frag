#version 430 core

in vec4 gl_FragCoord;

in vec2 texCoord;

uniform vec2 quadSize;
uniform float distance;
uniform float simulationHeight;
uniform vec3 cameraPosition;
uniform float eta;
uniform sampler2D environment;
uniform sampler2D backgroundTexture;
uniform sampler2D heightTexture;
layout(rg32f) uniform image2D backPositionTexture;

layout(location = 0) out vec4 color;

vec3 worldToTex(vec3 x) {
    vec3 offset = vec3(quadSize, distance + simulationHeight);
    vec3 scale = 1.0 / vec3(2.0 * quadSize, simulationHeight);
    return (x + offset) * scale;
}

vec2 reflectionToSpherical(vec3 r) {
    vec3 rp = vec3(r) + vec3(0.0, 0.0f, 1.0);
    float m = 2.0 * sqrt(dot(rp, rp));
    return r.xy / m + 0.5;
}

float heightFieldSphere(vec2 texCoords, vec2 c, float r) {
    const float radius = 0.25;
    vec2 tPos = texCoords - c;
    float hsq = max(r*r - dot(tPos, tPos), 0.0);
    return sqrt(hsq);
}

float heightField(vec2 texCoords) {
    // return texture(heightTexture, texCoords).r;
    return (4.0 * heightFieldSphere(texCoords, vec2(0.5), 0.25))
        + (5.0 * heightFieldSphere(texCoords, vec2(0.12, 0.12), 0.09))
        + (5.0 * heightFieldSphere(texCoords, vec2(0.12, 0.88), 0.09))
        + (5.0 * heightFieldSphere(texCoords, vec2(0.88, 0.12), 0.09))
        + (5.0 * heightFieldSphere(texCoords, vec2(0.88, 0.88), 0.09));
}

void main()
{
    // vec3 lightPos = worldToTex(vec3(5, 2, -3));
    vec3 lightPos = worldToTex(vec3(0.0, 0.0, -10.0));
    vec3 camPos = worldToTex(cameraPosition);

    vec3 t1 = vec3(texCoord, 1.0f);
    vec3 t0  = vec3(imageLoad(backPositionTexture, ivec2(gl_FragCoord.xy)).xy, 0.0f);
    vec3 t1m0 = t1 - t0;

    vec3 t = t0;
    for (int i = 0; i < 10; ++i) {
        float h = heightField(t.xy);
        t = t0 + (h * t1m0);
    }

    vec3 tRel = t - camPos;
    vec3 tDX = dFdx(tRel);
    vec3 tDY = dFdy(tRel);
    vec3 normal = normalize(cross(tDX, tDY));

    vec3 v = camPos - t;
    vec3 r = reflect(v, normal);
    vec2 sphereCoords = reflectionToSpherical(r);

    color = vec4(sqrt(texture(environment, sphereCoords).rgb), 1.0);
    
    // vec3 l = lightPos;// vec3(0.5, 0.5, 5.0);
    // float diffuse = 0.8* clamp(dot(normal, l - t), 0.0, 1.0);
    // color = vec4(vec3(sqrt(diffuse)), 1.0);
    // color = vec4(t.x, t.y, h, 1);
    // vec3 t = refract(v, normal, eta);
}
