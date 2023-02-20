#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

// Vulkan Sample Program Vertex Shader:

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
	vec4		uLightPos;			// x, y, z
	vec4		uLightColor;		// r, g, b
	vec4		uLightKaKdKs;		// ka, kd, ks
	float	    uTime;
} Scene;

layout( std140, set = 2, binding = 0 ) uniform objectBuf
{
	vec4		uColor;
} Object; 

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

layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in vec3 aColor;
layout( location = 3 ) in vec2 aTexCoord;

layout ( location = 0 ) out vec3 vColor;
layout ( location = 1 ) out vec2 vTexCoord;
layout ( location = 2 ) out vec3 vN;
layout ( location = 3 ) out vec3 vE;
layout ( location = 4 ) out vec3 vWorldPos;

void
main() {
	mat4  P = Scene.uProjection;
	mat4  V = Scene.uView;
	mat4  SO = Scene.uSceneOrient;
	mat4  M  = ObjectConstants.uM;
	mat4 VM = V * SO * M;
	mat4 PVM = P * VM;

	vColor = aColor;
	vTexCoord = aTexCoord;

	vE = vec3(
		V[3][0],
		V[3][1],
		V[3][2]
	);
	vWorldPos = (M * vec4(aVertex, 1.0)).xyz;
	vE = normalize(vWorldPos - vE);

	vN = normalize(mat3(M) * aNormal);

	gl_Position = PVM * vec4(aVertex, 1. );
}
