#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( location = 0 ) in vec2 voUV;
layout ( location = 1 ) flat in uint voTexID;
layout ( location = 2 ) in vec4 voColor;

layout ( location = 0 ) out vec4 fFragColor;

layout(set = 0, binding = 0) uniform sampler2D uTextures[64];

void main() {
    uint is_font = voTexID & (1 << 30);
    uint actual_texture = voTexID & ~(1<<30);
    bool is_texture = actual_texture < 32 && actual_texture >= 0;
    // opt use mix
    vec4 color = (is_texture) ? 
        texture(uTextures[actual_texture], voUV) : 
        vec4(1.0);
   
    if (is_font != 0) {
        color = color.rrrr;
        if (color.a < 0.25) {
            discard;
        }
        color.a = step(0.25, color.a);
    }

    color *= voColor;
    
    fFragColor.rgb = pow(color.rgb, vec3(2.2));
    fFragColor.a = clamp(color.a, 0.0, 1.0);

    if (fFragColor.a < 0.01) {
        discard;
    }
}