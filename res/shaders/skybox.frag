#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "utl.glsl"
#include "sky.glsl"
#include "tonemap.glsl"

layout ( push_constant ) uniform object_constants
{
	mat4 uVP;
	vec4 uDirectionLight;
} ObjectConstants;

layout ( location = 0 ) in vec3 vN;

layout ( location = 0 ) out vec4 fFragColor;

void
main() {
	vec3 sun = normalize(ObjectConstants.uDirectionLight.xyz);
	vec3 color = sky_color(vN, sun);

	// color = pow(color * 1.0, vec3(1.0/2.2));
	// color = vec3(0);
	color = max(color, 0.0);
	fFragColor = vec4( color, 1. );
}
