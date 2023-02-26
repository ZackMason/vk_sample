#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "material.glsl"

layout( std140, set = 0, binding = 0 ) uniform SporadicBuffer
{
	int		uMode;
	int		uUseLighting;
	int		uNumInstances;
	int 	pad;
} uSporadic;

struct ObjectData {
	mat4 model;

	uint material_id;
	uint padding[3 + 4*3];
};

layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} uObjectBuffer;

layout( push_constant ) uniform constants
{
	mat4		uVP;
	vec4		uCamPos;
} PushConstants;

layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in vec3 aColor;
layout( location = 3 ) in vec2 aTexCoord;

layout ( location = 0 ) out flat uint vMatId;
layout ( location = 1 ) out vec2 vTexCoord;
layout ( location = 2 ) out vec3 vN;
layout ( location = 3 ) out vec3 vWorldPos;

void
main() {
	mat4   M = uObjectBuffer.objects[gl_BaseInstance].model;
	mat4 PVM = PushConstants.uVP * M;

	vMatId = uObjectBuffer.objects[gl_BaseInstance].material_id;
	vTexCoord = aTexCoord;
	
	vWorldPos = (M * vec4(aVertex, 1.0)).xyz;

	vN = normalize(transpose(inverse(M)) * vec4(aNormal, 0.0)).xyz;

	gl_Position = PVM * vec4(aVertex, 1. );
}
