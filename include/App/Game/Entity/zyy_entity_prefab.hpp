#pragma once

#include "App/Game/Entity/entity_concept.hpp"
#include "App/Game/Weapons/base_weapon.hpp"

#include "App/Game/Rendering/particle.hpp"

namespace zyy {

struct prefab_t {
    entity_type type=entity_type::environment;
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
        physics::rigidbody_on_collision_function on_collision_end{0};
        physics::rigidbody_on_collision_function on_trigger{0};
        physics::rigidbody_on_collision_function on_trigger_end{0};


        struct shape_t {
            physics::collider_shape_type shape{physics::collider_shape_type::NONE};
            u32 flags{};
            union {
                struct box_t {
                    v3f size{};
                } box;
                math::sphere_t sphere;
                struct capsule_t {
                    f32 radius;
                    f32 height;
                } capsule;
            };
        };
        std::optional<shape_t> shapes[8]{}; 
    };
    std::optional<physics_t> physics{};

    std::optional<particle_system_settings_t> emitter{};
    std::optional<item::effect_t> effect{};

    brain_type brain_type = brain_type::invalid;

    struct child_t {
        const prefab_t* entity{0};
        v3f                 offset{0.0f};
    };
    child_t children[10]{};

    prefab_t& operator=(const prefab_t& o) {
        if (this != &o) {
            puts("Copy entity def");
            utl::copy(this, &o, sizeof(*this));
        }
        return *this;
    }
};

inline static prefab_t 
load_from_file(arena_t* arena, std::string_view path) {
    prefab_t entity;

    std::ifstream file{path.data(), std::ios::binary};

    if(!file.is_open()) {
        zyy_error("res", "Failed to open file");
        return entity;
    }

    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    // we might not just be reading from the file 
    // so we use arena to read the whole file
    // and go through serialization

    auto temp = begin_temporary_memory(arena);

    std::byte* data = push_bytes(temp.arena, size);
    file.read((char*)data, size);
    
    utl::memory_blob_t loader{data};

    entity = loader.deserialize<prefab_t>();

    end_temporary_memory(temp);

    return entity;
}

}
