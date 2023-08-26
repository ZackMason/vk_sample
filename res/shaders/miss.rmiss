#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require


#include "sky.glsl"
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT RayData data;

void main()
{
    // data.color = vec3(0.702, 0.2, 0.5765);
    data.color = vec3(0,0,1);
    data.color = sky_color(data.normal, normalize(vec3(1,2,3)));
    data.distance = -1.0;
}