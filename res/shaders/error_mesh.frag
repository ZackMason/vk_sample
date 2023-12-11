#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

#include "utl.glsl"
#include "pbr.glsl"

// Vulkan Sample Program Fragment Shader:

#define LIGHTING_INFO_PROBE_FLAG 1

layout( std140, set = 0, binding = 0 ) uniform sporadicBuf
{
	int		uMode;
	uint	uLightInfo;
	int		uNumInstances;
	float 	uTime;
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
	vec4		uColor;
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
layout ( location = 3 ) in vec3 vE;
layout ( location = 4 ) in vec3 vWorldPos;

layout ( location = 0 ) out vec4 fFragColor;

void
main( )
{
	vec3 rgb = vec3(1.0, 0.0, 1.0);
	
	fFragColor = vec4( rgb, 1. );
}
