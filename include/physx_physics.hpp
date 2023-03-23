#ifndef PHYSX_PHYSICS_HPP
#define PHYSX_PHYSICS_HPP

#include "core.hpp"

#include "App/Game/Physics/physics_world.hpp"

#include <unordered_map>

namespace physics {

using namespace physx;
// note(zack): SDK says not to create stuff during this callback
class rigidbody_event_callback : public physx::PxSimulationEventCallback {
    // std::unordered_map<u64, PxU32> active_contacts;

    virtual ~rigidbody_event_callback() = default;

    virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {}
	virtual void onWake(PxActor** actors, PxU32 count) {}
	virtual void onSleep(PxActor** actors, PxU32 count) {}
	virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) {}
	virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) {}

    virtual void onContact(const physx::PxContactPairHeader& pairHeader,
        const physx::PxContactPair* pairs, physx::PxU32 nbPairs
    ) override {
        using namespace physx;
        for(PxU32 i=0; i < nbPairs; i++) {
            const PxContactPair& cp = pairs[i];
            
            auto* rb0 = (rigidbody_t*)pairHeader.actors[0]->userData;
            auto* rb1 = (rigidbody_t*)pairHeader.actors[1]->userData;

            if(cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {      
                if (rb0->on_collision) {
                    rb0->on_collision(rb0, rb1);
                }
                if (rb1->on_collision) {
                    rb1->on_collision(rb1, rb0);
                }
            } else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                if (rb0->on_collision_end) {
                    rb0->on_collision(rb0, rb1);
                }
                if (rb1->on_collision_end) {
                    rb1->on_collision(rb1, rb0);
                }
            }
        }
    }
};

struct physx_backend_t {
    physx_state_t* state{0};
    physics_world_t* world{0};
};

inline static physx_backend_t*
physx_init_backend(physx_backend_t* pb, arena_t* arena) {
    assert(pb);
    pb->state = arena_alloc_ctor<physx_state_t>(
        arena, 1,
        *arena, megabytes(512)
    );

    init_physx_state(*pb->state);

    return pb;
}
////////////////////////////////////////////////////////////////////////////////////
// Impl
////////////////////////////////////////////////////////////////////////////////////
inline static physx_backend_t* get_physx(const api_t* api) {
    assert(api->type == backend_type::PHYSX);
    return (physx_backend_t*)api->backend;
}

static rigidbody_t*
physx_create_rigidbody_impl(
    api_t* api,
    rigidbody_type type,
    void* data
) {
    const auto* ps = get_physx(api);
    assert(api->rigidbody_count < PHYSICS_MAX_RIGIDBODY_COUNT);
    rigidbody_t* rb = &api->rigidbodies[api->rigidbody_count++];
    const auto t = cast_transform(math::transform_t{});
    switch (rb->type = type) {
        case rigidbody_type::DYNAMIC: {
            rb->api_data = ps->state->physics->createRigidDynamic(t);
            ps->world->scene->addActor(*(physx::PxActor*)rb->api_data);
            ((physx::PxActor*)rb->api_data)->userData = rb;
            rb->user_data = data;
        }   break;
        case rigidbody_type::STATIC: {
            rb->api_data = ps->state->physics->createRigidStatic(t);
            ps->world->scene->addActor(*(physx::PxActor*)rb->api_data);
            ((physx::PxActor*)rb->api_data)->userData = rb;
            rb->user_data = data;
        }   break;
        case_invalid_default;
    }
    return rb;
}

static void
physx_create_collider_impl(
    api_t* api,
    rigidbody_t* rigidbody,
    collider_shape_type type,
    void* info
) {
    const auto* ps = get_physx(api);
    assert(rigidbody->collider_count < RIGIDBODY_MAX_COLLIDER_COUNT);
    local_persist u64 s_collider_id = 1; 
    collider_t* col = &rigidbody->colliders[rigidbody->collider_count++];
    col->id = s_collider_id++;
    auto* material = ps->state->physics->createMaterial(0.5f, 0.5f, 0.1f);
    switch (col->type = type) {
        case collider_shape_type::CONVEX: {
            auto* ci = (collider_convex_info_t*)info;
            physx::PxDefaultMemoryInputData input((u8*)ci->mesh, safe_truncate_u64(ci->size));
            auto convex_mesh = ps->state->physics->createConvexMesh(input);
            col->shape = 
                physx::PxRigidActorExt::createExclusiveShape(
                    *(physx::PxRigidActor*)rigidbody->api_data,
                    physx::PxConvexMeshGeometry(convex_mesh), 
                    *material
                );
            ((physx::PxShape*)col->shape)->userData = col;
        }   break;
        case collider_shape_type::TRIMESH: {
            auto* ci = (collider_trimesh_info_t*)info;
            physx::PxDefaultMemoryInputData input((u8*)ci->mesh, safe_truncate_u64(ci->size));
            auto trimesh_mesh = ps->state->physics->createTriangleMesh(input);
            col->shape = 
                physx::PxRigidActorExt::createExclusiveShape(
                    *(physx::PxRigidActor*)rigidbody->api_data,
                    physx::PxTriangleMeshGeometry(trimesh_mesh), 
                    *material
                );
            ((physx::PxShape*)col->shape)->userData = col;
        }   break;
        case collider_shape_type::SPHERE: {
            auto* ci = (collider_sphere_info_t*)info;
            col->shape = physx::PxRigidActorExt::createExclusiveShape(
                *(physx::PxRigidActor*)rigidbody->api_data,
                physx::PxSphereGeometry(ci->radius), *material);
            ((physx::PxShape*)col->shape)->userData = col;
        }   break;
        case_invalid_default;
    }
}

