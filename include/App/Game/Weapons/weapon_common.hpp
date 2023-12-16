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


b32 pickup_item(zyy::entity_t* e, zyy::entity_t* o) {
    // if (e->type == zyy::entity_type::player) {
    if (e->primary_weapon.entity==0) {
        o->physics.queue_free();
        if (o->parent) {
            o->parent->remove_child(o);
        }
        e->primary_weapon.entity = o;
        o->flags &= ~zyy::EntityFlags_Pickupable;
        return 1;
    } else if (e->secondary_weapon.entity==0) {
        o->physics.queue_free();
        if (o->parent) {
            o->parent->remove_child(o);
        }
        e->secondary_weapon.entity = o;
        o->flags &= ~zyy::EntityFlags_Pickupable;
        o->transform.origin = axis::down * 1000.0f;
        return 1;
    } else if (e->inventory.has()) {
        if (e->inventory.add(o)) {
            o->physics.queue_free();
            if (o->parent) {
                o->parent->remove_child(o);
            }
            o->flags &= ~zyy::EntityFlags_Pickupable;
            o->transform.origin = axis::down * 1000.0f;
            return 1;
        }
    }
    return 0;
    // }   
}


void bullet_on_hit(physics::rigidbody_t* self, physics::rigidbody_t* other, physics::collider_t* self_shape, physics::collider_t* other_shape);
void rocket_on_hit(physics::rigidbody_t* self, physics::rigidbody_t* other, physics::collider_t* self_shape, physics::collider_t* other_shape);

export_fn(void) on_trigger_pickup_weapon(physics::rigidbody_t* trigger, physics::rigidbody_t* other, physics::collider_t* trigger_shape, physics::collider_t* other_shape) {
    auto* self = (zyy::entity_t*)trigger->user_data;   
    auto* other_e = (zyy::entity_t*)other->user_data;
    auto* world = (zyy::world_t*)trigger->api->user_world;
    if (other_e->type == zyy::entity_type::player) {
        pickup_item(other_e, self);
    }   
};

