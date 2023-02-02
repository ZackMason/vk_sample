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

        struct entity_pools_t {
                utl::free_list_t<game::entity::entity_t> entities;
                utl::free_list_t<game::entity::entity_t> badentities;
        } entity_pools;
        game::entity::player_t player;

        world_t() = default;
    };

    constexpr size_t pool_count() {
        return sizeof(world_t::entity_pools_t) / sizeof(utl::free_list_t<game::entity::entity_t>);
    }

    entity::entity_t* pool_start(world_t* world, size_t index) {
        assert(index < pool_count());
        return ((utl::free_list_t<game::entity::entity_t>*)(&world->entity_pools))[index].deque.front();
    }

    inline world_t*
    world_init(arena_t* arena, cam::camera_t* camera) {
        world_t* world = arena_alloc_ctor<world_t>(arena, 1);
        
        world->arena = arena;

        world->frame_arena[0] = arena_sub_arena(arena, kilobytes(4));
        world->frame_arena[1] = arena_sub_arena(arena, kilobytes(4));

        entity::player_init(&world->player, camera, 0);

        return world;
    }

    inline game::entity::entity_t*
    world_create_entity(world_t* world) {
        return world->entity_pools.entities.alloc(world->arena);
    }

    inline void
    world_destroy_entity(world_t* world, game::entity::entity_t*& e) {
        world->entity_pools.entities.free(e);
    }
};