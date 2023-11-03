#pragma once

#include "base_item.hpp"
#include "App/Game/Weapons/weapon_common.hpp"

export_fn(void) explosion_on_hit_effect(
    zyy::world_t* w, 
    zyy::item::effect_t* effect,
    zyy::entity_t* bullet_entity,
    zyy::entity_t* hit_entity,
    v3f p, v3f n
) {
    puts("Boom");
    if (bullet_entity) {
        explosion_at_point(w, bullet_entity, p, bullet_entity->stats.weapon.stats.damage, 10.0f, 0);
    } else {
        puts("No Bullet entity");
        zyy_warn(__FUNCTION__, "No Bullet entity");
        explosion_at_point(w, 0, p, 10.0f, 10.0f, effect);
    }
}
