#include "App/Game/Weapons/weapon_common.hpp"

void do_hit_effects(
    zyy::world_t* world,
    zyy::entity_t* self_entity,
    zyy::entity_t* hit_entity,
    zyy::item::effect_t* effect,
    v3f hit_pos,
    v3f hit_normal
) {
    effect->on_hit_effect(world, effect, self_entity, hit_entity, hit_pos, hit_normal);
    if (effect->next) {
        do_hit_effects(
            world,
            self_entity,
            hit_entity,
            effect,
            hit_pos,
            hit_normal
        );
    }
}

template <f32 t>
void co_blood(coroutine_t* co, frame_arena_t& frame_arena) {
    auto* e = (zyy::entity_t*)co->data;
    auto* ps = e->gfx.particle_system;
    auto* world = e->world;
    auto* stack = co_stack(co, frame_arena);
    auto* spawned = (u8*)co_push_array_stack(co, stack, u8, ps->max_count);
    auto* physics = world->physics;
    umm i=0;
    co_begin(co);
        std::memset(spawned, 0, sizeof(u8)*ps->max_count);
        while (true) {
            for (i=0; i < ps->live_count; i++) {
                if (spawned[i]) {continue;}
                v3f ro=e->global_transform().xform(ps->particles[i].position);
                v3f rd=e->global_transform().basis * ((ps->particles[i].velocity/20.0f));
                math::ray_t blood_ray{ro,rd};
                DEBUG_DIAGRAM_(blood_ray, 0.0001f);
                if (auto ray = physics->raycast_world(ro, rd); ray.hit) {
                    auto* rb = (physics::rigidbody_t*)ray.user_data;
                    if (rb->type==physics::rigidbody_type::STATIC) {
                        math::transform_t transform{ray.point + ray.normal * 0.01f};
                        math::ray_t blood_hit_ray{ray.point, ray.normal};
                        DEBUG_DIAGRAM(blood_hit_ray);
                        DEBUG_DIAGRAM(blood_ray);
                        transform.look_at(ray.point + ray.normal);
                        auto theta = f32(i) * 1000.0f;
                        transform.set_rotation(transform.get_orientation() * quat{sinf(theta), 0.0f, 0.0f, cosf(theta)});
                        transform.set_scale(v3f(0.6f));
                        world_place_bloodsplat(world, transform);
                        spawned[i] = 1;
                    }
                }
            }
            if (co->start_time == 0) { co->start_time = co->now; } if (co->now < (co->start_time) + (f32)t) { co_yield(co); } else { break; }
        }
        e->queue_free();
    co_end(co);
}

zyy::entity_t* spawn_explosion(
    zyy::world_t* world,
    v3f pos,
    u32 count,
    zyy::prefab_t particle_prefab,
    u32 material = 2
) {
    particle_prefab.coroutine = co_kill_in_x(1.490f);

    particle_prefab.emitter->max_count = count;

    auto* ps = zyy::tag_spawn(world, particle_prefab, pos);
    ps->gfx.material_id = material; // unlit material @hardcode
    ps->coroutine->start();

    return ps;
}

zyy::entity_t* spawn_blood(
    zyy::world_t* world,
    v3f pos,
    u32 count,
    zyy::prefab_t particle_prefab 
) {
    particle_prefab.coroutine = co_blood<4.0f>;
    // particle_prefab.coroutine = co_kill_in_x(4.0f);

    particle_prefab.emitter->max_count = count;

    auto* ps = zyy::tag_spawn(world, particle_prefab, pos);
    ps->gfx.material_id = 9; // blood material @hardcode
    ps->coroutine->start();

    return ps;
}

void bullet_on_hit(physics::rigidbody_t* self, physics::rigidbody_t* other, physics::collider_t* self_shape, physics::collider_t* other_shape) {
    auto* hit_entity = (zyy::entity_t*)other->user_data;
    auto* self_entity = (zyy::entity_t*)self->user_data;
    auto* world = (zyy::world_t*)self->api->user_world;
    b32 spawn_hit = 0;

    if (other->type == physics::rigidbody_type::STATIC) {
        // self_entity->queue_free(0);
        spawn_hit = 1;
    }
    spawn_hit = 1;

    auto pos = self_entity->global_transform().origin;
    auto damage = self_entity->stats.weapon.stats.damage;

    // spawn_puff(world, self_entity->global_transform().origin, 10);
    if (hit_entity->stats.character.health.max) {
        auto blood = zyy::db::environmental::blood_01;
        // spawn_blood(world, pos, 30, blood);
        // spawn_blood(world, pos, 30, blood);
        // spawn_blood(world, pos, 30, blood);
        if (hit_entity->stats.character.health.damage(damage)) {
            // entity was killed
            hit_entity->queue_free();

            // spawn_blood(world, pos, 30, blood);
            // spawn_blood(world, pos, 30, blood);
            // spawn_blood(world, pos, 30, blood);
            // spawn_blood(world, pos, 30, blood);
            // spawn_blood(world, pos, 30, blood);
            // spawn_blood(world, pos, 30, blood);
        }
        auto* event = zyy::new_event(world, self_entity, zyy::event_type::damage);

        event->damage_event.point = pos;
        event->damage_event.damage = damage;

        spawn_hit = 1;
    }

    if (spawn_hit) {
        auto* event = zyy::new_event(world, self_entity, zyy::event_type::hit);
        event->hit_event.point = pos;

        if (self_entity->stats.effect && self_entity->stats.effect->on_hit_effect) {
            do_hit_effects(
                world, 
                self_entity,
                hit_entity,
                self_entity->stats.effect,
                pos,
                axis::up
            );
        }
        self_entity->queue_free(0);

        // @hardcoded particle position
        if (self_entity->first_child && self_entity->first_child->gfx.particle_system) {
            self_entity->first_child->gfx.particle_system->spawn_timer = 1000.0f;
        } else {
            assert(!"Tried to get bullet particles system but failed");
        }

        // auto* hole = zyy::tag_spawn(world, zyy::db::misc::bullet_hole, self->position);
        // hit_entity->add_child(hole, true);
        // hole->transform.set_rotation(world->entropy.randnv<v3f>() * 100.0f);
        // hole->coroutine->start();
    }
}

