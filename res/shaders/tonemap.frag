#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "tonemap.glsl"

struct PPMaterial {
    float data[16];
};

layout ( location = 0 ) in vec2 vUV;

layout ( location = 0 ) out vec4 fFragColor;

layout(set = 0, binding = 0) uniform sampler2D uTextures[2];
layout(push_constant) uniform PPParams {
    PPMaterial data;
} uParameters;

#define uColor (uTextures[0])
#define uTonemap (uint(floor(uParameters.data.data[0])))
#define uExposure (uParameters.data.data[1])
#define uContrast (uParameters.data.data[2])
#define uGamma (uParameters.data.data[3])

#define ACES 0
#define EXPO 2
#define REIN 1
#define CHRT 3


vec3 aces_film(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

vec3 uncharted(vec3 x) {
    float a = 0.15;
    float b = 0.50;
    float c = 0.10;
    float d = 0.20;
    float e = 0.02;
    float f = 0.30;
    float w = 11.2;
    return ((x*(a*x+c*b)+d*e)/(x*(a*x+b)+d*f))-e/f;
}

vec3 reinhard(vec3 x) {
    return x / (vec3(1.0) + x);
}

void main() {
    vec3 color = texture(uColor, vUV).rgb;
    if (uTonemap == ACES) { color = aces_film(color); }
    if (uTonemap == REIN) { color = reinhard(color); }
    if (uTonemap == EXPO) { color = vec3(1.0) - exp(-color * uExposure); }
    if (uTonemap == CHRT) { color = uncharted(color); }
        
    color = pow(color, vec3(1.0/uGamma));
    fFragColor.rgb = color.rgb;
}