#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "raycommon.glsl"


layout(location = 0) rayPayloadInEXT RayData data;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform sampler2D uTextureCache[4096];

layout(set = 0, binding = 4, scalar) buffer MeshDesc_ { MeshDesc i[]; } uMeshDesc;

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; // Triangle indices
// layout(buffer_reference, scalar) buffer Materials {WaveFrontMaterial m[]; }; // Array of all materials on an object
// layout(buffer_reference, scalar) buffer MatIndices {int i[]; }; // Material ID for each triangle

hitAttributeEXT vec3 attribs;

void main()
{
    MeshDesc mesh = uMeshDesc.i[gl_InstanceCustomIndexEXT];
    Vertices vertices = Vertices(mesh.vertex_ptr);
    Indices indices = Indices(mesh.index_ptr);

    const vec3 bary = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    ivec3 tri = indices.i[gl_PrimitiveID];

    Vertex v0 = vertices.v[tri.x];
    Vertex v1 = vertices.v[tri.y];
    Vertex v2 = vertices.v[tri.z];

    vec3 n = normalize(v0.n * bary.x + v1.n * bary.y + v2.n * bary.z);
    vec3 p = (v0.p * bary.x + v1.p * bary.y + v2.p * bary.z);
    vec2 uv = (v0.t * bary.x + v1.t * bary.y + v2.t * bary.z);

    vec3 wp = (gl_ObjectToWorldEXT * vec4(p, 1.0)).xyz;
    vec3 wn = (gl_ObjectToWorldEXT * vec4(n, 0.0)).xyz;

    vec3 albedo = texture(uTextureCache[nonuniformEXT(mesh.texture_id%4096)], uv).rgb;
    
    data.distance = gl_RayTmaxEXT;
    data.normal = n;
    
	data.reflector = 1.0;// (mesh.texture_id == -1 ? 1.0f : 0.0f);
    data.color = vec3(tri) / vec3(tri+1);

    data.color = bary;

    data.color = wn;
    data.color = albedo;
    // data.color = vec3(0,1,0);
}