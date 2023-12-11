#ifndef UTIL_LOADING_HPP
#define UTIL_LOADING_HPP

#include "zyy_core.hpp"

inline gfx::mesh_list_t
load_bin_mesh_data(
    arena_t* arena,
    std::byte* data,
    utl::allocator_t* vertices,
    utl::allocator_t* indices    
) {
    gfx::mesh_list_t results;

    utl::memory_blob_t blob{data};

    // read file meta data
    const auto meta = blob.deserialize<u64>();
    const auto vers = blob.deserialize<u64>();
    const auto mesh = blob.deserialize<u64>();

    assert(meta == utl::res::magic::meta);
    assert(vers == utl::res::magic::vers);
    assert(mesh == utl::res::magic::mesh);

    results.count = blob.deserialize<u64>();
    zyy_warn(__FUNCTION__, "Loading {} meshes", results.count);
    tag_array(results.meshes, gfx::mesh_view_t, arena, results.count);
    // results.meshes  = push_struct<gfx::mesh_view_t>(arena, results.count);

    for (size_t i = 0; i < results.count; i++) {
        std::string name = blob.deserialize<std::string>();
        zyy_info(__FUNCTION__, "Mesh name: {}", name);
        const size_t vertex_count = blob.deserialize<u64>();
        const size_t vertex_bytes = sizeof(gfx::vertex_t) * vertex_count;

        // read vertices
        auto* v = (gfx::vertex_t*)vertices->allocate(vertex_bytes);
        const u32 vertex_start = safe_truncate_u64(v - (gfx::vertex_t*)vertices->arena.start);

        utl::copy(v, blob.read_data(), vertex_bytes);
        blob.advance(vertex_bytes);

        // read indices
        const size_t index_count = blob.deserialize<u64>();
        const size_t index_bytes = sizeof(u32) * index_count;

        auto* tris = (u32*)indices->allocate(index_bytes);

        utl::copy(tris, blob.read_data(), index_bytes);
        blob.advance(index_bytes);

        const u32 index_start = safe_truncate_u64(tris - (u32*)indices->arena.start);

        results.meshes[i].vertex_start = vertex_start;
        results.meshes[i].vertex_count = safe_truncate_u64(vertex_count);
        results.meshes[i].index_start = index_start;
        results.meshes[i].index_count = safe_truncate_u64(index_count);

        new (&results.meshes[i].aabb) math::rect3d_t();
        range_u64(j, 0, vertex_count) {
            results.meshes[i].aabb.expand(v[j].pos);
        }
    }
    const auto mate = blob.deserialize<u64>();
    assert(mate == utl::res::magic::mate);

    for (size_t i = 0; i < results.count; i++) {
        const auto& material = results.meshes[i].material = blob.deserialize<gfx::material_info_t>();
        zyy_info(__FUNCTION__, "Loaded material: {}, albedo: {}, normal: {}", material.name, material.albedo, material.normal);
    }

    results.aabb = {};
    range_u64(m, 0, results.count) {
        results.aabb.expand(results.meshes[m].aabb);
    }
    
    return results;
}

inline gfx::mesh_list_t
load_bin_mesh(
    arena_t* arena,
    utl::res::pack_file_t* packed_file,
    std::string_view path,
    // utl::pool_t<gfx::vertex_t>* vertices,
    utl::allocator_t* vertices,
    utl::allocator_t* indices
) {
    const size_t file_index = utl::res::pack_file_find_file(packed_file, path);
    
    std::byte* file_data = utl::res::pack_file_get_file(packed_file, file_index);

    
    if (file_data) {
        gfx::mesh_list_t results = load_bin_mesh_data(arena, file_data, vertices, indices);
    } else {
        zyy_warn("game_state::load_bin_mesh", "Failed to open mesh file: {}", path);
    }

    return {};
}


#endif