#pragma once

#include "base_weapon.hpp"
#include "App/Game/Items/base_item.hpp"

void do_hit_effects(
    zyy::world_t* world,
    zyy::entity_t* hit_entity,
    zyy::item::effect_t* effect,
    v3f hit_pos,
    v3f hit_normal
);

zyy::entity_t* spawn_blood(
    zyy::world_t* world,
    v3f pos,
    u32 count,
    zyy::db::prefab_t particle_prefab 
);
void bullet_on_hit(physics::rigidbody_t* self, physics::rigidbody_t* other);



