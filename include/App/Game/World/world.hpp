#pragma once

#include "zyy_core.hpp"

#include "App/game_state.hpp"
#include "App/Game/Entity/entity.hpp"
#include "App/Game/Rendering/render_system.hpp"

struct game_state_t;

struct world_generator_t;

namespace zyy {
    static constexpr u64 max_entities = 500000;

    // note(zack): everything I need to rebuild game state should be in this struct
    struct world_t {
        game_state_t* game_state;

        arena_t arena;
        frame_arena_t frame_arena;
        // arena_t frame_arena[2];
        // u64     frame_count{0};

        size_t          entity_count{0};
        size_t          entity_capacity{0};
        zyy::entity_id next_entity_id{1};
        zyy::entity_t  entities[65535]; // entities from [0,cap]
        zyy::entity_t* entity_id_hash[BIT(12)];
        zyy::entity_t* free_entities{0};

        zyy::entity_t* player;

        zyy::cam::camera_t camera;

        physics::api_t* physics{0};

        utl::rng::random_t<utl::rng::xor64_random_t> entropy;

        world_generator_t* world_generator{0};

        particle_cache_t* particle_cache{0};

        rendering::system_t* 
        render_system() {
            return game_state->render_system;
        }

        // world_t() = default;
    };

    inline world_t*
    world_init(arena_t* arena, game_state_t* game_state, physics::api_t* phys_api) {
        TIMED_FUNCTION;
        world_t* world = arena_alloc_ctor<world_t>(arena, 1);
        world->game_state = game_state;
        world->physics = phys_api;
        phys_api->user_world = world;
        
        world->arena = arena_sub_arena(arena, megabytes(256));

        world->frame_arena.arena[0] = arena_sub_arena(&world->arena, megabytes(64));
        world->frame_arena.arena[1] = arena_sub_arena(&world->arena, megabytes(64));

        // world->particle_cache = arena_alloc<particle_cache_t>(&world->arena);

        // world->particle_cache->instance_buffer = world->render_system()->instance_storage_buffer.pool.allocate(MAX_PARTICLE_SYSTEM_COUNT * 1024);

        return world;
    }

