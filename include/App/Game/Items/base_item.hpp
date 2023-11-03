#pragma once


#include "zyy_core.hpp"

#include "App/Game/Weapons/base_weapon.hpp"

namespace zyy {
    struct world_t;
    struct entity_t;
}

namespace zyy::item {
    namespace effect_type {
        enum {
            FIRE        = BIT(0),
            ICE         = BIT(1),
            SHOCK       = BIT(2),
            POISON      = BIT(3),
            BLEED       = BIT(4),
            EXPLOSION   = BIT(5),
        };
    };

    struct effect_t;
    using on_hit_effect_t = void(*)(zyy::world_t*, effect_t*, zyy::entity_t*, zyy::entity_t*, v3f p, v3f n);
    //                                       (world, effect, owner, bullet)
    using on_bullet_effect_function = void(*)(zyy::world_t*, effect_t*, zyy::entity_t*, zyy::entity_t*, zyy::entity_t*);

    struct effect_t {
        u64 type{};
        f32 rate{};
        f32 strength{};
        on_hit_effect_t on_hit_effect{0}; 
        on_bullet_effect_function on_bullet_effect{0};

        effect_t* next{0};
    };

};