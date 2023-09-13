#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "utl.glsl"
#include "rng.glsl"


layout(location = 2) rayPayloadInEXT bool shadowed;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0) uniform sampler2D uProbeSampler[2];
layout(binding = 2, set = 0) uniform sampler2D uTextureCache[4096];

#include "material.glsl"
layout(std430, set = 0, binding = 5, scalar) buffer ProbeBuffer {
	LightProbe probes[];
};

#include "raycommon.glsl"
#include "ddgi.glsl"

layout(location = 0) rayPayloadInEXT RayData data;

layout(set = 0, binding = 4, scalar) buffer MeshDesc_ { MeshDesc i[]; } uMeshDesc;

layout(std430, set = 0, binding = 6, scalar) readonly buffer ProbeSettingsBuffer {
	LightProbeSettings probe_settings;
};

layout(set = 0, binding = 8, scalar) readonly buffer EnvironmentBuffer {
	Environment uEnvironment;
};
layout(set = 0, binding = 9, scalar) readonly buffer PointLightBuffer {
	PointLight point_lights[];
};

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; // Triangle indices


hitAttributeEXT vec3 attribs;


void main()
{
    vec3 direction = data.surface.normal.xyz;
    MeshDesc mesh = uMeshDesc.i[gl_InstanceCustomIndexEXT];
    Vertices vertices = Vertices(mesh.vertex_ptr);
    Indices indices = Indices(mesh.index_ptr);

    const vec3 bary = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    ivec3 tri = indices.i[gl_PrimitiveID];

    Vertex v0 = vertices.v[tri.x];
    Vertex v1 = vertices.v[tri.y];
    Vertex v2 = vertices.v[tri.z];

    vec2 uv = (v0.t * bary.x + v1.t * bary.y + v2.t * bary.z);
    vec4 albedo = texture(uTextureCache[nonuniformEXT(mesh.texture_id%4096)], uv);

    vec3 n = normalize(v0.n * bary.x + v1.n * bary.y + v2.n * bary.z);
    vec3 p = (v0.p * bary.x + v1.p * bary.y + v2.p * bary.z);

    vec3 wp = (gl_ObjectToWorldEXT * vec4(p, 1.0)).xyz;
    vec3 wn = normalize((gl_ObjectToWorldEXT * vec4(n, 0.0)).xyz);

    vec3 L = normalize(uEnvironment.sun.direction.xyz);

    float tmin = 0.001;
    float tmax = 1000.0;
    vec3 origin = wp + wn * 0.001;
    float D = (gl_RayTminEXT + gl_HitTEXT) * sign(dot(-data.surface.normal, wn));
    data.surface.normal = wn;
    data.surface.point = wp;
    data.surface.albedo = albedo.rgb;

    shadowed = true;
    float shadow = 1.0;
    traceRayEXT(topLevelAS, 
        gl_RayFlagsTerminateOnFirstHitEXT | 
        // gl_RayFlagsOpaqueEXT | 
        gl_RayFlagsSkipClosestHitShaderEXT, 
        0xFF, 
        0, 
        0, 
        1, 
        origin, 
        tmin, 
        L, 
        tmax, 
        2);


    if (shadowed) {
        shadow = 0.0001530000113;

    }
    
    
    data.distance = 0.0;

    data.surface.normal = wn;
    data.surface.point = wp;
    data.surface.albedo = albedo.rgb;
    data.surface.emissive = vec3(0.0);
    Surface start_surface = data.surface;

    DirectionalLight sun = uEnvironment.sun;
    
    TotalLight light_solution = InitLightSolution();

    // directional_light(sun, data.surface, light_solution, shadow);

    // light_solution.direct.diffuse *= saturate(light_probe_irradiance(wp, wn, wn, probe_settings) * 0.295 * 3.14150);

    for (uint i = 0; i < uEnvironment.light_count; i++) {
        data.surface = start_surface;
        shadowed = true;
        shadow = 1.0;
        PointLight light = point_lights[i];

        // light to world
        // world to light
        L = -normalize(wp - point_lights[i].pos.xyz);
        float dist_to_light = max(0.0, distance(wp, point_lights[i].pos.xyz) - 0.1);
        float r2 = sqr(light.range);
        float d2 = sqr(dist_to_light);

        float nol = dot(L, wn);

        if (nol > 0.0) {
            traceRayEXT(topLevelAS, 
                gl_RayFlagsOpaqueEXT |
                gl_RayFlagsTerminateOnFirstHitEXT | 
                gl_RayFlagsSkipClosestHitShaderEXT, 
                0xFF, 
                0, 
                0, 
                1, 
                wp + wn * 0.03, 
                0.02, 
                L, 
                dist_to_light,
                2);
        
            if (shadowed) {
                // light_solution.direct.specular += vec3(1000,1000,0);
                shadow = 0.000000000000000001530000113;
            // } else {
                // shadow = 1.0;
            }
        }

        // light_solution.direct.diffuse += (30.0 * shadow * light.col.rgb * saturate(nol) * light.power) * point_light_attenuation(d2, r2);
        light_solution.direct.diffuse += saturate(nol) * sqr(light.col.rgb) * light.power * shadow * point_light_attenuation(d2, r2);
        // point_light(point_lights[i], start_surface, light_solution, shadow);

    }


    light_solution.indirect.diffuse += saturate(light_probe_irradiance(wp, direction.xyz, wn, probe_settings) * 0.00295 * probe_settings.boost);




    data.surface = start_surface;
    data.distance = D;
    data.light_solution = light_solution;
}