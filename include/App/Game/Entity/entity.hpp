#pragma once

#include "core.hpp"

#include "uid.hpp"

#include "App/Game/Util/camera.hpp"
#include "App/Game/Weapons/base_weapon.hpp"
#include "App/Game/Entity/entity_concept.hpp"
#include <variant>

namespace game {


// most of these should be inferred from entity data
enum EntityFlags : u64 {
    EntityFlags_Breakpoint = BIT(0),
    // EntityFlags_Spatial = BIT(1), // is this really needed??
    // EntityFlags_Renderable = BIT(2), // is this really needed??
    EntityFlags_Pickupable = BIT(3),
    EntityFlags_Interactable = BIT(4),
    EntityFlags_Dying = BIT(11),
    EntityFlags_Dead = BIT(12),
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

    effect,

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
    health_t health{};
    movement_t movement{};
};

struct entity_coroutine_t {
    entity_coroutine_t* next{0};
    coroutine_t coroutine;

    bool _is_running = false;

    void start() {
        if (!_is_running) {
            coroutine.line=0;
        }
        _is_running = true;
    }
    void run() {
        if (func && _is_running) {
            func(&coroutine);
            _is_running = coroutine.running;
        }
    }
    void (*func)(coroutine_t*);
};

DEFINE_TYPED_ID(entity_id);

struct entity_t;

struct entity_ref_t {
    entity_t* entity;
    entity_id id;

    entity_t* operator->();
};

struct world_t;

// using entity_update_function = void(*)(entity_t*, world_t*);
// using entity_interact_function = void(*)(entity_t*, world_t*);

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
            return math::transform_t{parent->global_transform().to_matrix() * transform.to_matrix()};
        }
        return transform;
    }

    struct renderable_t {
        u64 mesh_id{0};
        u32 albedo_id{0};
        u32 material_id{0};
        u32 animation_id{0};

        u32 instance_count{1};
        m44* instance_buffer{0};
    } gfx;

    struct physics_t {
        u32 flags{0};
        physics::rigidbody_t* rigidbody;
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

    std::optional<entity_coroutine_t> coroutine{};

    // entity_update_function on_update{nullptr};
    // entity_interact_function on_interact{nullptr};

    virtual ~entity_t() = default;
    constexpr entity_t() noexcept = default;

    constexpr bool is_renderable() const noexcept {
        return !(gfx.mesh_id == -1);
    }

    constexpr bool is_alive() const noexcept {
        return id != uid::invalid_id &&
            ((flags & (EntityFlags_Dying|EntityFlags_Dead)) == 0);
    }

    void queue_free() noexcept {
        flags |= EntityFlags_Dying;
    }

    void add_child(entity_t* child, bool maintain_world_pos = false) {
        assert(maintain_world_pos == false);
        child->parent = this;
        child->transform = math::transform_t{};
    }
};

static_assert(CEntity<entity_t>);

// inline void 
// entity_add_coroutine(entity_t* entity, entity_coroutine_t* coro) {
//     node_push(coro, entity->coroutine);
// }

inline void
entity_init(entity_t* entity, u64 mesh_id = -1) {
    // entity->flags |= EntityFlags_Spatial;
    // entity->flags |= mesh_id != -1 ? EntityFlags_Renderable : 0;
    entity->gfx.mesh_id = mesh_id;
}

inline void 
player_init(
    entity_t* player, 
    cam::camera_t* camera, 
    u64 mesh_id) {

    player->type = entity_type::player;

    player->gfx.mesh_id = mesh_id;
    player->camera_controller.camera = camera;
}

}; // namespace entity

#include "entity_db.hpp"
