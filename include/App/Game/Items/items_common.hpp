#pragma once 

#include "App/Game/Entity/entity.hpp"

namespace zyy::item {

export_fn(void) on_trigger_pickup_item(physics::rigidbody_t* trigger, physics::rigidbody_t* other, physics::collider_t* trigger_shape, physics::collider_t* other_shape) {
    auto* self = (zyy::entity_t*)trigger->user_data;
    auto* other_e = (zyy::entity_t*)other->user_data;
    auto* world = (zyy::world_t*)trigger->api->user_world;
    if (other_e->type == zyy::entity_type::player) {
        if (other_e->primary_weapon.entity) {
            node_push(self->stats.effect, other_e->primary_weapon.entity->stats.effect);
            self->queue_free();
            zyy_info(__FUNCTION__, "Picked up item");
        }
    }
};
}