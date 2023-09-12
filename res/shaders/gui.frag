#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( location = 0 ) in vec2 voUV;
layout ( location = 1 ) flat in uint voTexID;
layout ( location = 2 ) in vec4 voColor;

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

void main() {
    uint is_font = voTexID & (1 << 30);
    uint actual_texture = voTexID & ~(1<<30);
    bool is_texture = actual_texture < 64 && actual_texture >= 0;
    // opt use mix
    vec4 color = (is_texture) ? 
        texture(uTextures[actual_texture], voUV) : 
        vec4(1.0);
   
    if (is_texture) {
        if (is_font != 0) {
            color = color.rrrr;
            if (color.a < 0.25) {
                discard;
            }
            color.a = step(0.25, color.a);
        } else {
            color.rgb = aces_film(color.rgb);
            // color.rgb = vec3(voUV, 0.0);
            // color.a = 1.0;
            color.rgb = pow(color.rgb, vec3(1.0/2.2));
        }
    }

    color.rgba *= voColor.rgba * voColor.rgba;
    // color.rgba *= voColor.rgba;

    fFragColor.rgb = color.rgb;
    
    fFragColor.a = clamp(color.a, 0.0, 1.0);

    if (fFragColor.a < 0.01) {
        discard;
    }
}