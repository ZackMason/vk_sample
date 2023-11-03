#pragma once

#include "App/Game/Entity/entity_concept.hpp"
#include "App/Game/Weapons/base_weapon.hpp"

#include "App/Game/Rendering/particle.hpp"

namespace zyy {

using version_id = u64;

template <typename Function, size_t N = 128>
struct mod_function {
    char name[N] = {};
    Function function{0};

    constexpr mod_function() = default;

    constexpr mod_function(std::string_view func) 
        : function{0}
    {
        utl::ccopy(name, func.data(), std::min(array_count(name)-1, func.size()));
    }

    constexpr mod_function(Function f) 
        : function{f}
    {
    }

    constexpr mod_function<Function>& operator=(const Function& f) {
        function = f;
        return *this;
    } 

    constexpr mod_function<Function>& operator=(std::string_view s) {
        *this = mod_function<Function>{s};
        return *this;
    } 

    constexpr operator bool() const {
        return function || name[0];
    }

    constexpr operator Function() {
        return function;
    }

    template <typename ... Args>
    constexpr auto operator()(Args&& ... args) {
        return function(std::forward<Args>(args)...);
    }

    constexpr Function get(utl::mod_loader_t* loader = 0) {
        if (function) {
            return function;
        }
        assert(loader);
        if (loader) {
            function = loader->load_function<Function>(name);
        }
        return function;
    }

    constexpr Function get(utl::mod_loader_t* loader = 0) const {
        if (function) {
            return function;
        }
        assert(loader);
        if (loader) {
            return loader->load_function<Function>(name);
        }
        return 0;
    }
};

struct prefab_t {
    version_id VERSION{3};
    entity_type type=entity_type::environment;
    stack_string<128> type_name{};

    struct gfx_t {
        // string_t mesh_name{};
        // char mesh_name[128]{0};
        stack_string<128> mesh_name{};
        gfx::material_t material{};
        u32 material_id;
        stack_string<128> albedo_texture{};
        stack_string<128> normal_texture{};
        
        stack_string<128> animations{};
    } gfx;

    using coroutine_function = void(*)(coroutine_t*, frame_arena_t&);
    mod_function<coroutine_function> coroutine{0};
    std::optional<character_stats_t> stats{};
    std::optional<wep::base_weapon_t> weapon{};

    struct physics_t {
        u64 VERSION{0};
        u32 flags{PhysicsEntityFlags_None};
        mod_function<physics::rigidbody_on_collision_function> on_collision{0};
        mod_function<physics::rigidbody_on_collision_function> on_collision_end{0};
        mod_function<physics::rigidbody_on_collision_function> on_trigger{0};
        mod_function<physics::rigidbody_on_collision_function> on_trigger_end{0};

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

    mod_function<zyy::wep::spawn_bullet_function> spawn_bullet{0};
    mod_function<zyy::item::on_hit_effect_t> on_hit_effect{0};
    mod_function<zyy::item::on_hit_effect_t, 64> on_bullet_effect{0}; // @version 3 added
   

    std::optional<particle_system_settings_t> emitter{};
    // std::optional<item::effect_t> effect{}; // @version 3 removed

    brain_type brain_type = brain_type::invalid;

    u32 inventory_size=0; // @version 1 - added

    struct child_t {
        const prefab_t* entity{0};
        v3f             offset{0.0f};
    };
    std::array<child_t, 10> children{};

    prefab_t& operator=(const prefab_t& o) {
        if (this != &o) {
            puts("Copy entity def");
            utl::copy(this, &o, sizeof(*this));
        }
        return *this;
    }
};

}

template<>
zyy::prefab_t utl::memory_blob_t::deserialize<zyy::prefab_t>() {
    zyy::prefab_t prefab{};

    #define DESER(x) x = deserialize<decltype(x)>();

    DESER(prefab.VERSION);
    DESER(prefab.type);
    DESER(prefab.type_name);
    DESER(prefab.gfx);
    DESER(prefab.coroutine);
    DESER(prefab.stats);
    DESER(prefab.weapon);
    DESER(prefab.physics);
    DESER(prefab.spawn_bullet);
    prefab.spawn_bullet.function = 0;
    DESER(prefab.on_hit_effect);
    prefab.on_hit_effect.function = 0;
    DESER(prefab.emitter);
    if (prefab.VERSION <= 2) {
        struct dummy_effect_t {
            u64 type{};
            f32 rate{};
            f32 strength{};
            void(*on_hit_effect)(void); 

            void* next{};
        };
        std::optional<dummy_effect_t> dummy;
        DESER(dummy);
    }
    DESER(prefab.brain_type);
    if (prefab.VERSION >= 1) {
        DESER(prefab.inventory_size);
    }
    if (prefab.VERSION < 2) { // removed children
        DESER(prefab.children);
    }

    #undef DESER

    return prefab;
}

template<>
void utl::memory_blob_t::serialize<zyy::prefab_t>(arena_t* arena, const zyy::prefab_t& prefab) {
    serialize(arena, zyy::prefab_t{}.VERSION);
    serialize(arena, prefab.type);
    serialize(arena, prefab.type_name);
    serialize(arena, prefab.gfx);
    serialize(arena, prefab.coroutine);
    serialize(arena, prefab.stats);
    serialize(arena, prefab.weapon);
    serialize(arena, prefab.physics);
    serialize(arena, prefab.spawn_bullet);
    serialize(arena, prefab.on_hit_effect);
    serialize(arena, prefab.emitter);
    // serialize(arena, prefab.effect); // @Version 3 - removed
    serialize(arena, prefab.brain_type);
    serialize(arena, prefab.inventory_size);
    // serialize(arena, prefab.children); @Version 2 - removed
}

inline static zyy::prefab_t 
load_from_file(arena_t* arena, std::string_view path) {
    zyy::prefab_t entity;

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

    entity = loader.deserialize<zyy::prefab_t>();

    end_temporary_memory(temp);

    return entity;
}


