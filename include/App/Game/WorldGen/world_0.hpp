#pragma once

#include "App/Game/WorldGen/world_gen.hpp"
#include "App/Game/Entity/entity_db.hpp"

world_generator_t*
generate_world_test(arena_t* arena) {
    tag_struct(auto* generator, world_generator_t, arena);
    generator->arena = arena;
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
       world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.01f;
    });
    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = zyy::tag_spawn(world, zyy::db::characters::assassin, axis::up * 3.0f);
        player->physics.rigidbody->linear_dampening = 3.0f;
    });
    
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        // zyy::tag_spawn(world, zyy::db::rooms::sponza);
        zyy::tag_spawn(world, zyy::db::misc::platform_1000, axis::down);
    });
    return generator;
}

world_generator_t*
generate_homebase(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("Building Base", WORLD_STEP_TYPE_LAMBDA(environment) {
        constexpr zyy::prefab_t base{
            .type = zyy::entity_type::environment,
            .type_name = "Homebase",
            .gfx = {
                .mesh_name = "res/models/rooms/homebase.gltf",
                .material = gfx::material_t::metal(gfx::color::v4::light_gray),
            },
            .physics = zyy::prefab_t::physics_t {
                .flags = zyy::PhysicsEntityFlags_Static,
                .shapes = {
                    zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
                },
            },
        };
        zyy::tag_spawn(world, base);
    });

    return generator;
}


#define SPAWN_GUN(gun, where) do{\
    auto* shotgun = zyy::tag_spawn(world,   \
        (gun), (where));  \
    shotgun->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {   \
        auto* self = (zyy::entity_t*)trigger->user_data;    \
        auto* other_e = (zyy::entity_t*)other->user_data;   \
        auto* world = (zyy::world_t*)trigger->api->user_world;  \
        if (other_e->type == zyy::entity_type::player) {    \
            auto equip_prefab = gun;  \
            equip_prefab.physics = std::nullopt;    \
            auto* equip_gun = zyy::tag_spawn(world, equip_prefab);  \
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
            // equip_gun->transform.origin = other_e->transform.origin;
            // other_e->add_child(equip_gun);  
            // volatile int* crash{0}; *crash++;

world_generator_t*
generate_crash_test(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("Placing Weapons", WORLD_STEP_TYPE_LAMBDA(items) {
        SPAWN_GUN(zyy::db::weapons::shotgun, axis::right * 30.0f + axis::up * 2.0f);
    });

    return generator;
}

world_generator_t*
generate_forest(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        // zyy::tag_spawn(world,
        //     zyy::db::rooms::sponza, axis::up);
    });

    generator->add_step("Planting Trees", WORLD_STEP_TYPE_LAMBDA(environment) {
        auto* tree = zyy::tag_spawn(world, zyy::db::environmental::tree_01, axis::down);
        constexpr u32 tree_count = 4000;
        tree->gfx.instance(world->render_system()->instance_storage_buffer.pool, tree_count);
        
        for (size_t i = 0; i < tree_count; i++) {
            // tree->gfx.dynamic_instance_buffer[i + tree_count] = 
            tree->gfx.instance_buffer[i] = 
                math::transform_t{}
                    .translate(200.0f * planes::xz * (world->entropy.randv<v3f>() * 2.0f - 1.0f))
                    .rotate(axis::up, utl::rng::random_s::randf() * 10000.0f)
                    .to_matrix();
        }
    });

    generator->add_step("Planting Grass", WORLD_STEP_TYPE_LAMBDA(environment) {
        return;
        auto* grass = zyy::tag_spawn(world, zyy::db::environmental::grass_02);
        constexpr u32 grass_count = 90'000;
        grass->gfx.instance(world->render_system()->instance_storage_buffer.pool, grass_count);
        grass->gfx.material_id = 4;
        
        for (size_t i = 0; i < grass_count; i++) {
            // tree->gfx.dynamic_instance_buffer[i + tree_count] = 
            grass->gfx.instance_buffer[i] = 
                math::transform_t{}
                    .translate(200.0f * planes::xz * (world->entropy.randv<v3f>() * 2.0f - 1.0f))
                    .rotate(axis::up, utl::rng::random_s::randf() * 10000.0f)
                    .to_matrix();
        }
    });

    return generator;
}
world_generator_t*
generate_sponza(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("Sponza", WORLD_STEP_TYPE_LAMBDA(environment){
        auto* sponza = zyy::tag_spawn(world,
            zyy::db::rooms::sponza, axis::up);

        auto aabb = sponza->global_transform().xform_aabb(sponza->aabb);
        // world->render_system()->light_probes.grid_size = 3.0f;
        // world->render_system()->light_probes.grid_size = 2.0f;
        // rendering::update_probe_aabb(world->render_system(), aabb);
    });

    return generator;
}

