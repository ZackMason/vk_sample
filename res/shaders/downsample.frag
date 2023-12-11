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

layout(binding = 0, set = 0) uniform sampler2D uTextures[13];

#define kMipLevel int(uParameters.data.data[0])
#define uRead uTextures[kMipLevel]

layout ( location = 0 ) in vec2 vUV;
layout ( location = 0 ) out vec4 fFragColor;

void main() {
    vec2 texel_size = 1.0/textureSize(uRead,0).xy;
    float x = texel_size.x;
    float y = texel_size.y;

    vec2 uv = vUV;

    vec3 a = texture(uRead, vec2(uv.x - 2*x, uv.y + 2*y)).rgb;
    vec3 b = texture(uRead, vec2(uv.x      , uv.y + 2*y)).rgb;
    vec3 c = texture(uRead, vec2(uv.x + 2*x, uv.y + 2*y)).rgb;

    vec3 d = texture(uRead, vec2(uv.x - 2*x, uv.y)).rgb;
    vec3 e = texture(uRead, vec2(uv.x      , uv.y)).rgb;
    vec3 f = texture(uRead, vec2(uv.x + 2*x, uv.y)).rgb;
    
    vec3 g = texture(uRead, vec2(uv.x - 2*x, uv.y - 2*y)).rgb;
    vec3 h = texture(uRead, vec2(uv.x      , uv.y - 2*y)).rgb;
    vec3 i = texture(uRead, vec2(uv.x + 2*x, uv.y - 2*y)).rgb;
    
    vec3 j = texture(uRead, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 k = texture(uRead, vec2(uv.x + x, uv.y + y)).rgb;
    vec3 l = texture(uRead, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 m = texture(uRead, vec2(uv.x + x, uv.y - y)).rgb;

    vec3 color=vec3(0.0);
    vec3 w[5];
    switch(kMipLevel){
        case 0: {
            w[0] = (a+b+d+e) * (0.125/4.0);
            w[1] = (b+c+e+f) * (0.125/4.0);
            w[2] = (d+e+g+h) * (0.125/4.0);
            w[3] = (e+f+h+i) * (0.125/4.0);
            w[4] = (j+k+l+m) * (0.5/4.0);
            w[0] *= karis_average(w[0]);
            w[1] *= karis_average(w[1]);
            w[2] *= karis_average(w[2]);
            w[3] *= karis_average(w[3]);
            w[4] *= karis_average(w[4]);
            color = w[0]+w[1]+w[2]+w[3]+w[4];
        }   break;
        default: {
            color = e*0.125;
            color += (a+c+g+i)*0.03125;
            color += (b+d+f+h)*0.0625;
            color += (j+k+l+m)*0.125;
        }   break;
    }
    color = max(color, vec3(0.0001));
    fFragColor = vec4(color, 1.0);
}