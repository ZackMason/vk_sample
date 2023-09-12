#ifndef PHYSX_PHYSICS_HPP
#define PHYSX_PHYSICS_HPP

#include "zyy_core.hpp"

#include "App/Game/Physics/physics_world.hpp"

#include <unordered_map>

#include "extensions/PxRigidBodyExt.h"

namespace physics {

using namespace physx;

PxVec3 pvec(const v3f& v) {
    return PxVec3{v.x,v.y,v.z};
}

glm::mat3 tensor_to_mat3(const physx::PxVec3& inertiaTensor) {
    glm::mat3 mat;

    mat[0][0] = inertiaTensor.x;
    mat[0][1] = 0.0f;
    mat[0][2] = 0.0f;
    mat[1][0] = 0.0f;
    mat[1][1] = inertiaTensor.y;
    mat[1][2] = 0.0f;
    mat[2][0] = 0.0f;
    mat[2][1] = 0.0f;
    mat[2][2] = inertiaTensor.z;

    return mat;
}

// note(zack): SDK says not to create stuff during this callback
class rigidbody_event_callback : public physx::PxSimulationEventCallback {
    virtual ~rigidbody_event_callback() = default;

    virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {}
	virtual void onWake(PxActor** actors, PxU32 count) {}
	virtual void onSleep(PxActor** actors, PxU32 count) {}
	virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) {
        TIMED_FUNCTION;
        using namespace physx;

        for(PxU32 i=0; i < count; i++) {
            const PxTriggerPair& tp = pairs[i];
            
            auto* rb0 = (rigidbody_t*)tp.triggerActor->userData;
            auto* rb1 = (rigidbody_t*)tp.otherActor->userData;

            if (tp.status == PxPairFlag::eNOTIFY_TOUCH_FOUND) {
                if (rb0->on_trigger) {
                    rb0->on_trigger(rb0, rb1);
                }
            } else if (tp.status == PxPairFlag::eNOTIFY_TOUCH_LOST) {
                if (rb0->on_trigger_end) {
                    rb0->on_trigger_end(rb0, rb1);
                }
            }
        }
    }

	virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) {}

    virtual void onContact(const physx::PxContactPairHeader& pairHeader,
        const physx::PxContactPair* pairs, physx::PxU32 nbPairs
    ) override {
        TIMED_FUNCTION;
        using namespace physx;

        for(PxU32 i=0; i < nbPairs; i++) {
            const PxContactPair& cp = pairs[i];
            
            auto* rb0 = (rigidbody_t*)pairHeader.actors[0]->userData;
            auto* rb1 = (rigidbody_t*)pairHeader.actors[1]->userData;

            if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {      
                if (rb0->on_collision) {
                    rb0->on_collision(rb0, rb1);
                }
                if (rb1->on_collision) {
                    rb1->on_collision(rb1, rb0);
                }
            } else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                if (rb0->on_collision_end) {
                    rb0->on_collision_end(rb0, rb1);
                }
                if (rb1->on_collision_end) {
                    rb1->on_collision_end(rb1, rb0);
                }
            }
        }
    }
};

