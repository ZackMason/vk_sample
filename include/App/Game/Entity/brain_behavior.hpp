#pragma once

#include "App/Game/Entity/brain.hpp"
#include "App/Game/Entity/entity.hpp"
#include "App/Game/Physics/player_movement.hpp"

#define BRAIN_BEHAVIOR_FUNCTION(name) static void name(zyy::world_t* world, zyy::entity_t* entity, brain_t* brain, f32 dt)

BRAIN_BEHAVIOR_FUNCTION(player_behavior) {
    auto* input = &world->game_state->input();
    auto pc = input->gamepads[0].is_connected ? gamepad_controller(input) : keyboard_controller(input);
    auto* physics = world->physics;
    auto* player = world->player;
    auto* rigidbody = player->physics.rigidbody;
    const bool is_on_ground = rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
    const bool is_on_wall = rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;

    player->camera_controller.transform = 
        player->transform;
    
    const v3f move = pc.move_input;
    const v2f head_move = pc.look_input;

    // auto& yaw = gs_debug_camera_active ? gs_debug_camera.yaw : player->camera_controller.yaw;
    // auto& pitch = gs_debug_camera_active ? gs_debug_camera.pitch : player->camera_controller.pitch;
    auto& yaw = player->camera_controller.yaw;
    auto& pitch = player->camera_controller.pitch;

    const v3f forward = zyy::cam::get_direction(yaw, pitch);
    const v3f right   = glm::cross(forward, axis::up);

    // if (gs_imgui_state && gfx::gui::im::want_mouse_capture(*gs_imgui_state) == false) {
        yaw += head_move.x * dt;
        pitch -= head_move.y * dt;
    // }

    const f32 move_speed = 45.0f * (pc.sprint ? 1.75f : 1.0f) * (rigidbody->is_on_surface() ? 1.0f : 0.33f);

    // if (gs_debug_camera_active) {
    //     gs_debug_camera.transform.origin += ((forward * move.z + right * move.x + axis::up * move.y) * dt * move_speed);
    //     return;
    // } else {
    v3f move_xz = v3f{move.x, 0.0f, move.z};
    v3f wishdir = 
        glm::normalize(planes::xz * forward) * move_xz.z + 
        glm::normalize(planes::xz * right) * move_xz.x;

    if (wishdir.x || wishdir.y || wishdir.z) {
        wishdir = glm::normalize(wishdir);
    }

    local_persist f32 max_ground_speed = 3.60f; DEBUG_WATCH(&max_ground_speed)->max_f32 = 10.0f;
    local_persist f32 max_air_speed = 0.3f; DEBUG_WATCH(&max_air_speed)->max_f32 = 4.0f;
    local_persist f32 ground_accel = 2.2f; DEBUG_WATCH(&ground_accel)->max_f32 = 6.0f; // source engine 5.6 default
    local_persist f32 air_accel = 0.5f; DEBUG_WATCH(&air_accel)->max_f32 = 3.0f;
    local_persist f32 friction = 9.1f; DEBUG_WATCH(&friction)->max_f32 = 10.0f;
    
    f32 max_speed = is_on_ground ? max_ground_speed : max_air_speed;
    f32 accel = is_on_ground ? ground_accel : air_accel;
    
    auto movement = is_on_ground ? quake_ground_move : quake_air_move;
    rigidbody->velocity = movement(wishdir, rigidbody->velocity, friction, accel, max_speed, dt);
    if (is_on_ground == false) {
        rigidbody->velocity.y -= 9.81f * 0.05f * dt;
    } else {
        rigidbody->velocity.y = 0.0f;
    }
        // rigidbody->force += ((glm::normalize(planes::xz * forward) * move.z + right * move.x) * dt * move_speed);
    // }

    if(player->primary_weapon.entity) {
        player->primary_weapon.entity->transform.set_rotation(glm::quatLookAt(forward, axis::up));
        // player->primary_weapon.entity->transform.set_scale(v3f{3.0f});
        player->primary_weapon.entity->transform.origin =
            player->global_transform().origin +
            forward + axis::up * 0.3f;
            //  0.5f * (right + axis::up);
    }
    rigidbody->angular_velocity = v3f{0.0f};

    if (pc.jump && (is_on_ground || is_on_wall)) {
        rigidbody->velocity.y = 0.3f;// 50.0f * dt;
    }
    player->camera_controller.transform.origin = player->transform.origin + axis::up * 1.0f;

    const auto stepped = player->camera_controller.walk_and_bob(dt * (pc.sprint ? 1.75f : 1.0f), glm::length(swizzle::xz(move)) > 0.0f && is_on_ground, move.x);
    if (stepped) {
        // Platform.audio.play_sound(0x1);
    }

    if (player->primary_weapon.entity) {
        player->primary_weapon.entity->stats.weapon.update(dt);
    }
    if (pc.fire1 && player->primary_weapon.entity 
        // && gfx::gui::im::want_mouse_capture(*gs_imgui_state) == false
    ) {

        temp_arena_t fire_arena = world->frame_arena.get();
        u64 fired{0};
        auto* bullets = player->primary_weapon.entity->stats.weapon.fire(&fire_arena, dt, &fired);

        range_u64(bullet, 0, fired) {
            auto ro = player->camera_controller.transform.origin + forward * 1.7f;
            auto r_angle = utl::rng::random_s::randf() * 1000.0f;
            auto r_radius = utl::rng::random_s::randf() * bullets[bullet].spread;
            v2f polar{math::from_polar(r_angle, r_radius)};
            auto rd = glm::normalize(forward * 10.0f + right * polar.x + axis::up * polar.y);
            bullets[bullet].ray = math::ray_t{ro, rd};
            // if (auto ray = physics->raycast_world(physics, ro, rd); ray.hit) {
                // auto* rb = (physics::rigidbody_t*)ray.user_data;
                math::ray_t gun_shot{ro, rd};
                DEBUG_ADD_VARIABLE(gun_shot);
                // if (rb == player->physics.rigidbody) {
                //     zyy_warn(__FUNCTION__, "player shot them self");
                // }
                // auto hp = (ray.point);
                // zyy::entity_t* hit_entity=0;
                // if (rb->type == physics::rigidbody_type::DYNAMIC ||
                //     rb->type == physics::rigidbody_type::KINEMATIC
                // ) {
                //     // rb->inverse_transform_direction
                //     // auto f = rb->inverse_transform_direction(rd);
                //     // rb->add_force(rd*1.0f);
                //     hit_entity = (zyy::entity_t*)rb->user_data;
                //     rb->add_force_at_point(rd*50.0f, hp);
                //     math::ray_t force{hp, rd};
                //     DEBUG_ADD_VARIABLE(force);
                // } else {
                //     DEBUG_ADD_VARIABLE(ray.point);
                // }

                // auto* hole = zyy::spawn(world, world->render_system(), zyy::db::misc::bullet_hole, hp);
                // hole->transform.set_rotation(world->entropy.randnv<v3f>() * 100.0f);
                // hole->coroutine->start();
                // if (hit_entity) {
                //     if (hit_entity->stats.character.health.max) {
                //         if (hit_entity->stats.character.health.damage(bullets[bullet].damage)) {
                //             hit_entity->queue_free();
                //         }
                //     }
                //     // auto local_pos = hole->global_transform().origin - hit_entity->global_transform().origin;
                //     hit_entity->add_child(hole, true);
                //     hole->transform.origin = hit_entity->global_transform().inv_xform(hp);
                // }
            // }
        }
        range_u64(bullet, 0, fired) {
            zyy::wep::spawn_bullet(
                world, zyy::db::misc::bullet_01,
                bullets[bullet], dt
            );
        }
    }
}

BRAIN_BEHAVIOR_FUNCTION(skull_behavior) {
    auto* physics = world->physics;
    auto* player = world->player;
    auto* rb = entity->physics.rigidbody;
    auto max_speed = entity->stats.character.movement.move_speed;

    auto vel = entity->physics.rigidbody->velocity;
    auto s = glm::length(vel);

    v3f to_target = player->global_transform().origin - entity->global_transform().origin;
    v3f local_target = rb->inverse_transform_direction(to_target);

    if (to_target.y > -5.0f) {
        to_target.y = 5.0f;
    }

    local_persist f32 skull_air_accel = 5.0f; DEBUG_WATCH(&skull_air_accel)->max_f32 = 10.0f;
    local_persist f32 skull_max_air_speed = 5.0f; DEBUG_WATCH(&skull_max_air_speed)->max_f32 = 10.0f;

    v3f target_vel = quake_air_move(glm::normalize(to_target), vel, 0.0f, skull_air_accel, skull_max_air_speed, dt);
    
    rb->set_velocity(target_vel);
    // rb->add_force();
}