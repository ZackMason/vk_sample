#pragma once

// note(zack): this file is the sole interface between the app and the platform layer

#include <cstdint>
#include <cassert>
#include <cstring>

#include <string>

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
#define terabytes(x) (terabytes(x)*1024ull)
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
};

using app_func_t = void(__cdecl *)(app_memory_t*);

struct app_dll_t {
    void* dll{nullptr};
    app_func_t on_update{nullptr};
    app_func_t on_init{nullptr};
    app_func_t on_deinit{nullptr};
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
