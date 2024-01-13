#pragma once

#include "ztd_core.hpp"


namespace ztd {
    struct entity_t;
    struct world_t;
    struct prefab_t;
}

namespace ztd::item {
    struct effect_t;
}

namespace ztd::wep {
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

    using spawn_bullet_function = ztd::entity_t* (*)(
        ztd::world_t*,
        const ztd::prefab_t&,
        bullet_t        
    );

    export_fn(ztd::entity_t*) spawn_bullet(
        ztd::world_t* world,
        const ztd::prefab_t& prefab,
        bullet_t bullet);

    export_fn(ztd::entity_t*) spawn_rocket(
        ztd::world_t* world,
        const ztd::prefab_t& prefab,
        bullet_t bullet);
}