void rocket_on_hit(
    physics::rigidbody_t* self, 
    physics::rigidbody_t* other, 
    physics::collider_t* self_shape, 
    physics::collider_t* other_shape
) {
    auto* hit_entity = (zyy::entity_t*)other->user_data;
    auto* self_entity = (zyy::entity_t*)self->user_data;
    auto* world = (zyy::world_t*)self->api->user_world;

    if (other->type == physics::rigidbody_type::STATIC) {
        self_entity->queue_free();
    }

    explosion_at_point(world, 
        self_entity, 
        self_entity->global_transform().origin,
        self_entity->stats.weapon.stats.damage, 
        10.0f, 
        self_entity->stats.effect);

    self_entity->queue_free();

}

zyy::entity_t* explosion_at_point(
    zyy::world_t* world,
    zyy::entity_t* entity,
    v3f pos,
    f32 damage,
    f32 range,
    zyy::item::effect_t* effects
) {
    auto memory = begin_temporary_memory(&world->arena);

    auto* near = world->physics->sphere_overlap_world(
        memory.arena,
        pos,
        range
    );

    range_u64(i, 0, near->hit_count) {
        auto* rb = (physics::rigidbody_t*)near->hits[i].user_data;
        auto* n = (zyy::entity_t*)rb->user_data;

        assert(rb&&n);

        auto e_pos = n->global_transform().origin;

        if (n->physics.rigidbody) {
            auto delta = (e_pos - pos);
            auto delta_normal = glm::normalize(delta);
            auto delta_length = glm::dot(delta, delta) + 1.0f;

            auto force = delta_normal / delta_length; 
            n->physics.impulse += (force * 10.0f);
        }
        if (n->stats.character.health.max) {
            auto blood = zyy::db::environmental::blood_01;
            // spawn_blood(world, e_pos, 30, blood);
            // spawn_blood(world, e_pos, 30, blood);
            // spawn_blood(world, e_pos, 30, blood);
            // spawn_blood(world, e_pos, 30, blood);
            // spawn_blood(world, e_pos, 30, blood);
            // spawn_blood(world, e_pos, 30, blood);
            // spawn_blood(world, e_pos, 30, blood);
            while (effects) {
                if (effects->on_hit_effect) {
                    do_hit_effects(
                        world, 
                        entity,
                        n,
                        effects,
                        e_pos,
                        axis::up
                    );
                }
                node_next(effects);
            }

            if (n->stats.character.health.damage(damage)) {
                n->queue_free();
            }
        }
    }

    auto rnd_pos = [=](f32 s) {
        return s * utl::rng::random_s::randnv() + pos;
    };
    
    end_temporary_memory(memory);

    // auto explosion = zyy::db::particle::orb;
    // local_persist auto explosion = load_from_file(&world->arena, "./res/entity/explosion_up.entt");
    auto explosion = world->prefab_loader.load(&world->arena, "./res/entity/explosion_up.entt");
    spawn_explosion(world, rnd_pos(1.0f), 64, explosion, 4);
    // explosion.emitter->template_particle.life_time = 4.01f;
    // explosion.emitter->scale_over_life_time = math::range_t{14.0f, 4.2f};
    // spawn_explosion(world, rnd_pos(1.0f), 30, explosion, 8);
    // spawn_explosion(world, rnd_pos(1.0f), 30, explosion, 8);
    // explosion.emitter->scale_over_life_time = math::range_t{1.0f, 0.2f};
    // spawn_explosion(world, rnd_pos(1.0f), 30, explosion, 5);
    // spawn_explosion(world, rnd_pos(1.0f), 30, explosion, 6);
    // explosion.emitter->scale_over_life_time = math::range_t{0.50f, 0.1f};
    // spawn_explosion(world, rnd_pos(1.0f), 30, explosion, 5);
    // spawn_explosion(world, rnd_pos(1.0f), 30, explosion, 6);


    return 0;
}

