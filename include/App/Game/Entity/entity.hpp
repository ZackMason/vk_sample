#pragma once

#include "core.hpp"

#include "App/Game/Util/camera.hpp"

namespace game::entity {

using entity_id = u64;

static constexpr entity_id invalid_id = ~0ui32;

enum EntityFlags {
    EntityFlags_Spatial = BIT(0),
    EntityFlags_Renderable = BIT(1),
};

struct entity_t : node_t<entity_t> {
    entity_id   id{0};
    EntityFlags flags{};
    u64         tag{0};
    string_t    name;
    entity_t*   parent{nullptr};

    math::transform_t   transform;
    math::aabb_t<v3f>        aabb;

    math::transform_t global_transform() const {
        if (parent) {
            return math::transform_t{parent->global_transform().to_matrix() * transform.to_matrix()};
        }
        return transform;
    }

    struct renderable_t {
        u64 material_id{0};
        u64 mesh_id{0};
        u64 albedo_id{0};
    } gfx;

    virtual ~entity_t() = default;
    entity_t() noexcept = default;
    entity_t(const entity_t& o) noexcept = delete;
    entity_t(entity_t&& o) noexcept = delete;
    entity_t& operator=(entity_t&& o) noexcept = delete;
    entity_t& operator=(const entity_t& o) noexcept = delete;
};

struct character_t : public entity_t {
    struct health_t {
        f32 max{100.0f}, current{100.0f};
    } health;
};

struct player_t : public character_t {
    cam::first_person_controller_t camera_controller;
};

inline void 
player_init(player_t* player, cam::camera_t* camera, u64 mesh_id) {
    player->flags = EntityFlags_Spatial;
    player->gfx.mesh_id = mesh_id;
    player->camera_controller.camera = camera;
}

};