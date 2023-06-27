#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "utl.glsl"
#include "pbr.glsl"
#include "material.glsl"

layout( push_constant ) uniform constants
{
	mat4		uVP;
	vec4		uCamPos;
} PushConstants;

layout ( location = 0 ) in vec4 vColor;
layout ( location = 1 ) in vec2 vTexCoord;

layout ( location = 0 ) out vec4 fFragColor;

void
main()
{
	fFragColor = vColor;
}
