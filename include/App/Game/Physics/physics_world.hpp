#pragma once

//#include "PxPhysics.h"
#include "PxPhysicsAPI.h"
#include "PxRigidActor.h"
#include "PxActor.h"
#include "PxScene.h"

#include "characterkinematic/PxController.h"



#include "physics_utils.hpp"


namespace physics {

    enum struct physics_shape_type {
        NONE, CAPSULE, SPHERE, BOX, CONVEX, TRIMESH, SIZE
    };

    struct physx_state_t {
        arena_heap_t    default_allocator;
        error_callback_t error_callback;
        physx::PxFoundation* foundation{nullptr};
        physx::PxPvd* pvd{nullptr};
        physx::PxPhysics* physics{nullptr};
        physx::PxDefaultCpuDispatcher* dispatcher{nullptr};
        // physx::PxCooking* cooking{nullptr};
    };

    inline void
    init_physx_state(
        physx_state_t& state
    ) {
        using namespace physx;
        // local_persist PxDefaultAllocator gDefaultAllocatorCallback;
        // state.foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, state.error_callback);
        state.foundation = PxCreateFoundation(PX_PHYSICS_VERSION, state.default_allocator, state.error_callback);

        assert(state.foundation);

        // state.pvd = PxCreatePvd(*state.foundation);
        // PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10000);
        // state.pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

        state.physics = PxCreatePhysics(PX_PHYSICS_VERSION, *state.foundation, PxTolerancesScale(), true, state.pvd);

        assert(state.physics);
        state.dispatcher = PxDefaultCpuDispatcherCreate(0);

        // state.cooking = PxCreateCooking(PX_PHYSICS_VERSION, *state.foundation, PxCookingParams(state.physics->getTolerancesScale()));
        // assert(state.cooking);
    }


    struct physics_world_t {
        physx_state_t* state;
        physx::PxScene* scene{nullptr};
        physx::PxControllerManager* controller_manager{nullptr};
    };

    inline void
    physics_world_init(
        physx_state_t* state, 
        physics_world_t* world,
        physx::PxSimulationEventCallback* sim_callback,
        physx::PxSimulationFilterShader filter_shader = physx::PxDefaultSimulationFilterShader
    ) {
        assert(world);
        assert(state->physics);
        assert(state->dispatcher);

        ztd_info("physx", "physics_world_init: {}", (void*)world);
        world->state = state;

        physx::PxSceneDesc scene_desc(state->physics->getTolerancesScale());
        scene_desc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);

        scene_desc.filterShader = filter_shader;
        scene_desc.cpuDispatcher = state->dispatcher;
        scene_desc.simulationEventCallback = sim_callback;
        scene_desc.flags =  physx::PxSceneFlag::eENABLE_CCD | physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
        world->scene = state->physics->createScene(scene_desc);

        scene_desc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
        scene_desc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;

        if (world->controller_manager) {
            world->controller_manager->release();
            world->controller_manager = nullptr;
        }
        world->controller_manager = PxCreateControllerManager(*world->scene);

        assert(world->controller_manager);
    }


};