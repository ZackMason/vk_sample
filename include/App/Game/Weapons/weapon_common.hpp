#pragma once

#include "App/Game/Entity/ztd_entity_prefab.hpp"
#include "base_weapon.hpp"
#include "App/Game/Items/base_item.hpp"

void do_hit_effects(
    ztd::world_t* world,
    ztd::entity_t* self_entity,
    ztd::entity_t* hit_entity,
    ztd::item::effect_t* effect,
    v3f hit_pos,
    v3f hit_normal
);

ztd::entity_t* spawn_blood(
    ztd::world_t* world,
    v3f pos,
    u32 count,
    ztd::prefab_t particle_prefab 
);

ztd::entity_t* explosion_at_point(
    ztd::world_t* world,
    ztd::entity_t* entity,
    v3f pos,
    f32 damage,
    f32 range,
    ztd::item::effect_t* effects
);


b32 pickup_item(ztd::entity_t* e, ztd::entity_t* o) {
    // if (e->type == ztd::entity_type::player) {
    if (e->primary_weapon.entity==0) {
        o->physics.queue_free();
        if (o->parent) {
            o->parent->remove_child(o);
        }
        e->primary_weapon.entity = o;
        o->flags &= ~ztd::EntityFlags_Pickupable;
        return 1;
    } else if (e->secondary_weapon.entity==0) {
        o->physics.queue_free();
        if (o->parent) {
            o->parent->remove_child(o);
        }
        e->secondary_weapon.entity = o;
        o->flags &= ~ztd::EntityFlags_Pickupable;
        o->transform.origin = axis::down * 1000.0f;
        return 1;
    } else if (e->inventory.has()) {
        if (e->inventory.add(o)) {
            o->physics.queue_free();
            if (o->parent) {
                o->parent->remove_child(o);
            }
            o->flags &= ~ztd::EntityFlags_Pickupable;
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
    auto* self = (ztd::entity_t*)trigger->user_data;   
    auto* other_e = (ztd::entity_t*)other->user_data;
    auto* world = (ztd::world_t*)trigger->api->user_world;
    if (other_e->type == ztd::entity_type::player) {
        pickup_item(other_e, self);
    }   
};

