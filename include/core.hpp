#pragma once

// note(zack): this file is the sole interface between the app and the platform layer

#include <cstdint>
#include <cassert>
#include <cstring>
#include <stack>

#include <string>

#include "Obj_Loader.hpp"

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/color.h>

///////////////////////////////////////////////////////////////////////////////
// Type Defs
///////////////////////////////////////////////////////////////////////////////
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include<glm/gtc/quaternion.hpp>

using v2f = glm::vec2;
using v3f = glm::vec3;
using v4f = glm::vec4;

using v2i = glm::ivec2;
using v3i = glm::ivec3;
using v4i = glm::ivec4;

using quat = glm::quat;
using m22 = glm::mat2x2;
using m33 = glm::mat3;
using m34 = glm::mat3x4;
using m43 = glm::mat4x3;
using m44 = glm::mat4x4;
///////////////////////////////////////////////////////////////////////////////

#define fmt_str(...) (fmt::format(__VA_ARGS__))
#define println(...) do { fmt::print(__VA_ARGS__); } while(0)
#define gen_info(cat, str, ...) do { fmt::print(fg(fmt::color::white) | fmt::emphasis::bold, fmt_str("[info][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define gen_warn(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::yellow) | fmt::emphasis::bold, fmt_str("[warn][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define gen_error(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::red) | fmt::emphasis::bold, fmt_str("[error][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)

#define global_variable static
#define local_persist static

#define no_mangle extern "C"
#define export_dll __declspec(dllexport)
#define export_fn(rt_type) no_mangle rt_type export_dll __cdecl

#define array_count(arr) (sizeof((arr)) / (sizeof((arr)[0])))
#define kilobytes(x) (x*1024ull)
#define megabytes(x) (kilobytes(x)*1024ull)
#define gigabytes(x) (megabytes(x)*1024ull)
#define terabytes(x) (gigabytes(x)*1024ull)
#define align16(val) ((val + 15) & ~15)
#define align4(val) ((val + 3) & ~3)

inline u32
safe_truncate_u64(u64 value) {
    assert(value <= 0xFFFFFFFF);
    const u32 result = static_cast<u32>(value);
    return result;
}

constexpr int 
BIT(int x) {
	return 1 << x;
}

struct app_input_t {
    // time
    f32 time;
    f32 dt;

    // keyboard
    u8 keys[512];

    // gamepad
    
    // mouse
    struct mouse_t {
        u8 buttons[12];
        i32 pos[2];
        i32 scroll[2];
    } mouse;
};

inline void
app_input_reset(app_input_t* input) {
    std::memset(input, 0, sizeof(app_input_t) - 4 * sizeof(i32));
}

struct app_config_t {
    i32 window_size[2];
    
    // vk info
    u32 vk_ext_count{};
    char** vk_exts{nullptr};
    void* window_handle{nullptr}; // note(zack): vulkan needs the os window handle to initialize

    // param 1: vk instance
    // param 2: vk surface
    using vk_create_surface_cb = void(*)(void*, void*, void*); 
    vk_create_surface_cb create_vk_surface{nullptr};
};

struct app_memory_t {
    void* perm_memory{nullptr};
    size_t perm_memory_size{};

    app_config_t config;
    app_input_t input;

    bool running{true};

    void* (__cdecl* malloc)(size_t);
    void* (__cdecl* realloc)(void*, size_t);
    void (__cdecl* free)(void*);
};

using app_func_t = void(__cdecl *)(app_memory_t*);

struct app_dll_t {
    void* dll{nullptr};
    app_func_t on_update{nullptr};
    app_func_t on_init{nullptr};
    app_func_t on_deinit{nullptr};
};

template <typename T>
struct node_t {
    union {
        T* left;
        T* prev;
    };
    union {
        T* right;
        T* next;
    };
    node_t() {
        right = left = nullptr;
    }
};


struct arena_t {
    u8* start{0};
    size_t top{0};
    size_t size{0};
};

using temp_arena_t = arena_t;

inline void
arena_clear(arena_t* arena) {
    arena->top = 0;
}

inline u8*
arena_get_top(arena_t* arena) {
    return arena->start + arena->top;
}

inline u8*
arena_alloc(arena_t* arena, size_t bytes) {
    u8* p_start = arena->start + arena->top;
    arena->top += bytes;

    assert(arena->top < arena->size && "Arena overflow");

    return p_start;
}

template <typename T, typename ... Args>
T* arena_emplace(arena_t* arena, size_t count, Args&& ... args) {
    T* t = reinterpret_cast<T*>(arena_alloc(arena, sizeof(T) * count));
    for (size_t i = 0; i < count; i++) {
        new (t + i) T(std::forward<Args>(args)...);
    }
    return t;
}

arena_t 
arena_sub_arena(arena_t* arena, size_t size) {
    arena_t res;
    res.start = arena_alloc(arena, size);
    res.size = size;
    res.top = 0;
    return res;
}

struct string_t {
    union {
        char* data;
        const char* c_data;
    };
    size_t size;
    bool owns = false;

    string_t()
        : data{nullptr}, size{0}
    {

    }

    const char* c_str() const {
        return data;
    }

    void view(const char* str) {
        size = std::strlen(str) + 1;
        c_data = str;
        owns = false;
    }
    
    void own(arena_t* arena, const char* str) {
        size = std::strlen(str) + 1;
        data = (char*)arena_alloc(arena, size);
        std::memcpy(data, str, size);
        owns = true;
    }
};

struct string_node_t : node_t<string_node_t> {
    string_t string;
};

#define node_next(node) (node = node->next)
#define node_push(node, list) do { node->next = list; list = node; } while(0)
#define node_pop(node, list) do { node = list; list = list->next; } while(0)

#define node_for(type, node, itr) for(type* itr = (node); (itr); node_next(itr))

