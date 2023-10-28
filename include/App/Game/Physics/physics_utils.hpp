#pragma once

#include "foundation/PxAllocatorCallback.h"

#include <mutex>

#define PVD_HOST "127.0.0.1"
#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL; }

namespace physics {

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
    struct mem_block_t {
        std::byte*      data{nullptr};
        size_t          size{0};
        mem_block_t*    next{nullptr};
    };
    std::mutex mut{};
    mem_block_t* heap{nullptr};
    mem_block_t* empty_block{nullptr};

public:
    arena_t heap_arena{};
    arena_t arena{};
    
    arena_heap_t(size_t size) {
        arena = arena_create(size);        
        heap_arena = arena_create(kilobytes(128));
    }

private:
    size_t* get_size(std::byte* ptr) const noexcept {
        return &((size_t*)ptr)[-1];
    }

    void* allocate(size_t size, const char* type_name, const char* file_name, int line) override final {
        TIMED_FUNCTION;
        std::lock_guard lock{mut};

        // if (type_name) {
        //     zyy_info("px::alloc", "{}::allocate: {} bytes from {}:{}", type_name, size, file_name, line);
        // } else {
        //     zyy_info("px::alloc", "typeless::allocate: {} bytes from {}:{}", size, file_name, line);
        // }

        // todo(zack): search for freed blocks
        mem_block_t* prev{0};
        node_for(mem_block_t, heap, n) {
            if (n->size >= size) {
                if (prev) {
                    prev->next = n->next;
                } else { 
                    heap = n->next;
                }
                auto* temp_data = n->data;
                *n = {};
                node_push(n, empty_block);
                
                return temp_data;
            }
            prev = n;
        }

        arena.top = align16(arena.top);
        auto* ptr = push_bytes(&arena, size + sizeof(size_t)*2) + sizeof(size_t)*2;
        *get_size(ptr) = size;
        return ptr;
    }

    void deallocate(void* ptr) override final {
        TIMED_FUNCTION;
        std::lock_guard lock{mut};
        
        mem_block_t* block{};
        if (empty_block) {
            node_pop(block, empty_block);
        } else {
            tag_struct(block, mem_block_t, &heap_arena);
        }
        block->data = (std::byte*)ptr;
        block->size = *get_size(block->data); // need to find a way to get size......
        block->next = heap;
        heap = block;

        // zyy_info("px::free", "deallocate: {} bytes", block->size);
    }
};

class error_callback_t : public physx::PxErrorCallback {
public:
    virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
    {
        zyy_error("physx", "PhysX Error Callback({}): {}:{} -\n\t{}\n", (u32)code, file, line, message);
    }
};

};