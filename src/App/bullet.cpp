#include "App/Game/Weapons/bullet.hpp"
#include "App/Game/Entity/entity_db.hpp"
#include "App/Game/World/world.hpp"


#define co_kill_in_x(t) [](coroutine_t* co, frame_arena_t& frame_arena) {\
    auto* e = (ztd::entity_t*)co->data;\
    co_begin(co);\
        co_wait(co, t);\
        e->queue_free();\
    co_end(co);\
}

namespace ztd::wep {

    ztd::entity_t* spawn_puff(
        ztd::world_t* world,
        // const ztd::prefab_t& prefab,
        v3f pos,
        u32 count
    ) {
        auto particle_prefab = ztd::db::particle::orb;
        std::memcpy(particle_prefab.type_name.data(), "puff", 5);
        particle_prefab.coroutine = co_kill_in_x(4.0f);
        particle_prefab.emitter->acceleration = v3f{0.0f};
        particle_prefab.emitter->spawn_rate = 0.02f;
        particle_prefab.emitter->velocity_random = math::rect3d_t(v3f(-0.2f), v3f(0.2f));
        // particle_prefab.emitter->velocity_random = math::rect3d_t(v3f(-1.0f), v3f(1.0f));
        particle_prefab.emitter->template_particle.velocity = v3f{0.0f};
        particle_prefab.emitter->template_particle.life_time = 4.01f;
        auto* ps = ztd::tag_spawn(world, particle_prefab, pos);
        ps->gfx.material_id = 7; // particle material @hardcode
        ps->coroutine->start();
        
        return ps;
    }

    // export_fn(ztd::entity_t*) knife_attack(
    //     ztd::world_t* world,
    //     const ztd::prefab_t& _prefab,
    //     bullet_t bullet
    // ) {

    //     return 0;
    // }

    export_fn(ztd::entity_t*) sword_attack(
        ztd::world_t* world,
        const ztd::prefab_t& _prefab,
        bullet_t bullet
    ) {
        const ztd::prefab_t prefab{
            .type_name = "sword_attack",
            .coroutine = co_kill_in_x(0.2f),
            .physics = ztd::prefab_t::physics_t {
                .flags = PhysicsEntityFlags_Static,
                .shapes = {
                    ztd::prefab_t::physics_t::shape_t{
                        .shape = physics::collider_shape_type::SPHERE,
                        .flags = 1,
                        .sphere = {
                            .radius = 0.35f,
                        },
                    },
                },
            },
        };

        auto* bullet_entity = ztd::tag_spawn(world, prefab, bullet.ray.at(1.5f));
        bullet_entity->gfx.material_id = 7; // unlit material @hardcode
        bullet_entity->stats.weapon.stats.damage = bullet.damage;
        bullet_entity->physics.rigidbody->on_collision = bullet_on_hit;
        
        bullet_entity->physics.rigidbody->set_gravity(false);
        bullet_entity->physics.rigidbody->set_ccd(true);
        
        bullet_entity->physics.rigidbody->set_layer(physics_layers::player_bullets);
        bullet_entity->physics.rigidbody->set_group(ztd::player_bullet_collision_group);

        bullet_entity->coroutine->start();

        return bullet_entity;
    }


    export_fn(ztd::entity_t*) spawn_bullet(
        ztd::world_t* world,
        const ztd::prefab_t& prefab,
        bullet_t bullet        
    ) {
        auto* bullet_entity = ztd::tag_spawn(world, prefab, bullet.ray.at(0.5f));
        bullet_entity->gfx.material_id = 7; // unlit material @hardcode
        bullet_entity->stats.weapon.stats.damage = bullet.damage;
        bullet_entity->physics.rigidbody->on_collision = bullet_on_hit;
        bullet_entity->physics.rigidbody->add_impulse(bullet.ray.direction*20.0f);
        bullet_entity->physics.rigidbody->set_ccd(true);
        bullet_entity->physics.rigidbody->set_mass(0.01f);
        
        bullet_entity->physics.rigidbody->set_layer(physics_layers::player_bullets);
        bullet_entity->physics.rigidbody->set_group(ztd::player_bullet_collision_group);

        bullet_entity->coroutine->start();
        bullet_entity->add_child(spawn_puff(world, bullet.ray.origin, 10));

        bullet_entity->attached_sound = world->game_state->sfx->emit_event(sound_event::arcane_bolt, bullet.ray.at(0.5f));

        return bullet_entity;
    }

    export_fn(ztd::entity_t*) spawn_rocket(
        ztd::world_t* world,
        const ztd::prefab_t& prefab_,
        bullet_t bullet        
    ) {
        auto prefab = ztd::db::misc::rocket_bullet;
        auto* bullet_entity = ztd::tag_spawn(world, prefab, bullet.ray.at(0.5f));
        bullet_entity->gfx.material_id = 7; // unlit material @hardcode
        bullet_entity->stats.weapon.stats.damage = bullet.damage;
        bullet_entity->physics.rigidbody->on_collision = rocket_on_hit;
        bullet_entity->physics.rigidbody->add_impulse(bullet.ray.direction*10.0f);
        bullet_entity->physics.rigidbody->set_ccd(true);
        bullet_entity->physics.rigidbody->set_mass(0.01f);
        bullet_entity->physics.rigidbody->set_layer(ztd::physics_layers::player);
        bullet_entity->physics.rigidbody->set_group(~ztd::physics_layers::player);
        // bullet_entity->physics.rigidbody->set_layer(2ui32);
        // bullet_entity->physics.rigidbody->set_group(~2ui32);

        bullet_entity->coroutine->start();
        bullet_entity->add_child(spawn_puff(world, bullet.ray.origin, 10));

        return bullet_entity;
    }
}