auto SPAWN_FIRE_PARTICLE(auto* world, auto pos, auto mat, auto size) {
    auto* ps = zyy::tag_spawn(world, zyy::db::particle::plasma, pos);
    ps->gfx.particle_system = particle_system_create(&world->particle_arena, 64);
    ps->gfx.instance(world->render_system()->instance_storage_buffer.pool, 64, 1);
    ps->gfx.material_id = mat;
    ps->gfx.particle_system->acceleration = v3f{axis::up*9.81f*0.1f};
    ps->gfx.particle_system->spawn_rate = 0.02f;
    ps->gfx.particle_system->scale_over_life_time = math::range_t(0.06f * size, 0.01f * size);
    ps->gfx.particle_system->velocity_random = math::rect3d_t(planes::xz*-.0050f, v3f(.0050f));
    ps->gfx.particle_system->angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f));
    ps->gfx.particle_system->emitter_type = particle_emitter_type::box;
    ps->gfx.particle_system->box = math::rect3d_t(v3f(-.10f), v3f(.10f));
    ps->gfx.particle_system->stream_rate = 1;
    ps->gfx.particle_system->template_particle = particle_t {
        .position = v3f(0.0),
        .life_time = 1.50f,
        .color = gfx::color::v4::purple,
        .scale = 1.0f,
        .velocity = axis::up * 0.10f,
    };
    return ps;
}

world_generator_t*
generate_particle_test(arena_t* arena) {
    auto* generator = generate_world_test(arena);

    generator->add_step("Fire Particle", WORLD_STEP_TYPE_LAMBDA(environment) {
        zyy::tag_spawn(world, zyy::db::particles::fire, 5.0f*axis::right + axis::up)->gfx.material_id=5;
        zyy::tag_spawn(world, zyy::db::particles::fire, 5.0f*axis::right + axis::up)->gfx.material_id=4;
        // SPAWN_FIRE_PARTICLE(world, 5.0f*axis::right + axis::up, 5, 1.0f);
        // SPAWN_FIRE_PARTICLE(world, 5.0f*axis::right + axis::up, 4, 1.0f);
    });
    generator->add_step("Particle Spawner", WORLD_STEP_TYPE_LAMBDA(environment) {
        constexpr zyy::prefab_t spawner_p{
            .type = zyy::entity_type::environment,
            .type_name = "Spawner",
            .physics = zyy::prefab_t::physics_t {
                .flags = zyy::PhysicsEntityFlags_Static,
                .shapes = {
                    zyy::prefab_t::physics_t::shape_t{
                        .shape = physics::collider_shape_type::BOX,
                        .flags = 1,
                        .box = {
                            .size = v3f{1.0f},
                        },
                    },
                },
            },
        };
        auto* spawner = zyy::tag_spawn(world, spawner_p);
        spawner->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
            auto* p = (zyy::entity_t*)trigger->user_data;
            auto* o = (zyy::entity_t*)other->user_data;
            auto* world = (zyy::world_t*)trigger->api->user_world;

            auto particle_prefab = zyy::db::misc::teapot_particle;
            particle_prefab.coroutine = co_kill_in_ten;

            auto* teapot_particle = zyy::tag_spawn(world, particle_prefab);

            teapot_particle->gfx.material_id = 4;
            // teapot_particle->coroutine->start();
            teapot_particle->gfx.particle_system = particle_system_create(&world->particle_arena, 30000);
            teapot_particle->gfx.instance(world->render_system()->instance_storage_buffer.pool, 30000, 1);

            teapot_particle->gfx.particle_system->spawn_rate = 0.02f;
            teapot_particle->gfx.particle_system->scale_over_life_time = math::range_t(1.0f, 0.0f);
            teapot_particle->gfx.particle_system->velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f));
            teapot_particle->gfx.particle_system->angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f));
            teapot_particle->gfx.particle_system->emitter_type = particle_emitter_type::box;
            teapot_particle->gfx.particle_system->box = math::rect3d_t(v3f(-1.0f), v3f(1.0f));
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
generate_probe_test(arena_t* arena) {
    tag_struct(auto* generator, world_generator_t, arena);
    generator->arena = arena;
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
       world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.01f;
       world->render_system()->environment_storage_buffer.pool[0].sun.direction = v4f{0.0f};
    });
    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = zyy::tag_spawn(world, zyy::db::characters::assassin, axis::up * 3.0f + axis::left * 15.0f);
        player->physics.rigidbody->linear_dampening = 3.0f;
    });
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        zyy::tag_spawn(world, zyy::db::misc::platform_1000, axis::down);
        zyy::prefab_t house_prefab {
            .gfx = {
                .mesh_name = "res/models/rooms/house_01.gltf"
            }
        };
        zyy::prefab_t shaderball_prefab {
            .gfx = {
                .mesh_name = "res/models/dragon.gltf"
            }
        };
        zyy::tag_spawn(world, house_prefab, axis::backward * 10.12310f)->gfx.material_id = 1;
        auto* ball = zyy::tag_spawn(world, shaderball_prefab);
        ball->gfx.material_id = 2;
        // ball->transform.set_scale(v3f{0.05f});

        // rendering::create_point_light(world->render_system(), axis::up*2.0f, 20.0f, v3f{0.8f});

        world->render_system()->light_probes.grid_size = 1.0f + 1.6180f;
        // world->render_system()->light_probes.grid_size = 8.0f;
        // world->render_system()->light_probes.grid_size = 4.0f;
        // world->render_system()->light_probes.grid_size = .0f + 1.6180f;
        // world->render_system()->light_probes.grid_size = 2.0f;
        rendering::update_probe_aabb(world->render_system(), {v3f{-15.0f, 0.120, -20.0f}, v3f{25.0f, 25.0f, 20.0f}});
    });
    return generator;
}

