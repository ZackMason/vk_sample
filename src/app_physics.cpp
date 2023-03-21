#include "core.hpp"

#include "App\Game\Physics\physics_world.hpp"
#include "physx_physics.hpp"

namespace physics {

static void 
init_physx(api_t* api, arena_t* arena) {
    physx_state_t* physx_state = arena_alloc_ctor<physx_state_t>(
        arena, 1,
        *arena, megabytes(512)
    );
    api->backend = physx_state;

    init_physx_state(*physx_state);

    api->simulate           = physx_simulate;
    api->create_rigidbody   = physx_create_rigidbody;
    api->create_collider    = physx_create_collider;
    api->raycast_world      = physx_raycast_world;
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
        
        default:
            assert(0);
            break;
    }
}
