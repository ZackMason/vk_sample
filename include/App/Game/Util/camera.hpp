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
    return v3f{
        glm::cos((yaw)) * glm::cos((pitch)),
        glm::sin((pitch)),
        glm::sin((yaw)) * glm::cos((pitch))
    };
}

struct first_person_controller_t {
    camera_t* camera;

    math::transform_t transform{};
    f32 yaw{0};
    f32 pitch{0};

    void translate(const v3f& d) {
        transform.origin += d;

        camera->set_position(transform.origin);
        camera->look_at(transform.origin + get_direction(yaw, pitch));
    }
};

};