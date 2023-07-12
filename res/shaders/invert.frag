#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( location = 0 ) in vec2 vUV;

layout ( location = 0 ) out vec4 fFragColor;

layout(set = 0, binding = 0) uniform sampler2D uTextures[2];

#define uColor uTextures[0]

void main() {
    vec3 color = texture(uColor, vUV).rgb;

    fFragColor.rgb = 1.0 - color.rgb;
}