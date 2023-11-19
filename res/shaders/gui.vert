#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "packing.glsl"

layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec2 aTexCoord;
layout( location = 2 ) in uint aNrm;
layout( location = 3 ) in uint aTexID;
layout( location = 4 ) in uint aColor;

layout ( location = 0 ) out vec2 voUV;
layout ( location = 1 ) out uint voTexID;
layout ( location = 2 ) out vec4 voColor;
layout ( location = 3 ) out vec3 voNormal;

layout (push_constant, scalar) uniform constants {
    mat4 V;
    mat4 P;
} PushConstants;

void
main() {
    voUV = aTexCoord.xy;
    voTexID = aTexID;
    voColor = rgba_to_color(aColor);
    vec4 pos;

    if (aNrm != 0) {
        voNormal = unpack_normal(aNrm);
        pos = PushConstants.P * PushConstants.V * vec4(aVertex.xyz, 1.0);
    } else {
        voNormal = vec3(0,0,1);
        pos = vec4(aVertex.xy * 2.0 - 1.0, 0.00001 * gl_VertexIndex, 1);
    }
        
    gl_Position = pos;
}