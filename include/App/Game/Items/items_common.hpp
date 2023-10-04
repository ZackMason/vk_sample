#pragma once 

#include "App/Game/Entity/entity.hpp"

namespace zyy::item {

    void pickup_item(physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
        auto* self = (zyy::entity_t*)trigger->user_data;
        auto* other_e = (zyy::entity_t*)other->user_data;
        auto* world = (zyy::world_t*)trigger->api->user_world;
        if (other_e->type == zyy::entity_type::player) {
            if (other_e->primary_weapon.entity) {
                other_e->primary_weapon.entity->stats.effect = self->stats.effect;
            }
            other_e->stats.effect = self->stats.effect;

            self->queue_free();
        }
    };
}