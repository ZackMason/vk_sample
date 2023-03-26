#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "utl.glsl"
#include "pbr.glsl"
#include "material.glsl"

// Vulkan Sample Program Fragment Shader:

// also, can't specify packing
layout( std140, set = 0, binding = 0 ) uniform sporadicBuf
{
	int		uMode;
	int		uUseLighting;
	int		uNumInstances;
	int 	pad;
} Sporadic;

struct ObjectData {
	mat4 model;

	uint material_id;
	uint padding[3 + 4*3];
};

layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} uObjectBuffer;

layout( set = 4, binding = 0 ) uniform sampler2D uSampler;


layout( push_constant ) uniform constants
{
	mat4		uVP;
	vec4		uCamPos;
} PushConstants;


layout ( location = 0 ) in flat uint vMatId;
layout ( location = 1 ) in vec2 vTexCoord;
layout ( location = 2 ) in vec3 vN;
layout ( location = 3 ) in vec3 vWorldPos;

layout ( location = 0 ) out vec4 fFragColor;

const vec3 WHITE = vec3( 1., 1., 1. );

vec3 
apply_environment(
	vec3 lum, 
	float depth, 
	vec3 ro, vec3 rd,
	Environment env
) {
	float a = 0.02;
	float b = 0.01;
	float c = a/b;
	// float fog = c - exp(-ro.y*b) * (1.0 - exp(-depth*rd.y*b))/rd.y;
	float fog = a * exp(-ro.y*b)*(1.0-exp(-depth*rd.y*b))/rd.y;

	// return lum + env.fog_color.rgb * fog + env.ambient_color.rgb;
	return mix(lum, env.fog_color.rgb, fog) + env.ambient_color.rgb;
}

void
main( )
{
	vec3 rgb;
	bool toon = false;
	uint mat_id = vMatId;
	Material material = uMaterialBuffer.materials[mat_id];
	switch( Sporadic.uMode )
	{
		case 0:
			rgb = material.albedo.rgb;
			break;

		case 1:
			rgb = texture( uSampler, vTexCoord ).rgb;
			rgb = pow(rgb, vec3(2.2));
			toon = true;
			break;

		case 2:
			rgb = material.albedo.rgb;
			
			// rgb = ObjectConstants.albedo.rgb;
			break;
		case 3:
			toon = true;
			break;

		default:
			rgb = vec3( 1., 1., 0. );
	}

	float depth = length(PushConstants.uCamPos.xyz - vWorldPos);
	vec3 V = normalize(PushConstants.uCamPos.xyz - vWorldPos);
	// if( Sporadic.uUseLighting != 0 )
	vec3 albedo = rgb;
	{
		float metallic = material.metallic;
		float roughness = material.roughness * material.roughness;
		
		vec3 F0 = vec3(0.04); 
    	F0 = mix(F0, albedo, metallic);

		vec3 Lv = (vec3(0.0, 100.0, 0.0) - vWorldPos);
		vec3 Lc = vec3(0.9686, 0.9765, 0.902);

		vec3 N = normalize(vN);
		vec3 L = normalize(Lv);
		vec3 H = normalize(L+V);

		float NoH = saturate(dot(N, H));
		float LoH = saturate(dot(L, H));
		float NoL = (dot(N, L));
		float NoV = saturate(dot(N, V));
		float HoV = saturate(dot(V, H));

		vec3 spec = vec3(0.9);
		float gloss = 32.0;// * material.roughness;

		// float D = filament_DGGX2(NoH, roughness);
		// vec3  F = filament_Schlick(LoH, F0);
		// float S = filament_SmithGGXCorrelated(NoV, NoL, roughness);

		// vec3 Fr = D * S * F;// (D*V) * F;
		// vec3 fD = albedo * filament_DLambert();

		// fD *= 1.0 - metallic;

		// rgb = fD + Fr;

		vec3 la = vec3(0.4863, 0.6235, 0.8824) * 0.1;
		float li = smoothstep(0.0, 0.01, NoL);
		vec3 light = Lc * li;
		float kR = pow(NoH * li, gloss * gloss);
		float skR = smoothstep(0.005, 0.01, kR);

		float rim_amount = 0.37151;
		float rim = (1.0 - dot(V, N)) * NoL;
		float rim_intensity = smoothstep(rim_amount - 0.01, rim_amount + 0.01, rim);
		vec3 rim_color = vec3(0.7451, 0.8392, 0.8471);

		rgb = (albedo) * (light + la + skR + rim_intensity * rim_color);
	}
	// rgb = albedo;
	// rgb = apply_environment(rgb, depth, PushConstants.uCamPos.xyz, V, uEnvironment);
	// rgb = pow(rgb, vec3(2.2));

	fFragColor = vec4( rgb, 1. );
}
