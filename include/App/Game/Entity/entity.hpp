#pragma once

#include "core.hpp"

#include "App/Game/Util/camera.hpp"
#include "App/Game/Weapons/base_weapon.hpp"

#include "App/Game/Physics/physics_world.hpp"

#include <variant>

namespace game::entity {

// using entity_id = u64;

// static constexpr entity_id invalid_id = ~0ui32;

enum EntityFlags {
    EntityFlags_Spatial = BIT(0),
    EntityFlags_Renderable = BIT(1),
};

enum PhysicsEntityFlags {
    PhysicsEntityFlags_None = 0,
    PhysicsEntityFlags_Static = BIT(0),
    PhysicsEntityFlags_Dynamic = BIT(1),
    PhysicsEntityFlags_Trigger = BIT(2),
    PhysicsEntityFlags_Character = BIT(3),
};

namespace entity_type { enum {
    environment,

    player,
    enemy,

    weapon,
    item,

    trigger,

    SIZE
};};

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

struct entity_t : node_t<entity_t> {
    // entity_id   id{0};
    u64         type{};
    u64         flags{};
    u64         tag{0};
    string_t    name;
    entity_t*   parent{nullptr};

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
        phys::rigid_body_t rigid_body;
    } physics;


    struct stats_t {
        std::variant<character_stats_t, wep::base_weapon_t> chararcter, weapon;
    } stats;

    entity_t* primary_weapon{0};
    entity_t* secondary_weapon{0};

    cam::first_person_controller_t camera_controller;

    entity_coroutine_t* coroutine{0};

    virtual ~entity_t() = default;
    entity_t() noexcept = default;
    entity_t(const entity_t& o) noexcept = delete;
    entity_t(entity_t&& o) noexcept = delete;
    entity_t& operator=(entity_t&& o) noexcept = delete;
    entity_t& operator=(const entity_t& o) noexcept = delete;
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

inline void
physics_entity_init_character(
    entity_t* entity,
    u64 mesh_id = 0,
    phys::physics_world_t* physx_world = 0
) {
    entity_init(entity, mesh_id);

    entity->physics.rigid_body.state = physx_world->state;
    entity->physics.rigid_body.create_character(physx_world);
}

inline void
physics_entity_init_convex(
    entity_t* entity, 
    u64 mesh_id = 0, 
    std::byte* physx_data = 0, 
    u32 physx_size = 0,
    phys::physx_state_t* physx_world = 0
) {
    entity_init(entity, mesh_id);

    if (physx_world) {
        entity->physics.rigid_body.state = physx_world;
        entity->physics.rigid_body.load_convex(physx_data, physx_size, entity->global_transform());
    }
}

inline void
physics_entity_init_trimesh(
    entity_t* entity, 
    u64 mesh_id = 0, 
    std::byte* physx_data = 0, 
    u32 physx_size = 0,
    phys::physx_state_t* physx_world = 0
) {
    entity_init(entity, mesh_id);

    if (physx_world) {
        entity->physics.rigid_body.state = physx_world;
        entity->physics.rigid_body.load_trimesh(physx_data, physx_size, entity->global_transform());
    }
}


inline void 
player_init(entity_t* player, cam::camera_t* camera, u64 mesh_id, phys::physics_world_t* phys) {
    physics_entity_init_character(player, mesh_id, phys);

    player->type = entity_type::player;

    player->gfx.mesh_id = mesh_id;
    player->camera_controller.camera = camera;
}

namespace db {

struct entity_def_t {
    string_t type_name{};

    struct gfx_t {
        string_t mesh_name{};
        gfx::material_t material{};
        string_t albedo_tex{};
        string_t normal_tex{};

        string_t animations{};
    } gfx;

    std::optional<character_stats_t> stats{};
    std::optional<wep::base_weapon_t> weapon{};

