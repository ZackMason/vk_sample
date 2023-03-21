#pragma once

#include "core.hpp"

#include "uid.hpp"

#include "App/Game/Util/camera.hpp"
#include "App/Game/Weapons/base_weapon.hpp"

#include <variant>

namespace game::entity {

enum EntityFlags {
    EntityFlags_Spatial = BIT(0),
    EntityFlags_Renderable = BIT(1),
    EntityFlags_Pickupable = BIT(2),
};

enum PhysicsEntityFlags {
    PhysicsEntityFlags_None = 0,
    PhysicsEntityFlags_Static = BIT(0),
    PhysicsEntityFlags_Dynamic = BIT(1),
    PhysicsEntityFlags_Trigger = BIT(2),
    PhysicsEntityFlags_Character = BIT(3),
};

enum struct entity_type {
    environment,

    player,
    enemy,

    weapon,
    weapon_part,
    item,

    trigger,

    SIZE
};

struct health_t {
    f32 max{100.0f};
    f32 current{};

    constexpr health_t(f32 v = 100.0f) noexcept
        : max{v}, current{v} {}
};

struct movement_t {
    f32 move_speed{1.0f};
};

struct character_stats_t {
    health_t health;
    movement_t movement;
};

struct entity_coroutine_t {
    entity_coroutine_t* next{0};
    coroutine_t* corourtine{0};
    void (*func)(coroutine_t*) {[](coroutine_t* c){return;}};
};

DEFINE_TYPED_ID(entity_id);

struct entity_t;

struct entity_ref_t {
    entity_t* entity;
    entity_id id;

    // constexpr entity_ref_t(entity_t* e = nullptr) noexcept;
    //     : entity{e}, id{e?e->id:uid::invalid_id}
    // {
    // }

    entity_t* operator->();
    // {
    //     assert(entity->id == id);
    //     return entity;
    // }
};

struct entity_t : node_t<entity_t> {
    entity_id   id{uid::invalid_id};
    entity_type type{entity_type::SIZE};
    u64         flags{0};
    u64         tag{0};
    string_t    name;

    entity_t*   parent{nullptr};
    entity_t*   next_id_hash{nullptr};

    math::transform_t   transform;
    math::aabb_t<v3f>        aabb;

    math::transform_t global_transform() const {
        if (parent) {
            m33 new_basis = parent->global_transform().basis;
            range_u32(i, 0, 3) {
                new_basis[i] = glm::normalize(new_basis[i]);
            }
            return math::transform_t{new_basis, parent->global_transform().xform(transform.origin)};
            return math::transform_t{parent->global_transform().to_matrix() * transform.to_matrix()};
        }
        return transform;
    }

    struct renderable_t {
        u32 material_id{0};
        u64 mesh_id{0};
        u64 albedo_id{0};
    } gfx;

    struct physics_t {
        u32 flags{0};
        // phys::rigid_body_t rigid_body;
    } physics;


    struct stats_t {
        std::variant<character_stats_t, wep::base_weapon_t> character_, weapon_;

        wep::base_weapon_t weapon() const {
            return std::get<wep::base_weapon_t>(weapon_);
        }
        character_stats_t character() const {
            return std::get<character_stats_t>(character_);
        }
    } stats;

    entity_ref_t primary_weapon{0};
    entity_ref_t secondary_weapon{0};

    cam::first_person_controller_t camera_controller;

    entity_coroutine_t* coroutine{0};

    virtual ~entity_t() = default;
    entity_t() noexcept = default;
};

inline void 
entity_add_coroutine(entity_t* entity, entity_coroutine_t* coro) {
    node_push(coro, entity->coroutine);
}

inline void
entity_init(entity_t* entity, u64 mesh_id = -1) {
    entity->flags |= EntityFlags_Spatial;
    entity->flags |= mesh_id != -1 ? EntityFlags_Renderable : 0;
    entity->gfx.mesh_id = mesh_id;
}

// inline void
// physics_entity_init_character(
//     entity_t* entity,
//     u64 mesh_id = 0
//     // phys::physics_world_t* physx_world = 0
// ) {
//     entity_init(entity, mesh_id);

//     // entity->physics.rigid_body.state = physx_world->state;
//     // entity->physics.rigid_body.create_character(physx_world);
// }


// inline void
// physics_entity_init(
//     entity_t* entity, 
//     u64 mesh_id = 0, 
//     std::byte* physx_data = 0, 
//     u32 physx_size = 0,
//     phys::physx_state_t* physx_world = 0
// ) {
//     entity->physics.rigid_body.state = physx_world;
//     entity->physics.rigid_body.load(physx_data, physx_size, entity->global_transform());
// }

inline void 
player_init(
    entity_t* player, 
    cam::camera_t* camera, 
    u64 mesh_id) {
    // physics_entity_init_character(player, mesh_id, phys);

    player->type = entity_type::player;

    player->gfx.mesh_id = mesh_id;
    player->camera_controller.camera = camera;
}

// inline void
// entity_set_physics_transform(entity_t* entity) {
//     TIMED_FUNCTION;
//     physx::PxTransform t;
//     const auto& v = entity->global_transform().origin;
//     const auto& q = entity->global_transform().get_orientation();
//     t.p = {v.x, v.y, v.z};
//     t.q = {q.x, q.y, q.z, q.w};

//     bool is_dynamic = entity->physics.flags == PhysicsEntityFlags_Dynamic && entity->physics.rigid_body.dynamic;
//     bool is_static = entity->physics.flags == PhysicsEntityFlags_Static && entity->physics.rigid_body.rigid;

//     if (is_dynamic) {
//         entity->physics.rigid_body.dynamic->setGlobalPose(t, false);
//     } else if (is_static) {
//         entity->physics.rigid_body.rigid->setGlobalPose(t);
//     }
// }

// inline void
// entity_update_physics(entity_t* entity) {
//     if (entity->physics.flags & PhysicsEntityFlags_Dynamic) {
//         if (entity->physics.rigid_body.dynamic) {
//             TIMED_FUNCTION;
//             const auto physx_transform = entity->physics.rigid_body.dynamic->getGlobalPose();
//             const auto& [x, y, z] = physx_transform.p;
//             const auto& q = physx_transform.q;
//             entity->transform.origin = { x, y, z }; // Need to set to global position
//             entity->transform.basis = glm::toMat4(glm::quat{ q.w, q.x, q.y, q.z });
//         }
//     }
// }

}; // namespace entity

#include "entity_db.hpp"
