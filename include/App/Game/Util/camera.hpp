#pragma once

#include "zyy_core.hpp"

namespace zyy::cam {

struct camera_t : math::transform_t {
    math::ray_t get_ray(const v2f& uv);

    u32 current_priority = 0;

    void reset_priority() {
        current_priority = 0;
    }

    camera_t* acquire(u32 prio) {
        local_persist camera_t dummy;
        if (prio >= current_priority) {
            current_priority = prio;
            return this;
        } else {
            return &dummy;
        }
    }

    void set_position(const v3f& p) {
        origin = p;
    }
};

inline v3f 
get_direction(float yaw, float pitch) noexcept {
    // note(zack): probably dont need to normalize this, but i want to
    return glm::normalize(math::get_spherical(yaw, pitch));
}

struct orbit_camera_t {
    v3f position{0.0f};
    f32 distance{1.0f};
    f32 yaw{0.0f}, pitch{0.0f};

    void rotate(v2f rot) noexcept {
        yaw += rot.x;
        pitch += -rot.y;
    }

    v3f forward() const noexcept { return get_direction(yaw, pitch); }
    v3f backward() const noexcept { return -forward(); }
    v3f right() const noexcept { return glm::cross(forward(), axis::up); }
    v3f left() const noexcept { return -right(); }
    
    void move(v3f delta) {
        position += delta;
    }

    m44 view() {
        return glm::lookAt(position + get_direction(yaw, pitch) * -distance, position, axis::up);
    }
};

struct first_person_controller_t {
    camera_t* camera;

    math::transform_t transform{};

    math::position_pid_t hand_controller{};
    v3f hand_offset{axis::forward + axis::up * .9f};
    v3f hand{0.0f};
    v3f hand_velocity = {};
    v3f hand_angular_velocity = {};
    quat hand_orientation = quat_identity;
    v3f last_position{};

    u32 prio = 1;

    f32 yaw{0};
    f32 pitch{0};

    f32 head_height{1.0f};
    f32 head_offset{0.0f};
    f32 walk_time{0.0f};
    f32 right_offset{0.0};
    f32 step_time{0.5f};
    f32 step_timer{step_time};

    void emote1() {
        hand_angular_velocity.x -= 60.0f;
        hand_angular_velocity.z += 40.0f * utl::rng::random_s::randn();
        hand += axis::backward * 4.0f;
    }

    // returns true if you should want to play a sound
    bool walk_and_bob(f32 dt, b32 walking, f32 walking_right) {
        walk_time += 10.0f * dt * (walking ? 1.0f : 0.0f);
        head_offset += glm::sin(walk_time) * dt * (walking ? 1.0f : 0.0f) * 0.5f;
        // utl::noise::fbm(v2f{walk_time}) * dt;
        walk_time = tween::damp(walk_time, 0.0f, (walking ? 0.0f : 3.0f), dt);
        head_offset = tween::damp(head_offset, 0.0f, 0.95f + (walking ? 0.0f : 3.0f), dt);

        right_offset += dt * walking_right * 0.05f;
        const f32 MAX_RIGHT_OFFSET = 0.1f;

        right_offset = glm::clamp(right_offset, -MAX_RIGHT_OFFSET, MAX_RIGHT_OFFSET);
        right_offset = tween::damp(right_offset, 0.0f, 0.95f + (walking ? 0.0f : 3.0f), dt);

        step_timer -= dt * (walking ? 1.0f : -1.0f);
        step_timer = glm::min(step_timer, step_time);
        if (step_timer < 0.0f) {
            step_timer = step_time;
            return true;
        }    
        return false;
    }

    void translate(const v3f& d) {
        transform.origin += d;// head_height;
        camera->acquire(prio)->set_position(transform.origin);
        const auto forward = get_direction(yaw, pitch);
        const auto right = glm::cross(forward, axis::up);
        const auto new_up = glm::normalize(axis::up + right * right_offset);
        camera->acquire(prio)->look_at(transform.origin + get_direction(yaw, pitch), new_up);
    }
};

};