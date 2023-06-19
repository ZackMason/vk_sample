#pragma once

#include "core.hpp"

#include "uid.hpp"

#include "App/Game/Util/camera.hpp"
#include "App/Game/Weapons/base_weapon.hpp"
#include "App/Game/Entity/entity_concept.hpp"
#include <variant>

namespace game {

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

// type traits
constexpr bool 
is_pickupable(entity_type type) noexcept {
    return 
        type == entity_type::weapon || 
        type == entity_type::weapon_part;
}

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

    entity_t* operator->();
};

struct world_t;

DEFINE_TYPED_ID(script_id);

template <typename T>
concept CScript = requires(T script, world_t* w, f32 dt) {
    { T::script_name() } -> std::same_as<std::string_view>;
    { script.begin_play(w) } -> std::same_as<void>;
    { script.end_play(w) } -> std::same_as<void>;
    { script.update(w, dt) } -> std::same_as<void>;


    requires std::is_same_v<T*, decltype(script.next)>;
    requires std::is_same_v<script_id, decltype(script.id)>;
};

template <typename T>
concept CSerializedScript = requires(T script, utl::memory_blob_t& blob) {
    requires CScript<T>;

    { script.save(blob) } -> std::same_as<void>;
    { script.load(blob) } -> std::same_as<void>;
};

template <typename T, typename... Ts>
concept same_as_any = ( ... or std::same_as<T, Ts>);

template <CScript... TScript>
struct script_manager_t {
    inline static constexpr
        std::array<u64, sizeof...(TScript)> 
        type_ids{TScript::type_id()...};
    
    script_manager_t(world_t* world_) 
        : 
        world{world_}
    {
    }

    template <typename ScriptType>
    struct script_meta_t {
        ScriptType* scripts{nullptr};
        script_id   next_id{0x1};
    };

    std::tuple<script_meta_t<TScript>...> table;
    world_t* world{nullptr};
    
    // runtime: O(1), comptime: O(N)
    template <same_as_any<TScript...> T>
    static constexpr u64 get_type_index() {
        for(u64 i{0}; const auto id: type_ids) {
            if (id == T::type_id()) {
                return i;
            }
            i++;
        }
        // this is not reachable
        return 0xFFFF'FFFF'FFFF'FFFF;
    }
    
    // O(1)
    template <same_as_any<TScript...> T>
    script_id add_script(T* script) {
        assert(script->id == uid::invalid_id);
        node_push(script, std::get<script_meta_t<T>>(table).scripts);
        return script->id = std::get<script_meta_t<T>>(table).next_id++;
    }

    // O(N)
    template <same_as_any<TScript...> T>
    T* get_script(u64 index, script_id id) {
        constexpr u64 type_index = get_type_index<T>();
        node_for(auto, std::get<type_index>(table).scripts, script) {
            if (script->id == id) {
                return script;
            }
        }

        return nullptr;
    }

    // O(N)
    template <same_as_any<TScript...> T>
    void remove_script(T* s) {
        assert(s);
        remove_script<T>(s->id);
    }
    
    // O(N)
    template <same_as_any<TScript...> T>
    void remove_script(script_id id) {
        assert(id != uid::invalid_id);
        auto*& scripts = std::get<script_meta_t<T>>(table).scripts;
        if (scripts) {
            if (scripts->id == id) {
                scripts->id = uid::invalid_id;
                scripts = scripts->next;
                return;
            } 
            node_for(auto, scripts, script) {
                if (script->next && script->next->id == id) {
                    script->id = uid::invalid_id;
                    script->next = script->next->next;
                    return;
                }
            }
        }
    }

    void update_script(CScript auto* script, f32 dt) {
        if (script) {
            script->update(world, dt);
            update_script(script->next, dt);
        }
    }

    void begin_script(CScript auto* script) {
        if (script) {
            script->begin_play(world);
            begin_script(script->next);
        }
    }
    void end_script(CScript auto* script) {
        if (script) {
            script->end_play(world);
            end_script(script->next);
        }
    }

    size_t script_count(CScript auto* script) {
        return script?1+script_count(script->next):0;        
    }

    void update(f32 dt) {
        std::apply([&](auto&&... arg){ ((update_script(arg.scripts, dt)), ...); }, table);
    }

    void begin_play() {
        std::apply([&](auto&&... arg){ ((begin_script(arg.scripts)), ...); }, table);
    }
    void end_play() {
        std::apply([&](auto&&... arg){ ((end_script(arg.scripts)), ...); }, table);
    }
};

#define USCRIPT(Name) \
    Name* next{0};\
    script_id id{uid::invalid_id};\
    static constexpr std::string_view script_name() { return #Name; }\
    static constexpr u64 type_id() { return sid(script_name()); }

struct player_script_t {
    USCRIPT(player_script_t)

    int score{0};

    void save(utl::memory_blob_t& blob) {
        blob.serialize(0, score);
    }

    void load(utl::memory_blob_t& blob) {
        score = blob.deserialize<int>();
    }

    void end_play(world_t* world) {}
    void begin_play(world_t* world) {
        gen_info("PlayerScript", "Hello World: {} - {}", script_name(), id);
    }

    void update(world_t* world, f32 dt) {
        score++;
    }
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

    entity_coroutine_t* coroutine{0};

    script_id script{uid::invalid_id};

    virtual ~entity_t() = default;
    constexpr entity_t() noexcept = default;

    constexpr bool is_alive() const noexcept {
        return id != uid::invalid_id;
    }
};

static_assert(CEntity<entity_t>);

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
