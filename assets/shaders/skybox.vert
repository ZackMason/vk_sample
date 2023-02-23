#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( push_constant ) uniform object_constants
{
	mat4 uVP;
	vec4 uDirectionLight;
} ObjectConstants;


layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in vec3 aColor;
layout( location = 3 ) in vec2 aTexCoord;

layout ( location = 0 ) out vec3 vN;

void
main() {
	vN = normalize(aVertex);

	gl_Position = ObjectConstants.uVP * vec4(aVertex * 100.0, 1. );
}
