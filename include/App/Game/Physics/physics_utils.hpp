#pragma once


#include "foundation/PxAllocatorCallback.h"

#include <mutex>

#define PVD_HOST "127.0.0.1"
#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL; }

namespace game::phys {

class arena_heap_t : public physx::PxAllocatorCallback {
    struct mem_block_t {
        std::byte*      data{nullptr};
        size_t          size{0};
        mem_block_t*    next{nullptr};
    };

    std::mutex mut{};
    arena_t arena{};
    arena_t heap_arena{};
    mem_block_t* heap{nullptr};
    mem_block_t* empty_block{nullptr};

public:
    arena_heap_t(arena_t& parent_arena, size_t size) {
        arena = arena_sub_arena(&parent_arena, size);
        arena.start = (std::byte*)align16((std::uintptr_t)arena.start);
        heap_arena = arena_sub_arena(&arena, kilobytes(32));
    }

private:
    size_t* get_size(std::byte* ptr) const noexcept {
        return &((size_t*)ptr)[-1];
    }

    void* allocate(size_t size, const char* type_name, const char* file_name, int line) override final {
        std::lock_guard lock{mut};

        if (type_name) {
            gen_info("px::alloc", "{}::allocate: {} bytes from {}:{}", type_name, size, file_name, line);
        } else {
            gen_info("px::alloc", "typeless::allocate: {} bytes from {}:{}", size, file_name, line);
        }

        // todo(zack): search for freed blocks
        mem_block_t* prev{0};
        node_for(mem_block_t, heap, n) {
            if (n->size >= size && size >= (n->size << 1)) {
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
        auto* ptr = arena_alloc(&arena, size + sizeof(size_t)*2) + sizeof(size_t)*2;
        *get_size(ptr) = size;
        return ptr;
    }

    void deallocate(void* ptr) override final {
        std::lock_guard lock{mut};
        
        mem_block_t* block{};
        if (empty_block) {
            node_pop(block, empty_block);
        } else {
            block = arena_alloc_ctor<mem_block_t>(&heap_arena);
        }
        block->data = (std::byte*)ptr;
        block->size = *get_size(block->data); // need to find a way to get size......
        block->next = heap;
        heap = block;

        gen_info("px::free", "deallocate: {} bytes", block->size);
    }
};

class error_callback_t : public physx::PxErrorCallback {
public:
    virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file,
        int line)
    {
        gen_error("physx", "PhysX Error Callback: {}:{} -\n\t{}\n", file, line, message);
    }
};

};