#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "tonemap.glsl"
#include "utl.glsl"

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
#define uBloom (uTextures[1])
#define uTonemap (uint(floor(uParameters.data.data[0])))
#define uExposure (uParameters.data.data[1])
#define uContrast (uParameters.data.data[2])
#define uGamma (uParameters.data.data[3])
#define uNumberOfColors (uParameters.data.data[4])
#define uPixelate (uParameters.data.data[5])

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

vec3 screenspace_dither(vec2 uv) {
    vec3 vDither = vec3( dot( vec2( 171.0, 231.0 ), uv.xy ) );
    vDither.rgb = fract( vDither.rgb / vec3( 103.0, 71.0, 97.0 ) );
    
    return vDither.rgb / 255.0;
}

float bayer2(vec2 a) {
    a = floor(a);
    return fract(a.x / 2. + a.y * a.y * .75);
}

#define bayer4(a)   (bayer2 (.5 *(a)) * .25 + bayer2(a))
#define bayer8(a)   (bayer4 (.5 *(a)) * .25 + bayer2(a))
#define bayer16(a)  (bayer8 (.5 *(a)) * .25 + bayer2(a))
#define bayer32(a)  (bayer16(.5 *(a)) * .25 + bayer2(a))
#define bayer64(a)  (bayer32(.5 *(a)) * .25 + bayer2(a))

vec3 paletize(vec3 rgb, float n) {
    return floor(rgb*(n-1.0)+0.5)/(n-1);
}

#define INV_SQRT_OF_2PI 0.39894228040143267793994605993439  // 1.0/SQRT_OF_2PI
#define INV_PI 0.31830988618379067153776752674503


vec4 smart_denoise(sampler2D tex, vec2 uv, float sigma, float kSigma, float threshold)
{
    float radius = round(kSigma*sigma);
    float radQ = radius * radius;
    
    float invSigmaQx2 = .5 / (sigma * sigma);      // 1.0 / (sigma^2 * 2.0)
    float invSigmaQx2PI = INV_PI * invSigmaQx2;    // 1.0 / (sqrt(PI) * sigma)
    
    float invThresholdSqx2 = .5 / (threshold * threshold);     // 1.0 / (sigma^2 * 2.0)
    float invThresholdSqrt2PI = INV_SQRT_OF_2PI / threshold;   // 1.0 / (sqrt(2*PI) * sigma)
    
    vec4 centrPx = texture(tex,uv);
    
    float zBuff = 0.0;
    vec4 aBuff = vec4(0.0);
    vec2 size = vec2(textureSize(tex, 0));
    
    for(float x=-radius; x <= radius; x++) {
        float pt = sqrt(radQ-x*x);  // pt = yRadius: have circular trend
        for(float y=-pt; y <= pt; y++) {
            vec2 d = vec2(x,y);

            float blurFactor = exp( -dot(d , d) * invSigmaQx2 ) * invSigmaQx2PI; 
            
            vec4 walkPx =  texture(tex,uv+d/size);

            vec4 dC = walkPx-centrPx;
            float deltaFactor = exp( -dot(dC, dC) * invThresholdSqx2) * invThresholdSqrt2PI * blurFactor;
                                 
            zBuff += deltaFactor;
            aBuff += deltaFactor*walkPx;
        }
    }
    return aBuff/zBuff;
}

vec3 valve_dither(vec2 vScreenPos, float colorDepth)
{
    // lestyn's RGB dither (7 asm instructions) from Portal 2 X360, slightly modified for VR
    vec3 vDither = vec3(dot(vec2(131.0, 312.0), vScreenPos.xy));
    vDither.rgb = fract(vDither.rgb / vec3(103.0, 71.0, 97.0)) - vec3(0.5, 0.5, 0.5);
    return (vDither.rgb / colorDepth) * 0.375;
}

void main() {
    vec2 uv = vUV;

    const float pixelate = uPixelate;
    vec3 bloom = textureLod(uBloom, uv, 0).rgb;
    if (pixelate != 0) {
        uv = saturate(floor(uv*pixelate)/pixelate);
    }

    vec3 color = textureLod(uColor, uv, 0).rgb;

    color = mix(color, (bloom), 0.01);


    if (uNumberOfColors != 0) {
        color = paletize(color, uNumberOfColors);
    }

    if (uTonemap == ACES) { color = aces_film(color); }
    if (uTonemap == REIN) { color = reinhard(color); }
    if (uTonemap == EXPO) { color = vec3(1.0) - exp(-color * uExposure); }
    if (uTonemap == CHRT) { color = uncharted(color); }
        
    // color = sqrt(color);
    // color = sqr(color);

    color = pow(color, vec3(1.0/uGamma));

    // color = bloom;
    fFragColor.rgb = color.rgb;
    fFragColor.a = 1.0;
}