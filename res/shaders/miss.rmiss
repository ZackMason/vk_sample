#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable

#include "sky.glsl"
#include "utl.glsl"
#include "lighting.glsl"
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT RayData data;

layout(std430, set = 0, binding = 8, scalar) readonly buffer EnvironmentBuffer {
	Environment uEnvironment;
};

void main()
{
    // data.color = vec3(0,0,1);
    // data.color = vec3(0.8706, 0.8118, 0.6549);
    // data.color = vec3(0.1961, 0.451, 0.9255);
    // data.color = sqrt(sqrt(sky_color(data.normal, normalize(vec3(1,30,2))))); // breaks light probes
    vec3 L = normalize(uEnvironment.sun.direction.xyz);
    data.light_solution.direct.diffuse = sqrt(sqrt(sky_color(data.surface.normal, L))); // breaks light probes
    // data.color = vec3(0.99, 0.98, 0.82);
    
    data.distance = gl_RayTmaxEXT + 1.0;
}