#pragma once

#include "foundation/PxAllocatorCallback.h"

#include <mutex>

#define PVD_HOST "127.0.0.1"
#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL; }

namespace physics {

math::transform_t
cast_transform(const physx::PxTransform& transform) {
    v3f origin{
        transform.p.x,
        transform.p.y,
        transform.p.z
    };
    quat orientation{};
    orientation.x = transform.q.x;
    orientation.y = transform.q.y;
    orientation.z = transform.q.z;
    orientation.w = transform.q.w;
    
    return math::transform_t{origin, orientation};
}

physx::PxTransform
cast_transform(v3f origin, quat orientation) {
    physx::PxVec3 p;
    p.x = origin.x;
    p.y = origin.y;
    p.z = origin.z;
    
    physx::PxQuat q{ orientation.x, orientation.y, orientation.z, orientation.w };

    return physx::PxTransform(p, q);
}

physx::PxTransform
cast_transform(const math::transform_t& transform) {
    physx::PxVec3 p;
    p.x = transform.origin.x;
    p.y = transform.origin.y;
    p.z = transform.origin.z;

    const auto temp_q = transform.get_orientation();
    physx::PxQuat q{ temp_q.x, temp_q.y, temp_q.z, temp_q.w };

    return physx::PxTransform(p, q);
}

class arena_heap_t : public physx::PxAllocatorCallback {
    std::mutex mut{};

public:
    ~arena_heap_t() {
        arena_clear(&allocator.arena);
    }

    utl::allocator_t allocator{};


    void* allocate(size_t size, const char* type_name, const char* file_name, int line) override final {
        TIMED_FUNCTION;
        std::lock_guard lock{mut};

        return allocator.allocate(size);
    }

    void deallocate(void* ptr) override final {
        TIMED_FUNCTION;
        std::lock_guard lock{mut};

        allocator.free(ptr);
    }
};

class error_callback_t : public physx::PxErrorCallback {
public:
    virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
    {
        ztd_error("physx", "PhysX Error Callback({}): {}:{} -\n\t{}\n", (u32)code, file, line, message);
    }
};

};