class character_hit_report : public physx::PxUserControllerHitReport
{
public:
    void onControllerHit(const physx::PxControllersHit& hit) override { }
    void onObstacleHit(const physx::PxControllerObstacleHit& hit) override { }
    void onShapeHit(const physx::PxControllerShapeHit& hit) override
    {
        TIMED_FUNCTION;
        // physx::PxRigidActor* a = hit.shape->getActor();
        // auto* rb0 = (rigidbody_t*)hit.controller->getActor()->userData;
        // auto* rb1 = (rigidbody_t*)a->userData;

        // if (rb0->on_collision) {
        //     rb0->on_collision(rb0, rb1);
        // }
        // if (rb1->on_collision) {
        //     rb1->on_collision(rb1, rb0);
        // }

        PxRigidDynamic* actor = hit.shape->getActor()->is<PxRigidDynamic>();
        if(actor) {
            if(actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
                return;

            // We only allow horizontal pushes. Vertical pushes when we stand on dynamic objects creates
            // useless stress on the solver. It would be possible to enable/disable vertical pushes on
            // particular objects, if the gameplay requires it.
            const PxVec3 upVector = hit.controller->getUpDirection();
            const PxF32 dp = hit.dir.dot(upVector);

            if(fabsf(dp)<1e-3f)
            {
                const PxTransform globalPose = actor->getGlobalPose();
                const PxVec3 localPos = globalPose.transformInv(toVec3(hit.worldPos));
                // framerate bug with hit.length I believe
                PxRigidBodyExt::addForceAtLocalPos(*actor, hit.dir*hit.length*1000.0f, localPos, PxForceMode::eFORCE);
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

void physx_rigidbody_set_collision_flags(rigidbody_t* rb) {
    PxFilterData filter{};
    filter.word0 = rb->layer;
    filter.word1 = rb->group;
        
    for (u32 i = 0; i < rb->collider_count; i++) {
        ((PxShape*)rb->colliders[i].shape)->setQueryFilterData(filter);
        ((PxShape*)rb->colliders[i].shape)->setSimulationFilterData(filter);
    }
}

void physx_rigidbody_set_mass(rigidbody_t* rb, f32 x) {
    if (rb->type == rigidbody_type::DYNAMIC) {
        PxRigidDynamic* actor = (PxRigidDynamic*)rb->api_data;
        
        actor->setMass(x);
    }
}

void physx_rigidbody_set_ccd(rigidbody_t* rb, bool x) {
    if (rb->type == rigidbody_type::DYNAMIC) {
        PxRigidDynamic* actor = (PxRigidDynamic*)rb->api_data;
        
        actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    }
    if (rb->type == rigidbody_type::KINEMATIC) {

    }
    if (rb->type == rigidbody_type::CHARACTER) {
        
    }
}

void physx_rigidbody_set_gravity(rigidbody_t* rb, bool x) {
    if (rb->type == rigidbody_type::DYNAMIC) {
        PxRigidDynamic* actor = (PxRigidDynamic*)rb->api_data;
        
        actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !x);
    }
}

void physx_rigidbody_set_velocity(rigidbody_t* rb, const v3f& v) {
    if (rb->type == rigidbody_type::DYNAMIC) {
        PxRigidDynamic* actor = (PxRigidDynamic*)rb->api_data;
        actor->setLinearVelocity(pvec(v));
    }
}

void physx_rigidbody_add_impulse(rigidbody_t* rb, const v3f& v) {
    if (rb->type == rigidbody_type::DYNAMIC) {
        PxRigidDynamic* actor = (PxRigidDynamic*)rb->api_data;
        
        const PxVec3 pos = pvec(v3f{0.0});
        PxRigidBodyExt::addForceAtLocalPos(*actor, pvec(v), pos, PxForceMode::eIMPULSE);
    }
}


void physx_rigidbody_add_force(rigidbody_t* rb, const v3f& v) {
    if (rb->type == rigidbody_type::DYNAMIC) {
        PxRigidDynamic* actor = (PxRigidDynamic*)rb->api_data;
        
        const PxVec3 pos = pvec(v3f{0.0});
        PxRigidBodyExt::addForceAtLocalPos(*actor, pvec(v), pos, PxForceMode::eFORCE);
    }
}

void physx_rigidbody_add_force_at_point(rigidbody_t* rb, const v3f& v, const v3f& p) {
    if (rb->type == rigidbody_type::DYNAMIC) {
        PxRigidDynamic* actor = (PxRigidDynamic*)rb->api_data;
        
        const PxTransform globalPose = actor->getGlobalPose();
        const PxVec3 localPos = globalPose.transformInv(pvec(p));
        PxRigidBodyExt::addForceAtLocalPos(*actor, pvec(v), localPos, PxForceMode::eFORCE);
    }
}

void physx_collider_set_active(collider_t* collider, bool x) {
    auto* shape = (PxShape*)collider->shape;
    shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, x);
}

void physx_collider_set_trigger(collider_t* collider, bool x) {
    auto* shape = (PxShape*)collider->shape;
    physx_collider_set_active(collider, !x);
    shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, x);
    shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, !x);
}

// void physx_rigidbody_set_active(api_t* api, rigidbody_t* rb, bool x) {
//     // const auto* ps = get_physx(api);
    
// }

void
physx_remove_rigidbody(api_t* api, rigidbody_t* rb) {
    const auto* ps = get_physx(api);
    assert(ps);
    assert(rb);
    assert(ps->world);
    assert(ps->world->scene);
    if (rb->type == rigidbody_type::CHARACTER) {
        auto* controller = (physx::PxController*)rb->api_data;
        
        ps->world->scene->removeActor(*controller->getActor());
        
    } else {
        ps->world->scene->removeActor(*(physx::PxActor*)rb->api_data);
    }
}

void
physx_add_rigidbody(api_t* api, rigidbody_t* rb) {
    const auto* ps = get_physx(api);
    assert(ps);
    assert(rb);
    assert(ps->world);
    assert(ps->world->scene);
    if (rb->type == rigidbody_type::CHARACTER) {
        auto* controller = (physx::PxController*)rb->api_data;
        // ps->world->scene->addActor(*controller->getActor());
        controller->getActor()->userData = rb;
    } else {
        ps->world->scene->addActor(*(physx::PxActor*)rb->api_data);
        ((physx::PxActor*)rb->api_data)->userData = rb;
    }
}

static rigidbody_t*
physx_create_rigidbody_impl(
    api_t* api,
    rigidbody_type type,
    void* data,
    v3f position,
    quat orientation
) {
    const auto* ps = get_physx(api);
    assert(api->rigidbody_count < PHYSICS_MAX_RIGIDBODY_COUNT);
    rigidbody_t* rb = &api->rigidbodies[api->rigidbody_count++];
    *rb = {api};
    
    const auto t = cast_transform(math::transform_t{position, orientation});
    switch (rb->type = type) {
        case rigidbody_type::KINEMATIC:
        case rigidbody_type::DYNAMIC: {
            auto* body = ps->state->physics->createRigidDynamic(t);
            rb->api_data = body;

            if (rb->type == rigidbody_type::KINEMATIC) {
                body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
            }

            physx_add_rigidbody(api, rb);
            rb->inertia = tensor_to_mat3(((physx::PxRigidBody*)rb->api_data)->getMassSpaceInertiaTensor());
            rb->inverse_inertia = tensor_to_mat3(((physx::PxRigidBody*)rb->api_data)->getMassSpaceInvInertiaTensor());
            rb->user_data = data;
        }   break;
        case rigidbody_type::CHARACTER: {
            PxCapsuleControllerDesc desc;
            desc.height = 1.7f;
            desc.radius = .5f;
            desc.climbingMode = PxCapsuleClimbingMode::eEASY;
            desc.behaviorCallback = nullptr;
            desc.reportCallback = arena_alloc<character_hit_report>(api->arena);
            desc.material = ps->state->physics->createMaterial(0.5f, 0.5f, 0.1f);
            auto* controller = ps->world->controller_manager->createController(desc);
            controller->setPosition({position.x, position.y, position.z});
            // controller->setSlopeLimit(3.14159f*0.25f);
            rb->api_data = controller;
            rb->user_data = data;

            physx_add_rigidbody(api, rb);
            assert(api->character_count < PHYSICS_MAX_CHARACTER_COUNT);
            api->characters[api->character_count++] = rb;
        }   break;
        case rigidbody_type::STATIC: {
            auto* body = ps->state->physics->createRigidStatic(t);
            rb->api_data = body;

            physx_add_rigidbody(api, rb);
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
    *col = {};
    col->id = s_collider_id++;
    local_persist auto* material = ps->state->physics->createMaterial(0.5f, 0.5f, 0.1f);
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
            ((physx::PxShape*)col->shape)->setLocalPose(physx::PxTransform(physx::PxVec3(ci->origin.x, ci->origin.y, ci->origin.z)));
            ((physx::PxShape*)col->shape)->userData = col;
        }   break;
        case collider_shape_type::BOX: {
            auto* ci = (collider_box_info_t*)info;
            col->shape = physx::PxRigidActorExt::createExclusiveShape(
                *(physx::PxRigidActor*)rigidbody->api_data,
                physx::PxBoxGeometry(PxVec3{ci->size.x, ci->size.y, ci->size.z}), *material);
            ((physx::PxShape*)col->shape)->userData = col;
        }   break;
        case_invalid_default;
    }
    PxFilterData filter{};
    filter.word0 = rigidbody->layer;
    filter.word1 = rigidbody->group;
        
    ((PxShape*)col->shape)->setQueryFilterData(filter);
    ((PxShape*)col->shape)->setSimulationFilterData(filter);
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
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
        pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
        return physx::PxFilterFlag::eDEFAULT;
    }
    if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1)) {
        pairFlags =
            physx::PxPairFlag::eCONTACT_DEFAULT |
            physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
            physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
            physx::PxPairFlag::eNOTIFY_TOUCH_LOST |
            physx::PxPairFlag::eDETECT_DISCRETE_CONTACT |
            physx::PxPairFlag::eDETECT_CCD_CONTACT |
            physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

        if (PxFilterObjectIsKinematic(attributes0) && PxFilterObjectIsKinematic(attributes1))
        {
            pairFlags.clear(physx::PxPairFlag::eSOLVE_CONTACT);
        }

        // generate callbacks for collisions between kinematic and dynamic objects
        if (PxFilterObjectIsKinematic(attributes0) != PxFilterObjectIsKinematic(attributes1))
        {
            // return physx::PxFilterFlag::eCALLBACK;
        }
    }
    
    return PxFilterFlag::eDEFAULT;
}

