#pragma once

#include "core.hpp"

#include "App/Game/Entity/entity.hpp"

struct app_t;

namespace game {
    static constexpr u64 max_entities = 500000;

    struct world_t {
        app_t* app;

        arena_t* arena;
        arena_t frame_arena[2];
        u64     frame_count{0};

        // note(zack): this is UB
        struct entity_pools_t {
            utl::free_list_t<game::entity::entity_t> entities;
            utl::free_list_t<game::entity::physics_entity_t> physics_entities;
        } entity_pools; 
        game::entity::player_t player;

        game::cam::camera_t camera;

        world_t() = default;
    };

    constexpr size_t pool_count() {
        return sizeof(world_t::entity_pools_t) / sizeof(utl::free_list_t<game::entity::entity_t>);
    }

    auto* get_pool(world_t* world, size_t index) {
        assert(index < pool_count());
        return &((utl::free_list_t<game::entity::entity_t>*)(&world->entity_pools))[index];
    }
    entity::entity_t* pool_start(world_t* world, size_t index) {
        assert(index < pool_count());
        return ((utl::free_list_t<game::entity::entity_t>*)(&world->entity_pools))[index].deque.front();
    }

    inline world_t*
    world_init(arena_t* arena) {
        world_t* world = arena_alloc_ctor<world_t>(arena, 1);
        
        world->arena = arena;

        world->frame_arena[0] = arena_sub_arena(arena, kilobytes(4));
        world->frame_arena[1] = arena_sub_arena(arena, kilobytes(4));

        return world;
    }

    inline game::entity::entity_t*
    world_create_entity(world_t* world, u64 pool = 0) {
        auto* e = get_pool(world, pool)->alloc(world->arena);
        e->pool_id = pool;
        return e;
    }

    inline void
    world_destroy_entity(world_t* world, game::entity::entity_t*& e) {
        get_pool(world, e->pool_id)->free(e);
    }
    
inline entity::entity_t*
spawn(
    world_t* world,
    utl::str_hash_t& mesh_hash,
    const entity::db::entity_def_t& def,
    const v3f& pos = {}
) {
    if (def.physics) {
        if (def.physics->flags & entity::PhysicsEntityFlags_Character) {
            entity::player_init(
                &world->player, 
                &world->camera, 
                utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data)
            );
            return &world->player;
        } else if (def.physics->flags & entity::PhysicsEntityFlags_Static) {

        } else if (def.physics->flags & entity::PhysicsEntityFlags_Dynamic) {

        } 
        return world->entity_pools.physics_entities.deque.back();
    } else {
        auto* e = game::world_create_entity(world, 0);
        entity::entity_init(e, utl::str_hash_find(mesh_hash, def.gfx.mesh_name.c_data));
        e->name.view(def.type_name);
        return e;
    }
    return nullptr;
}

};