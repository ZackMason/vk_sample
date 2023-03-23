#include "core.hpp"

#include "App\Game\Physics\physics_world.hpp"
#include "physx_physics.hpp"

namespace physics {

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

    api->create_rigidbody   = physx_create_rigidbody;
    api->create_collider    = physx_create_collider;
    api->raycast_world      = physx_raycast_world;

    api->create_scene       = physx_create_scene;
    api->destroy_scene      = physx_destroy_scene;
}

};

using namespace physics;

export_fn(void)
physics_init_api(api_t* api, backend_type type, arena_t* arena) {
    assert(api && arena);
    switch (api->type = type) {
        case backend_type::PHYSX: {
            init_physx(api, arena);
        } break;
        case_invalid_default;
    }
}
