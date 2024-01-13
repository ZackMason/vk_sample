#pragma once 

#include "App/Game/Entity/entity.hpp"

namespace ztd::item {

export_fn(void) on_trigger_pickup_item(physics::rigidbody_t* trigger, physics::rigidbody_t* other, physics::collider_t* trigger_shape, physics::collider_t* other_shape) {
    auto* self = (ztd::entity_t*)trigger->user_data;
    auto* other_e = (ztd::entity_t*)other->user_data;
    auto* world = (ztd::world_t*)trigger->api->user_world;
    if (other_e->type == ztd::entity_type::player) {
        if (other_e->primary_weapon.entity) {
            node_push(self->stats.effect, other_e->primary_weapon.entity->stats.effect);
            self->queue_free();
            ztd_info(__FUNCTION__, "Picked up item");
        }
    }
};
}