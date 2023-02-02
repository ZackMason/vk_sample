#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "utl.glsl"
#include "pbr.glsl"

// Vulkan Sample Program Fragment Shader:

// also, can't specify packing
layout( std140, set = 0, binding = 0 ) uniform sporadicBuf
{
	int		uMode;
	int		uUseLighting;
	int		uNumInstances;
	int 	pad;
} Sporadic;
        
layout( std140, set = 1, binding = 0 ) uniform sceneBuf
{
	mat4		uProjection;
	mat4		uView;
	mat4		uSceneOrient;
	vec4		uLightPos; 
	vec4		uLightColor;
	vec4		uLightKaKdKs;
	float		uTime;
} Scene;

layout( std140, set = 2, binding = 0 ) uniform objectBuf
{
	mat4		uModel;
	vec4		uColor;
	float		uShininess;
} Object; 

layout( set = 3, binding = 0 ) uniform sampler2D uSampler;

layout ( push_constant ) uniform object_constants
{
	mat4 uM;
	vec4 albedo;

    float ao;
    float emission;
    float metallic;
    float roughness;

    uint flags;     // for material effects
    uint opt_flags; // for performance
    uint padding[2];
} ObjectConstants;

layout ( location = 0 ) in vec3 vColor;
layout ( location = 1 ) in vec2 vTexCoord;
layout ( location = 2 ) in vec3 vN;
layout ( location = 3 ) in vec3 vL;
layout ( location = 4 ) in vec3 vE;

layout ( location = 0 ) out vec4 fFragColor;

const vec3 WHITE = vec3( 1., 1., 1. );

void
main( )
{
	vec3 rgb;
	switch( Sporadic.uMode )
	{
		case 0:
			rgb = vColor;
			break;

		case 1:
			rgb = texture( uSampler, vTexCoord ).rgb;
			break;

		case 2:
			rgb = ObjectConstants.albedo.rgb;
			break;

		default:
			rgb = vec3( 1., 1., 0. );
	}

	if( Sporadic.uUseLighting != 0 )
	{
		// float ka = Scene.uLightKaKdKs.x;
		// float kd = Scene.uLightKaKdKs.y;
		// float ks = Scene.uLightKaKdKs.z;

		vec3 albedo = rgb;
		float metallic = ObjectConstants.metallic;
		float roughness = ObjectConstants.roughness * ObjectConstants.roughness;
		
		vec3 F0 = vec3(0.04); 
    	F0 = mix(F0, albedo, metallic);
	
		vec3 normal = normalize(vN);
		vec3 light  = normalize(vL);
		vec3 eye    = normalize(vE);
		vec3 H 		= normalize(light+eye);

		float NoH = saturate(dot(normal, H));
		float LoH = saturate(dot(light, H));
		float NoL = saturate(dot(normal,light));
		float NoV = saturate(dot(normal, eye));


		float NDF = filament_DGGX(NoH, roughness);
		float G = filament_SmithGGX(NoV, LoH, roughness);
		vec3 F = filament_schlick(F0, LoH);

		vec3 kD = filament_Burley(roughness, NoV, NoL, LoH) * albedo;
		kD *= 1.0 - metallic;

		rgb = (kD + (NDF*G*F)) * 2.0;

	// // a-d-s lighting equations:
	// 	vec3 ambient = ka * rgb;
	// 	float d = 0.;
	// 	float s = 0.;
	// 	if( dot(normal,light) > 0. )
	// 	{
	// 	        d = dot(normal,light);

	// 	        vec3 ref = reflect( -light, normal );
	// 	        if( dot(eye,ref) > 0. )
	// 	        {
	// 	                s = pow( dot(eye,ref), Object.uShininess );
	// 	        }
	// 	}
	// 	vec3 diffuse  = kd * d * rgb;
	// 	vec3 specular = ks * s * WHITE;
	// 	rgb = ambient + diffuse + specular;
	}

	fFragColor = vec4( rgb, 1. );
}
