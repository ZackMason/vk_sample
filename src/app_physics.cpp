#include "core.hpp"

#ifndef GEN_LINK_PHYSICS_API_PHYSX
#define GEN_LINK_PHYSICS_API_PHYSX 1
#endif

#if GEN_LINK_PHYSICS_API_PHYSX
#include "App\Game\Physics\physics_world.hpp"
#include "physx_physics.hpp"
#endif

#include "custom_physics.hpp"


#if GEN_INTERNAL
debug_table_t gs_debug_table;
size_t gs_main_debug_record_size = __COUNTER__;
#endif

debug_table_t* get_debug_table() {
#if GEN_INTERNAL
    return &gs_debug_table;
#else
    return nullptr;
#endif
}

size_t get_debug_table_size() {
#if GEN_INTERNAL
    return gs_main_debug_record_size;
#else
    return 0;
#endif
}

namespace physics {

#if GEN_LINK_PHYSICS_API_PHYSX
static void 
init_physx(api_t* api, arena_t* arena) {
    api->arena = arena;
    physx_backend_t* backend = arena_alloc<physx_backend_t>(
        arena
    );
    api->backend = physx_init_backend(backend, arena);

    api->simulate           = physx_simulate;
    api->set_rigidbody      = physx_set_rigidbody;
    api->sync_rigidbody     = physx_sync_rigidbody;

    api->add_rigidbody      = physx_add_rigidbody;
    api->remove_rigidbody   = physx_remove_rigidbody;

    api->create_rigidbody   = physx_create_rigidbody;
    api->create_collider    = physx_create_collider;
    api->raycast_world      = physx_raycast_world;

    api->create_scene       = physx_create_scene;
    api->destroy_scene      = physx_destroy_scene;

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
physics_init_api(api_t* api, backend_type type, arena_t* arena) {
    assert(api && arena);
    switch (api->type = type) {
        case backend_type::CUSTOM: {
            init_custom(api, arena);
        } break;
#if GEN_LINK_PHYSICS_API_PHYSX
        case backend_type::PHYSX: {
            init_physx(api, arena);
        } break;
#endif
        case_invalid_default;
    }
}
