#include "App/Game/Weapons/bullet.hpp"
#include "App/Game/Entity/entity_db.hpp"
#include "App/Game/World/world.hpp"


#define co_kill_in_x(t) [](coroutine_t* co, frame_arena_t& frame_arena) {\
    auto* e = (zyy::entity_t*)co->data;\
    co_begin(co);\
        co_wait(co, t);\
        e->queue_free();\
    co_end(co);\
}

namespace zyy::wep {

    zyy::entity_t* spawn_puff(
        zyy::world_t* world,
        // const zyy::prefab_t& prefab,
        v3f pos,
        u32 count
    ) {
        auto particle_prefab = zyy::db::particle::orb;
        std::memcpy(particle_prefab.type_name.data(), "puff", 5);
        particle_prefab.emitter->template_particle.velocity = v3f{0.0f};
        auto* ps = zyy::tag_spawn(world, particle_prefab, pos);
        ps->gfx.material_id = 7; // particle material @hardcode
        ps->coroutine->start();
        
        return ps;
    }


    export_fn(zyy::entity_t*) spawn_bullet(
        zyy::world_t* world,
        const zyy::prefab_t& prefab,
        bullet_t bullet        
    ) {

        auto* bullet_entity = zyy::tag_spawn(world, prefab, bullet.ray.at(0.5f));
        bullet_entity->gfx.material_id = 7; // unlit material @hardcode
        bullet_entity->stats.weapon.stats.damage = bullet.damage;
        bullet_entity->physics.rigidbody->on_collision = bullet_on_hit;
        bullet_entity->physics.rigidbody->add_impulse(bullet.ray.direction*200.0f);
        bullet_entity->physics.rigidbody->set_ccd(true);
        bullet_entity->physics.rigidbody->set_mass(0.01f);
        bullet_entity->physics.rigidbody->set_layer(2ui32);
        bullet_entity->physics.rigidbody->set_group(~2ui32);

        bullet_entity->coroutine->start();
        bullet_entity->add_child(spawn_puff(world, bullet.ray.origin, 10));

        return bullet_entity;
    }

    export_fn(zyy::entity_t*) spawn_rocket(
        zyy::world_t* world,
        const zyy::prefab_t& prefab_,
        bullet_t bullet        
    ) {
        auto prefab = zyy::db::misc::rocket_bullet;
        auto* bullet_entity = zyy::tag_spawn(world, prefab, bullet.ray.at(0.5f));
        bullet_entity->gfx.material_id = 7; // unlit material @hardcode
        bullet_entity->stats.weapon.stats.damage = bullet.damage;
        bullet_entity->physics.rigidbody->on_collision = rocket_on_hit;
        bullet_entity->physics.rigidbody->add_impulse(bullet.ray.direction*100.0f);
        bullet_entity->physics.rigidbody->set_ccd(true);
        bullet_entity->physics.rigidbody->set_mass(0.01f);
        bullet_entity->physics.rigidbody->set_layer(2ui32);
        bullet_entity->physics.rigidbody->set_group(~2ui32);

        bullet_entity->coroutine->start();
        bullet_entity->add_child(spawn_puff(world, bullet.ray.origin, 10));

        return bullet_entity;
    }
}