////////////////////////////////////////////////////////////////////////////////////
// Export
////////////////////////////////////////////////////////////////////////////////////


void
physx_create_scene(api_t* api, const void* filter = 0) {
    TIMED_FUNCTION;
    zyy_info(__FUNCTION__, "Creating scene");
    auto* ps = get_physx(api);
    assert(ps->state);
    
    ps->world = arena_alloc<physics::physics_world_t>(api->arena);
    // ps->world = arena_alloc<physics::physics_world_t>(api->arena);
    if (filter) {
        physics::physics_world_init(ps->state, ps->world, arena_alloc<rigidbody_event_callback>(api->arena), (physx::PxSimulationFilterShader)filterShader);
    } else {
        physics::physics_world_init(ps->state, ps->world, arena_alloc<rigidbody_event_callback>(api->arena), filterShader);
    }

    // ps->world->scene->setSimulationEventCallback(new rigidbody_event_callback);
    assert(ps->world->scene->getSimulationEventCallback());
}

void
physx_destroy_scene(api_t* api) {
    auto* ps = get_physx(api);
    ps->world->scene->release();
    ps->world->scene = nullptr;
    assert(0);
}


// these dont do anything??
rigidbody_t*
physx_create_rigidbody(api_t* api, void* entity, rigidbody_type type, const v3f& pos, const quat& orientation) {
    TIMED_FUNCTION;
    auto* ps = get_physx(api);
    auto* rb = physx_create_rigidbody_impl(api, type, entity, pos, orientation);
    rb->api = api;
    return rb;
}

