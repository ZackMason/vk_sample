#pragma once

#include "core.hpp"

#include "App/app.hpp"
#include "App/Game/Entity/entity.hpp"
#include "App/Game/Rendering/render_system.hpp"

namespace game {
    static constexpr u64 max_entities = 500000;

    // note(zack): everything I need to rebuild game state should be in this struct
    struct world_t {
        app_t* app;

        arena_t arena;
        arena_t frame_arena[2];
        u64     frame_count{0};

        size_t                  entity_count{0};
        size_t                  entity_capacity{0};
        game::entity_id next_entity_id{1};
        game::entity_t  entities[65535];
        game::entity_t* entity_id_hash[BIT(12)];
        game::entity_t* free_entities{0};

        game::entity_t* player;

        game::cam::camera_t camera;

        physics::api_t* physics{0};

        utl::rng::random_t<utl::rng::xor64_random_t> entropy;

        // world_t() = default;
    };

    inline world_t*
    world_init(arena_t* arena, app_t* app, physics::api_t* phys_api) {
        TIMED_FUNCTION;
        world_t* world = arena_alloc_ctor<world_t>(arena, 1);
        world->app = app;
        world->physics = phys_api;
        
        world->arena = arena_sub_arena(arena, megabytes(32));

        world->frame_arena[0] = arena_sub_arena(&world->arena, kilobytes(4));
        world->frame_arena[1] = arena_sub_arena(&world->arena, kilobytes(4));

        return world;
    }

    inline world_t* 
    clone(world_t* src, arena_t* arena) noexcept {
        assert(src);

        world_t* new_world = world_init(arena, src->app, src->physics);

        new_world->entity_count = src->entity_count;
        new_world->entity_capacity = src->entity_capacity;
        new_world->next_entity_id = src->next_entity_id;

        new_world->camera = src->camera;
        
        new_world->entropy = src->entropy;

        range_u64(i, 0, src->entity_capacity) {
            new_world->entities[i] = src->entities[i];
        }

        return new_world;
    }

    inline void 
    add_entity_to_id_hash(world_t* world, entity_t* entity) {
        // todo(zack): better hash,
        const u64 id_hash = utl::rng::fnv_hash_u64(entity->id);
        const u64 id_bucket = id_hash & (array_count(world->entity_id_hash)-1);

        if (world->entity_id_hash[id_bucket]) {
            entity->next_id_hash = world->entity_id_hash[id_bucket];
        }
        world->entity_id_hash[id_bucket] = entity;
    }

    inline void 
    remove_entity_from_id_hash(world_t* world, entity_t* entity) {
        const u64 id_hash = utl::rng::fnv_hash_u64(entity->id);
        const u64 id_bucket = id_hash & (array_count(world->entity_id_hash)-1);

        if (world->entity_id_hash[id_bucket]) {
            entity_t* last = nullptr;
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

    inline static entity_t*
    find_entity_by_id(world_t* world, entity_id id) {
        // TIMED_FUNCTION;
        const u64 id_hash = utl::rng::fnv_hash_u64(id);
        const u64 id_bucket = id_hash & (array_count(world->entity_id_hash)-1);

        if (world->entity_id_hash[id_bucket]) {
            auto* e = world->entity_id_hash[id_bucket];
            while(e && e->id != id) {
                e = e->next_id_hash;
            }
            return e;
        } 
        return nullptr;
    }

    inline entity_t*
    world_create_entity(world_t* world) {
        TIMED_FUNCTION;
        assert(world);
        assert(world->entity_capacity < array_count(world->entities));

        if (world->free_entities) {
            entity_t* e{0};
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
    world_destroy_entity(world_t* world, entity_t*& e) {
        TIMED_FUNCTION;
        assert(world->entity_count > 0);
        assert(e->id != uid::invalid_id);
        remove_entity_from_id_hash(world, e);

        if (e->physics.rigidbody) {
            world->physics->remove_rigidbody(world->physics, e->physics.rigidbody);
        }

        constexpr bool maintain_references = true;
        if constexpr (maintain_references) {
            e->id = uid::invalid_id;
            node_push(e, world->free_entities);
        } else {
            *e = world->entities[(world->entity_count - 1)];
        }

        world->entity_count--;
        e = nullptr;
    }

    inline void
    world_update(world_t* world, f32 dt) {

    }
    
inline entity_t*
spawn(
    world_t* world,
    rendering::system_t* rs,
    const db::entity_def_t& def,
    const v3f& pos = {}
) {
    TIMED_FUNCTION;
    utl::res::pack_file_t* resource_file = world->app->resource_file;

    entity_t* entity = world_create_entity(world);
    entity_init(entity, rendering::get_mesh_id(rs, def.gfx.mesh_name));
    entity->aabb = rendering::get_mesh_aabb(rs, def.gfx.mesh_name);
    entity->name.own(&world->app->string_arena, def.type_name);
    entity->transform.origin = pos;

    entity->type = def.type;
    entity->physics.flags = def.physics ? def.physics->flags : 0;
    // entity->physics.rigid_body.type = def.physics ? def.physics->shape : physics::collider_shape_type::NONE;

    switch(entity->type) {
        case entity_type::weapon:
            entity->flags |= EntityFlags_Pickupable;
            break;
    }

    if (def.stats) {
        entity->stats.character_ = *def.stats;
    } else if (def.weapon) {
        entity->stats.weapon_ = *def.weapon;
    }

    if (def.physics && def.physics->flags & PhysicsEntityFlags_Character) {
        player_init(
            entity,
            &world->camera, 
            rendering::get_mesh_id(rs, def.gfx.mesh_name)
        );
        world->player = entity;
    } 
    if (def.physics) {
        physics::rigidbody_t* rb{0};
        if (def.physics->flags & PhysicsEntityFlags_Character) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::CHARACTER);
        } else if (def.physics->flags & PhysicsEntityFlags_Static) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::STATIC);
        } else if (def.physics->flags & PhysicsEntityFlags_Dynamic) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::DYNAMIC);
        }
        assert(rb);
        entity->physics.rigidbody = rb;
        rb->position = pos;
        rb->orientation = entity->global_transform().get_orientation();
        std::string collider_name;
        switch(def.physics->shape) {
            case physics::collider_shape_type::CONVEX: {
                collider_name = fmt_str("{}.convex.physx", def.gfx.mesh_name);
                physics::collider_convex_info_t ci;
                ci.mesh = utl::res::pack_file_get_file_by_name(resource_file, collider_name);
                ci.size = safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name));
                world->physics->create_collider(
                    world->physics,
                    rb, def.physics->shape, &ci
                );
            }   break;
            case physics::collider_shape_type::TRIMESH: {
                collider_name = fmt_str("{}.trimesh.physx", def.gfx.mesh_name);
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
    return entity;
}

};