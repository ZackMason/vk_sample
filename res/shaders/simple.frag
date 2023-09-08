#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable

#define PROBE_USE_SH_COLOR
// #define PROBE_USE_SAMPLER_COLOR
#define PROBE_USE_SAMPLER_DEPTH


#include "utl.glsl"

#include "pbr.glsl"

layout( set = 4, binding = 0 ) uniform sampler2D uSampler[4096];
layout( set = 4, binding = 1 ) uniform sampler2D uProbeTexture[2];

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

	vec4 bounds;

	uint material_id;
	uint padding[3 + 4*2];
};

layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} uObjectBuffer;

layout(std430, set = 1, binding = 2) readonly buffer IndirectBuffer {
	IndirectIndexedDraw draw[];
} uIndirectBuffer;

layout(std430, set = 2, binding = 0) readonly buffer MaterialBuffer {
	Material materials[];
} uMaterialBuffer;

layout(std430, set = 3, binding = 0) readonly buffer EnvironmentBuffer {
	Environment uEnvironment;
};


layout(std430, set = 5, binding = 0, scalar) readonly buffer ProbeBuffer {
	LightProbe probes[];
};

layout(std430, set = 5, binding = 1, scalar) readonly buffer ProbeSettingsBuffer {
	LightProbeSettings probe_settings;
};

layout( push_constant ) uniform constants
{
	mat4 uV;
	mat4 uP;
} PushConstants;


layout ( location = 0 ) in flat uint vMatId;
layout ( location = 1 ) in vec2 vTexCoord;
layout ( location = 2 ) in vec3 vN;
layout ( location = 3 ) in vec3 vWorldPos;

layout ( location = 6 ) in flat uint vAlbedoId;
layout ( location = 7 ) in flat uint vNormalId;
layout ( location = 8 ) in vec3 vCameraPos;

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
	fog = 1.0 - exp(-depth/10.0 * env.fog_density);

	// return lum + env.fog_color.rgb * fog + env.ambient_color.rgb;
	return mix(lum, env.fog_color.rgb, fog) + env.ambient_color.rgb * env.ambient_strength;
}

// St. Peter's Basilica SH
// https://www.shadertoy.com/view/lt2GRD
struct SHCoefficients
{
	vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
};

// compute here http://www.pauldebevec.com/Probes/
const SH9Irradiance SH_STPETER = SH9Irradiance(
	vec3(0.2314, 0.3216, 0.3608),
	vec3(0.1759131, 0.1436266, 0.1260569),
	vec3(-0.0247311, -0.0101254, -0.0010745),
	vec3(0.0346500, 0.0223184, 0.0101350),
	vec3(0.0198140, 0.0144073, 0.0043987),
	vec3(-0.0469596, -0.0254485, -0.0117786),
	vec3(-0.0898667, -0.0760911, -0.0740964),
	vec3(0.0050194, 0.0038841, 0.0001374),
	vec3(-0.0818750, -0.0321501, 0.0033399)
);


const SH9Irradiance SH_GRACE = SH9Irradiance(
    vec3( 0.7953949,  0.4405923,  0.5459412 ),
    vec3( 0.3981450,  0.3526911,  0.6097158 ),
    vec3(-0.3424573, -0.1838151, -0.2715583 ),
    vec3(-0.2944621, -0.0560606,  0.0095193 ),
    vec3(-0.1123051, -0.0513088, -0.1232869 ),
    vec3(-0.2645007, -0.2257996, -0.4785847 ),
    vec3(-0.1569444, -0.0954703, -0.1485053 ),
    vec3( 0.5646247,  0.2161586,  0.1402643 ),
    vec3( 0.2137442, -0.0547578, -0.3061700 )
);
const SHCoefficients SH_GRACEC = SHCoefficients(
    vec3( 0.7953949,  0.4405923,  0.5459412 ),
    vec3( 0.3981450,  0.3526911,  0.6097158 ),
    vec3(-0.3424573, -0.1838151, -0.2715583 ),
    vec3(-0.2944621, -0.0560606,  0.0095193 ),
    vec3(-0.1123051, -0.0513088, -0.1232869 ),
    vec3(-0.2645007, -0.2257996, -0.4785847 ),
    vec3(-0.1569444, -0.0954703, -0.1485053 ),
    vec3( 0.5646247,  0.2161586,  0.1402643 ),
    vec3( 0.2137442, -0.0547578, -0.3061700 )
);


// Equation 13 Ramamoorthi
vec3 SHIrradiance(vec3 nrm)
{
	return sh9_to_irradiance(nrm, SH_STPETER);
	return sh9_to_irradiance(nrm, SH_GRACE);
	const SHCoefficients c = SH_GRACEC;
	// const SHCoefficients c = SH_STPETER;
	
	const float c1 = 0.429043;
	const float c2 = 0.511664;
	const float c3 = 0.743125;
	const float c4 = 0.886227;
	const float c5 = 0.247708;
	return (
		c1 * c.l22 * (nrm.x * nrm.x - nrm.y * nrm.y) +
		c3 * c.l20 * nrm.z * nrm.z +
		c4 * c.l00 -
		c5 * c.l20 +
		2.0 * c1 * c.l2m2 * nrm.x * nrm.y +
		2.0 * c1 * c.l21  * nrm.x * nrm.z +
		2.0 * c1 * c.l2m1 * nrm.y * nrm.z +
		2.0 * c2 * c.l11  * nrm.x +
		2.0 * c2 * c.l1m1 * nrm.y +
		2.0 * c2 * c.l10  * nrm.z
		);
}

