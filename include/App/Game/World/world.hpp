#pragma once

#include "core.hpp"

#include "App/Game/Entity/entity.hpp"

struct app_t;

namespace game {
    static constexpr u64 max_entities = 500000;

    struct world_t {
        app_t* app;

        arena_t arena;
        arena_t frame_arena[2];
        u64     frame_count{0};

        utl::free_list_t<game::entity::entity_t> entities;

        game::entity::entity_t player;

        game::cam::camera_t camera;

        phys::physics_world_t* physics_world{0};

        world_t() = default;
    };

    entity::entity_t* entity_itr(world_t* world) {
        return world->entities.deque.front();
    }

    inline world_t*
    world_init(arena_t* arena, phys::physx_state_t& physx_state) {
        world_t* world = arena_alloc_ctor<world_t>(arena, 1);
        
        world->arena = arena_sub_arena(arena, megabytes(256));

        world->frame_arena[0] = arena_sub_arena(&world->arena, kilobytes(4));
        world->frame_arena[1] = arena_sub_arena(&world->arena, kilobytes(4));

        world->physics_world = arena_alloc<phys::physics_world_t>(&world->arena);

        phys::physics_world_init(physx_state, world->physics_world);

        return world;
    }

    inline game::entity::entity_t*
    world_create_entity(world_t* world) {
        auto* e = world->entities.alloc(&world->arena);
        return e;
    }

    inline void
    world_destroy_entity(world_t* world, game::entity::entity_t*& e) {
        world->entities.free(e);
    }
    
inline entity::entity_t*
spawn(
    world_t* world,
    utl::str_hash_t& mesh_hash,
    const entity::db::entity_def_t& def,
    utl::res::pack_file_t* resource_file,
    const v3f& pos = {}
) {
    if (def.physics) {
        if (def.physics->flags & entity::PhysicsEntityFlags_Character) {
            entity::player_init(
                &world->player, 
                &world->camera, 
                utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data),
                world->physics_world
            );
            return &world->player;
        } else if (def.physics->flags & entity::PhysicsEntityFlags_Static) {
            switch(def.physics->shape) {
                case phys::physics_shape_type::TRIMESH:{
                    const auto collider_name = fmt_str("{}.trimesh.physx", def.gfx.mesh_name.c_data);
                    auto* e = world_create_entity(world);
                    entity::physics_entity_init_trimesh(
                        e,
                        utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data),
                        utl::res::pack_file_get_file_by_name(resource_file, collider_name),
                        safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name)),
                        world->physics_world->state
                    );
                    return e;
                    break;
                }
            }

        } else if (def.physics->flags & entity::PhysicsEntityFlags_Dynamic) {

        } 
        return world->entities.deque.back();
    } else {
        auto* e = game::world_create_entity(world);
        entity::entity_init(e, utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data));
        e->name.view(def.type_name);
        return e;
    }
    return nullptr;
}

};