collider_t*
physx_create_collider(api_t* api, rigidbody_t* rigidbody, collider_shape_type type, void* collider_info) {
    TIMED_FUNCTION;
    physx_create_collider_impl(api, rigidbody, type, collider_info);
    
    rigidbody->colliders[rigidbody->collider_count-1].rigidbody = rigidbody;
    return &rigidbody->colliders[rigidbody->collider_count-1];
}

raycast_result_t
physx_raycast_world(const api_t* api, v3f ro, v3f rd) {
    TIMED_FUNCTION;
    auto* ps = get_physx(api);
    auto dist = glm::length(rd);
    if (dist == 0.0f) {
        zyy_warn(__FUNCTION__, "Ray direction is zero");
    } else {
        rd = glm::normalize(rd);
    }
    const physx::PxVec3 pro{ ro.x, ro.y, ro.z };
    const physx::PxVec3 prd{ rd.x, rd.y, rd.z };
    physx::PxRaycastBuffer hit{};

    PxQueryFilterData filter_data(PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC);

    raycast_result_t result{};
    result.hit = ps->world->scene->raycast(pro, prd, dist, hit, PxHitFlag::eDEFAULT, filter_data);
    if (result.hit) {
        result.user_data = hit.block.actor->userData;
        
        const auto [nx,ny,nz] = hit.block.normal;
        const auto [px,py,pz] = hit.block.position;
        result.distance = hit.block.distance;
        result.point = v3f{px,py,pz};
        result.normal = v3f{nx,ny,nz};
    }
    return result;
}

