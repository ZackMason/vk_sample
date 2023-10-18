#pragma once

#include "App/Game/Entity/brain.hpp"
#include "App/Game/Entity/entity.hpp"
#include "App/Game/Entity/entity_db.hpp"
#include "App/Game/Physics/player_movement.hpp"

#define BRAIN_BEHAVIOR_FUNCTION(name) static void name(zyy::world_t* world, zyy::entity_t* entity, brain_t* brain, f32 dt)

void entity_blackboard_common(zyy::entity_t* entity) {
    auto* brain = &entity->brain;
    auto* rng = utl::hash_get(&brain->blackboard.points, "rng"sv, brain->blackboard.arena);
    auto* rng_move = utl::hash_get(&brain->blackboard.points, "rng_move"sv, brain->blackboard.arena);
    auto* self = utl::hash_get(&brain->blackboard.points, "self"sv, brain->blackboard.arena);
    auto* health = utl::hash_get(&brain->blackboard.floats, "health"sv, brain->blackboard.arena);

    *utl::hash_get(&brain->blackboard.bools, "has_weapon"sv, brain->blackboard.arena) = 
        entity->primary_weapon.entity != nullptr;
    assert(rng);

    *health = entity->stats.character.health.current;
    *self = entity->global_transform().origin;

    *rng = entity->world->entropy.randv<v3f>();
    *rng_move = entity->world->entropy.randnv<v3f>() * planes::xz;
}

BRAIN_BEHAVIOR_FUNCTION(player_behavior) {
    auto* input = &world->game_state->input();
    auto& imgui = world->game_state->gui.state;
    auto pc = input->gamepads[0].is_connected ? gamepad_controller(input) : keyboard_controller(input);
    auto* physics = world->physics;
    auto* player = world->player;
    auto* rigidbody = player->physics.rigidbody;
    const bool is_on_ground = rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
    const bool is_on_wall = rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;
    const bool ignore_mouse = gfx::gui::im::want_mouse_capture(imgui) || gs_debug_camera_active;
    const bool ignore_keyboard = gfx::gui::im::want_mouse_capture(imgui) || gs_debug_camera_active;

    if (pc.swap) {
        std::swap(
            player->primary_weapon.entity,
            player->secondary_weapon.entity
        );
    }


    player->camera_controller.transform = 
        player->transform;
    
    v3f move = pc.move_input;
    const v2f head_move = pc.look_input;

    // auto& yaw = gs_debug_camera_active ? gs_debug_camera.yaw : player->camera_controller.yaw;
    // auto& pitch = gs_debug_camera_active ? gs_debug_camera.pitch : player->camera_controller.pitch;
    auto& yaw = player->camera_controller.yaw;
    auto& pitch = player->camera_controller.pitch;

    const v3f forward = zyy::cam::get_direction(yaw, pitch);
    const v3f right   = glm::cross(forward, axis::up);

    if (!ignore_mouse) {
        yaw += head_move.x * dt;
        pitch -= head_move.y * dt;
    }
    if (ignore_keyboard) {
        move = v3f{0.0f};
    }

    // const f32 move_speed = 45.0f * (pc.sprint ? 1.75f : 1.0f) * (rigidbody->is_on_surface() ? 1.0f : 0.33f);

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
    local_persist f32 gravity_strength = 1.0f; DEBUG_WATCH(&gravity_strength);
    f32 max_speed = is_on_ground ? max_ground_speed : max_air_speed;
    f32 accel = is_on_ground ? ground_accel : air_accel;
    
    auto movement = is_on_ground ? quake_ground_move : quake_air_move;
    rigidbody->velocity = movement(wishdir, rigidbody->velocity, friction, accel, max_speed, dt);
    if (is_on_ground == false) {
        rigidbody->velocity.y -= 9.81f * 0.05f * dt * gravity_strength;
    } else {
        // rigidbody->velocity.y = 0.0f;
    }
        // rigidbody->force += ((glm::normalize(planes::xz * forward) * move.z + right * move.x) * dt * move_speed);
    // }

    if(player->primary_weapon.entity) {
        player->primary_weapon.entity->transform.set_rotation(glm::quatLookAt(forward, axis::up));
        // player->primary_weapon.entity->transform.set_scale(v3f{3.0f});
        player->primary_weapon.entity->transform.origin =
            player->global_transform().origin +
            forward + axis::up * .9f;
            //  0.5f * (right + axis::up);
    }
    rigidbody->angular_velocity = v3f{0.0f};

    if (pc.jump && (is_on_ground || is_on_wall)) {
        rigidbody->velocity.y = 0.3f;// 50.0f * dt;
    }
    player->camera_controller.transform.origin = player->transform.origin + axis::up * 1.0f;

    const auto stepped = player->camera_controller.walk_and_bob(dt * (pc.sprint ? 1.75f : 1.0f), glm::length(swizzle::xz(move)) > 0.0f && is_on_ground, move.x);
    player->camera_controller.transform.origin += 
        axis::up * player->camera_controller.head_height + 
        axis::up * player->camera_controller.head_offset;
    player->camera_controller.translate(v3f{0.0f});

    auto& hand_pid_x = player->camera_controller.hand_controller.x;
    auto& hand_pid_y = player->camera_controller.hand_controller.y;
    auto& hand_pid_z = player->camera_controller.hand_controller.z;
    DEBUG_WATCH(&hand_pid_x.proportional_gain);
    DEBUG_WATCH(&hand_pid_y.proportional_gain);
    DEBUG_WATCH(&hand_pid_z.proportional_gain);
    DEBUG_WATCH(&hand_pid_x.derivative_gain);
    DEBUG_WATCH(&hand_pid_x.derivative_gain);
    DEBUG_WATCH(&hand_pid_y.derivative_gain);
    DEBUG_WATCH(&hand_pid_y.integral_gain);
    DEBUG_WATCH(&hand_pid_z.integral_gain);
    DEBUG_WATCH(&hand_pid_z.integral_gain);

    // if(player->primary_weapon.entity) {
    //     auto fforward = -player->primary_weapon.entity->transform.basis[2];
    //     player->primary_weapon.entity->transform.origin += 
    //         player->camera_controller.hand_controller.compute(dt,
    //             player->primary_weapon.entity->transform.origin,
    //             player->global_transform().origin +
    //             forward + axis::up * 0.3f);

    //     player->camera_controller.last_position = player->primary_weapon.entity->transform.origin;

    // }
    
    
    if (stepped) {
        // Platform.audio.play_sound(0x1);
    }

    auto* weapon_entity = player->primary_weapon.entity;
    auto& weapon = player->primary_weapon.entity->stats.weapon;

    if (player->primary_weapon.entity) {
        weapon.update(dt);
    }
    if (!ignore_mouse && pc.fire1 && player->primary_weapon.entity 
        // && gfx::gui::im::want_mouse_capture(*gs_imgui_state) == false
    ) {
        auto fire_arena = begin_temporary_memory(&world->frame_arena.get());
        u64 fired{0};
        auto* bullets = weapon.fire(fire_arena.arena, dt, &fired);

        range_u64(bullet, 0, fired) {
            auto ro = weapon_entity->global_transform().origin + forward * 1.7f;
            auto r_angle = utl::rng::random_s::randf() * 1000.0f;
            auto r_radius = utl::rng::random_s::randf() * bullets[bullet].spread;
            v2f polar{math::from_polar(r_angle, r_radius)};
            auto rd = glm::normalize(forward * 10.0f + right * polar.x + axis::up * polar.y);
            bullets[bullet].ray = math::ray_t{ro, rd};
            math::ray_t gun_shot{ro, rd};
            DEBUG_DIAGRAM(gun_shot);
        }
        
        range_u64(bullet, 0, fired) {
            auto* b = weapon.bullet_fn(
                world, zyy::db::misc::plasma_bullet,
                bullets[bullet]
            );
            b->stats.effect = player->primary_weapon.entity->stats.effect;
        }

        end_temporary_memory(fire_arena);
    }
}

