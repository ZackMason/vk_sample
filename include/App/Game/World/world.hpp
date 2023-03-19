#pragma once

#include "core.hpp"

#include "App/app.hpp"
#include "App/Game/Entity/entity.hpp"

namespace game {
    static constexpr u64 max_entities = 500000;

    // everything I need to rebuild game state should be in this struct
    struct world_t {
        app_t* app;

        arena_t arena;
        arena_t frame_arena[2];
        u64     frame_count{0};

        size_t                  entity_count{0};
        game::entity::entity_id next_entity_id{0};
        game::entity::entity_t  entities[65535];
        // game::entity::entity_t  entities[4096];

        game::entity::entity_t* player;

        game::cam::camera_t camera;

        phys::physics_world_t* physics_world{0};

        utl::rng::random_t<utl::rng::xor64_random_t> entropy;

        world_t() = default;
    };

    entity::entity_t* entity_itr(world_t* world) {
        return world->entities;
        // return world->entities.deque.front();
    }

    inline world_t*
    world_init(arena_t* arena, app_t* app, phys::physx_state_t& physx_state) {
        TIMED_FUNCTION;
        world_t* world = arena_alloc_ctor<world_t>(arena, 1);
        world->app = app;
        
        world->arena = arena_sub_arena(arena, megabytes(32));

        world->frame_arena[0] = arena_sub_arena(&world->arena, kilobytes(4));
        world->frame_arena[1] = arena_sub_arena(&world->arena, kilobytes(4));

        world->physics_world = arena_alloc<phys::physics_world_t>(&world->arena);

        phys::physics_world_init(physx_state, world->physics_world);

        return world;
    }

    inline game::entity::entity_t*
    world_create_entity(world_t* world) {
        TIMED_FUNCTION;
        assert(world->entity_count < array_count(world->entities));
        auto* e = world->entities + world->entity_count++;
        e->id = world->next_entity_id++;
        return e;
        // auto* e = world->entities.alloc(&world->arena);
        // return e;
    }

    inline void
    world_destroy_entity(world_t* world, game::entity::entity_t*& e) {
        TIMED_FUNCTION;
        assert(world->entity_count > 0);
        assert(e->id != uid::invalid_id);
        *e = world->entities[(world->entity_count - 1)];
        e = nullptr;
        world->entity_count--;
    }

    inline void
    world_update(world_t* world, f32 dt) {

    }
    
inline entity::entity_t*
spawn(
    world_t* world,
    utl::str_hash_t& mesh_hash,
    const entity::db::entity_def_t& def,
    const v3f& pos = {}
) {
    TIMED_FUNCTION;
    utl::res::pack_file_t* resource_file = world->app->resource_file;

    entity::entity_t* entity = world_create_entity(world);
    entity::entity_init(entity, utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data));
    entity->name.view(def.type_name);
    entity->transform.origin = pos;

    entity->type = def.type;
    entity->physics.flags = def.physics ? def.physics->flags : 0;
    entity->physics.rigid_body.type = def.physics ? def.physics->shape : phys::physics_shape_type::NONE;

    switch(entity->type) {
        case entity::entity_type::weapon:
            entity->flags |= entity::EntityFlags_Pickupable;
            break;
    }

    if (def.stats) {
        entity->stats.character_ = *def.stats;
    } else if (def.weapon) {
        entity->stats.weapon_ = *def.weapon;
    }

    if (def.physics) {
        if (def.physics->flags & entity::PhysicsEntityFlags_Character) {
            entity::player_init(
                entity,
                &world->camera, 
                utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data),
                world->physics_world
            );
            world->player = entity;
        } else if (def.physics->flags & entity::PhysicsEntityFlags_Static) {
            std::string collider_name;
            switch(def.physics->shape) {
                case phys::physics_shape_type::TRIMESH:
                    collider_name = fmt_str("{}.trimesh.physx", def.gfx.mesh_name.c_data);
                    break;
                case phys::physics_shape_type::CONVEX:
                    collider_name = fmt_str("{}.convex.physx", def.gfx.mesh_name.c_data);
                    break;
            }
            entity::physics_entity_init(
                entity,
                utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data),
                utl::res::pack_file_get_file_by_name(resource_file, collider_name),
                safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name)),
                world->physics_world->state
            );
            world->physics_world->scene->addActor(*entity->physics.rigid_body.rigid);
            return entity;
        } else if (def.physics->flags & entity::PhysicsEntityFlags_Dynamic) {
            switch(def.physics->shape) {
                case phys::physics_shape_type::CONVEX:{
                    const auto collider_name = fmt_str("{}.convex.physx", def.gfx.mesh_name.c_data);
                    entity::physics_entity_init(
                        entity,
                        utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data),
                        utl::res::pack_file_get_file_by_name(resource_file, collider_name),
                        safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name)),
                        world->physics_world->state
                    );
                    world->physics_world->scene->addActor(*entity->physics.rigid_body.dynamic);
                    return entity;
                    break;
                }
            }
        } 
    }
    return entity;
}

};