auto dampen(auto currentValue, auto targetValue, float dampingFactor, float deltaTime) {
    auto valueDifference = targetValue - currentValue;

    auto changeAmount = valueDifference * dampingFactor * deltaTime;

    currentValue += changeAmount;

    return currentValue;
}

struct move_filter : public PxQueryFilterCallback
{
	PxQueryHitType::Enum preFilter(
    const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) {
        return PxQueryHitType::Enum::eBLOCK;
        // return PxQueryHitType::Enum::eNONE;
    }


	virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor)
	{
        return PxQueryHitType::Enum::eBLOCK;
        // return PxQueryHitType::Enum::eNONE;
	}
} gs_move_filter;

void
physx_simulate(api_t* api, f32 dt) {
    TIMED_FUNCTION;
    const auto* ps = get_physx(api);

    // zyy_info(__FUNCTION__, "dt: {}", dt);

    range_u64(i, 0, api->character_count) {
        auto* rb = api->characters[i];
        if (rb) {
            auto* controller = (physx::PxController*)rb->api_data;
            auto* transform = (math::transform_t*)((u8*)rb->user_data + api->entity_transform_offset);
            // rb->integrate(dt, 9.81f * 0.1f * 0.0f);
            auto v = rb->velocity * dt * 100.0f;
            f32 vm = glm::length(rb->velocity);
            const auto [lpx,lpy,lpz] = controller->getPosition();

            const PxU32 move_flags = controller->move(
                {v.x, v.y, v.z}, 0.0f, 0.0f, {0, &gs_move_filter}
            );
            const auto [px,py,pz] = controller->getPosition();
            rb->position = v3f{px,py,pz};

            if(move_flags & PxControllerCollisionFlag::eCOLLISION_DOWN) {
                rb->flags |= rigidbody_flags::IS_ON_GROUND;
			} else {
                rb->flags &= ~rigidbody_flags::IS_ON_GROUND;
            }
            if(move_flags & PxControllerCollisionFlag::eCOLLISION_SIDES) {
                rb->velocity = (rb->position - v3f{lpx, lpy, lpz});
                rb->flags |= rigidbody_flags::IS_ON_WALL;
            } else {
                if(move_flags & PxControllerCollisionFlag::eCOLLISION_UP) {
                    rb->velocity = (v3f{px,py,pz} - v3f{lpx, lpy, lpz});
                    // rb->velocity.y = 0.0f;
                }
                rb->flags &= ~rigidbody_flags::IS_ON_WALL;
            }
            if (glm::length(rb->velocity) > vm) { rb->velocity = glm::normalize(rb->velocity) * vm; }

            transform->origin = rb->position;
        }
    }

    temp_arena_t scratch = *api->arena;
    scratch.top = align16(scratch.top);
    scratch.size = (scratch.size / kilobytes(16)) * kilobytes(16);

    ps->world->scene->simulate(
        dt,
        0
        // ,arena_get_top(&scratch), 
        // safe_truncate_u64(arena_get_remaining(&scratch))
    );
    ps->world->scene->fetchResults(true);

    if (api->entity_transform_offset) {
        PxU32 nb_active_actors;
        PxActor** active_actors = ps->world->scene->getActiveActors(nb_active_actors);
        range_u64(i, 0, nb_active_actors) {
            auto* body = (rigidbody_t*)active_actors[i]->userData;
            assert(body);
            auto* transform = (math::transform_t*)((u8*)body->user_data + api->entity_transform_offset);
            if (body->type == rigidbody_type::DYNAMIC) {
                if (auto* rb = active_actors[i]->is<physx::PxRigidBody>()) {
                    auto t = rb->getGlobalPose();
                    body->position = transform->origin = v3f{t.p.x, t.p.y, t.p.z};
                    transform->basis = glm::toMat3(body->orientation = quat{t.q.w, t.q.x, t.q.y, t.q.z});
                    const auto [vx,vy,vz] = rb->getLinearVelocity();
                    const auto [ax,ay,az] = rb->getAngularVelocity();
                    body->velocity = v3f{vx,vy,vz};
                    body->angular_velocity = v3f{ax,ay,az};
                }
            } else if (body->type == rigidbody_type::CHARACTER) {
                // if you are wondering, yes we actually make it to here
                // if (auto* cct = ((physx::PxController*)body->api_data)) {
                //     auto [x,y,z] = cct->getPosition();
                //     transform->origin = v3f{x,y,z};
                // }
            } 
        }
    }
}

