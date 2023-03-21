#ifndef PHYSX_PHYSICS_HPP
#define PHYSX_PHYSICS_HPP

#include "core.hpp"

namespace physics {
////////////////////////////////////////////////////////////////////////////////////
// Impl
////////////////////////////////////////////////////////////////////////////////////
inline static physx_state_t* get_physx(const api_t* api) {
    assert(api->type == backend_type::PHYSX);
    return (physx_state_t*)api->backend;
}

static void
physx_create_rigidbody_impl(
    const api_t* api,
    rigidbody_type type,
    const math::transform_t& transform
) {

}

static void
physx_create_collider_impl(
    const api_t* api,
    const rigidbody_t* rigidbody,
    collider_shape_type type,
    void* info
) {

}

////////////////////////////////////////////////////////////////////////////////////
// Export
////////////////////////////////////////////////////////////////////////////////////

rigidbody_t*
physx_create_rigidbody(const api_t* api, void* entity, rigidbody_type type) {
    auto* ps = get_physx(api);
    physx_create_rigidbody_impl(api, type, math::transform_t{});
    return nullptr;
}

collider_t*
physx_create_collider(const api_t* api, const rigidbody_t* rigidbody, collider_shape_type type, void* collider_info) {
    physx_create_collider_impl(api, rigidbody, type, collider_info);
    return nullptr;
}

raycast_result_t
physx_raycast_world(const api_t*, v3f ro, v3f rd) {
    return raycast_result_t{};
}

void
physx_simulate(f32 dt) {

}

};

#endif