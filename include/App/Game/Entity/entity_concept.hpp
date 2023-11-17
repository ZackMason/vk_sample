#ifndef GAME_ENTITY_CONCEPT_HPP
#define GAME_ENTITY_CONCEPT_HPP

#include "uid.hpp"
#include "zyy_character_stats.hpp"
#include "zyy_inventory.hpp"
#include "zyy_interactable.hpp"

namespace zyy {

template <typename EntityType>
concept CEntity = requires(EntityType entity) {
    requires std::derived_from<math::transform_t, decltype(entity.transform)>;
    requires std::is_same_v<uid::id_type, decltype(entity.id)>;

    requires std::is_same_v<math::rect3d_t, decltype(entity.aabb)>;
};
    
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

    PhysicsEntityFlags_Dying = BIT(11),
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

    door,
    container,

    SIZE
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


#if ZYY_INTERNAL
struct DEBUG_entity_meta_info_t {
    const char* prefab_name;
    const char* file_name;
    const char* function;
    u64         line_number;
    f32         game_time;
};

#endif

}

#endif