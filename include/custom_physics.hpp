#ifndef CUSTOM_PHYSICS_HPP
#define CUSTOM_PHYSICS_HPP

#include "core.hpp"

#include <unordered_map>

namespace physics {
void
custom_remove_rigidbody(api_t* api, rigidbody_t* rb) {
}

void
custom_add_rigidbody(api_t* api, rigidbody_t* rb) {
}

static rigidbody_t*
custom_create_rigidbody_impl(
    api_t* api,
    rigidbody_type type,
    void* data
) {
    assert(api->rigidbody_count < PHYSICS_MAX_RIGIDBODY_COUNT);
    rigidbody_t* rb = &api->rigidbodies[api->rigidbody_count++];
    *rb = {};
    return rb;
}

static void
custom_create_collider_impl(
    api_t* api,
    rigidbody_t* rigidbody,
    collider_shape_type type,
    void* info
) {
    assert(rigidbody->collider_count < RIGIDBODY_MAX_COLLIDER_COUNT);
    local_persist u64 s_collider_id = 1; 
    collider_t* col = &rigidbody->colliders[rigidbody->collider_count++];
    *col = {};
    col->id = s_collider_id++;
}


////////////////////////////////////////////////////////////////////////////////////
// Export
////////////////////////////////////////////////////////////////////////////////////

void
custom_create_scene(api_t* api, const void* filter = 0) {
    gen_info(__FUNCTION__, "Creating scene");
}

void
custom_destroy_scene(api_t*) {
    assert(0);
}

rigidbody_t*
custom_create_rigidbody(api_t* api, void* entity, rigidbody_type type) {
    return custom_create_rigidbody_impl(api, type, entity);
}

collider_t*
custom_create_collider(api_t* api, rigidbody_t* rigidbody, collider_shape_type type, void* collider_info) {
    custom_create_collider_impl(api, rigidbody, type, collider_info);
    return &rigidbody->colliders[rigidbody->collider_count-1];
}

raycast_result_t
custom_raycast_world(const api_t* api, v3f ro, v3f rd) {
    return {};
}

void
custom_simulate(api_t* api, f32 dt) {
}

void
custom_sync_rigidbody(api_t* api, rigidbody_t* rb) {
}

void
custom_set_rigidbody(api_t* api, rigidbody_t* rb) {
    const auto& p = rb->position;
    const auto& q = rb->orientation;
    const auto& v = rb->velocity;
    const auto& av = rb->angular_velocity;
}

};
#endif