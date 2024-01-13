#ifndef PRCGEN_TERRAIN_HPP
#define PRCGEN_TERRAIN_HPP

#include "ztd_core.hpp"

namespace prcgen::terrain {

    inline void
    make_terrain(gfx::mesh_builder_t* builder, f32 terrain_dim, f32 terrain_grid_size) {
        for (f32 x = -terrain_dim; x < terrain_dim; x += terrain_grid_size) {
            for (f32 y = -terrain_dim; y < terrain_dim; y += terrain_grid_size) {
                gfx::vertex_t v[4];
                v[0].pos = v3f{x                    , 0.0f, y};
                v[1].pos = v3f{x                    , 0.0f, y + terrain_grid_size};
                v[2].pos = v3f{x + terrain_grid_size, 0.0f, y};
                v[3].pos = v3f{x + terrain_grid_size, 0.0f, y + terrain_grid_size};

                constexpr f32 noise_scale = 0.2f;
                for (size_t i = 0; i < 4; i++) {
                    v[i].pos.y -= utl::noise::noise21(v2f{v[i].pos.x, v[i].pos.z} * noise_scale) * 4.0f;
                    v[i].col = gfx::color::v3::dirt * 
                        (utl::noise::noise21(v2f{v[i].pos.x, v[i].pos.z} * noise_scale) 
                        * 0.5f + 0.5f);
                }

                const v3f n = glm::normalize(glm::cross(v[1].pos - v[0].pos, v[2].pos - v[0].pos));
                for (size_t i = 0; i < 4; i++) {
                    v[i].nrm = n;
                }
                
                // builder->add_quad(v);
            }
        }

    // game_state->mesh_cache.add(
    //     &game_state->mesh_arena,
    //     mesh_builder.build(&game_state->mesh_arena)
    // );
    // utl::str_hash_create(game_state->mesh_hash);
    // utl::str_hash_create(game_state->mesh_hash_meta);

    // utl::str_hash_add(game_state->mesh_hash, "ground", 0);    
    // rendering::add_mesh(game_state->render_system, "ground", game_state->mesh_cache.get(0));

    }
};

#endif