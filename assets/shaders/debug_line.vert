#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

// Vulkan Sample Program Vertex Shader:
        
layout( std140, set = 0, binding = 0 ) uniform sceneBuf
{
	mat4		uProjection;
	mat4		uView;
	mat4		uSceneOrient;
	vec4		uLightPos;			// x, y, z
	vec4		uLightColor;		// r, g, b
	vec4		uLightKaKdKs;		// ka, kd, ks
	float	    uTime;
} Scene;

layout( location = 0 ) in vec3 aVertex;
layout( location = 1 ) in vec3 aColor;

layout ( location = 0 ) out vec3 vColor;

void main() {
	mat4  P = Scene.uProjection;
	mat4  V = Scene.uView;
	mat4  SO = Scene.uSceneOrient;
	
	mat4 VM = V * mat4(1.0f);
	mat4 PVM = P * VM;

	vColor    = aColor;
	
	gl_Position = PVM * vec4( aVertex, 1. );
}
