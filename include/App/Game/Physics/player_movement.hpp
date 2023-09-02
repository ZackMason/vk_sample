#pragma once

#include "zyy_core.hpp"

static v3f
quake_accelerate(v3f wishdir, v3f velocity, float accelerate, float max_velocity, float dt) {
    float p_vel = glm::dot(velocity, wishdir);
    float a_vel = glm::clamp(max_velocity - p_vel, 0.0f, accelerate * dt);
    // if (p_vel + a_vel > max_velocity) {
    //     a_vel = max_velocity - p_vel;
    // }
    return velocity + wishdir * a_vel;
}

static v3f
quake_ground_move(v3f wishdir, v3f velocity, float friction, float accel, float max_velocity, float dt) {
    float fall_speed = velocity.y;

    float speed = glm::length(velocity);
    if (speed > 0.0f) {
        float drop = speed * friction * dt;
        velocity *= glm::max(speed-drop, 0.0f) / speed;
    }
    velocity = quake_accelerate(wishdir, velocity, accel, max_velocity, dt);
    // velocity.y = fall_speed;
    return velocity;
}

static v3f
quake_air_move(v3f wishdir, v3f velocity, float friction, float accel, float max_velocity, float dt) {
    return quake_accelerate(wishdir, velocity, accel, max_velocity, dt);
}


