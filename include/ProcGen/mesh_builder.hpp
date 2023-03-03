#ifndef MESH_BUILDER_HPP
#define MESH_BUILDER_HPP

#include "core.hpp"

#include "ProcGen/marching_cubes.hpp"

namespace prcgen {


struct mesh_builder_t {
    utl::pool_t<gfx::vertex_t>& vertices;
    utl::pool_t<u32>& indices;

    mesh_builder_t(
        utl::pool_t<gfx::vertex_t>& vertices_,
        utl::pool_t<u32>& indices_
    ) : vertices{vertices_}, indices{indices_} {
        vertex_start = safe_truncate_u64(vertices_.count);
        index_start = safe_truncate_u64(indices_.count);
    }

    u32 vertex_start{0};
    u32 vertex_count{0};
    u32 index_start{0};
    u32 index_count{0};

    gfx::mesh_list_t
    build(arena_t* arena) {
        gfx::mesh_list_t m;
        m.count = 1;
        m.meshes = arena_alloc_ctor<gfx::mesh_view_t>(arena, 1);
        m.meshes[0].vertex_start = vertex_start;
        m.meshes[0].vertex_count = vertex_count;
        m.meshes[0].index_start = index_start;
        m.meshes[0].index_count = index_count;
        return m;
    }

    mesh_builder_t& add_vertex(
        v3f pos,
        v2f uv = v2f{0.0f, 0.0f},
        v3f nrm = gfx::color::v3::green,
        v3f col = gfx::color::v3::white
    ) {
        *vertices.allocate(1) = gfx::vertex_t{.pos = pos, .nrm = nrm, .col = col, .tex = uv};
        vertex_count++;
        return *this;
    }
    mesh_builder_t& add_vertex(gfx::vertex_t&& vertex) {
        *vertices.allocate(1) = std::move(vertex);
        vertex_count++;
        return *this;
    }
    mesh_builder_t& add_index(u32 index) {
        *indices.allocate(1) = index;
        index_count++;
        return *this;
    }


    mesh_builder_t& add_triangles(math::triangle_t* tris, u64 count) {
        loop_iota_u64(i, count) {
            auto* v = vertices.allocate(3);

            const auto [t0, t1, t2] = tris[i].p;

            const v3f norm = glm::normalize(glm::cross(t1-t0,t2-t0));

            v[0].pos = t0;
            v[1].pos = t1;
            v[2].pos = t2;

            loop_iota_u64(j, 3) {
                v[j].nrm = norm;
                v[j].col = norm;
                v[j].tex = v2f{0.0}; // todo(zack): triplanar
            }
        }

        vertex_count += safe_truncate_u64(count) * 3;
        return *this;
    }

    mesh_builder_t& add_box(math::aabb_t<v3f> box) {
        math::triangle_t tris[32];
        const auto s = box.size();
        const auto p = box.min;
        const auto sx = s.x;
        const auto sy = s.y;
        const auto sz = s.z;
        const auto sxv = v3f{s.x, 0.0f, 0.0f};
        const auto syv = v3f{0.0f, s.y, 0.0f};
        const auto szv = v3f{0.0f, 0.0f, s.z};
        u64 i = 0;
        tris[i++].p = {p, p + sxv, p + sxv + syv};
        tris[i++].p = {p, p + syv + sxv, p + syv};

        tris[i++].p = {p, p + szv + sxv, p + szv};
        tris[i++].p = {p, p + szv + sxv, p + szv};
        
        tris[i++].p = {p, p + szv + syv, p + szv};
        tris[i++].p = {p, p + szv + syv, p + szv};
        
        return add_triangles(tris, 6);
    }
    mesh_builder_t& add_quad(gfx::vertex_t vertex[4]) {
        u32 v_start = safe_truncate_u64(vertices.count);
        gfx::vertex_t* v = vertices.allocate(6);
        v[0] = vertex[0];
        v[1] = vertex[1];
        v[2] = vertex[2];
        
        v[3] = vertex[2];
        v[4] = vertex[1];
        v[5] = vertex[3];
        // u32* tris = indices.allocate(6);

        // tris[0] = v_start + 0;
        // tris[1] = v_start + 1;
        // tris[2] = v_start + 2;

        // tris[3] = v_start + 1;
        // tris[4] = v_start + 3;
        // tris[5] = v_start + 2;

        vertex_count += 6;
        // index_count += 6;
        return *this;
    }

};

};

#endif
