#pragma once

#include "core.hpp"

#include "PxPhysicsAPI.h"

#include "PxPhysics.h"
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
        physx_state_t(arena_t& arena, size_t size) 
            : default_allocator{arena, size}
        {
        }
        
        arena_heap_t    default_allocator;
        error_callback_t error_callback;
        physx::PxFoundation* foundation{nullptr};
        physx::PxPvd* pvd{nullptr};
        physx::PxPhysics* physics{nullptr};
        physx::PxDefaultCpuDispatcher* dispatcher{nullptr};
        physx::PxCooking* cooking{nullptr};
    };

    inline void
    init_physx_state(
        physx_state_t& state
    ) {
        using namespace physx;
        state.foundation = PxCreateFoundation(PX_PHYSICS_VERSION, state.default_allocator, state.error_callback);

        assert(state.foundation);

        // state.pvd = PxCreatePvd(*state.foundation);
        // PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10000);
        // state.pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

        state.physics = PxCreatePhysics(PX_PHYSICS_VERSION, *state.foundation, PxTolerancesScale(), true, state.pvd);

        assert(state.physics);
        state.dispatcher = PxDefaultCpuDispatcherCreate(0);

        state.cooking = PxCreateCooking(PX_PHYSICS_VERSION, *state.foundation, PxCookingParams(state.physics->getTolerancesScale()));
        assert(state.cooking);
    }

    struct physics_world_t;

    struct rigid_body_t {
        physx_state_t*          state;

        bool is_dynamic = false;
        union {
            physx::PxRigidActor*    rigid{nullptr};
            physx::PxRigidDynamic*  dynamic;
        };
        physx::PxController*    controller{nullptr};
        physx::PxShape*         shape{nullptr};
        physx::PxMaterial*      material{nullptr};
        physics_shape_type      type{};

        v3f velocity{};
        v3f angular_velocity{};

        void create_character(physics_world_t* physx_world);

        void apply_impulse_to_center(v3f i) {
            dynamic->addForce({i.x, i.y, i.z}, physx::PxForceMode::eIMPULSE);
        }

        void set_angular_velocity(v3f v) {
            angular_velocity = v;            
            dynamic->setAngularVelocity({v.x,v.y,v.z}, true);
        }

        void set_velocity(v3f v) {
            velocity = v;
            dynamic->setLinearVelocity({v.x,v.y,v.z}, true);
        }

        void load_capsule(float radius, float height) {
            type = physics_shape_type::CAPSULE;
        }

        void load_sphere(float radius, const math::transform_t& transform) {
            type = physics_shape_type::SPHERE;

            physx::PxVec3 p;
            
            p.x = transform.origin.x;
            p.y = transform.origin.y;
            p.z = transform.origin.z;

            const auto temp_q = transform.get_orientation();
            physx::PxQuat q{ temp_q.x, temp_q.y, temp_q.z, temp_q.w };

            physx::PxTransform t(p, q);
            
            material = state->physics->createMaterial(0.5f, 0.5f, 0.1f);
            if (is_dynamic) {
                dynamic = state->physics->createRigidDynamic(t);
                shape = physx::PxRigidActorExt::createExclusiveShape(*dynamic, physx::PxSphereGeometry(1.0f), *material);
            } else {
                rigid = state->physics->createRigidStatic(t);
                shape = physx::PxRigidActorExt::createExclusiveShape(*rigid, physx::PxSphereGeometry(1.0f), *material);
            }
        }

        void load(std::byte* data, size_t size, const math::transform_t& transform) {
            switch(type) {
                case physics_shape_type::SPHERE:
                    load_sphere(1.0f, transform);
                    break;
                case physics_shape_type::TRIMESH:
                    load_trimesh(data, size, transform);
                    break;
                case physics_shape_type::CONVEX:
                    load_convex(data, size, transform);
                    break;
                default:
                    gen_warn("rigidbody", "Failed to load shape");
            }
        }
        void load_trimesh(std::byte* data, size_t size, const math::transform_t& transform) {
            assert(state);
            physx::PxVec3 p;
            p.x = transform.origin.x;
            p.y = transform.origin.y;
            p.z = transform.origin.z;

            const auto temp_q = transform.get_orientation();
            physx::PxQuat q{ temp_q.x, temp_q.y, temp_q.z, temp_q.w };

            physx::PxTransform t(p, q);
            
            rigid = state->physics->createRigidStatic(t);
            assert(rigid);

            material = state->physics->createMaterial(0.5f, 0.5f, 0.1f);
            physx::PxDefaultMemoryInputData input((u8*)data, safe_truncate_u64(size));
            auto tri_mesh = state->physics->createTriangleMesh(input);
        
            shape = physx::PxRigidActorExt::createExclusiveShape(*rigid, physx::PxTriangleMeshGeometry(tri_mesh), *material);
            type = physics_shape_type::TRIMESH;
        }

        void load_convex(std::byte* data, size_t size, const math::transform_t& transform) {
            assert(state);
            physx::PxVec3 p;
            p.x = transform.origin.x;
            p.y = transform.origin.y;
            p.z = transform.origin.z;

            const auto temp_q = transform.get_orientation();
            physx::PxQuat q{ temp_q.x, temp_q.y, temp_q.z, temp_q.w };

            physx::PxTransform t(p, q);
            
            dynamic = state->physics->createRigidDynamic(t);
            assert(dynamic);

            material = state->physics->createMaterial(0.5f, 0.5f, 0.1f);
            physx::PxDefaultMemoryInputData input((u8*)data, safe_truncate_u64(size));
            auto convex_mesh = state->physics->createConvexMesh(input);
        
            shape = physx::PxRigidActorExt::createExclusiveShape(*dynamic, physx::PxConvexMeshGeometry(convex_mesh), *material);
            type = physics_shape_type::CONVEX;
        }
    };


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

        gen_info("physx", "physics_world_init: {}", (void*)world);
        world->state = state;

        physx::PxSceneDesc scene_desc(state->physics->getTolerancesScale());
        scene_desc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);

        scene_desc.filterShader = filter_shader;
        scene_desc.cpuDispatcher = state->dispatcher;
        scene_desc.simulationEventCallback = sim_callback;
        scene_desc.flags = physx::PxSceneFlag::eENABLE_CCD |  physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
        world->scene = state->physics->createScene(scene_desc);

        if (world->controller_manager) {
            world->controller_manager->release();
            world->controller_manager = nullptr;
        }
        world->controller_manager = PxCreateControllerManager(*world->scene);
        assert(world->controller_manager);
    }


    void rigid_body_t::create_character(physics_world_t* physx_world) {
        physx::PxMaterial* mat = state->physics->createMaterial(0.5f, 0.5f, 0.1f);

        physx::PxCapsuleControllerDesc desc;
        desc.height = 1.5f;
        desc.radius = 1.0f;
        desc.climbingMode = physx::PxCapsuleClimbingMode::eEASY;
        desc.behaviorCallback = nullptr;
        desc.reportCallback = nullptr;
        desc.material = mat;

        controller = physx_world->controller_manager->createController(desc);
        assert(controller);
    }

};