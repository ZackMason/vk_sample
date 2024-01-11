#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable

#include "utl.glsl"

layout ( location = 0 ) in vec2 voUV;
layout ( location = 1 ) flat in uint voTexID;
layout ( location = 2 ) in vec4 voColor;
layout ( location = 3 ) in vec3 voNormal;

layout ( location = 0 ) out vec4 fFragColor;

vec3 aces_film(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

layout(set = 0, binding = 0) uniform sampler2D uTextures[4096];

layout (push_constant, scalar) uniform constants {
    mat4 V;
    mat4 P;
} PushConstants;

void main() {
    vec3 normal = vec3(0.0)==voNormal?vec3(0.0):normalize(voNormal);
    uint is_font = voTexID & (1 << 30);
    uint actual_texture = voTexID & ~(1<<30);
    bool is_texture = actual_texture < 4096 && actual_texture >= 0;
    // opt use mix
    vec4 tex = (is_texture) ? 
        texture(uTextures[actual_texture], voUV) : 
        vec4(1.0);

    vec4 color = tex;
   
    if (is_texture) {
        if (is_font > 0) { // is font
            color = color.rrrr;
            if (color.a < 0.15) {
                discard;
            }
            // color.a = step(0.15, color.a);
        } else { // texture
            // color.rgb = aces_film(color.rgb);
            // color.rgb = vec3(voUV, 0.0);
            // color.a = 1.0;
            // color.a = step(0.005, color.a);
            // color.rgb = pow(color.rgb, vec3(1.0/2.2));
        }
    } 

    // color.rgb *= voColor.rgb;
    // color *= voColor;// * voColor;
    color *= voColor;
    
    fFragColor.rgb = color.rgb;

    // if (is_texture && is_font == 0) {
    //     color.a = tex.a;
    // }

    fFragColor.a = saturate(color.a);

    if (vec3(0.0)!=voNormal) {
        // fFragColor.rgb *= saturate(dot(normal, normalize(vec3(1,2,3))));
    }

    // if (fFragColor.a < 0.01) {
    //     discard;
    // }

    // fFragColor.rgb *= fFragColor.rgb;
}