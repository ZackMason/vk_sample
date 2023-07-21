#pragma once

#include "core.hpp"

namespace game::cam {

struct camera_t : math::transform_t {
    math::ray_t get_ray(const v2f& uv);

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
        yaw += rot.y;
        pitch += rot.x;
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
    f32 yaw{0};
    f32 pitch{0};

    // f32 head_height{1.0f};

    void translate(const v3f& d) {
        transform.origin += d;// head_height;
        camera->set_position(transform.origin);
        camera->look_at(transform.origin + get_direction(yaw, pitch));
    }
};

};