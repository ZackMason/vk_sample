#include "zyy_core.hpp"

#ifndef ZYY_LINK_PHYSICS_API_PHYSX
#define ZYY_LINK_PHYSICS_API_PHYSX 1
#endif

#if ZYY_LINK_PHYSICS_API_PHYSX
#include "App\Game\Physics\physics_world.hpp"
#include "physx_physics.hpp"
#endif

#include "custom_physics.hpp"

platform_api_t Platform;

#if ZYY_INTERNAL
debug_table_t gs_debug_table;
size_t gs_main_debug_record_size = __COUNTER__;
#endif

debug_table_t* get_debug_table() {
#if ZYY_INTERNAL
    return &gs_debug_table;
#else
    return nullptr;
#endif
}

size_t get_debug_table_size() {
#if ZYY_INTERNAL
    return gs_main_debug_record_size;
#else
    return 0;
#endif
}

namespace physics {

#if ZYY_LINK_PHYSICS_API_PHYSX
static void 
init_physx(api_t* api, arena_t* arena) {
    api->arena = arena;
    physx_backend_t* backend = push_struct<physx_backend_t>(
        arena
    );
    api->backend = physx_init_backend(backend, arena);

    api->simulate           = physx_simulate;
    api->set_rigidbody      = physx_set_rigidbody;
    api->sync_rigidbody     = physx_sync_rigidbody;

    api->add_rigidbody      = physx_add_rigidbody;
    api->remove_rigidbody   = physx_remove_rigidbody;

    api->create_rigidbody     = physx_create_rigidbody;
    api->create_collider      = physx_create_collider;
    api->raycast_world        = physx_raycast_world;
    api->sphere_overlap_world = physx_sphere_overlap_world;

    api->create_scene       = physx_create_scene;
    api->destroy_scene      = physx_destroy_scene;

    // api->rigidbody_set_active = physx_rigidbody_set_active;
    api->collider_set_trigger = physx_collider_set_trigger;
    api->collider_set_active = physx_collider_set_active;

    api->rigidbody_add_impulse = physx_rigidbody_add_impulse;
    api->rigidbody_add_force = physx_rigidbody_add_force;
    api->rigidbody_set_velocity = physx_rigidbody_set_velocity;
    api->rigidbody_add_force_at_point = physx_rigidbody_add_force_at_point;

    api->rigidbody_set_gravity = physx_rigidbody_set_gravity;
    api->rigidbody_set_ccd = physx_rigidbody_set_ccd;
    api->rigidbody_set_mass = physx_rigidbody_set_mass;
    api->rigidbody_set_collision_flags = physx_rigidbody_set_collision_flags;

    api->
        get_debug_table_size= get_debug_table_size;
    api->get_debug_table    = get_debug_table;
}
#endif

static void 
init_custom(api_t* api, arena_t* arena) {
    api->arena = arena;
    
    api->simulate           = custom_simulate;
    api->set_rigidbody      = custom_set_rigidbody;
    api->sync_rigidbody     = custom_sync_rigidbody;

    api->add_rigidbody      = custom_add_rigidbody;
    api->remove_rigidbody   = custom_remove_rigidbody;

    api->create_rigidbody   = custom_create_rigidbody;
    api->create_collider    = custom_create_collider;
    api->raycast_world      = custom_raycast_world;

    api->create_scene       = custom_create_scene;
    api->destroy_scene      = custom_destroy_scene;

    api->
        get_debug_table_size= get_debug_table_size;
    api->get_debug_table    = get_debug_table;
}
};

using namespace physics;

export_fn(void)
physics_init_api(api_t* api, backend_type type, platform_api_t* platform, arena_t* arena) {
    Platform = *platform;
    assert(api && arena);
    api->Platform = &Platform;
    switch (api->type = type) {
        case backend_type::CUSTOM: {
            init_custom(api, arena);
        } break;
#if ZYY_LINK_PHYSICS_API_PHYSX
        case backend_type::PHYSX: {
            init_physx(api, arena);
        } break;
#endif
        case_invalid_default;
    }
}
