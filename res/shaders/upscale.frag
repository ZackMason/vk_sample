#version 460
#extension GL_EXT_scalar_block_layout : enable
#extension GL_ARB_separate_shader_objects  : enable

#include "tonemap.glsl"

struct PPMaterial {
    float data[16];
};

layout(push_constant, scalar) uniform PPParams {
    PPMaterial data;
} uParameters;

layout(binding = 0, set = 0) uniform sampler2D uTextures[2];

#define uRead uTextures[0]
#define kFilterRadius (uParameters.data.data[0])

layout ( location = 0 ) in vec2 vUV;
layout ( location = 0 ) out vec4 fFragColor;

void main() {
    float x = kFilterRadius;
    float y = kFilterRadius;

    vec2 uv = vUV;

    vec3 a = texture(uRead, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 b = texture(uRead, vec2(uv.x    , uv.y + y)).rgb;
    vec3 c = texture(uRead, vec2(uv.x + x, uv.y + y)).rgb;

    vec3 d = texture(uRead, vec2(uv.x - x, uv.y)).rgb;
    vec3 e = texture(uRead, vec2(uv.x    , uv.y)).rgb;
    vec3 f = texture(uRead, vec2(uv.x + x, uv.y)).rgb;
    
    vec3 g = texture(uRead, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 h = texture(uRead, vec2(uv.x    , uv.y - y)).rgb;
    vec3 i = texture(uRead, vec2(uv.x + x, uv.y - y)).rgb;
    
    vec3 color=e*4.0;
    color += (b+d+f+h)*2.0;
    color += (a+c+g+i);
    color *= 1.0 / 16.0;

    fFragColor = vec4(color,1.);
}