world_generator_t*
generate_room_03(arena_t* arena) {
    tag_struct(auto* generator, world_generator_t, arena);
    generator->arena = arena;
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
        world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.01f;
        //world->render_system()->environment_storage_buffer.pool[0].sun.direction = v4f{0.0};
    });
    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = zyy::tag_spawn(world, zyy::db::characters::assassin, axis::up * 3.0f + axis::left * 15.0f);
        player->physics.rigidbody->linear_dampening = 3.0f;
    });
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        zyy::tag_spawn(world, zyy::db::misc::platform_1000, axis::down);
        zyy::prefab_t prefab = zyy::db::rooms::room_03;
        auto* room = zyy::tag_spawn(world, prefab);
        room->gfx.material_id = 1;

        // rendering::create_point_light(world->render_system(), axis::up*2.0f, 20.0f, v3f{0.8f});
        auto aabb = room->global_transform().xform_aabb(room->aabb);

        // world->render_system()->light_probes.grid_size = 1.0f + 1.6180f;
        // world->render_system()->light_probes.grid_size = .0f + 1.6180f;
        world->render_system()->light_probes.grid_size = 25.0f;
        rendering::update_probe_aabb(world->render_system(), aabb);
    });
    return generator;
}

world_generator_t*
generate_world_1(arena_t* arena) {
    tag_struct(auto* generator, world_generator_t, arena);
    generator->arena = arena;
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
       world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.01f;
       world->render_system()->environment_storage_buffer.pool[0].sun.direction = v4f{0.0};
    });
    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = zyy::tag_spawn(world, zyy::db::characters::assassin, axis::up * 3.0f + axis::right * 15.0f);
        player->physics.rigidbody->linear_dampening = 3.0f;
        // SPAWN_GUN(zyy::db::weapons::smg, axis::forward * 15.0f + axis::up * 3.0f);
        SPAWN_GUN(zyy::db::weapons::rifle, axis::forward * 15.0f + axis::up * 3.0f);
        SPAWN_GUN(zyy::db::weapons::rpg, axis::forward * 25.0f + axis::up * 3.0f);

        zyy::tag_spawn(world, zyy::db::items::lightning_powerup, axis::backward * 15.0f + axis::up * 3.0f);
        range_u64(i, 0, 200) {
            auto spos = v3f{10.0f, 0.0f, 10.0f} + v3f{f32(i/10)*3.0f, 1.0f, f32(i%10)*3.0f};
            auto* person = zyy::tag_spawn(world, zyy::db::bads::person, spos);
            person->physics.rigidbody->set_layer(1ui32);
        }
        // person->physics.rigidbody->set_group(~1ui32);

    });
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        // zyy::tag_spawn(world, zyy::db::rooms::sponza);
        zyy::tag_spawn(world, zyy::db::misc::platform_1000, axis::down);
        
        auto enemy_room = zyy::db::rooms::parkcore_02;
        enemy_room.coroutine = [](coroutine_t* co, frame_arena_t& frame_arena) {
            auto* self = (zyy::entity_t*)co->data;
            auto* world = (zyy::world_t*)self->physics.rigidbody->api->user_world; // fix this shit wtf
            zyy::entity_t* skull = 0;
            auto* stack = co_stack(co, frame_arena);
            i8* count = co_push_stack(co, stack, i8);

            co_begin(co);
                *count = 10;
                while (*count > 0) {
                    skull = zyy::tag_spawn(world, zyy::db::bads::skull, self->global_transform().origin + axis::up * 15.0f + planes::xz * utl::rng::random_s::randv());
                    skull->physics.rigidbody->set_gravity(false);
                    skull->physics.rigidbody->set_ccd(true);
                    skull->physics.rigidbody->set_layer(1ui32);
                    // skull->physics.rigidbody->set_group(~1ui32);
                    skull->gfx.material_id = 2;
                    *count -= 1;
                    co_yield(co);
                }

            co_end(co);
        };
        enemy_room.physics = zyy::prefab_t::physics_t {
            .flags = zyy::PhysicsEntityFlags_Static,
            .shapes = {
                zyy::prefab_t::physics_t::shape_t{
                    .shape = physics::collider_shape_type::TRIMESH,
                },
                zyy::prefab_t::physics_t::shape_t{
                    .shape = physics::collider_shape_type::SPHERE,
                    .flags = 1,
                    .sphere = {
                        .radius = 10.0f,
                    },
                },
            },
        };
        auto* e_room = zyy::tag_spawn(world, enemy_room);
            e_room->gfx.material_id = 1;
            e_room->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
                auto* self = (zyy::entity_t*)trigger->user_data;
                auto* o = (zyy::entity_t*)other->user_data;
                auto* world = (zyy::world_t*)trigger->api->user_world;
                if (o->type == zyy::entity_type::player) 
                {
                    self->coroutine->start();
                }
            };

        auto aabb = e_room->aabb;
        aabb.scale(v3f{1.2f, 1.0f, 1.2f});

        aabb.min.y = 1.0f;
        aabb.max.y *= 0.8f;

        auto light_aabb = aabb;
        light_aabb.scale(v3f{0.5f});

        range_u64(l, 0, 0) {
            // spawn random lights
            rendering::create_point_light(
                world->render_system(),
                utl::rng::random_s::aabb(light_aabb),
                100.0f, 25.0f, v3f{1.5f, .9f, .3f} * utl::rng::random_s::randv()
            );
        }

        world->render_system()->light_probes.grid_size = 8.0f;
        rendering::update_probe_aabb(world->render_system(), aabb);
    });
    return generator;
}