namespace utl {

template <typename T>
struct deque {
private:
    T* head_{nullptr};
    T* tail_{nullptr};
    size_t size_{0};
public:

    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] bool is_empty() const noexcept {
        return head_ == nullptr;
    }

    const T* front() const noexcept {
        return head_;
    }

    const T* back() const noexcept {
        return tail_;
    }

    // returns null if failed
    T* remove(T* val) noexcept {
        if (is_empty()) {
            return nullptr;
        }
        if (front() == val) {
            return pop_front();
        } else if (back() == val) {
            return pop_back();
        }

        for (T* n{head_->next}; n; n->next) {
            if (n == val) {
                if (val->next) {
                    val->next->prev = val->prev;
                }
                if (val->prev) {
                    val->prev->next = val->next;
                }

                val->next = val->prev = nullptr;
                return val;
            }
        }
        return nullptr;
    }

    T* erase(size_t index) noexcept {
        return nullptr;
    }

    T* pop_front() noexcept {
        if (is_empty()) {
            return nullptr;
        }
        T* old = head_;
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        } else {
            head_->prev = nullptr;
        }
        return old;
    }

    T* pop_back() noexcept {
        if (is_empty()) {
            return nullptr;
        }
        T* old = tail_;
        tail_ = tail_->prev;
        if (tail_ == nullptr) {
            head_ = nullptr;
        } else {
            tail_->next = nullptr;
        }
        return old;
    }

    void push_back(T* val) {
        val->prev = tail_;
        val->next = nullptr;
        if (is_empty()) {
            head_ = tail_ = val;
        } else {
            tail_->next = val;
            tail_ = val;
        }
    }

    void push_front(T* val) {
        val->prev = nullptr;
        val->next = head_;
        if (is_empty()) {
            head_ = tail_ = val;
        } else {
            head_->next = val;
            head_ = val;
        }
    }
};

template <typename T>
struct free_list_t {
    utl::deque<T> deque;
    T* head{nullptr};
    T* first_free{nullptr};
    size_t min_stack_pos{~static_cast<size_t>(0)};
    size_t count{0};
    size_t free_count{0};

    template<typename allocator_t>
    T* alloc(arena_t* arena) {
        count += 1;
        if (first_free) { 
            T* node {nullptr};  
            node_pop(node, first_free);
            
            new (node) T();          

            deque.push_back(head);
            // node_push(node, head);

            free_count -= 1;
            return node;
        } else {
            // if (allocator.get_marker() < min_stack_pos) {
            //     min_stack_pos = allocator.get_marker();
            // }
            T* node = (T*)arena_alloc(arena, sizeof(T));// allocator.alloc(sizeof(T));
            new (node) T();
            
            // node_push(node, head);
            deque.push_back(node);

            return node;
        }
    }

    void free_index(size_t _i) noexcept {
        T* c = head;
        for (size_t i = 0; i < _i && c; i++, node_next(c)) {
        }
        if (c) {
            free(c);
        }
    }

    // calls T dtor
    void free(T*& o) noexcept {
        if (o == nullptr) { return; }

        T* n = deque.remove(o);
        if (n) {
            node_push(n, first_free);
            o->~T();
            o = nullptr;
            count -= 1;
            free_count += 1;
        }
        return;

        if (head == o) { // if node to free is first
            T* t{nullptr};
            // node_pop(t, head);

            node_push(t, first_free);
            o->~T();
            o = nullptr;
            count -= 1;
            free_count += 1;
            return;
        }
        T* c = head; // find node and free it, while keeping list connected
        while(c && c->next != o) {
            node_next(c);
        }
        if (c && c->next == o) {
            T* t = c->next;
            c->next = c->next->next;
            
            node_push(t, first_free);
            o->~T();
            o = nullptr;
            count -= 1;
            free_count += 1;
        }
    }
};

// note(zack): this is in core because it will probably be loaded 
// in a work thread in platform layer, so that we dont have to worry
// about halting the thread when we reload the dll

struct vertex_t {
    v3f pos;
    v3f nrm;
    v3f col;
    v2f tex;
};

struct parsed_mesh_t {
    vertex_t* vertices{nullptr};
    size_t vertex_count{0};

    u32* indices{nullptr};
    size_t index_count{0};

    string_t mesh_name;
    // string_t material_name; 
};

struct load_results_t {
    parsed_mesh_t* meshes;
    size_t count;
};

auto 
make_v3f(const auto& v) -> v3f {
    return {v.X, v.Y, v.Z};
}

load_results_t
load_mesh(arena_t* arena, std::string_view path) {
    objl::Loader loader;
    loader.LoadFile(fmt_str("{}", path).c_str());

    load_results_t results{};
    results.meshes = (parsed_mesh_t*)arena_alloc(arena,
        sizeof(parsed_mesh_t) * loader.LoadedMeshes.size());


    for (const auto& lmesh: loader.LoadedMeshes) {
        auto& mesh = results.meshes[results.count++];
        mesh.vertices = (vertex_t*)arena_get_top(arena);
        mesh.vertex_count = lmesh.Vertices.size();
        for (const auto& vert: lmesh.Vertices) {
            vertex_t v;
            v.pos = make_v3f(vert.Position);
            v.nrm = make_v3f(vert.Normal);
            v.tex = v2f(vert.TextureCoordinate.X, vert.TextureCoordinate.Y);
            *(vertex_t*)arena_alloc(arena, sizeof(vertex_t)) = v;
        }
        
        mesh.indices = (u32*)arena_get_top(arena);
        mesh.index_count = lmesh.Indices.size();
        for (u32 idx: lmesh.Indices) {
            *(u32*)arena_alloc(arena, sizeof(u32)) = idx;
        }
    }

    return results;
}

};