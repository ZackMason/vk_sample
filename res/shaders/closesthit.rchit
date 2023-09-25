#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "utl.glsl"
#include "rng.glsl"
#include "sky.glsl"


layout(location = 2) rayPayloadInEXT bool shadowed;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0) uniform sampler2D uProbeSampler[2];
layout(binding = 2, set = 0) uniform sampler2D uTextureCache[4096];

#define LIGHT_PROBE_SET_INDEX 0
#define LIGHT_PROBE_BINDING_INDEX 5
#define LIGHT_PROBE_SET_READ_WRITE readonly


#include "material.glsl"
#include "ddgi.glsl"

#include "engine.glsl"
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT RayData data;

layout(set = 0, binding = 4, scalar) readonly buffer GfxEntity_ { Entity e[]; } uEntityBuffer;
// layout(set = 0, binding = 4, scalar) buffer MeshDesc_ { MeshDesc i[]; } uMeshDesc;

layout(std430, set = 0, binding = 6, scalar) readonly buffer ProbeSettingsBuffer {
	LightProbeSettings probe_settings;
};

layout(set = 0, binding = 3, scalar) readonly buffer EnvironmentBuffer {
	Environment uEnvironment;
};
layout(set = 0, binding = 8, scalar) readonly buffer PointLightBuffer {
	PointLight point_lights[];
};

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; 
layout(buffer_reference, scalar) buffer Materials {Material m[]; };

hitAttributeEXT vec3 attribs;

layout( push_constant ) uniform constants
{
    mat4 kRandomRotation;
    uint64_t kScenePtr;
    uint kFrame;
    uint kSuperSample;
    // uint kScenePtr[2];
};

void main()
{
    vec3 direction = data.direction.xyz;
    // MeshDesc mesh = uMeshDesc.i[gl_InstanceCustomIndexEXT];
    Entity entity = uEntityBuffer.e[gl_InstanceCustomIndexEXT];
    Vertices vertices = Vertices(entity.vertex_start);
    Indices indices = Indices(entity.index_start);
    Material material = Materials(entity.material).m[0];

    uint triplanar_material = material.flags & MATERIAL_TRIPLANAR;


    const vec3 bary = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    ivec3 tri = indices.i[gl_PrimitiveID];

    Vertex v0 = vertices.v[tri.x];
    Vertex v1 = vertices.v[tri.y];
    Vertex v2 = vertices.v[tri.z];

    vec2 uv = (v0.t * bary.x + v1.t * bary.y + v2.t * bary.z);
    // vec4 albedo = vec4(1.0);

    vec3 n = normalize(v0.n * bary.x + v1.n * bary.y + v2.n * bary.z);
    vec3 p = (v0.p * bary.x + v1.p * bary.y + v2.p * bary.z);

    vec3 wp = (gl_ObjectToWorldEXT * vec4(p, 1.0)).xyz;
    vec3 wn = normalize((gl_ObjectToWorldEXT * vec4(n, 0.0)).xyz);


    vec4 albedo = triplanar_material != 0 ?
        texture_triplanar(uTextureCache[nonuniformEXT(entity.albedo%4096)], wp*0.5, wn) :
        texture(uTextureCache[nonuniformEXT(entity.albedo%4096)], uv);

    vec3 L = normalize(uEnvironment.sun.direction.xyz);

    float tmin = 0.001;
    float tmax = 1000.0;
    vec3 origin = wp + wn * 0.001;
    float D = (gl_RayTminEXT + gl_HitTEXT) * sign(dot(-data.direction, wn));

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
    data.color = vec3(0.0);

    Surface start_surface;
    start_surface.normal = wn;
    start_surface.point = wp;
    start_surface.albedo = (albedo.rgb * material.albedo.rgb);
    start_surface.emissive = material.emission * start_surface.albedo;

    // DirectionalLight sun = uEnvironment.sun;
    // sun.color.rgb = sqrt(sqrt(max(vec3(0.0), sky_color(direction.xyz, sun.direction.xyz))));
    
    TotalLight light_solution = InitLightSolution();

    // directional_light(sun, start_surface, light_solution, shadow);
                

    // light_solution.direct.diffuse *= saturate(light_probe_irradiance(wp, wn, wn, probe_settings) * 0.295 * 3.14150);

    for (uint i = 0; i < uEnvironment.light_count; i++) {
        shadowed = true;
        shadow = 1.0;
        PointLight light = point_lights[i];

        // light to world
        // world to light
        L = -normalize(wp - point_lights[i].pos.xyz);
        float dist_to_light = max(0.0, distance(wp, point_lights[i].pos.xyz) - 0.02);
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
                wp + wn * 0.02, 
                0.01, 
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

        light_solution.direct.diffuse += saturate(nol) * sqr(light.col.rgb) * light.power * shadow * point_light_attenuation(d2, r2);
    }



    if (kFrame != 0) {
        light_solution.direct.diffuse += max(vec3(0.0), light_probe_irradiance(wp, direction.xyz, wn, probe_settings) * 0.95 * probe_settings.boost);
    }

    apply_light(start_surface, light_solution, data.color);
    data.distance = D;

    // data.color
    // data.light_solution = light_solution;
}


