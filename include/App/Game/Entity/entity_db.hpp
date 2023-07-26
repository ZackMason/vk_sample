#ifndef ENTITY_DB_HPP
#define ENTITY_DB_HPP

#include "App/Game/Entity/entity.hpp"

namespace game::db {

struct prefab_t {
    entity_type type{entity_type::environment};
    char type_name[128]{0};

    struct gfx_t {
        // string_t mesh_name{};
        char mesh_name[128]{0};
        gfx::material_t material{};
        char albedo_texture[128]{0};
        char normal_texture[128]{0};
        
        char animations[128]{0};
    } gfx;

    void (*coroutine)(coroutine_t*, frame_arena_t&){0};
    std::optional<character_stats_t> stats{};
    std::optional<wep::base_weapon_t> weapon{};

    struct physics_t {
        u32 flags{PhysicsEntityFlags_None};
        physics::rigidbody_on_collision_function on_collision{0};
        physics::rigidbody_on_collision_function on_trigger{0};

        struct shape_t {
            physics::collider_shape_type shape{physics::collider_shape_type::NONE};
            u32 flags{};
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
            };
        };
        std::optional<shape_t> shapes[8]{}; 
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
        const prefab_t* entity{0};
        v3f                 offset{0.0f};
    };
    child_t children[10]{};

    prefab_t& operator=(const prefab_t& o) {
        if (this != &o) {
            puts("Copy entity def");
            std::memcpy(this, &o, sizeof(*this));
        }
        return *this;
    }
};

inline static prefab_t 
load_from_file(arena_t temp_arena, std::string_view path) {
    prefab_t entity;

    std::ifstream file{path.data(), std::ios::binary};

    if(!file.is_open()) {
        gen_error("res", "Failed to open file");
        return entity;
    }

    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::byte* data = arena_alloc(&temp_arena, size);
    file.read((char*)data, size);
    utl::memory_blob_t loader{data};

    entity = loader.deserialize<prefab_t>();

    return entity;
}

#define DB_ENTRY static constexpr prefab_t