    struct physics_t {
        u64 flags{PhysicsEntityFlags_None};
        phys::physics_shape_type shape{phys::physics_shape_type::NONE};
        union {
            struct box_t {
                v3f size{};
            } box;
            struct sphere_t {
                f32 radius;
            } sphere;
            struct capsule_t {
                f32 radius;
                f32 height;
            } capsule;
        } shape_def;
    };
    std::optional<physics_t> physics{};

    struct particle_emitter_t {
        u32 count{};
        f32 rate{};
        v3f vel{};
        v3f acl{};
    };
    std::optional<particle_emitter_t> emitter{};

    struct child_t {
        const entity_def_t* entity{0};
        v3f                 offset{0.0f};
    };
    child_t children[10]{};
};

#define DB_ENTRY static constexpr entity_def_t

namespace misc {

DB_ENTRY
teapot {
    .type_name = "Teapot",
    .gfx = {
        .mesh_name = "assets/models/utah-teapot.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
door {
    .type_name = "door",
    .gfx = {
        .mesh_name = "door",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = entity_def_t::physics_t {
        .flags = PhysicsEntityFlags_Trigger,
        .shape = phys::physics_shape_type::BOX,
        .shape_def = {
            .box = {
                .size = v3f{1.0f},
            },
        },
    },
};

}; //namespace misc

namespace rooms {

DB_ENTRY
room_0 {
    .type_name = "room_0",
    .gfx = {
        .mesh_name = "assets/models/rooms/room_0.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    // .physics = entity_def_t::physics_t {
    //     .flags = PhysicsEntityFlags_Static,
    //     .shape = phys::physics_shape_type::TRIMESH,
    // },
};

DB_ENTRY
map_01 {
    .type_name = "map_01",
    .gfx = {
        .mesh_name = "assets/models/map_01.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = entity_def_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shape = phys::physics_shape_type::TRIMESH,
    },
};

}; //namespace rooms

namespace items {

}; //namespace items

namespace weapons {

DB_ENTRY
shotgun {
    .type_name = "shotgun",
    .gfx = {
        .mesh_name = "shotgun",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_shotgun(),
};

DB_ENTRY
rifle {
    .type_name = "rifle",
    .gfx = {
        .mesh_name = "rifle",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_rifle(),
};

DB_ENTRY
pistol {
    .type_name = "pistol",
    .gfx = {
        .mesh_name = "pistol",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_pistol(),
};

}; // namespace weapons

// Character Class Definitions

namespace characters {
DB_ENTRY
soldier {
    .type_name = "soldier",
    .gfx = {
        .mesh_name = "assets/models/capsule.obj",
    },
    .stats = character_stats_t {
        .health = {
            120
        },
        .movement = {
            .move_speed = 1.0f,
        },
    },
    .physics = entity_def_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic | PhysicsEntityFlags_Character,
        .shape = phys::physics_shape_type::CAPSULE,
        .shape_def = {
            .capsule = {
                .radius = 1.0f,
                .height = 3.0f,
            },
        },
    },
    .children = {
        {
            .entity = &weapons::rifle,
            .offset = v3f{0.0f},
        },
        {
            .entity = &weapons::shotgun,
            .offset = v3f{0.0f},
        },
    },
};

DB_ENTRY
assassin {
    .type_name = "assassin",
    .gfx = {
        .mesh_name = "assets/models/capsule.obj",
    },
    .stats = character_stats_t {
        .health = {
            80
        },
        .movement = {
            .move_speed = 1.3f,
        },
    },
    .physics = entity_def_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic | PhysicsEntityFlags_Character,
        .shape = phys::physics_shape_type::CAPSULE,
        .shape_def = {
            .capsule = {
                .radius = 1.0f,
                .height = 3.0f,
            },
        },
    },
    .children = {
        {
            .entity = &weapons::rifle,
            .offset = v3f{0.0f},
        },
        {
            .entity = &weapons::pistol,
            .offset = v3f{0.0f},
        },
    },
};

}; // namespace characters

}; // namespace db

};