world_generator_t*
generate_world_0(arena_t* arena) {
    tag_struct(auto* generator, world_generator_t, arena);
    generator->arena = arena;
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
       world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.01f;
    });
    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = zyy::tag_spawn(world, zyy::db::characters::assassin, axis::up * 3.0f + axis::right * 150.0f);
        player->physics.rigidbody->linear_dampening = 3.0f;
        world->render_system()->light_probes.grid_size = 10.0f;
        rendering::update_probe_aabb(world->render_system(), {v3f{-200.0f, 1.0f, -100.0f}, v3f{200.0f, 50.0f, 100.0f}});
    });
    generator->add_step("Placing Weapons", WORLD_STEP_TYPE_LAMBDA(items) {
        SPAWN_GUN(zyy::db::weapons::shotgun, axis::right * 135.0f + axis::up * 3.0f);
        SPAWN_GUN(zyy::db::weapons::smg, axis::right * 125.0f + axis::up * 3.0f);
    });
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        // zyy::tag_spawn(world, zyy::db::rooms::sponza);
        zyy::tag_spawn(world, zyy::db::misc::platform_1000, axis::down);
        auto house_prefab = zyy::prefab_t{
            .gfx = {
                .mesh_name = "res/models/rooms/house_02.gltf"
            }
        };
        zyy::tag_spawn(world, house_prefab, v3f{45.0f, 0.0f, 45.0f});
        zyy::tag_spawn(world, zyy::db::rooms::temple_01, axis::left * 150.0f)->gfx.material_id = 0;
        zyy::tag_spawn(world, zyy::db::rooms::parkcore_01, axis::right * 150.0f)->gfx.material_id = 1;
        auto enemy_room = zyy::db::rooms::parkcore_02;
        enemy_room.coroutine = [](coroutine_t* co, frame_arena_t& frame_arena) {
            auto* self = (zyy::entity_t*)co->data;
            auto* world = (zyy::world_t*)self->physics.rigidbody->api->user_world; // fix this shit wtf
            zyy::entity_t* skull = 0;
            auto* stack = co_stack(co, frame_arena);
            i8* count = co_push_stack(co, stack, i8);

            co_begin(co);
                *count = 10;
                while (*count > 0) {
                    skull = zyy::tag_spawn(world, zyy::db::bads::skull, self->global_transform().origin + axis::up * 15.0f + planes::xz * utl::rng::random_s::randv());
                    skull->physics.rigidbody->set_gravity(false);
                    skull->physics.rigidbody->set_ccd(true);
                    skull->physics.rigidbody->set_layer(1ui32);
                    // skull->physics.rigidbody->set_group(~1ui32);
                    *count -= 1;
                    co_yield(co);
                }

            co_end(co);
        };
        enemy_room.physics = zyy::prefab_t::physics_t {
            .flags = zyy::PhysicsEntityFlags_Static,
            .shapes = {
                zyy::prefab_t::physics_t::shape_t{
                    .shape = physics::collider_shape_type::TRIMESH,
                },
                zyy::prefab_t::physics_t::shape_t{
                    .shape = physics::collider_shape_type::SPHERE,
                    .flags = 1,
                    .sphere = {
                        .radius = 10.0f,
                    },
                },
            },
        };
        auto* e_room = zyy::tag_spawn(world, enemy_room, axis::forward * 50.0f);
            e_room->gfx.material_id = 1;
            e_room->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
                auto* self = (zyy::entity_t*)trigger->user_data;
                auto* o = (zyy::entity_t*)other->user_data;
                auto* world = (zyy::world_t*)trigger->api->user_world;
                if (o->type == zyy::entity_type::player) 
                {
                    self->coroutine->start();
                }
            };
    });
    // generator->add_step("Placing Bads", WORLD_STEP_TYPE_LAMBDA(bads) {
    //     range_u64(i, 0, 50) {
    //         auto* skull = zyy::tag_spawn(world, zyy::db::bads::skull, axis::right * 150.0f + axis::forward * 5.0f + planes::xz * utl::rng::random_s::randv());
    //         skull->physics.rigidbody->set_gravity(false);
    //     }
    // });
    generator->add_step("Particles", WORLD_STEP_TYPE_LAMBDA(environment) {
        auto* teapot_particle = zyy::tag_spawn(world, zyy::db::misc::teapot_particle);

        teapot_particle->gfx.material_id = 4;

        teapot_particle->gfx.particle_system = particle_system_create(&world->particle_arena, 1000);
        teapot_particle->gfx.instance(world->render_system()->instance_storage_buffer.pool, 1000, 1);

        teapot_particle->gfx.particle_system->spawn_rate = 0.0002f;
        teapot_particle->gfx.particle_system->scale_over_life_time = math::range_t(1.0f, 0.0f);
        teapot_particle->gfx.particle_system->velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f));
        teapot_particle->gfx.particle_system->angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f));
        
        teapot_particle->gfx.particle_system->template_particle = particle_t {
            .position = v3f(0.0),
            .life_time = 4.0f,
            .color = gfx::color::v4::purple,
            .scale = 1.0f,
            .velocity = axis::up * 5.0f,
        };
    });
    generator->add_step("Platforms", WORLD_STEP_TYPE_LAMBDA(environment) {
        return;
        range_f32(x, -10, 10) {
            range_f32(y, -10, 10) {
                auto* platform = zyy::tag_spawn(world,
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
        // return;
        loop_iota_u64(i, 500) {
            auto* e = zyy::spawn(
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
        return;
        auto* tree = zyy::tag_spawn(world, zyy::db::environmental::tree_01, axis::down);
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

zyy::entity_t* spawn_torch(zyy::world_t* world, v3f pos, f32 rotation = 0.0f) {
    auto prefab = zyy::db::misc::torch_01;
    zyy::prefab_t null_prefab{};
    prefab.children[0].entity = &null_prefab;
    auto* torch = zyy::tag_spawn(world, prefab, pos);
    torch->transform.set_rotation(v3f{0.0f, rotation, 0.0f});
    torch->gfx.material_id = 1;
    torch->first_child->add_child(SPAWN_FIRE_PARTICLE(world, v3f{0.0f}, 5, 4.0f));
    rendering::create_point_light(
        world->render_system(),
        torch->first_child->global_transform().origin,
        50.0f, 55.0f, v3f{1.5f, .9f, .3f} * utl::rng::random_s::randv()
    );
    return torch;
}   
zyy::entity_t* spawn_door(zyy::world_t* world, v3f pos, f32 rotation = 0.0f) {
    auto prefab = zyy::db::misc::door;
    // zyy::prefab_t null_prefab{};
    // prefab.children[0].entity = &null_prefab;
    auto* torch = zyy::tag_spawn(world, prefab, pos);
    torch->transform.set_rotation(v3f{0.0f, rotation, 0.0f});
    torch->physics.rigidbody->orientation = glm::toQuat(torch->transform.basis);
    torch->physics.rigidbody->position = torch->global_transform().origin;
    world->physics->set_rigidbody(0, torch->physics.rigidbody);
    torch->gfx.material_id = 1;
    
    return torch;
}   

zyy::entity_t* spawn_room(zyy::world_t* world, zyy::prefab_t prefab, v3f pos = v3f{0.0f}) {
    auto* room = zyy::tag_spawn(world, prefab, pos);
    room->gfx.material_id = 1;
    // room->flags |= (zyy::EntityFlags_Pickupable);

    room->add_child(
        spawn_door(world, v3f{-25.0f, 0.0f, 0.0f}, 3.1415f * 0.5f)
    , true);
    room->add_child(
        spawn_door(world, v3f{25.0f, 0.0f, 0.0f}, -3.1415f * 0.5f)
    , true);
    room->add_child(
        spawn_door(world, v3f{0.0f, 0.0f, 25.0f}, 0.0f)
    , true);
    room->add_child(
        spawn_door(world, v3f{0.0f, 0.0f,-25.0f}, 3.1415f)
    , true);

    room->add_child(spawn_torch(world, v3f{-25.0f, 2.0f, 8.0f}, -3.1415*0.5f), true);
    room->add_child(spawn_torch(world, v3f{-25.0f, 2.0f, -8.0f}, -3.1415*0.5f), true);
    room->add_child(spawn_torch(world, v3f{25.0f, 2.0f, 8.0f}, 3.1415f*0.5f), true);
    room->add_child(spawn_torch(world, v3f{25.0f, 2.0f, -8.0f}, 3.1415f*0.5f), true);
    room->add_child(spawn_torch(world, v3f{-8.0f, 2.0f, 25.0f}), true);
    room->add_child(spawn_torch(world, v3f{-8.0f, 2.0f, -25.0f}, 3.1415f), true);
    room->add_child(spawn_torch(world, v3f{8.0f, 2.0f, 25.0f}), true);
    room->add_child(spawn_torch(world, v3f{8.0f, 2.0f, -25.0f}, 3.1415f), true);
    return room;
}

enum RoomDirection {
    RoomDirection_None  = 0,
    RoomDirection_North = BIT(0),
    RoomDirection_South = BIT(1),
    RoomDirection_East  = BIT(2),
    RoomDirection_West  = BIT(3),
    RoomDirection_Up    = BIT(4),
    RoomDirection_Down  = BIT(5),

    RoomDirection_Horizontal    = RoomDirection_North|RoomDirection_South|RoomDirection_East|RoomDirection_West,
    RoomDirection_Vertical      = RoomDirection_Up|RoomDirection_Down,
    RoomDirection_All           = RoomDirection_Horizontal|RoomDirection_Vertical,
};

enum ModuleType {
    ModuleType_Room = BIT(0),
    ModuleType_Shop = BIT(1),
    ModuleType_Item = BIT(2),
    ModuleType_Exit = BIT(3),
};

#define make_room(name) \
    zyy::prefab_t {\
        .gfx = {\
            .mesh_name = name,\
        },\
        .physics = zyy::prefab_t::physics_t {\
            .flags = zyy::PhysicsEntityFlags_Static,\
            .shapes = {\
                zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},\
            },\
        },\
    }

struct module_filter_t {
    u32 directions{0};
    u32 type{0};

    f32 match(module_filter_t o) const {
        f32 r{0.0f};
        r += type == o.type ? 100.0f : -100.0f;
        r += directions == o.directions ? 100.0f : 0.0f;
        r += (directions & o.directions) ? 50.0f : 0.0f;
        return r;
    }
};

struct module_prefab_t {
    zyy::prefab_t prefab{};
    // u32 directions{0};
    module_filter_t filter{};

    
};

global_variable module_prefab_t gs_modules[] = {
    module_prefab_t{.prefab = make_room("res/models/modules/module_0.gltf"),   .filter={.directions=RoomDirection_None}},
    module_prefab_t{.prefab = make_room("res/models/modules/module_000.gltf"), .filter={.directions=RoomDirection_Horizontal}},
    module_prefab_t{.prefab = make_room("res/models/modules/module_001.gltf"), .filter={.directions=RoomDirection_North}},
    module_prefab_t{.prefab = make_room("res/models/modules/module_002.gltf"), .filter={.directions=RoomDirection_North|RoomDirection_West}},
    module_prefab_t{.prefab = make_room("res/models/modules/module_003.gltf"), .filter={.directions=RoomDirection_North|RoomDirection_West|RoomDirection_East}},
    // module_prefab_t{.prefab = make_room("res/models/modules/module_005.gltf"), .filter={.directions=RoomDirection_Vertical|RoomDirection_Horizontal}},
    // module_prefab_t{.prefab = make_room("res/models/modules/module_004.gltf"), .filter={.directions=RoomDirection_Up|RoomDirection_Horizontal}},
    module_prefab_t{.prefab = make_room("res/models/modules/module_006.gltf"), .filter={.directions=RoomDirection_Down|RoomDirection_Horizontal}},
};

struct room_module_t {
    v3u coord{};
    u32 type{ModuleType_Room};

    zyy::prefab_t prefab{};

    union {
        struct {
            // do not move
            room_module_t* north;
            room_module_t* south;
            room_module_t* east;
            room_module_t* west;
            room_module_t* up;
            room_module_t* down;
        };
        room_module_t* direction[6];
    };

    u32 opened_directions() const {
        u32 r=0;
        range_u32(i,0,6) {
            r|=direction[i]?1<<i:0;
        }
        return r;
    }
    u32 closed_directions() const {
        return (~opened_directions())&0b111111;
    }
};

constexpr u32 direction_index(u32 x) {
    u32 i = 0;
    while(x!=1) {
        x>>=1; i++;
    }
    return i;
}

constexpr u32 anti_direction(u32 x) {
    switch(x) {
        case RoomDirection_North: return RoomDirection_South;
        case RoomDirection_South: return RoomDirection_North;
        case RoomDirection_West: return RoomDirection_East;
        case RoomDirection_East: return RoomDirection_West;
        case RoomDirection_Up: return RoomDirection_Down;
        case RoomDirection_Down: return RoomDirection_Up;
        case_invalid_default;
    }
    return 0xffffffff;
}

constexpr v3i direction_offset(u32 x) {
    switch(x) {
        case RoomDirection_North: return v3i{0,0,1};
        case RoomDirection_South: return v3i{0,0,-1};
        case RoomDirection_West: return v3i{-1,0,0};
        case RoomDirection_East: return v3i{1,0,0};
        case RoomDirection_Up: return v3i{0,1,0};
        case RoomDirection_Down: return v3i{0,-1,0};
        case_invalid_default;
    }
    return v3i{0};
}

struct module_generator_t {
    inline static constexpr v3u dim{8,3,8};
    zyy::world_t* world{nullptr};

    v3f room_dimensions{16.0f, 8.0f, 16.0f};
    room_module_t rooms[dim.x*dim.y*dim.z];
    room_module_t* tiles[dim.x][dim.y][dim.z];

    u64 spawned{0};
    v3u start_room{0};

    void print() const {
        puts("============================");
        v3u p{0};

        range_u64(y, 0, dim.y) {
            range_u64(z, 0, dim.z) {
                range_u64(x, 0, dim.x) {
                    p = v3u{x,y,z};
                    switch (at(p)->type){
                        case ModuleType_Room: printf("o"); break;
                        case ModuleType_Shop: printf("$"); break;
                        case ModuleType_Exit: printf("<"); break;
                        default: printf("x"); break;
                    }
                    if (x<dim.x-1&&at(p)->east) printf("--"); 
                    else printf("  ");
                }

                printf("\n");

                if (z < dim.z-1) {
                    range_u64(x, 0, dim.x) {
                        if (at(v3u{x, y, z})->north) printf("|");
                        else printf(" ");
                        printf("  ");
                    }
                }
                printf("\n");
            }

            puts("============================");
        }
    }

    bool bounds(v3u x) const {
        return x.x < dim.x && x.y < dim.y && x.z < dim.z;
    }

    v3u safe_move(v3u x, v3i o) const {
        v3i n = v3i(x) + o;
        return bounds(v3u(n)) ? v3u(n) : x;
    }

    room_module_t* at(v3u x) const {
        assert(bounds(x));
        return tiles[x.x][x.y][x.z];
    }

    void get_path(v3u a, room_module_t** visited, u64& visited_top) {
        room_module_t* path[dim.x*dim.y*dim.z];

        u64 stack = 0;
        path[stack++] = at(a);
        visited[visited_top++] = at(a);

        auto is_visited = [&](room_module_t* r) {
            if (r==nullptr) return true;
            range_u64(i, 0, visited_top) {
                if (visited[i] == r) return true;
            }
            return false;
        };

        while(stack) {
            auto* room = path[--stack];
            assert(room);
            
            range_u64(adj, 0, 6) {
                if (is_visited(room->direction[adj]) == false) {
                    visited[visited_top++] = room->direction[adj];
                    path[stack++] = room->direction[adj];
                }
            }
        }
    }

    bool path_connected(v3u a, v3u b) {
        // room_module_t** visited = new room_module_t*[dim.x*dim.y*dim.z];
        // defer {
        //     delete[] visited;
        // };
        room_module_t* visited[dim.x*dim.y*dim.z];
        
        u64 visited_top = 0;

        get_path(a, visited, visited_top);

        range_u64(i, 0, visited_top) {
            if (visited[i]->coord==b) {
                return true;
            }
        }
        return false;
    }

    void connect_rooms(v3u a, v3u b, u32 a_to_b_direction) {
        assert(at(a)->direction[direction_index(a_to_b_direction)] == 0);
        assert(at(b)->direction[direction_index(anti_direction(a_to_b_direction))] == 0);

        at(a)->direction[direction_index(a_to_b_direction)] = at(b);
        at(b)->direction[direction_index(anti_direction(a_to_b_direction))] = at(a);
    }

    void fill_modules() {
        u64 i = 0;
        range_u64(z, 0, dim.z) {
            range_u64(y, 0, dim.y) {
                range_u64(x, 0, dim.x) {
                    tiles[x][y][z] = rooms + i++;
                    tiles[x][y][z]->coord = v3u{x,y,z};
                }
            }
        }
    }

    v3u random_room_on_path(v3u p) {
        room_module_t* visited[dim.x*dim.y*dim.z];
        u64 visited_top = 0;

        get_path(p, visited, visited_top);

        return visited_top > 1 ?
            visited[(world->entropy.rand()%(visited_top-1))+1]->coord :
            p;
    }

    // returns opened room coord
    v3u open_random_room_on_path(v3u p) {
        u64 tries = 100;
        while(tries--) {
            auto r = random_room_on_path(p);
            u32 d = u32(world->entropy.rand()>>8) % 6;
            if (path_open(r, 1<<d) == false && open_path(r, 1<<d)) {
                return at(r)->direction[d]->coord;
            }
        }
        zyy_warn(__FUNCTION__, "Failed to open path");
        return p;
    }

    // checks for bounds
    // returns true on success
    bool open_path(v3u a, u32 direction) {
        v3i o = direction_offset(direction);
        v3u b = safe_move(a, o);
        if (b!=a && path_open(a, direction) == false) {
            connect_rooms(a,b, direction);
            return true;
        }
        return false;
    }

    bool path_open(v3u a, u32 direction) const {
        return at(a)->direction[direction_index(direction)] != nullptr;
    }

    void build_maze(u32 iters = 160) {
        v3u p = random_tile();

        at(p)->type = ModuleType_Exit; // todo better exit placement

        while(iters--) {
            p = open_random_room_on_path(p);
        }
    }

    void place_rooms() {
        range_u64(z, 0, dim.z) {
            range_u64(y, 0, dim.y) {
                range_u64(x, 0, dim.x) {
                    auto* room = tiles[x][y][z];
                    f32 best_match = 0.0f;
                    u64 best=0;

                    module_filter_t room_filter{
                        .directions = room->opened_directions(),
                        .type = room->type,
                    };
                    
                    // Note(Zack): Idea: pick random weighted by match
                    range_u64(p, 0, array_count(gs_modules)) {
                        auto* m = gs_modules+p;
                        f32 match = m->filter.match(room_filter);

                        if (best_match < match) {
                            best_match = match;
                            best = p;
                        }
                    }

                    if(best_match>0.0f) {
                        zyy::tag_spawn(world, gs_modules[best].prefab, tile_to_world(room->coord));
                    } else {
                        zyy_warn(__FUNCTION__, "Missing module prefab for filter: direction: {}, type: {}", room_filter.directions, room_filter.type);
                    }
                }
            }
        }
    }

    void generate() {
        build_maze();
        print();
        place_rooms();
    }

    v3f tile_to_world(v3u t) const {
        return v3f(t) * room_dimensions;
    }

    v3u random_tile() const {
        return v3u{
            u32(world->entropy.rand() % dim.x),
            u32(world->entropy.rand() % dim.y),
            u32(world->entropy.rand() % dim.z)
        };
    }

    
    math::rect3d_t aabb() const {
        math::rect3d_t t{};
        t.expand(v3f{0.0f});
        t.expand(tile_to_world(dim+v3u{1}));
        return t;
    }

    explicit module_generator_t(zyy::world_t* world_) : world{world_} {
        utl::memzero(tiles, sizeof(room_module_t*)*array_count(rooms));
        utl::memzero(rooms, sizeof(room_module_t)*array_count(rooms));
        fill_modules();
    }
};

struct dungeon_generator_t {
    v3f room_dimensions{40.0f, 20.0f, 40.0f}; // half
    zyy::entity_t* tiles[8][8];
    zyy::world_t* world{nullptr};

    u64 rooms_spawned{0};
    v2u start_room{};


    
    zyy::prefab_t rooms[4] = {
        make_room("res/models/rooms/room_02.gltf"),
        make_room("res/models/rooms/room_03.gltf"),
        make_room("res/models/rooms/room_04.gltf"),
        make_room("res/models/rooms/room_05.gltf")
    };

    #undef make_room

    void generate() {
        if (rooms_spawned >= 16) return;

        v2u room_coord;

        do {
            room_coord = random_tile();
        } while(tiles[room_coord.x][room_coord.y]==nullptr);

        auto random_room = rooms[world->entropy.rand()%array_count(rooms)];
        auto* room = spawn_room(world, random_room, tile_position(room_coord));
        tiles[room_coord.x][room_coord.y] = room;

        if (rooms_spawned==0) {
            start_room = room_coord;
        }

        rooms_spawned++;
    }

    v3f tile_position(v2u pos) const {
        return v3f(pos.x, 0.0f, pos.y) * (2.0f * room_dimensions * planes::xz);
    }

    v2u random_tile() const {
        return v2u{
            u32(world->entropy.rand() % 8),
            u32(world->entropy.rand() % 8)
        };
    }


    explicit dungeon_generator_t(zyy::world_t* world_) : world{world_} {
        utl::memzero(tiles, sizeof(zyy::entity_t*)*16);
    }
};


world_generator_t*
generate_world_maze(arena_t* arena) {
    tag_struct(auto* generator, world_generator_t, arena);
    generator->arena = arena;
    generator->add_step("Environment", WORLD_STEP_TYPE_LAMBDA(environment) {
       world->render_system()->environment_storage_buffer.pool[0].fog_density = 0.01f;
       world->render_system()->environment_storage_buffer.pool[0].sun.direction = v4f{0.0};
    });
    generator->add_step("Player", WORLD_STEP_TYPE_LAMBDA(player) {
        auto* player = zyy::tag_spawn(world, zyy::db::characters::assassin, axis::up * 3.0f + axis::forward * 15.0f);
        SPAWN_GUN(zyy::db::weapons::smg, v3f(-10.5f, 4.5f, 0.0f));
    });
    generator->add_step("World Geometry", WORLD_STEP_TYPE_LAMBDA(environment) {
        zyy::tag_spawn(world, zyy::db::misc::platform_1000, axis::down);

        module_generator_t* mgen = new module_generator_t{world};
        mgen->generate();

        world->render_system()->light_probes.grid_size = 5.0f;
        rendering::update_probe_aabb(world->render_system(), mgen->aabb());

        delete mgen;


        

        dungeon_generator_t creator{world};

        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();
        // creator.generate();

        world->player->transform.origin = creator.tile_position(creator.start_room);

    });
    return generator;
}

