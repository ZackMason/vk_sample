#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "utl.glsl"
#include "sky.glsl"
#include "tonemap.glsl"
#include "noise.glsl"

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
	
	vec3 extinction = get_extinction(vN, sun);
 	
	vec2 uv = vec2(
		atan(vN.z, vN.x),
		atan(length(vN.xz)/vN.y)
	);

	vec3 color = vec3(0.0);

	float b = 0.7;
	float thr = 1e-1;
	color = 
		mix(
			color, vec3(0.0588, 0.0235, 0.2196),
			smoothstep(
				b-thr, b+thr, 1.0-uv.y+noise(uv*4.0)*0.1
			)
		);

	// color = vec3(uv.y);

	// ground fade
    if (vN.y < -0.05) {
        color = mix(vec3(1.0), vec3(0.3412, 0.1569, 0.1569), .750-vN.y);
	}
    
	fFragColor = vec4( color, 1. );
}
