#include "App/Game/Weapons/weapon_common.hpp"

void do_hit_effects(
    zyy::world_t* world,
    zyy::entity_t* hit_entity,
    zyy::item::effect_t* effect,
    v3f hit_pos,
    v3f hit_normal
) {
    effect->on_hit_effect(world, effect, hit_entity, hit_pos, hit_normal);
    if (effect->next) {
        do_hit_effects(
            world,
            hit_entity,
            effect,
            hit_pos,
            hit_normal
        );
    }
}

#define co_blood(t) [](coroutine_t* co, frame_arena_t& frame_arena) {\
    auto* e = (zyy::entity_t*)co->data;\
    auto* ps = e->gfx.particle_system;\
    auto* stack = co_stack(co, frame_arena);\
    auto* world = *(zyy::world_t**)co_push_stack(co, stack, zyy::world_t*);\
    auto* spawned = (u8*)co_push_array_stack(co, stack, u8, ps->max_count);\
    auto* physics = world->physics;\
    umm i=0;\
    co_begin(co);\
        std::memset(spawned, 0, sizeof(u8)*ps->max_count);\
        while (true) {\
            for (i=0; i < ps->live_count; i++) {\
                if (spawned[i]) {continue;}\
                v3f ro=e->global_transform().xform(ps->particles[i].position);\
                v3f rd=e->global_transform().basis * ((ps->particles[i].velocity/20.0f));\
                math::ray_t blood_ray{ro,rd};\
                DEBUG_DIAGRAM_(blood_ray, 0.0001f);\
                if (auto ray = physics->raycast_world(physics, ro, rd); ray.hit) {\
                    auto* rb = (physics::rigidbody_t*)ray.user_data;\
                    if (rb->type==physics::rigidbody_type::STATIC) {\
                        math::transform_t transform{ray.point + ray.normal * 0.01f};\
                        math::ray_t blood_hit_ray{ray.point, ray.normal};\
                        DEBUG_DIAGRAM(blood_hit_ray);\
                        DEBUG_DIAGRAM(blood_ray);\
                        transform.look_at(ray.point + ray.normal);\
                        auto theta = f32(i) * 1000.0f;\
                        transform.set_rotation(transform.get_orientation() * quat{sinf(theta), 0.0f, 0.0f, cosf(theta)});\
                        transform.set_scale(v3f(0.6f));\
                        world_place_bloodsplat(world, transform);\
                        spawned[i] = 1;\
                    }\
                }\
            }\
            if (co->start_time == 0) { co->start_time = co->now; } if (co->now < (co->start_time) + (f32)t) { co_yield(co); } else { break; }\
        }\
        e->queue_free();\
    co_end(co);\
}

zyy::entity_t* spawn_blood(
    zyy::world_t* world,
    v3f pos,
    u32 count,
    zyy::db::prefab_t particle_prefab 
) {
    particle_prefab.coroutine = co_blood(4.0f);

    auto* ps = zyy::spawn(world, world->render_system(), particle_prefab, pos);
    ps->gfx.material_id = 3; // blood material @hardcode
    ps->coroutine->start();
    auto* stack = co_stack(&ps->coroutine->coroutine, world->frame_arena);
    zyy::world_t** co_world = co_push_stack(&ps->coroutine->coroutine, stack, zyy::world_t*);
    *co_world = world;
    // zyy::entity_set_mesh_texture(world, ps, "blood");
    ps->gfx.particle_system = particle_system_create(&world->particle_arena, count);
    ps->gfx.instance(world->render_system()->instance_storage_buffer.pool, count, 1);

    ps->gfx.particle_system->spawn_rate = 0.005f;
    ps->gfx.particle_system->scale_over_life_time = math::aabb_t<f32>(3.0f, 2.50f);
    ps->gfx.particle_system->velocity_random = math::aabb_t<v3f>(v3f(-4.0f), v3f(4.0f));
    ps->gfx.particle_system->angular_velocity_random = math::aabb_t<v3f>(v3f(-4.0f), v3f(4.0f));
    ps->gfx.particle_system->emitter_type = particle_emitter_type::box;
    ps->gfx.particle_system->box = math::aabb_t<v3f>(v3f(-0.10f), v3f(0.10f));
    ps->gfx.particle_system->stream_rate = 
    ps->gfx.particle_system->_stream_count = 1;
    
    ps->gfx.particle_system->template_particle = particle_t {
        .position = v3f(0.0),
        .life_time = 10.0f,
        .color = gfx::color::v4::purple,
        .scale = 1.0f,
        .velocity = axis::up * 5.0f,
    };

    return ps;
}

void bullet_on_hit(physics::rigidbody_t* self, physics::rigidbody_t* other) {
    auto* hit_entity = (zyy::entity_t*)other->user_data;
    auto* self_entity = (zyy::entity_t*)self->user_data;
    auto* world = (zyy::world_t*)self->api->user_world;
    b32 spawn_hit = 0;

    if (other->type == physics::rigidbody_type::STATIC) {
        self_entity->queue_free();
        spawn_hit = 1;
    }

    auto pos = self_entity->global_transform().origin;

    // spawn_puff(world, self_entity->global_transform().origin, 10);
    if (hit_entity->stats.character.health.max) {
        auto blood = zyy::db::environmental::blood_01;
        spawn_blood(world, pos, 30, blood);
        spawn_blood(world, pos, 30, blood);
        spawn_blood(world, pos, 30, blood);
        if (hit_entity->stats.character.health.damage(self_entity->stats.weapon.stats.damage)) {
            // entity was killed
            hit_entity->queue_free();
            spawn_blood(world, pos, 30, blood);
            spawn_blood(world, pos, 30, blood);
            spawn_blood(world, pos, 30, blood);
            spawn_blood(world, pos, 30, blood);
            spawn_blood(world, pos, 30, blood);
            spawn_blood(world, pos, 30, blood);
        }
        // spawn_hit = 1;

        if (self_entity->stats.effect.on_hit_effect) {
            do_hit_effects(
                world, 
                hit_entity,
                &self_entity->stats.effect,
                pos,
                axis::up
            );
        }

        self_entity->queue_free();
    }
    if (spawn_hit) {
        // auto* hole = zyy::spawn(world, world->render_system(), zyy::db::misc::bullet_hole, self->position);
        // hit_entity->add_child(hole, true);
        // hole->transform.set_rotation(world->entropy.randnv<v3f>() * 100.0f);
        // hole->coroutine->start();
    }
}


