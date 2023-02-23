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
	
	vec3 color = rayleigh_scatter(vN, sun);
	vec3 extinction = get_extinction(vN, sun);
 	
	vec2 uv = vec2(
		atan(vN.y, vN.x),
		atan(length(vN.xy)/vN.z)
	);

	color.rgb += max(0.0, 1.0-pow(dot(color.rgb,color.rgb),.1250));// * stars(uv, 32, 0.04510, max(0.0, 2.0-length(extinction)));
    if (vN.y < 0) {
        color = mix(vec3(1.0), vec3(0.3412, 0.1569, 0.1569), .750-vN.y);
	}
    
	color = pow(color * 1.0, vec3(2.2));

	color = ACESFilm(color);

	// color = pow(color * 1.0, vec3(1.0/2.2));


	fFragColor = vec4( color, 1. );
}
