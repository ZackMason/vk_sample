#pragma once

#include "App/Game/WorldGen/world_gen.hpp"
#include "App/Game/Entity/entity_db.hpp"

#ifndef SPAWN_GUN
#define SPAWN_GUN(gun, where) do{\
    auto* shotgun = zyy::spawn(world, world->render_system(),   \
        (gun), (where));  \
    shotgun->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {   \
        auto* self = (zyy::entity_t*)trigger->user_data;    \
        auto* other_e = (zyy::entity_t*)other->user_data;   \
        auto* world = (zyy::world_t*)trigger->api->user_world;  \
        if (other_e->type == zyy::entity_type::player) {    \
            auto equip_prefab = gun;  \
            equip_prefab.physics = std::nullopt;    \
            auto* equip_gun = zyy::spawn(world, world->render_system(), equip_prefab);  \
            equip_gun->flags &= ~zyy::EntityFlags_Pickupable;\
            equip_gun->parent = 0;\
            if (other_e->primary_weapon.entity==0){\
                other_e->primary_weapon.entity = equip_gun; \
                self->queue_free(); \
            }else if (other_e->secondary_weapon.entity==0){\
                other_e->secondary_weapon.entity = equip_gun; \
                self->queue_free(); \
            } else {equip_gun->queue_free();}\
        }   \
    }; } while(0)
#endif

world_generator_t*
generate_town_01(arena_t* arena) {
    tag_struct(auto* generator, world_generator_t, arena);
    generator->arena = arena;
    
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
       world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.01f;
    });

    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = zyy::spawn(world, world->render_system(), zyy::db::characters::assassin, axis::up * 3.0f);
    });
    
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        zyy::spawn(world, world->render_system(), zyy::db::misc::platform_1000, axis::down);
        zyy::spawn(world, world->render_system(), zyy::db::rooms::town_01);
    });

    
    return generator;
}

#undef SPAWN_GUN
