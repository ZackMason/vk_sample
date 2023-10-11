#pragma once

#include "zyy_core.hpp"


namespace zyy {
    struct entity_t;
    struct world_t;
    struct prefab_t;
}

namespace zyy::item {
    struct effect_t;
}

namespace zyy::wep {
    enum struct bullet_type {
        PELLET, LASER, ROCKET, GRENADE, SIZE
    };

    struct bullet_t {
        math::ray_t ray{};
        f32 damage{};
        f32 spread{};

        item::effect_t* effects{};

        bullet_type type{bullet_type::SIZE};
    };

    using spawn_bullet_function = zyy::entity_t* (*)(
        zyy::world_t*,
        const zyy::prefab_t&,
        bullet_t        
    );

    zyy::entity_t* spawn_bullet(
        zyy::world_t* world,
        const zyy::prefab_t& prefab,
        bullet_t bullet);

    zyy::entity_t* spawn_rocket(
        zyy::world_t* world,
        const zyy::prefab_t& prefab,
        bullet_t bullet);
}