template <size_t N>
void collect_nearby(
    arena_t* arena,
    auto* physics, 
    stack_buffer<interest_point_t, N>& buffer, 
    v3f position, 
    f32 distance
) {
    buffer.clear();

    auto* near = physics->sphere_overlap_world(
        physics,
        arena,
        position,
        distance
    );

    range_u64(i, 0, near->hit_count) {
        if (buffer.is_full()) break;
        auto* rb = (physics::rigidbody_t*)near->hits[i].user_data;
        auto* n = (zyy::entity_t*)rb->user_data;
        interest_point_t interest = {};
        interest.data = n;
        interest.point = n->global_transform().origin;
        interest.type = interest_type::entity;
        buffer.push(std::move(interest));
    }
}

template <size_t N>
void collect_nearby_enemy(
    arena_t* arena,
    auto* physics, 
    stack_buffer<interest_point_t, N>& buffer, 
    v3f position, 
    f32 distance,
    std::span<brain_type> enemy_types
) {
    buffer.clear();

    auto* near = physics->sphere_overlap_world(
        physics,
        arena,
        position,
        distance
    );

    range_u64(i, 0, near->hit_count) {
        if (buffer.is_full()) break;
        auto* rb = (physics::rigidbody_t*)near->hits[i].user_data;
        auto* n = (zyy::entity_t*)rb->user_data;

        if (std::find(enemy_types.begin(), enemy_types.end(), n->brain.type) == enemy_types.end()) {
            continue;
        }

        interest_point_t interest = {};
        interest.data = n;
        interest.point = n->global_transform().origin;
        interest.type = interest_type::entity;
        buffer.push(std::move(interest));
    }
}

