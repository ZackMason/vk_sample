#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "material.glsl"

layout( std140, set = 0, binding = 0 ) uniform sporadicBuf
{
	int		uMode;
	int		uUseLighting;
	int		uNumInstances;
	float 	uTime;
} Sporadic;

struct ObjectData {
	mat4 model;

	uint material_id;
	uint padding[3 + 4*3];
};

layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} uObjectBuffer;

struct InstanceData {
	mat4 model;
};

layout(std430, set = 1, binding = 1) readonly buffer InstanceBuffer {
	InstanceData instances[];
} uInstanceData;

layout(std430, set = 1, binding = 2) readonly buffer IndirectBuffer {
	IndirectIndexedDraw draw[];
} uIndirectBuffer;

layout( push_constant ) uniform constants
{
	mat4		uVP;
	vec4		uCamPos;
	uint 		uAlbedoId;
	uint 		uNormalId;
} PushConstants;

layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in vec3 aColor;
layout( location = 3 ) in vec2 aTexCoord;

layout ( location = 0 ) out flat uint vMatId;
layout ( location = 1 ) out vec2 vTexCoord;
layout ( location = 2 ) out vec3 vN;
layout ( location = 3 ) out vec3 vWorldPos;
layout ( location = 4 ) out vec3 vViewPos;
layout ( location = 5 ) out vec3 vViewNormal;
layout ( location = 6 ) out flat uint vAlbedoId;
layout ( location = 7 ) out flat uint vNormalId;

void
main() {
	IndirectIndexedDraw draw = uIndirectBuffer.draw[gl_DrawID];
	uint object_id = draw.object_id;

	mat4   M = uObjectBuffer.objects[object_id].model;
	vec3 vertex = aVertex;
	uint instance_offset = uObjectBuffer.objects[object_id].padding[2];


	if (draw.instance_count > 1) {
		M = M * uInstanceData.instances[gl_InstanceIndex + instance_offset].model;

		vec3 world_pos = (M * vec4(vertex, 1.0)).xyz;
		vertex.xz += max(vertex.y, 0.0) * sin(Sporadic.uTime * 0.1 + world_pos.x) * 0.05 + cos(Sporadic.uTime * 0.12 + world_pos.z)*0.1;
	}
	mat4 PVM = PushConstants.uVP * M;

	vMatId = uObjectBuffer.objects[object_id].material_id;

	vAlbedoId = uIndirectBuffer.draw[gl_DrawID].albedo_id;
	vNormalId = uIndirectBuffer.draw[gl_DrawID].normal_id;

	vTexCoord = aTexCoord;
	
	vWorldPos = (M * vec4(vertex, 1.0)).xyz;

	vN = normalize(transpose(inverse(M)) * vec4(aNormal, 0.0)).xyz;

	gl_Position = PVM * vec4(vertex, 1. );
}
