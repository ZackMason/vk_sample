#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require


#include "sky.glsl"
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT RayData data;

void main()
{
    data.color = vec3(0,0,1);
    // data.color = sqrt(sky_color(data.normal, normalize(vec3(1,30,2)))); // breaks light probes
    data.color = vec3(0.8706, 0.8118, 0.6549);
    data.color = vec3(0.99, 0.98, 0.82);
    
    data.distance = -1.0;
}