template <size_t N>
void collect_nearby(
    arena_t* arena,
    auto* world,
    auto* physics, 
    stack_buffer<skull_brain_t*, N>& buffer, 
    v3f position, 
    f32 distance
) {
    buffer.clear();

    auto* near = physics->sphere_overlap_world(
        physics,
        arena,
        position,
        distance
    );

    range_u64(i, 0, near->hit_count) {
        if (buffer.is_full()) break;
        auto* rb = (physics::rigidbody_t*)near->hits[i].user_data;
        auto* n = (zyy::entity_t*)rb->user_data;
        
        if (n->brain.type != brain_type::flyer) {
            continue;
        }

        buffer.push(&n->brain.skull);
    }

}

BRAIN_BEHAVIOR_FUNCTION(person_behavior) {
    auto* physics = world->physics;
    auto* rigidbody = entity->physics.rigidbody;
    const bool is_on_ground = rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
    const bool is_on_wall = rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;
    auto& health = entity->stats.character.health;
    auto& blkbrd = entity->brain.blackboard;

    auto* last_health = utl::hash_get(&blkbrd.floats, "health"sv, blkbrd.arena);

    f32 health_delta = glm::max(0.0f, *last_health - health.current);

    entity_blackboard_common(entity);

    v3f position = entity->global_transform().origin;

    stack_buffer<interest_point_t, 32> buffer = {};
    brain_type enemy_types[] = {brain_type::player, brain_type::flyer};
    collect_nearby_enemy(
        &world->frame_arena.get(),
        physics,
        buffer,
        entity->global_transform().origin,
        20.0f,
        enemy_types
    );

    brain->person.fear += (health_delta / health.max) * 20.0f;
    brain->person.fear = tween::damp(brain->person.fear, 0.4f, 0.05f, dt);

    auto* has_target = utl::hash_get(&blkbrd.bools, "has_target"sv, blkbrd.arena);
    auto* target = utl::hash_get(&blkbrd.points, "target"sv, blkbrd.arena);
    auto* closest_weapon = utl::hash_get(&blkbrd.points, "closest_weapon"sv, blkbrd.arena);
    auto* fear = utl::hash_get(&blkbrd.floats, "fear"sv, brain->blackboard.arena);
    
    *fear = brain->person.fear;
    *has_target = buffer.empty() == false;

    if (*has_target) {
        *target = buffer[0].point;
        range_u64(i, 0, buffer.count()) {
            auto* e = (zyy::entity_t*)buffer[i].data;
            if (e->flags & zyy::EntityFlags_Pickupable) {
                *closest_weapon = buffer[i].point;
            }
        }
    }

    brain->person.tree.tick(dt, &blkbrd);

    v3f move = blkbrd.move;

    v3f forward = axis::forward;
    v3f right = axis::right;
       
    v3f move_xz = v3f{move.x, 0.0f, move.z};
    v3f wishdir = move_xz;

    if (wishdir.x || wishdir.y || wishdir.z) {
        wishdir = glm::normalize(wishdir);
    }

    local_persist f32 max_ground_speed = 3.60f; DEBUG_WATCH(&max_ground_speed)->max_f32 = 10.0f;
    local_persist f32 max_air_speed = 0.3f; DEBUG_WATCH(&max_air_speed)->max_f32 = 4.0f;
    local_persist f32 ground_accel = 2.2f; DEBUG_WATCH(&ground_accel)->max_f32 = 6.0f; // source engine 5.6 default
    local_persist f32 air_accel = 0.5f; DEBUG_WATCH(&air_accel)->max_f32 = 3.0f;
    local_persist f32 friction = 9.1f; DEBUG_WATCH(&friction)->max_f32 = 10.0f;
    local_persist f32 gravity_strength = 1.0f; DEBUG_WATCH(&gravity_strength);
    f32 max_speed = is_on_ground ? max_ground_speed : max_air_speed;
    f32 accel = is_on_ground ? ground_accel : air_accel;
    
    auto movement = is_on_ground ? quake_ground_move : quake_air_move;
    rigidbody->velocity = movement(wishdir, rigidbody->velocity, friction, accel, max_speed, dt);
    if (is_on_ground == false) {
        rigidbody->velocity.y -= 9.81f * 0.05f * dt * gravity_strength;
    } 
    
    rigidbody->angular_velocity = v3f{0.0f};
    
}

BRAIN_BEHAVIOR_FUNCTION(skull_behavior) {
    auto* physics = world->physics;
    auto* player = world->player;
    auto* rb = entity->physics.rigidbody;
    auto max_speed = entity->stats.character.movement.move_speed;

    // collect_nearby(
    //     &world->frame_arena.get(),
    //     world,
    //     physics,
    //     brain->skull.neighbors,
    //     entity->global_transform().origin,
    //     20.0f
    // );

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