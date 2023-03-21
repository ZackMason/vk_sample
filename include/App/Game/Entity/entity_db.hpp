#ifndef ENTITY_DB_HPP
#define ENTITY_DB_HPP

#include "App/Game/Entity/entity.hpp"

namespace game::entity::db {

struct entity_def_t {
    entity::entity_type type{entity::entity_type::environment};
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
        u32 flags{PhysicsEntityFlags_None};
        physics::collider_shape_type shape{physics::collider_shape_type::NONE};
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
    .type = entity_type::environment,
    .type_name = "Teapot",
    .gfx = {
        .mesh_name = "assets/models/utah-teapot.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = entity_def_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic,
        .shape = physics::collider_shape_type::CONVEX,
        // .shape = physics::collider_shape_type::SPHERE,
        // .shape_def = {
        //     .sphere = {
        //         .radius = 1.0f,
        //     },
        // },
    },
};

DB_ENTRY
door {
    .type = entity_type::environment,
    .type_name = "door",
    .gfx = {
        .mesh_name = "door",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = entity_def_t::physics_t {
        .flags = PhysicsEntityFlags_Trigger,
        .shape = physics::collider_shape_type::BOX,
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
    .type = entity_type::environment,
    .type_name = "room_0",
    .gfx = {
        .mesh_name = "assets/models/rooms/room_0.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    // .physics = entity_def_t::physics_t {
    //     .flags = PhysicsEntityFlags_Static,
    //     .shape = physics::collider_shape_type::TRIMESH,
    // },
};

DB_ENTRY
map_01 {
    .type = entity_type::environment,
    .type_name = "map_01",
    .gfx = {
        .mesh_name = "assets/models/map_01.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = entity_def_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shape = physics::collider_shape_type::TRIMESH,
    },
};

}; //namespace rooms

namespace items {

}; //namespace items

namespace weapons {

DB_ENTRY
shotgun {
    .type = entity_type::weapon,
    .type_name = "shotgun",
    .gfx = {
        .mesh_name = "assets/models/rifle.obj",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_shotgun(),
};

DB_ENTRY
rifle {
    .type = entity_type::weapon,
    .type_name = "rifle",
    .gfx = {
        .mesh_name = "rifle",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_rifle(),
};

DB_ENTRY
pistol {
    .type = entity_type::weapon,
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
    .type = entity_type::player,
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
        .shape = physics::collider_shape_type::CAPSULE,
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
    .type = entity_type::player,
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
        .shape = physics::collider_shape_type::CAPSULE,
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

using namespace std::string_view_literals;

template <size_t N>
using entity_lut_t = std::array<std::pair<std::string_view, const entity_def_t*>, N>;

constexpr static entity_lut_t<2> gs_database{{
    {"pistol", &weapons::pistol},
    {"shotgun", &weapons::shotgun},
}};

template <size_t N>
constexpr const entity_def_t*
query(
    std::string_view name,
    const entity_lut_t<N>& database
) noexcept {
    for (const auto& [key, val]: database) {
        if (key == name) {
            return val;
        }
    }
    return nullptr;
}

}; // namespace db

#endif