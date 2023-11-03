#pragma once

#include "App/Game/Entity/zyy_entity_prefab.hpp"
#include "base_weapon.hpp"
#include "App/Game/Items/base_item.hpp"

void do_hit_effects(
    zyy::world_t* world,
    zyy::entity_t* self_entity,
    zyy::entity_t* hit_entity,
    zyy::item::effect_t* effect,
    v3f hit_pos,
    v3f hit_normal
);

zyy::entity_t* spawn_blood(
    zyy::world_t* world,
    v3f pos,
    u32 count,
    zyy::prefab_t particle_prefab 
);

zyy::entity_t* explosion_at_point(
    zyy::world_t* world,
    zyy::entity_t* entity,
    v3f pos,
    f32 damage,
    f32 range,
    zyy::item::effect_t* effects
);



void bullet_on_hit(physics::rigidbody_t* self, physics::rigidbody_t* other);
void rocket_on_hit(physics::rigidbody_t* self, physics::rigidbody_t* other);

export_fn(void) on_trigger_pickup_weapon(physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
    auto* self = (zyy::entity_t*)trigger->user_data;   
    auto* other_e = (zyy::entity_t*)other->user_data;
    auto* world = (zyy::world_t*)trigger->api->user_world;
    if (other_e->type == zyy::entity_type::player) {
        if (other_e->primary_weapon.entity==0) {
            self->physics.queue_free();
            if (self->parent) {
                self->parent->remove_child(self);
            }
            other_e->primary_weapon.entity = self;
            self->flags &= ~zyy::EntityFlags_Pickupable;
        } else if (other_e->secondary_weapon.entity==0) {
            self->physics.queue_free();
            if (self->parent) {
                self->parent->remove_child(self);
            }
            other_e->secondary_weapon.entity = self;
            self->flags &= ~zyy::EntityFlags_Pickupable;
        } 
    }   
};

