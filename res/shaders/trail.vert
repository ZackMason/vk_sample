#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout( push_constant ) uniform constants
{
	mat4		uVP;
	vec4		uCamPos;
} PushConstants;

layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in vec3 aColor;
layout( location = 3 ) in vec2 aTexCoord;

layout ( location = 0 ) out vec3 vColor;
layout ( location = 1 ) out vec2 vTexCoord;

void
main() {
	vTexCoord = aTexCoord;
	vColor = aColor;

	gl_Position = PushConstants.uVP * vec4(aVertex, 1.0);
}
