#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "utl.glsl"
#include "pbr.glsl"

layout( push_constant ) uniform constants
{
    mat4 vp;
    mat4 basis;
    vec3 color;
    float arm_scale;
} PushConstants;

layout ( location = 0 ) in vec3 vColor;
layout ( location = 1 ) in vec2 vTexCoord;
layout ( location = 2 ) in vec3 vN;
layout ( location = 3 ) in vec3 vWorldPos;

layout ( location = 0 ) out vec4 fFragColor;

const vec3 WHITE = vec3( 1., 1., 1. );

void
main( )
{
	vec3 rgb = vColor;

	vec3 albedo = rgb;
	{
		float metallic = 0.0;
		float roughness = 0.25;
		
		vec3 F0 = vec3(0.04); 
    	F0 = mix(F0, albedo, metallic);

		vec3 Lv = (vec3(0.0, 100.0, 0.0) - vWorldPos);
		vec3 Lc = vec3(0.8431, 0.8784, 0.5137);

		vec3 N = normalize(vN);
		vec3 L = normalize(Lv);
		// vec3 H = normalize(L+V);

		// float NoH = saturate(dot(N, H));
		// float LoH = saturate(dot(L, H));
		float NoL = (dot(N, L));
		// float NoV = saturate(dot(N, V));
		// float HoV = saturate(dot(V, H));

		rgb = (albedo) * (NoL + 0.3);
	}
	// rgb = albedo;
	// rgb = apply_environment(rgb, depth, PushConstants.uCamPos.xyz, V, uEnvironment);

	fFragColor = vec4( rgb, 1. );
}
