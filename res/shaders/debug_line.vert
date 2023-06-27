#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( push_constant ) uniform object_constants
{
	mat4 uVP;
} ObjectConstants;


layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec3 aColor;

layout ( location = 0 ) out vec3 vColor;

void main() {
	vColor    = aColor;
	
	gl_Position = ObjectConstants.uVP * vec4( aVertex, 1.);
}