void
physx_sync_rigidbody(api_t* api, rigidbody_t* rb) {
    TIMED_FUNCTION;
    assert(rb && rb->api_data);
    // if ((rb->flags & rigidbody_flags::SKIP_SYNC) == 0) {
    //     rb->flags &= ~rigidbody_flags::SKIP_SYNC;
    //     return;
    // }

    if (rb->type == rigidbody_type::DYNAMIC) {
        physx::PxTransform t = ((physx::PxRigidActor*)rb->api_data)->getGlobalPose();
        rb->position = v3f{t.p.x, t.p.y, t.p.z};
        rb->orientation = glm::quat{t.q.w, t.q.x, t.q.y, t.q.z};
        if (rb->type == physics::rigidbody_type::DYNAMIC) {
            auto* rigid = (physx::PxRigidBody*)rb->api_data;
            const auto [vx,vy,vz] = rigid->getLinearVelocity();
            const auto [ax,ay,az] = rigid->getAngularVelocity();
            rb->velocity = v3f{vx,vy,vz};
            rb->angular_velocity = v3f{ax,ay,az};
        }
    } else if (rb->type == rigidbody_type::CHARACTER) {
        auto* controller = ((physx::PxController*)rb->api_data);
        const auto& v = rb->velocity;
        const auto [px,py,pz]  = controller->getPosition();
        rb->position = v3f{px,py,pz};
    }
    // rb->flags &= ~rigidbody_flags::SKIP_SYNC;
}

// set -> sim -> sync
void
physx_set_rigidbody(api_t* api, rigidbody_t* rb) {
    TIMED_FUNCTION;
    const auto& p = rb->position;
    const auto& q = rb->orientation;
    const auto& v = rb->velocity;
    const auto& av = rb->angular_velocity;
    
    if (rb->type == rigidbody_type::CHARACTER) {
        auto* controller = (physx::PxController*)rb->api_data;
        controller->setPosition({p.x,p.y,p.z});
    } else {
        physx::PxTransform t;
        t.p = {p.x, p.y, p.z};
        t.q = {q.x, q.y, q.z, q.w};

        ((physx::PxRigidActor*)rb->api_data)->setGlobalPose(t, false);

        if (rb->type == physics::rigidbody_type::DYNAMIC) {
            ((physx::PxRigidDynamic*)rb->api_data)->setLinearVelocity({v.x,v.y,v.z}, false);
            ((physx::PxRigidDynamic*)rb->api_data)->setAngularVelocity({av.x,av.y,av.z}, false);
        }
    }
}

};
#endif