static PxFilterFlags filterShader(
    PxFilterObjectAttributes attributes0,
    PxFilterData filterData0,
    PxFilterObjectAttributes attributes1,
    PxFilterData filterData1,
    PxPairFlags& pairFlags,
    const void* constantBlock,
    PxU32 constantBlockSize)
{
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
    // pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
    // pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
    return PxFilterFlag::eDEFAULT;
}

////////////////////////////////////////////////////////////////////////////////////
// Export
////////////////////////////////////////////////////////////////////////////////////


void
physx_create_scene(api_t* api, const void* filter = 0) {
    gen_info(__FUNCTION__, "Creating scene");
    auto* ps = get_physx(api);
    assert(ps->state);
    // bug with arena
    ps->world = new physics::physics_world_t{};
    // ps->world = arena_alloc<physics::physics_world_t>(api->arena);
    if (filter) {
        physics::physics_world_init(ps->state, ps->world, new rigidbody_event_callback{}, (physx::PxSimulationFilterShader)filterShader);
    } else {
        physics::physics_world_init(ps->state, ps->world, new rigidbody_event_callback{}, filterShader);
    }

    // ps->world->scene->setSimulationEventCallback(new rigidbody_event_callback);
    assert(ps->world->scene->getSimulationEventCallback());
}

void
physx_destroy_scene(api_t*) {
    assert(0);
}

rigidbody_t*
physx_create_rigidbody(api_t* api, void* entity, rigidbody_type type) {
    auto* ps = get_physx(api);
    return physx_create_rigidbody_impl(api, type, entity);
}

collider_t*
physx_create_collider(api_t* api, rigidbody_t* rigidbody, collider_shape_type type, void* collider_info) {
    physx_create_collider_impl(api, rigidbody, type, collider_info);
    return &rigidbody->colliders[rigidbody->collider_count-1];
}

raycast_result_t
physx_raycast_world(const api_t* api, v3f ro, v3f rd) {
    auto* ps = get_physx(api);
    const physx::PxVec3 pro{ ro.x, ro.y, ro.z };
    const physx::PxVec3 prd{ rd.x, rd.y, rd.z };
    physx::PxRaycastBuffer hit;

    raycast_result_t result{};
    result.hit = ps->world->scene->raycast(pro, prd, 1000.0f, hit);
    if (result.hit) {
        if (hit.block.actor->userData) {
            result.user_data = hit.block.actor->userData;
        }

        const auto [nx,ny,nz] = hit.block.normal;
        const auto [px,py,pz] = hit.block.position;
        result.distance = hit.block.distance;
        result.point = v3f{px,py,pz};
        result.normal = v3f{nx,ny,nz};
    }
    return result;
}

void
physx_simulate(api_t* api, f32 dt) {
    const auto* ps = get_physx(api);
    const auto mark = arena_get_mark(api->arena);
    ps->world->scene->simulate(
        dt
        // 0, 
        // arena_get_top(api->arena), 
        // safe_truncate_u64(arena_get_remaining(api->arena))
    );
    ps->world->scene->fetchResults(true);
    arena_set_mark(api->arena, mark);
}

void
physx_sync_rigidbody(api_t* api, rigidbody_t* rb) {
    assert(rb && rb->api_data);
    physx::PxTransform t = ((physx::PxRigidActor*)rb->api_data)->getGlobalPose();

    if ((rb->flags & rigidbody_flags::SKIP_SYNC) == 0) {
        rb->position = v3f{t.p.x, t.p.y, t.p.z};
        rb->orientation = glm::quat{t.q.w, t.q.x, t.q.y, t.q.z};
        if (rb->type == physics::rigidbody_type::DYNAMIC) {
            const auto [vx,vy,vz] = ((physx::PxRigidBody*)rb->api_data)->getLinearVelocity();
            const auto [ax,ay,az] = ((physx::PxRigidBody*)rb->api_data)->getAngularVelocity();
            rb->velocity = v3f{vx,vy,vz};
            rb->angular_velocity = v3f{ax,ay,az};
        }
    }
    
    rb->flags &= ~rigidbody_flags::SKIP_SYNC;
}

void
physx_set_rigidbody(api_t* api, rigidbody_t* rb) {
    const auto& p = rb->position;
    const auto& q = rb->orientation;
    const auto& v = rb->velocity;
    const auto& av = rb->angular_velocity;
    physx::PxTransform t;
    t.p = {p.x, p.y, p.z};
    t.q = {q.x, q.y, q.z, q.w};

    ((physx::PxRigidActor*)rb->api_data)->setGlobalPose(t, false);
    if (rb->type == physics::rigidbody_type::DYNAMIC) {
        ((physx::PxRigidBody*)rb->api_data)->setLinearVelocity({v.x,v.y,v.z}, false);
        ((physx::PxRigidBody*)rb->api_data)->setAngularVelocity({av.x,av.y,av.z}, false);
    }
}

};

#endif