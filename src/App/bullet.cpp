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
        // const zyy::db::prefab_t& prefab,
        v3f pos,
        u32 count
    ) {
        auto particle_prefab = zyy::db::particle::plasma;
        particle_prefab.coroutine = co_kill_in_x(4.0f);

        auto* ps = zyy::spawn(world, world->render_system(), particle_prefab, pos);
        ps->gfx.material_id = 4; // particle material @hardcode
        ps->coroutine->start();
        
        ps->gfx.particle_system = particle_system_create(&world->particle_arena, count);
        ps->gfx.instance(world->render_system()->instance_storage_buffer.pool, count, 1);

        // ps->gfx.particle_system->spawn_rate = 0.005f;
        ps->gfx.particle_system->spawn_rate = 0.005f;
        ps->gfx.particle_system->scale_over_life_time = math::aabb_t<f32>(.10f, .050f);
        ps->gfx.particle_system->velocity_random = math::aabb_t<v3f>(v3f(-4.0f), v3f(4.0f));
        ps->gfx.particle_system->angular_velocity_random = math::aabb_t<v3f>(v3f(-4.0f), v3f(4.0f));
        ps->gfx.particle_system->emitter_type = particle_emitter_type::box;
        ps->gfx.particle_system->box = math::aabb_t<v3f>(v3f(-0.10f), v3f(0.10f));
        ps->gfx.particle_system->stream_rate = 
        ps->gfx.particle_system->_stream_count = 1;
        ps->gfx.particle_system->world_space = 1;
        
        ps->gfx.particle_system->template_particle = particle_t {
            .position = v3f(0.0),
            .life_time = 0.50f,
            .color = gfx::color::v4::purple,
            .scale = 1.0f,
            .velocity = axis::up * 5.0f,
        };

        return ps;
    }


    zyy::entity_t* spawn_bullet(
        zyy::world_t* world,
        const zyy::db::prefab_t& prefab,
        bullet_t bullet,
        f32 dt
    ) {

        auto* bullet_entity = zyy::spawn(world, world->render_system(), prefab, bullet.ray.at(0.5f));
        bullet_entity->gfx.material_id = 2; // unlit material @hardcode
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
}