namespace misc {

DB_ENTRY
sphere {
    .type = entity_type::environment,
    .type_name = "Sphere",
    .gfx = {
        .mesh_name = "res/models/sphere.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
teapot_particle {
    .type = entity_type::environment,
    .type_name = "Teapot Particle",
    .gfx = {
        .mesh_name = "res/models/particles/teapot_particle.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
teapot {
    .type = entity_type::environment,
    .type_name = "Teapot",
    .gfx = {
        .mesh_name = "res/models/utah-teapot.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic,
    #if 0 // use convex
        .shape = physics::collider_shape_type::CONVEX,
    #else 
        .shapes = {
            prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
    #endif
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
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static | PhysicsEntityFlags_Trigger,
        .shapes = {
            prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::BOX,
                .flags = 1,
                .box = {
                    .size = v3f{1.0f},
                },
            },
        },
    },
};

DB_ENTRY
platform_1000 {
    .type = entity_type::environment,
    .type_name = "platform_1000",
    .gfx = {
        .mesh_name = "res/models/platform_1000x1000.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::BOX,
                .box = {
                    .size = v3f{1000.0f, 1.0f, 1000.0f},
                },
            },
        },
    },
};

void co_platform(coroutine_t* co, frame_arena_t& frame_arena) {
    // return;
    auto* e = (game::entity_t*)co->data;
    auto& y_pos = e->physics.rigidbody->position.y;


    const auto lerp = tween::generic<f32>(tween::in_out_elastic);
    auto* stack = co_stack(co, frame_arena);
    f32* start  = co_push_stack(co, stack, f32);
    f32* end    = co_push_stack(co, stack, f32);

    co_begin(co);

        *start = y_pos;
        *end = math::fcmp(*start, 20.0f) ? 0.0f : 20.0f;

        DEBUG_ADD_VARIABLE(e->physics.rigidbody->position.y);
        DEBUG_ADD_VARIABLE(v3f(e->physics.rigidbody->position.x, *end, e->physics.rigidbody->position.z));

        // Platform.audio.play_sound(assets::sounds::unlock.id);

        co_lerp(co, y_pos, *start, *end, 10.0f, lerp);

    co_end(co);
}

DB_ENTRY
platform_3x3 {
    .type = entity_type::environment,
    .type_name = "platform",
    .gfx = {
        .mesh_name = "res/models/platform_3x3.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .coroutine = co_platform,
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Kinematic | PhysicsEntityFlags_Trigger,
        .shapes = {
            prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::BOX,
                .box = {
                    .size = v3f{3.0f, .2f, 3.0f},
                },
            },
            prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::BOX,
                .flags = 1,
                .box = {
                    // .size = v3f{5.5f, 32.0f, 5.5f},
                    .size = v3f{2.5f, 32.0f, 2.5f},
                },
            },
        },
    },
};

void co_kill_in_ten(coroutine_t* co, frame_arena_t& frame_arena) {
    auto* e = (game::entity_t*)co->data;

    co_begin(co);

        co_wait(co, 10.0f);
        e->queue_free();

    co_end(co);
}

DB_ENTRY
bullet_hole {
    .type = entity_type::environment,
    .type_name = "bullet_hole",
    .gfx = {
        .mesh_name = "res/models/bullet_hole.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
    .coroutine = co_kill_in_ten,
};


}; //namespace misc

namespace environmental {

DB_ENTRY
rock_01 {
    .type = entity_type::environment,
    .type_name = "rock",
    .gfx = {
        .mesh_name = "res/models/rocks/rock_01.obj",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
grass {
    .type = entity_type::environment,
    .type_name = "grass",
    .gfx = {
        .mesh_name = "res/models/grass.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
tree_01 {
    .type = entity_type::environment,
    .type_name = "tree",
    .gfx = {
        .mesh_name = "res/models/environment/tree_01.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
tree_group {
    .children = {
        {
            .entity = &tree_01,
            .offset = v3f{0.0f}
        },
        {
            .entity = &tree_01,
            .offset = v3f{2.0f,0.0f, 0.5f}
        },
        {
            .entity = &tree_01,
            .offset = v3f{1.0f,0.0f, -0.25f}
        },
    },
};

};

namespace rooms {

DB_ENTRY
tower_01 {
    .type = entity_type::environment,
    .type_name = "tower_01",
    .gfx = {
        .mesh_name = "res/models/rooms/tower_01.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
sponza {
    .type = entity_type::environment,
    .type_name = "Sponza",
    .gfx = {
        .mesh_name = "res/models/Sponza.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
            // prefab_t::physics_t::shape_t{
            //     .shape = physics::collider_shape_type::SPHERE,
            //     .flags = 1,
            //     .sphere = {
            //         .radius = 5.0f,
            //     },
            // },
        },
    },
};

DB_ENTRY
room_01 {
    .type = entity_type::environment,
    .type_name = "room_01",
    .gfx = {
        // .mesh_name = "res/models/rooms/room_01.obj",
        .mesh_name = "res/models/rooms/rock_room_test.fbx",
        // .mesh_name = "res/models/Sponza.gltf",
        // .mesh_name = "res/models/rooms/rock_room_test.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
room_0 {
    .type = entity_type::environment,
    .type_name = "room_0",
    .gfx = {
        .mesh_name = "res/models/rooms/room_0.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
map_01 {
    .type = entity_type::environment,
    .type_name = "map_01",
    .gfx = {
        .mesh_name = "res/models/map_01.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
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
        .mesh_name = "res/models/rifle.obj",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_shotgun(),
    // .physics = prefab_t::physics_t {
    //     .flags = PhysicsEntityFlags_Static,
    //     .shapes = {
    //         prefab_t::physics_t::shape_t{
    //             .shape = physics::collider_shape_type::SPHERE,
    //             .flags = 1,
    //             .sphere = {
    //                 .radius = 1.0f,
    //             },
    //         },
    //     },
    // },
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
        .mesh_name = "res/models/capsule.obj",
    },
    .stats = character_stats_t {
        .health = {
            120
        },
        .movement = {
            .move_speed = 1.0f,
        },
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Character | PhysicsEntityFlags_Dynamic,
        .shapes = {
            prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
        // .shape = physics::collider_shape_type::CAPSULE,
        // .shape_def = {
        //     .capsule = {
        //         .radius = 1.0f,
        //         .height = 3.0f,
        //     },
        // },
    },
    .children = {
        // {
        //     .entity = &weapons::rifle,
        //     .offset = v3f{0.0f},
        // },
        {
            .entity = &weapons::shotgun,
            .offset = v3f{1.0f, 1.0f, 0.50f},
        },
    },
};

DB_ENTRY
assassin {
    .type = entity_type::player,
    .type_name = "assassin",
    .gfx = {
        .mesh_name = "res/models/capsule.obj",
    },
    .stats = character_stats_t {
        .health = {
            80
        },
        .movement = {
            .move_speed = 1.3f,
        },
    },
    .physics = prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Character,
        .shapes = { 
            prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::CAPSULE,
                .capsule = {
                    .radius = 1.0f,
                    .height = 3.0f,
                },
            },
        },
    },
    .children = {
        // {
        //     .entity = &weapons::shotgun,
        //     .offset = v3f{0.0f},
        // },
        // {
        //     .entity = &weapons::pistol,
        //     .offset = v3f{0.0f},
        // },
    },
};

}; // namespace characters

using namespace std::string_view_literals;

template <size_t N>
using entity_lut_t = std::array<std::pair<std::string_view, const prefab_t*>, N>;

constexpr static entity_lut_t<2> gs_database{{
    {"pistol", &weapons::pistol},
    {"shotgun", &weapons::shotgun},
}};

template <size_t N>
constexpr const prefab_t*
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