vec4 texture_triplanar(sampler2D tex, vec3 p, vec3 n)
{
    vec3 blending = abs(n);
    blending = normalize(max(blending, 0.00001));
    
    // normalized total value to 1.0
    float b = (blending.x + blending.y + blending.z);
    blending /= b;
    
    vec4 xaxis = texture(tex, p.yz);
    vec4 yaxis = texture(tex, p.xz);
    vec4 zaxis = texture(tex, p.xy);
    
    // blend the results of the 3 planar projections.
    return vec4((xaxis * blending.x + yaxis * blending.y + zaxis * blending.z).rgb, 1.0);
}


vec3 light_probe_irradiance(vec3 p, vec3 n, LightProbeSettings settings) {
	vec3 np = light_probe_local_pos_normalized(settings, p);

	// if (np.x < -0.1 || np.y < -0.1 || np.z < -0.1 || np.x > 1.10 || np.y > 1.10 || np.z > 1.10) {
		
	// 	return vec3(1.0);
	// }

    ivec3 min_index = light_probe_probe_index(settings, p);

	LightProbe neighbors[8];
		
	for (uint i = 0; i < 8; i++) {
		uvec3 offset = uvec3(i, i>>1, i>>2) & 1;

		neighbors[i] = probes[index_3d(settings.dim, min_index + ivec3(offset))];
	}

	// vec3 cell_rcp = 1. / light_probe_grid_size(settings);
	// vec3 alpha = saturate((p - neighbors[0].p) * cell_rcp);
	// float weight = 0.0;
	// vec3 tri_weight = vec3(0.0);
	
	// for (uint i = 0; i < 8; i++) {
	// 	uvec3 offset = uvec3(i, i>>1, i>>2) & 1;
	// 	vec3 tri = mix(1.0 - alpha, alpha, offset);
	// 	tri_weight += tri;
	// 	weight += tri.x * tri.y * tri.z;
	// 	// return vec3(tri	);
	// }
	// return vec3(weight);

	// return vec3(min_index);
	// return vec3(min_index);
    
    // return (stupid_light_probe_irradiance(p, n, neighbors, probe_settings));
    return (light_probe_irradiance(p, n, neighbors, probe_settings));


}






void
main( )
{
	vec3 rgb = vec3(0.0);
	Material material = uMaterialBuffer.materials[vMatId];

	uint lit_material = material.flags & MATERIAL_LIT;


	uint triplanar_material = material.flags & MATERIAL_TRIPLANAR;

	int mode = Sporadic.uMode;

	mode = mode == 1 && vAlbedoId == (0xffffffff % 4096) ? 0 : mode;
	float alpha = 1.0f;
	switch(mode)
	{
		case 0:
			rgb = material.albedo.rgb;
			break;

		case 1:
			// rgb = texture( uSampler[1797], vTexCoord ).rgb;
			vec4 albedo = (triplanar_material > 0) ? 
				texture_triplanar(uSampler[vAlbedoId], vWorldPos * 0.5, vN).rgba :
				texture( uSampler[vAlbedoId], vTexCoord ).rgba;
			if (albedo.a < 0.5) { discard; }
			rgb = albedo.rgb * material.albedo.rgb;
			alpha = albedo.a * material.albedo.a;
			break;



		case 2:
			rgb = material.albedo.rgb;
			// rgb = PushConstants.uCol.rgb;
			
			// rgb = ObjectConstants.albedo.rgb;
			break;

		default:
			rgb = vec3( 1., 1., 0. );
	}

	float depth = length(vCameraPos.xyz - vWorldPos);
	vec3 V = normalize(vCameraPos.xyz - vWorldPos);
	
	vec3 albedo = rgb;

	// albedo = pow(albedo, vec3(1.0/2.2));

	float metallic = material.metallic;
	float roughness = material.roughness;

	roughness *= roughness;
	
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, albedo, metallic);

	vec3 Lv = normalize(vec3(0.0, 100.0, 50.0));
	vec3 Lc = vec3(0.9686, 0.9765, 0.902);

	vec3 N = normalize(vN);
	vec3 L = normalize(uEnvironment.sun.direction.xyz);
	vec3 H = normalize(L+V);

	float NoH = saturate(dot(N, H));
	float LoH = saturate(dot(L, H));
	float NoL = saturate(dot(N, L)) + 0.05; // sus
	float NoV = abs(dot(N, V)) + 1e-5; // sus
	float HoV = saturate(dot(V, H));
	
	float D = filament_DGGX2(NoH, roughness);
	vec3  F = filament_Schlick(LoH, F0);
	float S = filament_SmithGGXCorrelated(NoV, NoL, roughness);

	vec3 r_env = SHIrradiance(reflect(V, N));
	vec3 env = SHIrradiance(N);

	r_env = vec3(1.0);
	// env = light_probe_irradiance(vWorldPos, probe_settings);


	env = light_probe_irradiance(vWorldPos, N, probe_settings);

	// vec3 env = vec3(1.0);
	// vec3 r_env = vec3(1.0);


	vec3 Fr = (D * S) * F; // specular lobe
	vec3 Fd = 10.0 * env * albedo * max(material.ao, NoL) * filament_Burley(roughness, NoV, NoL, LoH); // diffuse lobe


	vec3 ec = (vec3(1.0) - F0) * 0.725 + F0 * 0.07; // @hardcoded no idea what these should be
	Fr *= 0.0;

	if (lit_material > 0) {
		// rgb = 10.0 * max(NoL, material.ao) * (Fd + Fr * ec * r_env);
		rgb = Fd + Fr * ec;
	} else {
		rgb = albedo * material.emission;
	}

	rgb = apply_environment(rgb, depth, vCameraPos.xyz, V, uEnvironment);
	
	// rgb = V;
	// rgb = N;
 
	rgb = env;	
	rgb = env / 2.0f;	

	fFragColor = vec4( rgb, alpha * alpha * alpha);
}