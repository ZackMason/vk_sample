#pragma once

#include "App/Game/WorldGen/world_gen.hpp"


WORLD_GEN_FUNCTION(world_0) {
    auto* render_system = world->render_system();

    auto* player = game::spawn(world, render_system, game::db::characters::assassin, axis::up * 300.0f);
    // player->physics.rigidbody->on_collision = player_on_collision;
    player->physics.rigidbody->linear_dampening = 9.0f;

    game::spawn(world, render_system,
        game::db::weapons::shotgun,
        axis::forward * 5.0f);
        // ->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
        //     puts("hello");
        // };

    range_f32(x, -10, 10) {
        range_f32(y, -10, 10) {
            auto* platform = game::spawn(world, render_system,
                game::db::misc::platform_3x3,
                v3f{x * 6.0f, 0, y * 6.0f});

            platform->physics.rigidbody->on_trigger_end =
            platform->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
                auto* p = (game::entity_t*)trigger->user_data;
                auto* o = (game::entity_t*)other->user_data;
                if (o->type == game::entity_type::player) {
                    p->coroutine->start();
                }
            };
        }
    }

    auto* teapot_particle = game::spawn(world, render_system, game::db::misc::teapot_particle);
    teapot_particle->gfx.particle_system = particle_system_create(&world->arena, 1000);
    teapot_particle->gfx.instance(render_system->instance_storage_buffer.pool, 1000, 1);


    teapot_particle->gfx.particle_system->spawn_rate = 0.0002f;
    teapot_particle->gfx.particle_system->scale_over_life_time = math::aabb_t<f32>{1.0f, 0.0f};
    teapot_particle->gfx.particle_system->velocity_random = math::aabb_t<v3f>{v3f{-4.0f}, v3f{4.0f}};
    teapot_particle->gfx.particle_system->angular_velocity_random = math::aabb_t<v3f>{v3f{-4.0f}, v3f{4.0f}};
    teapot_particle->gfx.particle_system->template_particle = particle_t {
        .position = v3f{0.0},
        .life_time = 4.0f,
        .color = gfx::color::v4::purple,
        .scale = 1.0f,
        .velocity = axis::up * 5.0f
    };

    // game::spawn(world, render_system,
    //     game::db::load_from_file(game_state->main_arena, "res/entity/shaderball.entt"),
    //     v3f{-10,1,10});

    // range_u64(i, 0, 1) {
    //     v3f pos = v3f{0.0f, 35.0f * f32(i), 0.0f};
    //     auto* r = game::spawn(world, render_system, game::db::rooms::tower_01, pos);
        // r->transform.set_rotation(axis::up * math::constants::pi32 * f32(i));
    // }
    auto* tree = game::spawn(world, render_system, game::db::environmental::tree_01, axis::down);
    constexpr u32 tree_count = 1'000;
    tree->gfx.instance(render_system->instance_storage_buffer.pool, tree_count);
    
    for (size_t i = 0; i < tree_count; i++) {
        // tree->gfx.dynamic_instance_buffer[i + tree_count] = 
        tree->gfx.dynamic_instance_buffer[i] = 
            math::transform_t{}
                .translate(400.0f * planes::xz * (utl::rng::random_s::randv() * 2.0f - 1.0f))
                .rotate(axis::up, utl::rng::random_s::randf() * 10000.0f)
                .to_matrix();
    }

    game::spawn(world, render_system, game::db::rooms::sponza);
    game::spawn(world, render_system, game::db::misc::platform_1000, axis::down);
    
    // utl::rng::random_t<utl::rng::xor64_random_t> rng;
    // loop_iota_u64(i, 50'000) {
    //     auto* e = game::spawn(
    //         world, 
    //         render_system,
    //         game::db::misc::teapot,
    //         rng.randnv<v3f>() * 1000.0f * planes::xz + axis::up * 8.0f
    //     );
    //     e->transform.set_scale(v3f{2.f});
    //     e->transform.set_rotation(rng.randnv<v3f>() * 1000.0f);
    //     e->gfx.material_id = rng.rand() % 6;
    //     // if (e->physics.rigidbody) {
    //     //     // e->physics.rigidbody->on_collision = teapot_on_collision;
    //     // }
    // }

    gen_info("game_state", "player id: {} - {}", 
        world->player->id,
        (void*)game::find_entity_by_id(world, world->player->id)
    );

}