    inline world_t* 
    clone(world_t* src, arena_t* arena) noexcept {
        assert(src);

        world_t* new_world = world_init(arena, src->game_state, src->physics);

        new_world->entity_count = src->entity_count;
        new_world->entity_capacity = src->entity_capacity;
        new_world->next_entity_id = src->next_entity_id;

        new_world->camera = src->camera;
        
        new_world->entropy = src->entropy;

        // range_u64(i, 0, src->entity_capacity) {
        //     new_world->entities[i] = src->entities[i];
        // }

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
            new (e) entity_t;
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

    inline void
    world_update(world_t* world, f32 dt) {
        world->frame_arena.active += 1;
        arena_clear(&world->frame_arena.get());
    }

    inline void
    world_update_physics(world_t* world) {
        TIMED_FUNCTION;
        const auto* input = &world->game_state->game_memory->input;
        for (size_t i{0}; i < world->entity_count; i++) {
            auto* e = world->entities + i;
            if (e->physics.rigidbody && e->physics.flags & zyy::PhysicsEntityFlags_Kinematic) {
                // e->physics.rigidbody->position = e->global_transform().origin;
                // e->physics.rigidbody->orientation = e->global_transform().get_orientation();
                // float apply_gravity = f32(e->physics.flags & zyy::PhysicsEntityFlags_Character);
                // e->physics.rigidbody->integrate(input->dt, apply_gravity * 9.81f * 0.02f);
                world->physics->set_rigidbody(0, e->physics.rigidbody);
                e->transform.origin = e->physics.rigidbody->position;
                e->transform.basis = glm::toMat3(e->physics.rigidbody->orientation);
            }
        }
    }

    // Note(Zack): Has a flag so that it maintains pointers
    inline void
    world_destroy_entity(world_t* world, entity_t*& e) {
        TIMED_FUNCTION;
        assert(world->entity_count > 0);
        assert(e->id != uid::invalid_id);

        e->flags = EntityFlags_Dead;
        remove_entity_from_id_hash(world, e);

        if (e->physics.rigidbody) {
            world->physics->remove_rigidbody(world->physics, e->physics.rigidbody);
        }

        constexpr bool maintain_references = true;
        if constexpr (maintain_references) {
            e->id = uid::invalid_id;
            node_push(e, world->free_entities);
        } else {
            // *e = world->entities[(world->entity_count - 1)];
        }

        world->entity_count--;
        e = nullptr;
    }

    inline void
    world_kill_free_queue(world_t* world) {
        for (size_t i{0}; i < world->entity_capacity; i++) {
            auto* e = world->entities + i;
            if (e->flags & EntityFlags_Dying) {
                world_destroy_entity(world, e);
            }
        }
    }
    
inline entity_t*
spawn(
    world_t* world,
    rendering::system_t* rs,
    const db::prefab_t& def,
    const v3f& pos = {}
) {
    using namespace std::string_view_literals;
    TIMED_FUNCTION;
    utl::res::pack_file_t* resource_file = world->game_state->resource_file;

    entity_t* entity = world_create_entity(world);
    entity_init(entity, rendering::safe_get_mesh_id(rs, def.gfx.mesh_name));

    if (def.gfx.mesh_name != ""sv) {
        entity->aabb = rendering::get_mesh_aabb(rs, def.gfx.mesh_name);
    }
    if (def.type_name != ""sv) {
        entity->name.own(&world->game_state->string_arena, def.type_name);
    }
    entity->transform.origin = pos;

    entity->type = def.type;
    entity->physics.flags = def.physics ? def.physics->flags : 0;
    // entity->physics.rigid_body.type = def.physics ? def.physics->shape : physics::collider_shape_type::NONE;

    if (def.coroutine) {
        entity->coroutine.emplace(
            zyy::entity_coroutine_t{
                .coroutine={world->game_state->game_memory->input.time, (void*)entity}, 
                .func=def.coroutine
            }
        );
    }

    switch(entity->type) {
        case entity_type::weapon:
            entity->flags |= EntityFlags_Pickupable;
            break;
    }

    if (def.stats) {
        entity->stats.character = *def.stats;
    } else if (def.weapon) {
        entity->stats.weapon = *def.weapon;
    }

    if (def.physics && def.physics->flags & PhysicsEntityFlags_Character) {
        player_init(
            entity,
            &world->camera, 
            def.gfx.mesh_name != ""sv ? rendering::get_mesh_id(rs, def.gfx.mesh_name) : 0
        );
        world->player = entity;
    } 
    if (def.physics) {
        physics::rigidbody_t* rb{0};
        if (def.physics->flags & PhysicsEntityFlags_Character) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::CHARACTER, entity->transform.origin, entity->transform.get_orientation());
        } else if (def.physics->flags & PhysicsEntityFlags_Static) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::STATIC, entity->transform.origin, entity->transform.get_orientation());
        } else if (def.physics->flags & PhysicsEntityFlags_Kinematic) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::KINEMATIC, entity->transform.origin, entity->transform.get_orientation());
        } else if (def.physics->flags & PhysicsEntityFlags_Dynamic) {
            rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::DYNAMIC, entity->transform.origin, entity->transform.get_orientation());
        }
        assert(rb);
        entity->physics.rigidbody = rb;
        rb->position = pos;
        rb->orientation = entity->global_transform().get_orientation();
        std::string collider_name;
        for (size_t i = 0; i < array_count(def.physics->shapes); i++) {
            if (def.physics->shapes[i] == std::nullopt) continue;

            const auto& shape = def.physics->shapes[i]->shape;

            physics::collider_t* collider{nullptr};

            switch(shape){
                case physics::collider_shape_type::CONVEX: {
                    collider_name = fmt_str("{}.convex.physx", def.gfx.mesh_name);
                    physics::collider_convex_info_t ci;
                    ci.mesh = utl::res::pack_file_get_file_by_name(resource_file, collider_name);
                    if (ci.mesh == nullptr) {
                        zyy_warn(__FUNCTION__, "Error loading convex physics mesh: {}", collider_name);
                    } else {
                        ci.size = safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name));
                        collider = world->physics->create_collider(
                            world->physics,
                            rb, shape, &ci
                        );
                    }
                }   break;
                case physics::collider_shape_type::TRIMESH: {
                    collider_name = fmt_str("{}.trimesh.physx", def.gfx.mesh_name);
                    physics::collider_trimesh_info_t ci;
                    ci.mesh = utl::res::pack_file_get_file_by_name(resource_file, collider_name);
                    if (ci.mesh == nullptr) {
                        zyy_warn(__FUNCTION__, "Error loading trimesh physics mesh: {}", collider_name);
                    } else {
                        ci.size = safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name));
                        collider = world->physics->create_collider(
                            world->physics,
                            rb, shape, &ci
                        );
                    }
                }   break;
                case physics::collider_shape_type::SPHERE: {
                    physics::collider_sphere_info_t ci;
                    ci.radius = def.physics->shapes[i]->sphere.radius;
                    entity->aabb.expand(v3f{def.physics->shapes[i]->sphere.radius});
                    entity->aabb.expand(v3f{-def.physics->shapes[i]->sphere.radius});
                    collider = world->physics->create_collider(
                        world->physics,
                        rb, shape, &ci
                    );
                }   break;
                case physics::collider_shape_type::BOX: {
                    physics::collider_box_info_t ci;
                    ci.size = def.physics->shapes[i]->box.size;
                    entity->aabb.expand(def.physics->shapes[i]->box.size*0.5f);
                    entity->aabb.expand(def.physics->shapes[i]->box.size*-0.5f);
                    collider = world->physics->create_collider(
                        world->physics,
                        rb, shape, &ci
                    );
                }   break;
            }

            if (collider && (def.physics->shapes[i]->flags & 1)) {
                collider->set_trigger(true);
            }
        }
        world->physics->set_rigidbody(0, rb);
    }

    range_u64(i, 0, array_count(def.children)) {
        if (def.children[i].entity) {
            auto child = spawn(world, rs, *def.children[i].entity);
            entity->add_child(child);
            child->transform.origin = def.children[i].offset;
        }
    }

    return entity;
}

};