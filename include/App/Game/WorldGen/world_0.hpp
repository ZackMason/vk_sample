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
        auto* player = zyy::spawn(world, world->render_system(), zyy::db::characters::assassin, axis::up * 3.0f);
        player->physics.rigidbody->linear_dampening = 9.0f;
    });
    
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        // zyy::spawn(world, world->render_system(), zyy::db::rooms::sponza);
        zyy::spawn(world, world->render_system(), zyy::db::misc::platform_1000, axis::down);
    });
    return generator;
}

world_generator_t*
generate_homebase(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("Building Base", WORLD_STEP_TYPE_LAMBDA(environment) {
        constexpr zyy::db::prefab_t base{
            .type = zyy::entity_type::environment,
            .type_name = "Homebase",
            .gfx = {
                .mesh_name = "res/models/rooms/homebase.gltf",
                .material = gfx::material_t::metal(gfx::color::v4::light_gray),
            },
            .physics = zyy::db::prefab_t::physics_t {
                .flags = zyy::PhysicsEntityFlags_Static,
                .shapes = {
                    zyy::db::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
                },
            },
        };
        zyy::spawn(world, world->render_system(), base);
    });

    return generator;
}

world_generator_t*
generate_crash_test(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("Placing Weapons", WORLD_STEP_TYPE_LAMBDA(items) {
        auto* shotgun = zyy::spawn(world, world->render_system(),
            zyy::db::weapons::shotgun, axis::forward * 10.0f);
        shotgun->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
            auto* self = (zyy::entity_t*)trigger->user_data;
            auto* other_e = (zyy::entity_t*)other->user_data;

            volatile int* crash{0}; *crash++;

            if (other_e->type == zyy::entity_type::player) {
                puts("pick up");
                other_e->add_child(self);
                other_e->primary_weapon.entity = self;
                self->flags &= ~zyy::EntityFlags_Pickupable;
            }
        };
    });

    return generator;
}

world_generator_t*
generate_sponza(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("Sponza", WORLD_STEP_TYPE_LAMBDA(environment){
        zyy::spawn(world, world->render_system(),
            zyy::db::rooms::sponza, axis::down);
    });

    return generator;
}
world_generator_t*
generate_particle_test(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("Particle Spawner", WORLD_STEP_TYPE_LAMBDA(environment) {
        constexpr zyy::db::prefab_t spawner_p{
            .type = zyy::entity_type::environment,
            .type_name = "Spawner",
            .physics = zyy::db::prefab_t::physics_t {
                .flags = zyy::PhysicsEntityFlags_Static,
                .shapes = {
                    zyy::db::prefab_t::physics_t::shape_t{
                        .shape = physics::collider_shape_type::BOX,
                        .flags = 1,
                        .box = {
                            .size = v3f{1.0f},
                        },
                    },
                },
            },
        };
        auto* spawner = zyy::spawn(world, world->render_system(), spawner_p);
        spawner->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
            auto* p = (zyy::entity_t*)trigger->user_data;
            auto* o = (zyy::entity_t*)other->user_data;
            auto* world = (zyy::world_t*)trigger->api->user_world;

            auto particle_prefab = zyy::db::misc::teapot_particle;
            particle_prefab.coroutine = zyy::db::misc::co_kill_in_ten;

            auto* teapot_particle = zyy::spawn(world, world->render_system(), particle_prefab);
            teapot_particle->coroutine->start();
            teapot_particle->gfx.particle_system = particle_system_create(&world->arena, 30000);
            teapot_particle->gfx.instance(world->render_system()->instance_storage_buffer.pool, 30000, 1);

            teapot_particle->gfx.particle_system->spawn_rate = 0.02f;
            teapot_particle->gfx.particle_system->scale_over_life_time = math::aabb_t<f32>(1.0f, 0.0f);
            teapot_particle->gfx.particle_system->velocity_random = math::aabb_t<v3f>(v3f(-4.0f), v3f(4.0f));
            teapot_particle->gfx.particle_system->angular_velocity_random = math::aabb_t<v3f>(v3f(-4.0f), v3f(4.0f));
            teapot_particle->gfx.particle_system->emitter_type = particle_emitter_type::box;
            teapot_particle->gfx.particle_system->box = math::aabb_t<v3f>(v3f(-1.0f), v3f(1.0f));
            teapot_particle->gfx.particle_system->stream_rate = 10;
            
            teapot_particle->gfx.particle_system->template_particle = particle_t {
                .position = v3f(0.0),
                .life_time = 4.0f,
                .color = gfx::color::v4::purple,
                .scale = 1.0f,
                .velocity = axis::up * 5.0f,
            };  
        };
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
        auto* player = zyy::spawn(world, world->render_system(), zyy::db::characters::assassin, axis::up * 3.0f);
        player->physics.rigidbody->linear_dampening = 9.0f;
    });
    generator->add_step("Placing Weapons", WORLD_STEP_TYPE_LAMBDA(items) {
        auto* shotgun = zyy::spawn(world, world->render_system(),
            zyy::db::weapons::shotgun);
        shotgun->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
            auto* self = (zyy::entity_t*)trigger->user_data;
            auto* other_e = (zyy::entity_t*)other->user_data;

            if (other_e->type == zyy::entity_type::player) {
                puts("pick up");
                other_e->add_child(self);
                other_e->primary_weapon.entity = self;
                self->flags &= ~zyy::EntityFlags_Pickupable;
            }
        };
    });
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        // zyy::spawn(world, world->render_system(), zyy::db::rooms::sponza);
        zyy::spawn(world, world->render_system(), zyy::db::misc::platform_1000, axis::down);
    });
    generator->add_step("Particles", WORLD_STEP_TYPE_LAMBDA(environment) {
        auto* teapot_particle = zyy::spawn(world, world->render_system(), zyy::db::misc::teapot_particle);
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
                auto* platform = zyy::spawn(world, world->render_system(),
                    zyy::db::misc::platform_3x3,
                    v3f{x * 6.0f, 0, y * 6.0f});

                platform->physics.rigidbody->on_trigger_end =
                platform->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
                    auto* p = (zyy::entity_t*)trigger->user_data;
                    auto* o = (zyy::entity_t*)other->user_data;
                    // if (o->type == zyy::entity_type::player) 
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
                zyy::db::misc::teapot,
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
        constexpr u32 tree_count = 2000;
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



