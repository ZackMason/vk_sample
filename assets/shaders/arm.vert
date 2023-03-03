#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout( push_constant ) uniform constants
{
    mat4 vp;
    mat4 basis;
    vec3 color;
    float arm_scale;
} PushConstants;

layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in vec3 aColor;
layout( location = 3 ) in vec2 aTexCoord;

layout ( location = 0 ) out vec3 vColor;
layout ( location = 1 ) out vec2 vTexCoord;
layout ( location = 2 ) out vec3 vN;
layout ( location = 3 ) out vec3 vWorldPos;

void
main() {
	mat4   M = mat4(PushConstants.basis);
	mat4 PVM = PushConstants.vp * M;

    vColor = PushConstants.color;
	vTexCoord = aTexCoord;

    vec3 vertex = aVertex * vec3(PushConstants.arm_scale, 1.0, 1.0);
	
	vWorldPos = (M * vec4(vertex, 1.0)).xyz;

	vN = normalize(transpose(inverse(M)) * vec4(aNormal, 0.0)).xyz;

	gl_Position = PVM * vec4(vertex, 1. );
}
