#pragma once

#include "App/Game/WorldGen/world_gen.hpp"

world_generator_t*
generate_world_test(arena_t* arena) {
    auto* generator = arena_alloc<world_generator_t>(arena);
    generator->arena = arena;
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
       world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.04f;
    });
    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = game::spawn(world, world->render_system(), game::db::characters::assassin, axis::up * 3.0f);
        player->physics.rigidbody->linear_dampening = 9.0f;
    });
    generator->add_step("Placing Weapons", WORLD_STEP_TYPE_LAMBDA(items) {
        game::spawn(world, world->render_system(),
            game::db::weapons::shotgun);
        // ->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
    });
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        // game::spawn(world, world->render_system(), game::db::rooms::sponza);
        game::spawn(world, world->render_system(), game::db::misc::platform_1000, axis::down);
    });
    return generator;
}

world_generator_t*
generate_world_0(arena_t* arena) {
    auto* generator = arena_alloc<world_generator_t>(arena);
    generator->arena = arena;
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
       world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.04f;
    });
    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = game::spawn(world, world->render_system(), game::db::characters::assassin, axis::up * 3.0f);
        player->physics.rigidbody->linear_dampening = 9.0f;
    });
    generator->add_step("Placing Weapons", WORLD_STEP_TYPE_LAMBDA(items) {
        game::spawn(world, world->render_system(),
            game::db::weapons::shotgun);
        // ->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
    });
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        // game::spawn(world, world->render_system(), game::db::rooms::sponza);
        game::spawn(world, world->render_system(), game::db::misc::platform_1000, axis::down);
    });
    generator->add_step("Particles", WORLD_STEP_TYPE_LAMBDA(environment) {
        auto* teapot_particle = game::spawn(world, world->render_system(), game::db::misc::teapot_particle);
        teapot_particle->gfx.particle_system = particle_system_create(&world->arena, 1000);
        teapot_particle->gfx.instance(world->render_system()->instance_storage_buffer.pool, 1000, 1);

        teapot_particle->gfx.particle_system->spawn_rate = 0.0002f;
        teapot_particle->gfx.particle_system->scale_over_life_time = math::aabb_t<f32>(1.0f, 0.0f);
        teapot_particle->gfx.particle_system->velocity_random = math::aabb_t<v3f>(v3f(-4.0f), v3f(4.0f));
        teapot_particle->gfx.particle_system->angular_velocity_random = math::aabb_t<v3f>(v3f(-4.0f), v3f(4.0f));
        
        teapot_particle->gfx.particle_system->template_particle = particle_t {
            .position = v3f(0.0),
            .life_time = 4.0f,
            .color = gfx::color::v4::purple,
            .scale = 1.0f,
            .velocity = axis::up * 5.0f,
        };
    });
    generator->add_step("Platforms", WORLD_STEP_TYPE_LAMBDA(environment) {
        range_f32(x, -10, 10) {
            range_f32(y, -10, 10) {
                auto* platform = game::spawn(world, world->render_system(),
                    game::db::misc::platform_3x3,
                    v3f{x * 6.0f, 0, y * 6.0f});

                platform->physics.rigidbody->on_trigger_end =
                platform->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
                    auto* p = (game::entity_t*)trigger->user_data;
                    auto* o = (game::entity_t*)other->user_data;
                    // if (o->type == game::entity_type::player) 
                    {
                        p->coroutine->start();
                    }
                };
            }
        }
    });
    generator->add_step("Teapots", WORLD_STEP_TYPE_LAMBDA(environment) {
        loop_iota_u64(i, 500) {
            auto* e = game::spawn(
                world, 
                world->render_system(),
                game::db::misc::teapot,
                world->entropy.randnv<v3f>() * 100.0f * planes::xz + axis::up * 8.0f
            );
            e->transform.set_scale(v3f{2.f});
            e->transform.set_rotation(world->entropy.randnv<v3f>() * 1000.0f);
            e->gfx.material_id = world->entropy.rand() % 6;
            // if (e->physics.rigidbody) {
            //     // e->physics.rigidbody->on_collision = teapot_on_collision;
            // }
        }
    });
    generator->add_step("Planting Trees", WORLD_STEP_TYPE_LAMBDA(environment) {
        auto* tree = game::spawn(world, world->render_system(), game::db::environmental::tree_01, axis::down);
        constexpr u32 tree_count = 50;
        tree->gfx.instance(world->render_system()->instance_storage_buffer.pool, tree_count);
        
        for (size_t i = 0; i < tree_count; i++) {
            // tree->gfx.dynamic_instance_buffer[i + tree_count] = 
            tree->gfx.dynamic_instance_buffer[i] = 
                math::transform_t{}
                    .translate(400.0f * planes::xz * (world->entropy.randv<v3f>() * 2.0f - 1.0f))
                    .rotate(axis::up, utl::rng::random_s::randf() * 10000.0f)
                    .to_matrix();
        }
    });
    return generator;
}



