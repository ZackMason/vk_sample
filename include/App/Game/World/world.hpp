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
        size_t                  entity_capacity{0};
        game::entity::entity_id next_entity_id{1};
        game::entity::entity_t  entities[65535];
        game::entity::entity_t* entity_id_hash[4096];
        game::entity::entity_t* free_entities{0};
        // game::entity::entity_t* entity_spatial_hash[4096];
        // game::entity::entity_t  entities[4096];

        game::entity::entity_t* player;

        game::cam::camera_t camera;

        physics::api_t* physics{0};

        utl::rng::random_t<utl::rng::xor64_random_t> entropy;

        world_t() = default;
    };

    void add_entity_to_id_hash(world_t* world, entity::entity_t* entity) {
        const u64 id_hash = utl::rng::fnv_hash_u64(entity->id);
        const u64 id_bucket = id_hash % 4096;

        if (world->entity_id_hash[id_bucket]) {
            entity->next_id_hash = world->entity_id_hash[id_bucket];
        }
        world->entity_id_hash[id_bucket] = entity;
    }

    void remove_entity_from_id_hash(world_t* world, entity::entity_t* entity) {
        const u64 id_hash = utl::rng::fnv_hash_u64(entity->id);
        const u64 id_bucket = id_hash % 4096;

        if (world->entity_id_hash[id_bucket]) {
            entity::entity_t* last = nullptr;
            auto* c = world->entity_id_hash[id_bucket];
            while(c && c->id != entity->id) {
                last = c;
                c = c->next_id_hash;
            }
            if (last && c) {
                last->next_id_hash = c->next_id_hash;
            } else if (!last && c) {
                world->entity_id_hash[id_bucket] = c->next_id_hash;
            }
            c->next_id_hash = 0;
        } else {
            assert(0);
        }
    }

    entity::entity_t*
    find_entity_by_id(world_t* world, entity::entity_id id) {
        // TIMED_FUNCTION;
        const u64 id_hash = utl::rng::fnv_hash_u64(id);
        const u64 id_bucket = id_hash % 4096;

        if (world->entity_id_hash[id_bucket]) {
            auto* e = world->entity_id_hash[id_bucket];
            while(e && e->id != id) {
                e = e->next_id_hash;
            }
            return e;
        } 
        return nullptr;
    }

    entity::entity_t* entity_itr(world_t* world) {
        return world->entities;
        // return world->entities.deque.front();
    }

    inline world_t*
    world_init(arena_t* arena, app_t* app, physics::api_t* api) {
        TIMED_FUNCTION;
        world_t* world = arena_alloc_ctor<world_t>(arena, 1);
        world->app = app;
        world->physics = api;
        
        world->arena = arena_sub_arena(arena, megabytes(32));

        world->frame_arena[0] = arena_sub_arena(&world->arena, kilobytes(4));
        world->frame_arena[1] = arena_sub_arena(&world->arena, kilobytes(4));

        // world->physics_world = arena_alloc<phys::physics_world_t>(&world->arena);

        // phys::physics_world_init(physx_state, world->physics_world);

        return world;
    }

    inline game::entity::entity_t*
    world_create_entity(world_t* world) {
        TIMED_FUNCTION;
        assert(world);
        assert(world->entity_capacity < array_count(world->entities));

        if (world->free_entities) {
            game::entity::entity_t* e{0};
            node_pop(e, world->free_entities);
            world->entity_count++;
            assert(e);
            e->id = world->next_entity_id++;
            add_entity_to_id_hash(world, e);
            return e;
        } else {
            auto* e = world->entities + world->entity_capacity++;
            world->entity_count++;
            assert(e);
            e->id = world->next_entity_id++;
            add_entity_to_id_hash(world, e);
            return e;
        }
    }

    // TODO(Zack): Change this so that it maintains pointer
    inline void
    world_destroy_entity(world_t* world, game::entity::entity_t*& e) {
        TIMED_FUNCTION;
        assert(world->entity_count > 0);
        assert(e->id != uid::invalid_id);
        remove_entity_from_id_hash(world, e);

        constexpr bool maintain_references = true;
        if constexpr (maintain_references) {
            e->id = uid::invalid_id;
            node_push(e, world->free_entities);

        } else {
            *e = world->entities[(world->entity_count - 1)];
        }


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
    // entity->physics.rigid_body.type = def.physics ? def.physics->shape : physics::collider_shape_type::NONE;

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

    if (def.physics && def.physics->flags & entity::PhysicsEntityFlags_Character) {
        entity::player_init(
            entity,
            &world->camera, 
            utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data)
            // world->physics_world
        );
        world->player = entity;
    } if (def.physics) {
        physics::rigidbody_t* rb{0};
        if (def.physics->flags & entity::PhysicsEntityFlags_Static) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::STATIC);
        } else if (def.physics->flags & entity::PhysicsEntityFlags_Dynamic) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::DYNAMIC);
        }
        entity->physics.rigidbody = rb;
        rb->position = pos;
        rb->orientation = entity->global_transform().get_orientation();
        std::string collider_name;
        switch(def.physics->shape) {
            case physics::collider_shape_type::CONVEX: {
                collider_name = fmt_str("{}.convex.physx", def.gfx.mesh_name.c_data);
                physics::collider_convex_info_t ci;
                ci.mesh = utl::res::pack_file_get_file_by_name(resource_file, collider_name);
                ci.size = safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name));
                world->physics->create_collider(
                    world->physics,
                    rb, def.physics->shape, &ci
                );
            }   break;
            case physics::collider_shape_type::TRIMESH: {
                collider_name = fmt_str("{}.trimesh.physx", def.gfx.mesh_name.c_data);
                physics::collider_trimesh_info_t ci;
                ci.mesh = utl::res::pack_file_get_file_by_name(resource_file, collider_name);
                ci.size = safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name));
                world->physics->create_collider(
                    world->physics,
                    rb, def.physics->shape, &ci
                );
            }   break;
            case physics::collider_shape_type::SPHERE: {
                physics::collider_sphere_info_t ci;
                ci.radius = def.physics->shape_def.sphere.radius;
                world->physics->create_collider(
                    world->physics,
                    rb, def.physics->shape, &ci
                );
            }   break;
        }
        world->physics->set_rigidbody(0, rb);
    }
    // if (def.physics && false) {
    //     if (def.physics->flags & entity::PhysicsEntityFlags_Static) {
    //         std::string collider_name;
    //         switch(def.physics->shape) {
    //             case phys::physics_shape_type::TRIMESH:
    //                 collider_name = fmt_str("{}.trimesh.physx", def.gfx.mesh_name.c_data);
    //                 break;
    //             case phys::physics_shape_type::CONVEX:
    //                 collider_name = fmt_str("{}.convex.physx", def.gfx.mesh_name.c_data);
    //                 break;
    //         }
    //         entity::physics_entity_init(
    //             entity,
    //             utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data),
    //             utl::res::pack_file_get_file_by_name(resource_file, collider_name),
    //             safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name)),
    //             world->physics_world->state
    //         );
    //         world->physics_world->scene->addActor(*entity->physics.rigid_body.rigid);
    //         return entity;
    //     } else if (def.physics->flags & entity::PhysicsEntityFlags_Dynamic) {
    //         entity->physics.rigid_body.is_dynamic = true;
    //         switch(def.physics->shape) {
    //             case phys::physics_shape_type::SPHERE:{
    //                 entity::physics_entity_init(
    //                     entity,
    //                     utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data),
    //                     0,0,
    //                     world->physics_world->state
    //                 );
    //                 world->physics_world->scene->addActor(*entity->physics.rigid_body.dynamic);
    //                 return entity;
    //             }break;
    //             case phys::physics_shape_type::CONVEX:{
    //                 const auto collider_name = fmt_str("{}.convex.physx", def.gfx.mesh_name.c_data);
    //                 entity::physics_entity_init(
    //                     entity,
    //                     utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data),
    //                     utl::res::pack_file_get_file_by_name(resource_file, collider_name),
    //                     safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name)),
    //                     world->physics_world->state
    //                 );
    //                 world->physics_world->scene->addActor(*entity->physics.rigid_body.dynamic);
    //                 return entity;
    //                 break;
    //             }
    //         }
    //     } 
    // }
    return entity;
}

};