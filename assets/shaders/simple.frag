#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable


// Vulkan Sample Program Fragment Shader:

// also, can't specify packing
layout( std140, set = 0, binding = 0 ) uniform sporadicBuf
{
	int		uMode;
	int		uUseLighting;
	int		uNumInstances;
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
	mat4		uNormal;
	vec4		uColor;
	float		uShininess;
} Object; 

layout( set = 3, binding = 0 ) uniform sampler2D uSampler;

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

		default:
			rgb = vec3( 1., 1., 0. );
	}

	if( Sporadic.uUseLighting != 0 )
	{
		float ka = Scene.uLightKaKdKs.x;
		float kd = Scene.uLightKaKdKs.y;
		float ks = Scene.uLightKaKdKs.z;

		vec3 normal = normalize(vN);
		vec3 light  = normalize(vL);
		vec3 eye    = normalize(vE);

	// a-d-s lighting equations:
		vec3 ambient = ka * rgb;
		float d = 0.;
		float s = 0.;
		if( dot(normal,light) > 0. )
		{
		        d = dot(normal,light);

		        vec3 ref = reflect( -light, normal );
		        if( dot(eye,ref) > 0. )
		        {
		                s = pow( dot(eye,ref), Object.uShininess );
		        }
		}
		vec3 diffuse  = kd * d * rgb;
		vec3 specular = ks * s * WHITE;
		rgb = ambient + diffuse + specular;
	}

	fFragColor = vec4( rgb, 1. );
}
