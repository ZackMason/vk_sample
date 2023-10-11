#ifndef GAME_ENTITY_CONCEPT_HPP
#define GAME_ENTITY_CONCEPT_HPP

#include "uid.hpp"

namespace zyy {

template <typename EntityType>
concept CEntity = requires(EntityType entity) {
    requires std::derived_from<math::transform_t, decltype(entity.transform)>;
    requires std::is_same_v<uid::id_type, decltype(entity.id)>;
    // requires std::is_same_v<uid::id_type, decltype(entity.parent_id)>;

    requires std::is_same_v<math::rect3d_t, decltype(entity.aabb)>;
    // requires std::is_trivially_constructible_v<EntityType>;
};

#define GENERATE_BASE_ENTITY_BODY(Name) \
    Name* next{nullptr};\
    void* next_in_hash{nullptr};\
    entity_id id{uid::invalid_id};\
    math::transform_t transform;\
    math::rect3d_t aabb;

    
// most of these should be inferred from entity data
enum EntityFlags : u64 {
    EntityFlags_Breakpoint = BIT(0),
    // EntityFlags_Spatial = BIT(1), // is this really needed??
    EntityFlags_DirtyTransform = BIT(2), // is this really needed??
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
    PhysicsEntityFlags_Kinematic = BIT(4),
};

enum struct entity_type {
    environment,

    player,
    bad,

    weapon,
    weapon_part,
    item,

    trigger,

    effect,

    SIZE
};

struct health_t {
    f32 max{0.0f};
    f32 current{0.0f};

    bool damage(f32 x) {
        if (max > 0.0f) {
            current -= x;
            return current <= 0.0f;
        } else {
            return false;
        }
    }

    constexpr health_t(f32 v = 0.0f) noexcept
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
    // entity_coroutine_t* next{0};
    coroutine_t coroutine;

    bool _is_running = false;

    void start() {
        if (!_is_running) {
            coroutine.line=0;
            _is_running = true;
        } else {
            // zyy_warn(__FUNCTION__, "{} tried to start coroutine that is already running", coroutine.data);
        }
    }
    inline void run(frame_arena_t& frame_arena) {
        if (func && _is_running) {
            func(&coroutine, frame_arena);
            _is_running = coroutine.running;
        }
    }
    void (*func)(coroutine_t*, frame_arena_t&);
};

DEFINE_TYPED_ID(entity_id);

struct entity_t;

struct entity_ref_t {
    entity_t* entity;
    entity_id id;

    entity_t* operator->();
};


}


#endif