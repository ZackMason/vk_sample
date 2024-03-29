#version 460
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable

#include "noise.glsl"
#include "utl.glsl"
#include "pbr.glsl"
#include "sky.glsl"

layout( set = 4, binding = 0 ) uniform sampler2D uSampler[4096];
// layout( set = 4, binding = 1 ) uniform sampler2D uProbeTexture[3];
layout( set = 4, binding = 1 ) uniform sampler2D uProbeSampler[3];

#define LIGHT_PROBE_SET_READ_WRITE readonly
#define LIGHT_PROBE_SET_INDEX 5
#define LIGHT_PROBE_BINDING_INDEX 0

#include "material.glsl"
#include "ddgi.glsl"

#define LIGHTING_INFO_PROBE_FLAG 1

layout( std140, set = 0, binding = 0 ) uniform sporadicBuf
{
	int		uMode;
	uint	uLightInfo;
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

layout(std430, set = 3, binding = 0, scalar) readonly buffer EnvironmentBuffer {
	Environment uEnvironment;
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
layout ( location = 4 ) in vec4 vDrawColor;

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

vec3 screenspace_dither(vec2 uv) {
    vec3 vDither = vec3( dot( vec2( 171.0, 231.0 ), uv.xy ) );
    vDither.rgb = fract( vDither.rgb / vec3( 103.0, 71.0, 97.0 ) );
    
    return vDither.rgb / 255.0;
}

vec3 paletize(vec3 rgb, float n) {
    return floor(rgb*(n-1.0)+0.5)/(n-1);
}

void
main( )
{
	TotalLight light_solution = InitLightSolution();

	vec3 rgb = vec3(0.0);
	vec4 base_color = vDrawColor;

	Material material = uMaterialBuffer.materials[vMatId];

	uint lit_material = material.flags & MATERIAL_LIT;
	uint water_material = material.flags & MATERIAL_WATER;
	float reflectivity = material.reflectivity;

	uint triplanar_material = material.flags & MATERIAL_TRIPLANAR;
	bool additive_emission = (material.flags & MATERIAL_EMISSIVE_ADDITIVE) > 0;
	int mode = Sporadic.uMode;

	mode = mode == 1 && vAlbedoId == (0xffffffff % 4096) ? 0 : mode;
	float alpha = 1.0f;
	switch(mode)
	{
		case 0:
			base_color *= material.albedo;
			break;

		case 1:
			// rgb = texture( uSampler[1797], vTexCoord ).rgb;
			vec4 albedo = (triplanar_material > 0) ? 
				texture_triplanar(uSampler[vAlbedoId], vWorldPos * 0.5, vN).rgba :
				texture( uSampler[vAlbedoId], vTexCoord ).rgba;
			base_color *= albedo * material.albedo;
			// alpha = albedo.a * material.albedo.a;
			break;

		case 2:
			base_color *= material.albedo;
			// rgb = PushConstants.uCol.rgb;
			
			// rgb = ObjectConstants.albedo.rgb;
			break;

		default:
			rgb = vec3( 1., 1., 0. );
	}

	float depth = length(vCameraPos.xyz - vWorldPos);

	if (water_material != 0) {
		// float water_noise = fbm(vWorldPos.xz/4.0 - Sporadic.uTime * 0.1, 4)
		// 	* fbm(vWorldPos.xz/2.0 + Sporadic.uTime * 0.25, 3);

		// light_solution.indirect.diffuse = vec3(0.2, 0.23, 0.4) ;
		vec2 water_uv = vTexCoord * 10.0;
		if (triplanar_material > 0) {
			water_uv = vWorldPos.xz * 2.0f;
		}

		float water_dist = saturate(depth/25.0f);
		float water_noise = cell(water_uv, Sporadic.uTime);
		water_noise *= water_noise;
		// water_noise = mix(water_noise, 1.0, saturate(depth/25.0f));
		// water_noise = smoothstep(0.2, 0.25, water_noise);

		base_color.rgb = mix(material.albedo.rgb * 1.4, material.albedo.rgb * 1.6, water_noise);
		alpha = 1.0;
		// rgb = mix(material.albedo.rgb * 1.4, material.albedo.rgb * 1.6, water_noise * (1.0 - water_dist));
		// float steps = 24.0;
		// rgb = paletize(rgb, steps);
	}


	vec3 V = normalize(vCameraPos.xyz - vWorldPos);
	
	vec3 albedo = base_color.rgb;
	alpha = base_color.a;
	
	// if (alpha < 0.15) { discard; }
	
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

	// pbr camera
	float aperture = .010;
	float shutter_speed = 0.05;
	float sensitivity = 1.0;
	float ev100 = exposure_settings(aperture, shutter_speed, sensitivity);
	float expo = exposure(ev100);

	Surface surface;
	surface.point = vWorldPos;
	surface.normal = N;
	surface.view = V;

	surface.f0 = F0;
	surface.base_color = vec4(1);
	surface.alpha = alpha;
	surface.albedo = (albedo.rgb);
	surface.roughness = roughness;
	surface.metallic = metallic;
	surface.emissive = sqr(surface.albedo) * step(0.5, material.emission) * 100.0;
	surface.occlusion = 1.0;

	// light_solution.direct.diffuse += max(0.0, NoL);

	rgb = vec3(0.0);
	if (lit_material > 0) 
	{
		if ((Sporadic.uLightInfo & 1) == 1) {
			light_solution.indirect.diffuse += light_probe_irradiance(vWorldPos, V, N, probe_settings) * 1.0;
		} else {
			float nol = saturate(dot(surface.normal, L));
			nol = nol * 0.7 + 0.3;
			light_solution.direct.diffuse = surface.albedo * max(nol, uEnvironment.ambient_strength) + uEnvironment.ambient_color.rgb;
		}

		apply_light(surface, light_solution, rgb);
		// rgb *= expo;
	} else {
		apply_light(surface, light_solution, rgb);
		// rgb = (surface.albedo) * (material.emission*40.0);// * pow(2, ev100 + material.emission - 3.0) * expo;
		// alpha = base_color.a;
		// rgb = sqr(surface.emissive) * 10.0;
		// rgb = material.albedo * material.emission * 1.0;
		// rgb = sqr(rgb);
	}
	
	// if (false)
	{
		vec3 reflected_view = reflect(-V, N);

    	rgb += reflectivity * sqrt(sqrt(max(vec3(0.0), sky_color(reflected_view, L)))); 
	}

	if (water_material != 0) {
		float steps = 24.0;
		rgb = paletize(rgb, steps);
	}

	rgb = apply_environment(rgb, depth, vCameraPos.xyz, V, uEnvironment);
	
	// rgb = V;

	// rgb = N;
 
	// rgb = env;	



	// rgb = env / 2.0f;	
		
	// alpha = 1.0;// saturate(alpha);
	alpha = saturate(alpha);

	fFragColor = vec4( rgb, (alpha));
	fFragColor = vec4( rgb, alpha * alpha);
}















