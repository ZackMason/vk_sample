#pragma once

// note(zack): this file is the sole interface between the app and the platform layer

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/color.h>

#define fmt_str(...) (fmt::format(__VA_ARGS__))
#define fmt_sv(...) (std::string_view{fmt::format(__VA_ARGS__)})
#define println(...) do { fmt::print(__VA_ARGS__); } while(0)
#define gen_info(cat, str, ...) do { fmt::print(fg(fmt::color::white) | fmt::emphasis::bold, fmt_str("[info][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define gen_warn(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::yellow) | fmt::emphasis::bold, fmt_str("[warn][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define gen_error(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::red) | fmt::emphasis::bold, fmt_str("[error][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define gen_profile(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::blue) | fmt::emphasis::bold, fmt_str("[profile][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)

#if defined(GAME_USE_SIMD)

#if defined(_WIN64) || defined(_WIN32)
#include <immintrin.h>
#else
#error "No Unix SIMD yet"
#endif

#endif

#define WIN32_MEAN_AND_LEAN
#define NOMINMAX

#if defined(RAND_GETPID)

#elif defined(_WIN64) || defined(_WIN32)
    #include <process.h>
    
    #define RAND_GETPID _getpid()
    
    #undef DELETE
    #undef min
    #undef max
    #undef near
    #undef far
    


#elif defined(__unix__) || defined(__unix) \
      || (defined(__APPLE__) && defined(__MACH__))
    #include <unistd.h>
    #define RAND_GETPID getpid()
#else
    #define RAND_GETPID 0
#endif

#include <cstdint>
#include <cassert>
#include <cstring>
#include <span>
#include <array>
#include <fstream>
#include <chrono>
#include <functional>

#include <string>
#include <string_view>

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

#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

// #include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/color_space.hpp>

using v2f = glm::vec2;
using v3f = glm::vec3;
using v4f = glm::vec4;

using v2i = glm::ivec2;
using v3i = glm::ivec3;
using v4i = glm::ivec4;

using quat = glm::quat;
using m22 = glm::mat2x2;
using m33 = glm::mat3x3;
using m34 = glm::mat3x4;
using m43 = glm::mat4x3;
using m44 = glm::mat4x4;

namespace axis {
    constexpr inline static v3f up          {0.0f, 1.0f, 0.0f};
    constexpr inline static v3f down        {0.0f,-1.0f, 0.0f};
    constexpr inline static v3f right       {1.0f, 0.0f, 0.0f};
    constexpr inline static v3f left        {-1.0f,0.0f, 0.0f};
    constexpr inline static v3f forward     {0.0f, 0.0f,-1.0f};
    constexpr inline static v3f backward    {0.0f, 0.0f, 1.0f};
};

struct arena_t {
    std::byte* start{0};
    size_t top{0};
    size_t size{0};
};

struct frame_arena_t {
    arena_t arena[2];
    u32     active{0};
};

using temp_arena_t = arena_t;

///////////////////////////////////////////////////////////////////////////////

#define global_variable static
#define local_persist static

#define case_invalid_default default: assert(!"Invalid Default Case!"); break
#define no_mangle extern "C"
#define export_dll __declspec(dllexport)
#define export_fn(rt_type) no_mangle export_dll rt_type __cdecl

struct _auto_deferer_t {
    std::function<void(void)> fn;

    template<typename Fn>
    _auto_deferer_t(Fn _fn) 
        : fn{_fn}
    {
    }

    template<typename Fn>
    _auto_deferer_t& operator=(Fn _fn) {
        fn = _fn;
        return *this;
    }

    ~_auto_deferer_t() {
        fn();
    }
};


#define VAR_CAT(a,b) a ## b
#define VAR_CAT_(a,b) VAR_CAT(a, b)
#define defer _auto_deferer_t VAR_CAT_(_res_auto_defer_, __LINE__) = [&]()


#define array_count(arr) (sizeof((arr)) / (sizeof((arr)[0])))
#define kilobytes(x) (x*1024ull)
#define megabytes(x) (kilobytes(x)*1024ull)
#define gigabytes(x) (megabytes(x)*1024ull)
#define terabytes(x) (gigabytes(x)*1024ull)
#define align16(val) ((val + 15) & ~15)
#define align4(val) ((val + 3) & ~3)


#define to_secs(x) (f32(x/1000))

struct coroutine_t {
	f32& now;
	u64 line;
	f32 start_time;
	void* data;
};

// #define co_lerp(coro, val, start, end, duration) do { if (coro->start_time == 0) { coro->start_time = coro->now; } val = glm::mix(start, end, (coro->now - coro->start_time)/duration); co_yield_until(coro,  coro->now > (coro->start_time) + (f32)duration); coro->start_time = 0; } while(0)
#define co_begin(coro) switch (coro->line) {case 0: coro->line = 0;
#define co_yield(coro) do { coro->line = __LINE__; return; case __LINE__:;} while(0)
#define co_yield_until(coro, condition) while (!(condition)) { co_yield(coro); }
#define co_wait(coro, duration) do {if (coro->start_time == 0) { coro->start_time = coro->now; } co_yield_until(coro, coro->now > (coro->start_time) + (f32)duration); coro->start_time = 0; } while (0)
#define co_end(coro) do { coro->line = __LINE__; } while (0); }
#define co_reset(coro) do { coro->line = 0; } while (0); }
#define co_set_label(coro, label) case label:
#define co_goto_label(coro, label) do { coro->line = (label); return; } while(0)
// #define co_is_finished(coro) (coro->line == -1)
#define co_lerp(coro, value, start, end, duration, func) \
do { \
    coro->start_time = coro->now;\
    while (coro->now < coro->start_time + (duration)) { \
        (value) = (func)((start), (end), ((coro->now - coro->start_time)/(duration))); \
        co_yield(coro); \
    } \
    (value) = (end); \
} while (0)

#define loop_iota_u64(itr, stop) for (size_t itr = 0; itr < stop; itr++)
#define range_u32(itr, start, stop) for (u32 itr = (start); itr < (stop); itr++)
#define range_u64(itr, start, stop) for (size_t itr = (start); itr < (stop); itr++)
#define loop_iota_i32(itr, stop) for (int itr = 0; itr < stop; itr++)

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

namespace mouse_button_id {
    enum {
        left,
        right,
        middle,
        extra_0,
        extra_1,
        extra_2,
        extra_3,
        extra_4,
        SIZE,
    };
};
namespace key_id { enum {
    SPACE =32,
    APOSTROPHE =39, /* ' */
    COMMA =44, /* , */
    MINUS =45, /* - */
    PERIOD =46, /* . */
    SLASH =47, /* / */
    NUM_0 =48,
    NUM_1 =49,
    NUM_2 =50,
    NUM_3 =51,
    NUM_4 =52,
    NUM_5 =53,
    NUM_6 =54,
    NUM_7 =55,
    NUM_8 =56,
    NUM_9 =57,
    SEMICOLON =59, /* ; */
    EQUAL =61, /* = */
    A =65,
    B =66,
    C =67,
    D =68,
    E =69,
    F =70,
    G =71,
    H =72,
    I =73,
    J =74,
    K =75,
    L =76,
    M =77,
    N =78,
    O =79,
    P =80,
    Q =81,
    R =82,
    S =83,
    T =84,
    U =85,
    V =86,
    W =87,
    X =88,
    Y =89,
    Z =90,
    LEFT_BRACKET =91, /* [ */
    BACKSLASH =92, /* \ */
    RIGHT_BRACKET =93, /* ] */
    GRAVE_ACCENT =96, /* ` */
    WORLD_1 =161, /* non-US #1 */
    WORLD_2 =162, /* non-US #2 */
    ESCAPE =256,
    ENTER =257,
    TAB =258,
    BACKSPACE =259,
    INSERT =260,
    DELETE =261,
    RIGHT =262,
    LEFT =263,
    DOWN =264,
    UP =265,
    PAGE_UP =266,
    PAGE_DOWN =267,
    HOME =268,
    END =269,
    CAPS_LOCK =280,
    SCROLL_LOCK =281,
    NUM_LOCK =282,
    PRINT_SCREEN =283,
    PAUSE =284,
    F1 =290,
    F2 =291,
    F3 =292,
    F4 =293,
    F5 =294,
    F6 =295,
    F7 =296,
    F8 =297,
    F9 =298,
    F10 =299,
    F11 =300,
    F12 =301,
    F13 =302,
    F14 =303,
    F15 =304,
    F16 =305,
    F17 =306,
    F18 =307,
    F19 =308,
    F20 =309,
    F21 =310,
    F22 =311,
    F23 =312,
    F24 =313,
    F25 =314,
    KP_0 =320,
    KP_1 =321,
    KP_2 =322,
    KP_3 =323,
    KP_4 =324,
    KP_5 =325,
    KP_6 =326,
    KP_8 =328,
    KP_9 =329,
    KP_DECIMAL =330,
    KP_DIVIDE =331,
    KP_MULTIPLY =332,
    KP_SUBTRACT =333,
    KP_ADD =334,
    KP_ENTER =335,
    KP_EQUAL =336,
    LEFT_SHIFT =340,
    LEFT_CONTROL =341,
    LEFT_ALT =342,
    LEFT_SUPER =343,
    RIGHT_SHIFT =344,
    RIGHT_CONTROL =345,
    RIGHT_ALT =346,
    RIGHT_SUPER =347,
    MENU = 348,
    SIZE
};};

namespace button_id {
    enum {
        action_left,
        action_down,
        action_right,
        action_up,
        shoulder_left,
        shoulder_right,
        options,
        start,
        left_thumb,
        right_thumb,
        guide,
        dpad_left,
        dpad_down,
        dpad_right,
        dpad_up,
        SIZE
    };
};

struct gamepad_button_state_t {
	i8 is_held;
	i8 is_pressed;
	i8 is_released;
};

struct gamepad_t {
	i32 is_connected{false};
	i32 is_analog{true};

	v2f left_stick{0.0f};
	v2f right_stick{0.0f};

    f32 left_trigger{0.0f};
    f32 right_trigger{0.0f};

    gamepad_button_state_t buttons[button_id::SIZE];
	
};

struct app_input_t {
    // time
    f32 time;
    f32 dt;

    // keyboard
    u8 keys[512];

    struct pressed_t {
        u8 keys[512];
        u8 mouse_btns[12];
    } pressed;
    
    struct released_t {
        u8 keys[512];
        u8 mouse_btns[12];
    } released;

    // gamepad
    gamepad_t gamepads[2];
    
    // mouse
    struct mouse_t {
        u8 buttons[12];
        f32 pos[2];
        f32 delta[2];
        i32 scroll[2];
    } mouse;

};

inline void
app_input_reset(app_input_t* input) {
    std::memset(input, 0, sizeof(app_input_t) - 4 * sizeof(i32));
}

struct app_config_t {
    struct graphic_config_t {
        v2i     window_size{640, 480};
        bool    vsync{false};
        bool    fullscreen{false};
        u8      msaa{0};
    } graphics_config;

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

struct audio_settings_t {
    u8 channels{100};
    u8 master_volumn{100};
    u8 sfx_volumn{100};
    u8 music_volumn{100};

    u8 mute{0};
};

namespace utl {
    struct closure_t;
};


#if GEN_INTERNAL

inline u64
get_nano_time() {
    std::chrono::time_point<std::chrono::high_resolution_clock> time_stamp = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::nanoseconds>(time_stamp.time_since_epoch()).count();
}

struct debug_record_t {
    const char* file_name;
    const char* func_name;
    int         line_num;
    u64         cycle_count{0};
    u64         hit_count{0};

    u16         hist_id{0};
    u64         history[4096];
};

enum debug_event_type {
    DebugEventType_BeginBlock,
    DebugEventType_EndBlock,
    DebugEventType_BeginFrame,
};

struct debug_event_t {
    u64         clock_count{0};
    u64         thread_id{0};
    u16         core_index{0};
    u16         record_index{0};
    u32         type{0};
};

#define MAX_DEBUG_EVENT_COUNT (16*65536)
#define MAX_DEBUG_RECORD_COUNT (256)

#include <atomic>
struct debug_table_t {
    std::atomic<u64> event_index{0};
    debug_event_t   events[MAX_DEBUG_EVENT_COUNT];

    u64             record_count{0};
    debug_record_t  records[MAX_DEBUG_RECORD_COUNT];
};

extern debug_table_t gs_debug_table;

inline void
record_debug_event(int record_index, debug_event_type event_type) {
    u64 index = gs_debug_table.event_index;
    assert(index < MAX_DEBUG_EVENT_COUNT);
    debug_event_t* event = gs_debug_table.events + index;
    event->clock_count = get_nano_time();
    event->core_index = 0;
    event->thread_id = 0;
    event->type = event_type;
    event->record_index = (u16)record_index;
}

extern size_t gs_main_debug_record_size;

#define TIMED_FUNCTION timed_block_t VAR_CAT_(_TimedBlock_, __LINE__)(__COUNTER__, __FILE__, __LINE__, __FUNCTION__)
#define TIMED_BLOCK(Name) timed_block_t VAR_CAT_(_TimedBlock_, __LINE__)(__COUNTER__, __FILE__, __LINE__, #Name)

struct timed_block_t {
    debug_record_t* record;
    i32 counter;
    timed_block_t(i32 _counter, const char* _file_name, i32 _line_number, const char* _function_name) {
        counter = _counter;
        record = gs_debug_table.records + _counter;
        record->file_name = _file_name;
        record->line_num = _line_number;
        record->func_name = _function_name;

        record_debug_event(_counter, DebugEventType_BeginBlock);

        record->cycle_count = get_nano_time();
        record->hit_count++;
    }

    ~timed_block_t() {
        record->history[record->hist_id] = record->cycle_count = get_nano_time() - record->cycle_count;
        record->hist_id = (record->hist_id + 1) % array_count(record->history);

        record_debug_event(counter, DebugEventType_BeginBlock);
    }
};

#else 

#define TIMED_BLOCK
#define TIMED_FUNCTION

#endif

#include "app_physics.hpp"

struct app_memory_t {
    void* perm_memory{nullptr};
    size_t perm_memory_size{};

    app_config_t config;
    app_input_t input;

    bool running{true};

    // void* (__cdecl* malloc)(size_t);
    // void* (__cdecl* realloc)(void*, size_t);
    // void (__cdecl* free)(void*);

    struct audio_closures_t {
        utl::closure_t* load_sound_closure;
        utl::closure_t* play_sound_closure;
        utl::closure_t* stop_sound_closure;
    } audio;

    physics::api_t* physics{nullptr};
};

using app_func_t = void(__cdecl *)(app_memory_t*);

struct app_dll_t {
#if _WIN32
    void* dll{nullptr};
#else
#error "no linux"
#endif
    app_func_t on_update{nullptr};
    app_func_t on_render{nullptr};
    app_func_t on_init{nullptr};
    app_func_t on_deinit{nullptr};
    app_func_t on_unload{nullptr};
    app_func_t on_reload{nullptr};
};

// nodes

#define node_next(node) (node = node->next)
#define node_push(node, list) do { node->next = list; list = node; } while(0)
#define node_pop(node, list) do { node = list; list = list->next; } while(0)

#define node_for(type, node, itr) for(type* itr = (node); (itr); node_next(itr))

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
    node_t() noexcept {
        right = left = nullptr;
    }
    virtual ~node_t() = default;
};

// allocators


inline arena_t
arena_create(void* p, size_t bytes) noexcept {
    arena_t arena;
    arena.start = (std::byte*)p;
    arena.size = bytes;
    return arena;
}

inline size_t
arena_get_remaining(arena_t* arena) noexcept {
    return arena->size - arena->top;
}

inline size_t
arena_get_mark(arena_t* arena) noexcept {
    return arena->top;
}

inline void
arena_set_mark(arena_t* arena, size_t m) noexcept {
    arena->top = m;
}

inline void
arena_clear(arena_t* arena) {
    arena->top = 0;
}

inline std::byte*
arena_get_top(arena_t* arena) {
    return arena->start + arena->top;
}

#define arena_display_info(arena, name) \
    (fmt_sv("{}: {} {} / {} {} - {:.2f}%", \
    (name), \
    (arena)->top > gigabytes(1) ? (arena)->top/gigabytes(1) : (arena)->top > megabytes(1) ? (arena)->top/megabytes(1) : (arena)->top > kilobytes(1) ? (arena)->top / kilobytes(1) : (arena)->top,\
    (arena)->top > gigabytes(1) ? "Gb" : (arena)->top > megabytes(1) ? "Mb" : (arena)->top > kilobytes(1) ? "Kb" : "B",\
    (arena)->size > gigabytes(1) ? (arena)->size/gigabytes(1) : (arena)->size > megabytes(1) ? (arena)->size/megabytes(1) : (arena)->size > kilobytes(1) ? (arena)->size / kilobytes(1) : (arena)->size,\
    (arena)->size > gigabytes(1) ? "Gb" : (arena)->size > megabytes(1) ? "Mb" : (arena)->size > kilobytes(1) ? "Kb" : "B",\
    (f32((arena)->top) / f32((arena)->size) * 100.0f)))
    
template <typename T>
inline T*
arena_alloc(arena_t* arena) {
    std::byte* p_start = arena->start + arena->top;
    arena->top += sizeof(T);

    assert(arena->top <= arena->size && "Arena overflow");

    return (T*)p_start;
}

inline std::byte*
arena_alloc(arena_t* arena, size_t bytes) {
    std::byte* p_start = arena->start + arena->top;
    arena->top += bytes;

    assert(arena->top <= arena->size && "Arena overflow");

    return p_start;
}

template <typename T>
inline T*
arena_alloc_ctor(arena_t* arena, size_t count = 1) {
    T* t = reinterpret_cast<T*>(arena_alloc(arena, sizeof(T) * count));
    for (size_t i = 0; i < count; i++) {
        new (t + i) T();
    }
    return t;
}

template <typename T, typename ... Args>
inline T*
arena_alloc_ctor(arena_t* arena, size_t count, Args&& ... args) {
    T* t = reinterpret_cast<T*>(arena_alloc(arena, sizeof(T) * count));
    for (size_t i = 0; i < count; i++) {
        new (t + i) T(std::forward<Args>(args)...);
    }
    return t;
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

inline frame_arena_t
arena_create_frame_arena(
    arena_t* arena, 
    size_t bytes
) {
    frame_arena_t frame;
    frame.arena[0].start = arena_alloc(arena, bytes);
    frame.arena[1].start = arena_alloc(arena, bytes);
    frame.arena[0].size = bytes;
    frame.arena[1].size = bytes;
    return frame;
}

namespace utl {

struct closure_t {
    void (*func)(void*){0};
    void* data{0};

    template <typename ... Args>
    void dispatch(Args ... args) {
        ((void(*)(void*, Args...))func)(data, args...);// std::forward<Args>(args)...);
    }

    template <typename ReturnType, typename ... Args>
    ReturnType dispatch_request(Args&& ... args) {
        return ((ReturnType(*)(void*, Args...))func)(data, std::forward<Args>(args)...);
    }
};

template <typename T>
struct pool_t : public arena_t {
    size_t count{0};
    size_t capacity{0};

    pool_t() = default;

    explicit pool_t(T* data, size_t _cap) 
        : capacity{_cap}
    {
        start = (std::byte*)data;
        size = _cap * sizeof(T);
    }

    pool_t(arena_t* arena, size_t _cap)
        : capacity{_cap}
    {
        auto sub_arena = arena_sub_arena(arena, _cap*sizeof(T));
        start = sub_arena.start;
        size = sub_arena.size;
    }

    T& operator[](size_t id) {
        return data()[id];
    }

    T& back() {
        return operator[](count-1);
    }

    T& front() {
        return operator[](0);
    }

    T* data() {
        return (T*)start;
    }

    T* create(size_t p_count) {
        T* o = allocate(p_count);
        for (size_t i = 0; i < p_count; i++) {
            new (o+i) T();
        }
        return o;
    }

    T* allocate(size_t p_count) {
        count += p_count;
        return (T*)arena_alloc(this, sizeof(T) * p_count);
    }

    void free(T* ptr) {
        ptr->~T();
        std::swap(*ptr, back());
        count--;
    }

    // note(zack): calls T dtor
    void clear() {
        if constexpr (std::is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < count; i++) {
                (data() + i)->~T();
            }
        }
        arena_clear(this);
        count = 0;
    }    
};

}; // namespace utl


// string

struct string_t {
    union {
        char* data;
        const char* c_data;
    };
    size_t size;
    bool owns = false;

    constexpr string_t(std::string_view sv)
        : c_data{sv.data()}, size{sv.size()}
    {
    }

    template <size_t N>
    constexpr string_t(char const (&text)[N])
        : string_t{std::string_view{text}}
    {
    }

    constexpr string_t()
        : c_data{nullptr}, size{0}
    {
    }

    operator std::string_view() const noexcept {
        return std::string_view{c_data, size};
    }
    std::string_view sv() const noexcept {
        return std::string_view{c_data, size};
    }

    const char* c_str() const {
        return c_data;
    }

    string_t& view(std::string_view str) {
        size = str.size();
        c_data = str.data();
        owns = false;
        return *this;
    }
    
    string_t& own(arena_t* arena, std::string_view str) {
        size = str.size();
        data = (char*)arena_alloc(arena, size+1);
        std::memcpy(data, str.data(), size);
        data[size] = 0;
        owns = true;
        return *this;
    }

    [[nodiscard]] bool operator==(std::string_view str) {
        return std::string_view{*this} == str;
    }
};

struct string_node_t : node_t<string_node_t> {
    string_t string;
};

using string_id_t = u64;
using sid_t = string_id_t;


// https://stackoverflow.com/questions/48896142/is-it-possible-to-get-hash-values-as-compile-time-constants
template <typename Str>
static constexpr string_id_t sid(const Str& toHash) {
    static_assert(sizeof(string_id_t) == 8, "string hash not u64");
	string_id_t result = 0xcbf29ce484222325; // FNV offset basis

	for (char c : toHash) {
		result ^= c;
		result *= 1099511628211; // FNV prime
	}

	return result;
}

template <size_t N>
static constexpr string_id_t sid(char const (&toHash)[N]) {
	return sid(std::string_view(toHash));
}

constexpr string_id_t operator"" _sid(const char* str, std::size_t size) {
    return sid(std::string_view(str));
}

struct type_info_t {
    sid_t       type_id{};
    size_t      size{};
    const char* name{};
};

#define GEN_TYPE_ID(type) (sid(#type))
#define GEN_TYPE_INFO(type) (type_info_t{sid(#type), sizeof(type), #type})
#define GEN_REFLECT_VAR(type, var) size_t offset{offsetof(type, var)}; const char* name{#var};
#define GEN_REFLECT_INFO(type, var, ...) struct VAR_CAT_(type, VAR_CAT(var, _info)) { GEN_REFLECT_VAR(type, var) }

struct any_t {
    enum struct type_tag {
        UINT8,
        UINT16,
        UINT32,
        UINT64,
        INT8,
        INT16,
        INT32,
        INT64,
        FLOAT32,
        FLOAT64,
        STRING,
        VOID_STAR,
        TYPED_PTR,
        UNKNOWN,
    };

    type_tag tag{type_tag::UNKNOWN};

    #define ANY_T_RETURN_TYPE(type) if constexpr (std::is_same_v<T, type>) {return data.type; }

    template <typename T>
    const T& get() const noexcept {
        ANY_T_RETURN_TYPE(u8 );
        ANY_T_RETURN_TYPE(u16);
        ANY_T_RETURN_TYPE(u32);
        ANY_T_RETURN_TYPE(u64);
        ANY_T_RETURN_TYPE(i8 );
        ANY_T_RETURN_TYPE(i16);
        ANY_T_RETURN_TYPE(i32);
        ANY_T_RETURN_TYPE(i64);
        ANY_T_RETURN_TYPE(f32);
        ANY_T_RETURN_TYPE(f64);
        assert(0 && "any_t :: Unknown type");
    }

    union {
        u8 u8;
        u16 u16;
        u32 u32;
        u64 u64;
        i8 i8;
        i16 i16;
        i32 i32;
        i64 i64;
        f32 f32;
        f64 f64;
    } data;


};



namespace math {
    constexpr v3f get_spherical(f32 yaw, f32 pitch) {
        return v3f{
            glm::cos((yaw)) * glm::cos((pitch)),
            glm::sin((pitch)),
            glm::sin((yaw)) * glm::cos((pitch))
        };
    }

    v3f world_to_screen(const m44& vp, const v4f& p) noexcept {
        v4f sp = vp * p;
        sp /= sp.w;
        sp.x = sp.x * 0.5f + 0.5f;
        sp.y = sp.y * 0.5f + 0.5f;
        return sp;
    }

    v3f world_to_screen(const m44& vp, const v3f& p) noexcept {
        return world_to_screen(vp, v4f{p, 1.0f});
    }

    namespace constants {
        inline static constexpr auto pi32  = glm::pi<f32>();
        inline static constexpr auto tau32 = glm::pi<f32>()*2.0f;
        inline static constexpr auto e32   = 2.71828182845904523536f;

    }
    using perc_tau_t = f32;

    constexpr f32 to_radians(perc_tau_t p) noexcept {
        return p*constants::tau32;
    }
    constexpr f32 to_degrees(perc_tau_t p) noexcept {
        return p * 360.0f;
    }

    struct triangle_t {
        std::array<v3f,3> p;
    };

    struct plane_t {
        v3f n;
        f32 d;
    };

    struct sphere_t {
        v3f origin;
        f32 radius;
    };
    struct circle_t {
        v2f origin;
        f32 radius;
    };

    bool intersect(circle_t c, v2f p) {
        return glm::distance2(c.origin, p) < c.radius * c.radius;
    }

    bool intersect(sphere_t a, v3f b) {
        return glm::distance2(a.origin, b) < a.radius * a.radius;
    }

    struct frustum_t {
        v3f points[8];
    };

    // template <typename T>
    struct hit_result_t {
        bool intersect{false};
        f32 distance{0.0f};
        v3f position{0.f, 0.f, 0.f};
        // T& object;

        operator bool() const {
            return intersect;
        }
    };

    struct ray_t {
        v3f origin;
        v3f direction;
    };
    
template <typename T = v3f>
struct aabb_t {
    aabb_t() = default;
    aabb_t(const T& o) {
        expand(o);
    }

    aabb_t(const T& min_, const T& max_) noexcept
        : min{min_}, max{max_}
    {
    }

    T min{std::numeric_limits<float>::max()};
    T max{-std::numeric_limits<float>::max()};

    T size() const {
        return max - min;
    }

    T center() const {
        return (max+min)/2.0f;
    }

    void expand(const aabb_t<T>& o) {
        expand(o.min);
        expand(o.max);
    }

    void expand(const T& p) {
        min = glm::min(p, min);
        max = glm::max(p, max);
    }

    bool intersect(const aabb_t<v3f>& o) const {
        return  this->min.x <= o.max.x &&
                this->max.x >= o.min.x &&
                this->min.y <= o.max.y &&
                this->max.y >= o.min.y &&
                this->min.z <= o.max.z &&
                this->max.z >= o.min.z;
    }

    bool contains(const aabb_t<T>& o) const {
        return contains(o.min) && contains(o.max);
    }
    bool contains(const f32& p) const {
        return 
            min <= p &&
            p <= max;            
    }
    bool contains(const v2f& p) const {
        return 
            min.x <= p.x &&
            min.y <= p.y &&
            p.x <= max.x &&
            p.y <= max.y;
    }
    bool contains(const v3f& p) const {
        //static_assert(std::is_same<T, v3f>::value);
        // return min <= p && max <= p;
        return 
            min.x <= p.x &&
            min.y <= p.y &&
            min.z <= p.z &&
            p.x <= max.x &&
            p.y <= max.y &&
            p.z <= max.z;
    }

    // bool contains(const v2f& p) const {
    //     //static_assert(std::is_same<T, v3f>::value);
    
    //     return 
    //         min.x <= p.x &&
    //         min.y <= p.y &&
    //         p.x <= max.x &&
    //         p.y <= max.y;
    // }

    // bool contains(const f32 p) const {
    //     static_assert(std::is_same<T, f32>::value);
    //     return min <= p && p <= max;
    // }
};

inline hit_result_t          //<u64> 
ray_aabb_intersect(const ray_t& ray, const aabb_t<v3f>& aabb) {
    const v3f inv_dir = 1.0f / ray.direction;

    const v3f min_v = (aabb.min - ray.origin) * inv_dir;
    const v3f max_v = (aabb.max - ray.origin) * inv_dir;

    const f32 tmin = std::max(std::max(std::min(min_v.x, max_v.x), std::min(min_v.y, max_v.y)), std::min(min_v.z, max_v.z));
    const f32 tmax = std::min(std::min(std::max(min_v.x, max_v.x), std::max(min_v.y, max_v.y)), std::max(min_v.z, max_v.z));

    hit_result_t hit;
    hit.intersect = false;

    if (tmax < 0.0f || tmin > tmax) {
        return hit;
    }

    if (tmin < 0.0f) {
        hit.distance = tmax;
    } else {
        hit.distance = tmin;
    }
    hit.intersect = true;
    hit.position = ray.origin + ray.direction * hit.distance;
    return hit;
}

struct transform_t {
	constexpr transform_t(const m44& mat = m44{1.0f}) : basis(mat), origin(mat[3]) {};
	constexpr transform_t(const m33& _basis, const v3f& _origin) : basis(_basis), origin(_origin) {};
	constexpr transform_t(const v3f& position, const v3f& scale = {1,1,1}, const v3f& rotation = {0,0,0})
	    : basis(m33(1.0f)
    ) {
		origin = (position);
		set_rotation(rotation);
		set_scale(scale);
	}
    
	m44 to_matrix() const {
        m44 res = basis;
        res[3] = v4f(origin,1.0f);
        return res;
    }
    
	operator m44() const {
		return to_matrix();
	}
    f32 get_basis_magnitude(u32 i) {
        assert(i < 3);
        return glm::length(basis[i]);
    }
	void translate(const v3f& delta) {
        origin += delta;
    }
	void scale(const v3f& delta) {
        basis = glm::scale(m44{basis}, delta);
    }
	void rotate(const v3f& axis, f32 rads) {
        set_rotation((get_orientation() * glm::angleAxis(rads, axis)));

        // m44 rot = m44(1.0f);
        // rot = glm::rotate(rot, delta.z, { 0,0,1 });
        // rot = glm::rotate(rot, delta.y, { 0,1,0 });
        // rot = glm::rotate(rot, delta.x, { 1,0,0 });
        // basis = (basis) * m33(rot);
    }
	void rotate_quat(const glm::quat& delta) {
        set_rotation(get_orientation() * delta);
    }
    
    glm::quat get_orientation() const {
        return glm::normalize(glm::quat_cast(basis));
    }

	transform_t inverse() const {
        transform_t transform;
        transform.basis = glm::transpose(basis);
        transform.origin = basis * -origin;
        return transform;
    }

	void look_at(const v3f& target, const v3f& up = { 0,1,0 }) {
        auto mat = glm::lookAt(origin, target, up);
        basis = mat;
        origin = mat[3];
    }
    constexpr void set_scale(const v3f& scale) {
        for (int i = 0; i < 3; i++) {
            basis[i] = glm::normalize(basis[i]) * scale[i];
        }
    }
	constexpr void set_rotation(const v3f& rotation) {
        f32 scales[3];
        range_u32(i, 0, 3) {
            scales[i] = glm::length(basis[i]);
        }
        basis = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
        range_u32(i, 0, 3) {
            basis[i] *= scales[i];
        }
    }
	void set_rotation(const glm::quat& quat) {
        basis = glm::toMat3(glm::normalize(quat));
    }

    void normalize() {
        for (decltype(basis)::length_type i = 0; i < 3; i++) {
            basis[i] = glm::normalize(basis[i]);
        }
    }

	// in radians
	v3f get_euler_rotation() const {
        return glm::eulerAngles(get_orientation());
    }

	transform_t affine_invert() const {
        auto t = *this;
		t.basis = glm::inverse(t.basis);
		t.origin = t.basis * -t.origin;
        return t;
	}

	v3f inv_xform(const v3f& vector) const
	{
		const v3f v = vector - origin;
		return glm::transpose(basis) * v;
	}

	v3f xform(const v3f& vector) const {
		return v3f(basis * vector) + origin;
	}
    
	ray_t xform(const ray_t& ray) const { 
		return ray_t{
			xform(ray.origin),
			glm::normalize(basis * ray.direction)
		};
	}

	aabb_t<v3f> xform_aabb(const aabb_t<v3f>& box) const {
		aabb_t<v3f> t_box;
		t_box.min = t_box.max = origin;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
                const auto a = basis[j][i] * box.min[j];
                const auto b = basis[j][i] * box.max[j];
                t_box.min[i] += a < b ? a : b;
                t_box.max[i] += a < b ? b : a;
            }
        }

		return t_box;
	}
    
	m33 basis = m33(1.0f);
	v3f origin = v3f(0, 0, 0);
};

struct statistic_t {
    f64         average{};
    aabb_t<f64> range{};
    u32         count{};
};

inline void
begin_statistic(statistic_t& stat) {
    stat.average = 0.0;
    stat.count = 0;
    stat.range.max = std::numeric_limits<f64>::min();
    stat.range.min = std::numeric_limits<f64>::max();
}

inline void
update_statistic(statistic_t& stat, f64 value) {
    stat.range.expand(value);
    stat.average += value;
    stat.count += 1;
}

inline void
end_statistic(statistic_t& stat) {
    if (stat.count) {
        stat.average /= (f64)stat.count;
    } else {
        stat.average = stat.range.min = 0.0;
        stat.range.max = 1.0; // to avoid div by 0
    }
}


}; // namespace math


namespace gfx {

// note(zack): this is in core because it will probably be loaded 
// in a work thread in platform layer, so that we dont have to worry
// about halting the thread when we reload the dll


using color4 = v4f;
using color3 = v3f;
using color32 = u32;
struct font_t;

struct material_t {
    v4f albedo{};

    f32 ao{};
    f32 emission{};
    f32 metallic{};
    f32 roughness{};

    u32 flags{};     // for material effects
    u32 opt_flags{}; // for performance
    u32 padding[2+4];

    static constexpr material_t plastic(const v4f& color) {
        return material_t {
            .albedo = color,
            .ao = 0.6f,
            .emission = 0.0f,
            .metallic = 0.0f,
            .roughness = 0.3f,
        };
    }

    static constexpr material_t metal(const v4f& color, f32 roughness = 0.5f) {
        return material_t {
            .albedo = color,
            .ao = 0.6f,
            .emission = 0.0f,
            .metallic = 1.0f,
            .roughness = roughness,
        };
    }
};

namespace gui {
    struct vertex_t;
};

inline v2f 
font_get_size(
    font_t* font,
    std::string_view text);

inline void
font_render(
    arena_t* arena,
    font_t* font,
    std::string_view text,
    v2f& position,
    v2f screen_size,
    utl::pool_t<gui::vertex_t>* vertices,
    utl::pool_t<u32>* indices,
    color32 text_color);


namespace color {
    constexpr color32 to_color32(const color4& c) {
        return 
            (u8(std::min(u16(c.x * 255.0f), 255ui16)) << 0 ) |
            (u8(std::min(u16(c.y * 255.0f), 255ui16)) << 8 ) |
            (u8(std::min(u16(c.z * 255.0f), 255ui16)) << 16) |
            (u8(std::min(u16(c.w * 255.0f), 255ui16)) << 24) ;
    }

    constexpr color32 to_color32(const color3& c) {
        return to_color32(color4(c, 1.0f));
    }

    constexpr color4 to_color4(const color32 c) {
        return color4{
            ((c&0xff)       >> 0),
            ((c&0xff00)     >> 8),
            ((c&0xff0000)   >> 16),
            ((c&0xff000000) >> 24)            
        } / 255.0f;
    }
    constexpr color3 to_color3(const color32 c) {
        return color3{to_color4(c)};
    }

    constexpr bool is_hex_digit(char c) {
        return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
    }

    constexpr u32 hex_value(char c) {
        constexpr std::array<std::pair<char, u32>, 22> lut {{
            {'0', 0u}, {'1', 1u}, {'2', 2u}, {'3', 3u}, {'4', 4u},
            {'5', 5u}, {'6', 6u}, {'7', 7u}, {'8', 8u}, {'9', 9u},
            {'a', 10u},{'b', 11u},{'c', 12u},{'d', 13u},{'e', 14u},
            {'f', 15u},{'A', 10u},{'B', 11u},{'C', 12u},{'D', 13u},
            {'E', 14u},{'F', 15u},
        }};
        for (const auto& [k, v]: lut) { 
            if (k == c) return v;
        }

        return 0;
    }

    template <typename Str>
    constexpr color4 str_to_rgba(const Str& str) {
        assert(str.size() == 9 && "invalid rgba size");
        
        for (const auto c: str) {
            assert(is_hex_digit(c) || c == '#');
        }

        color4 res;
        res.x = f32(hex_value(str[1]) * 16u + hex_value(str[2])) / 255.0f;
        res.y = f32(hex_value(str[3]) * 16u + hex_value(str[4])) / 255.0f;
        res.z = f32(hex_value(str[5]) * 16u + hex_value(str[6])) / 255.0f;
        res.w = f32(hex_value(str[7]) * 16u + hex_value(str[8])) / 255.0f;

        // res = glm::min(res, color4(1.0f));
        res.x = std::min(res.x, 1.0f);
        res.y = std::min(res.y, 1.0f);
        res.z = std::min(res.z, 1.0f);
        res.w = std::min(res.w, 1.0f);

        return res;
    }

    constexpr color32 operator"" _rgba(const char* str, std::size_t size) {
        auto c = str_to_rgba(std::string_view(str, size));
        return to_color32(c);
    }

    constexpr color32 operator"" _abgr(const char* str, std::size_t size) {
        auto c = str_to_rgba(std::string_view(str, size));
        u32 res = 0u;
        res = 
            (u8(c.x * 255.0f) << 24) |
            (u8(c.y * 255.0f) << 16) |
            (u8(c.z * 255.0f) << 8)  |
            (u8(c.w * 255.0f) << 0)  ;

        return res; 
    }

    constexpr color4 operator"" _c4(const char* str, std::size_t size) {
        return str_to_rgba(std::string_view(str, size));
    }

    constexpr color3 operator"" _c3(const char* str, std::size_t size) {
        return color3{str_to_rgba(std::string_view(str, size))};
    }

    namespace rgba {
        static constexpr auto clear = "#00000000"_rgba;
        static constexpr auto black = "#000000ff"_rgba;
        static constexpr auto dark_gray = "#111111ff"_rgba;
        static constexpr auto gray = "#808080ff"_rgba;
        static constexpr auto light_gray  = "#d3d3d3ff"_rgba;
        static constexpr auto white = "#ffffffff"_rgba;
        static constexpr auto cream = "#fafafaff"_rgba;
        static constexpr auto red   = "#ff0000ff"_rgba;
        static constexpr auto green = "#00ff00ff"_rgba;
        static constexpr auto blue  = "#0000ffff"_rgba;
        static constexpr auto light_blue  = "#2222f2ff"_rgba;
        static constexpr auto yellow= "#ffff00ff"_rgba;
        static constexpr auto purple= "#fa11faff"_rgba;
        static constexpr auto sand  = "#C2B280ff"_rgba;
        static constexpr auto material_fg = "#03dac6ff"_rgba;
        static constexpr auto material_bg = "#6200eeff"_rgba;
    };

    namespace abgr {
        static auto clear = "#00000000"_abgr;
        static auto black = "#000000ff"_abgr;
        static auto white = "#ffffffff"_abgr;
        static auto red   = "#ff0000ff"_abgr;
        static auto green = "#00ff00ff"_abgr;
        static auto blue  = "#0000ffff"_abgr;
        static auto yellow= "#ffff00ff"_abgr;
        static auto sand  = "#C2B280ff"_abgr;
    };

    namespace v4 {
        static constexpr auto clear = "#00000000"_c4;
        static constexpr auto black = "#000000ff"_c4;
        static constexpr auto white = "#ffffffff"_c4;
        static constexpr auto ray_white   = "#f1f1f1ff"_c4;
        static constexpr auto light_gray  = "#d3d3d3ff"_c4;
        static constexpr auto dark_gray   = "#2a2a2aff"_c4;
        static constexpr auto red   = "#ff0000ff"_c4;
        static constexpr auto green = "#00ff00ff"_c4;
        static constexpr auto blue  = "#0000ffff"_c4;
        static constexpr auto purple= "#ff00ffff"_c4;
        static constexpr auto cyan  = "#00ffffff"_c4;
        static constexpr auto yellow= "#ffff00ff"_c4;
        static constexpr auto sand  = "#C2B280ff"_c4;
        static constexpr auto dirt  = "#9b7653ff"_c4;
    };

    namespace v3 {
        static constexpr auto clear = "#00000000"_c3;
        static constexpr auto black = "#000000ff"_c3;
        static constexpr auto white = "#ffffffff"_c3;
        static constexpr auto gray  = "#333333ff"_c3;
        static constexpr auto red   = "#ff0000ff"_c3;
        static constexpr auto green = "#00ff00ff"_c3;
        static constexpr auto cyan  = "#00ffffff"_c3;
        static constexpr auto blue  = "#0000ffff"_c3;
        static constexpr auto purple= "#ff00ffff"_c3;
        static constexpr auto yellow= "#ffff00ff"_c3;
        static constexpr auto sand  = "#C2B280ff"_c3;
        static constexpr auto dirt  = "#9b7653ff"_c3;
    };

}; // namespace color


namespace gui {
    struct vertex_t {
        v2f pos;
        v2f tex;
        u32 img;
        u32 col;
    };

    struct ctx_t {
        utl::pool_t<vertex_t>* vertices;
        utl::pool_t<u32>* indices;
        gfx::font_t* font;
        app_input_t* input;
        v2f screen_size{};
    };

    enum eTypeFlag {
        eTypeFlag_Theme,
        eTypeFlag_Panel,
        eTypeFlag_Text,

        eTypeFlag_None,
    };

    struct theme_t {
        color32 fg_color{};
        color32 bg_color{};
        color32 text_color{color::rgba::cream};
        color32 active_color{color::rgba::yellow};
        color32 disabled_color{};
        color32 border_color{};
        u32     shadow_distance{1};
        color32 shadow_color{};

        f32 padding{1.0f};
        f32 margin{1.0f};
    };

    struct text_t;
    struct image_t;
    struct button_t;

    struct panel_t : public math::aabb_t<v2f> {
        eTypeFlag flag{eTypeFlag_Panel};
        panel_t* next{nullptr};

        text_t* text_widgets{nullptr};
        button_t* button_widgets{nullptr};

        sid_t name;

        theme_t theme;
    };

    struct button_t : public math::aabb_t<v2f> {
        void* user_data{0};
        
        using callback_fn = void(*)(void*);
        callback_fn on_press{0};

        theme_t theme;
    };

    struct text_t {
        eTypeFlag type{eTypeFlag_Text};
        text_t* next{nullptr};

        string_t text{};
        v2f offset{};
        u32 font_size{};

        theme_t theme;
    };
    
    inline void
    ctx_clear(
        ctx_t* ctx
    ) {
        ctx->vertices->clear();
        ctx->indices->clear();
    }

    inline void
    string_render(
        ctx_t* ctx,
        string_t text,
        const v2f& position,
        const color32& text_color = color::rgba::white
    ) {
        v2f cursor = position;
        font_render(0, ctx->font, 
            text.c_str(), 
            cursor,
            ctx->screen_size,
            ctx->vertices,
            ctx->indices,
            text_color
        );
    }
    inline void
    string_render(
        ctx_t* ctx,
        string_t text,
        v2f& position,
        const color32& text_color = color::rgba::white
    ) {
        font_render(0, ctx->font, 
            text.c_str(), 
            position,
            ctx->screen_size,
            ctx->vertices,
            ctx->indices,
            text_color
        );
    }

    inline void 
    text_render(
        ctx_t* ctx,
        panel_t* parent,
        text_t* text
    ) {
        v2f c = parent->min + text->offset;
        font_render(0, ctx->font, 
            text->text.c_str(), 
            c,
            ctx->screen_size,
            ctx->vertices,
            ctx->indices,
            text->theme.text_color
        );

        if (text->theme.shadow_distance) {
            c = parent->min + text->offset + v2f{(f32)text->theme.shadow_distance},
            font_render(0, ctx->font, 
                text->text.c_str(), 
                c,
                ctx->screen_size,
                ctx->vertices,
                ctx->indices,
                text->theme.shadow_color
            );
        }

        if (text->next) {
            text_render(ctx, parent, text->next);
        }
    }

    inline void 
    draw_circle(
        ctx_t* ctx,
        v2f pos, f32 radius,
        u32 color, u32 res = 32
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count);

        vertex_t* center = ctx->vertices->allocate(1);

        center->pos = pos / ctx->screen_size;
        center->col = color;
        center->img = ~(0ui32);

        vertex_t* lv{0};
        range_u32(i, 0, res) {
            const auto a = math::constants::tau32 * f32(i)/f32(res-2);
            vertex_t* v = ctx->vertices->allocate(1);

            v->pos = center->pos + v2f{glm::cos(a), glm::sin(a)} * radius / ctx->screen_size;
            v->col = color;
            v->img = ~(0ui32);

            if (lv) {
                u32* tris = ctx->indices->allocate(3);
                *tris++ = v_start;
                *tris++ = v_start + i - 1;
                *tris++ = v_start + i;
            }
            lv = v;
        }
    }

    inline void
    draw_line(
        ctx_t* ctx,
        v2f a, v2f b,
        f32 line_width,
        u32 color
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count);
        const v2f d{(b-a)/ctx->screen_size};
        const v2f ud{glm::normalize(d)};
        const v2f n{v2f{-ud.y, ud.x}};

        vertex_t* v = ctx->vertices->allocate(4);
        u32* i = ctx->indices->allocate(6);
        v[0] = vertex_t { .pos = a/ctx->screen_size + n * (line_width/ctx->screen_size * 0.5f), .tex = v2f{0.0f, 1.0f}, .img = ~(0ui32), .col = color};
        v[1] = vertex_t { .pos = a/ctx->screen_size - n * (line_width/ctx->screen_size * 0.5f), .tex = v2f{0.0f, 0.0f}, .img = ~(0ui32), .col = color};
        v[2] = vertex_t { .pos = b/ctx->screen_size + n * (line_width/ctx->screen_size * 0.5f), .tex = v2f{1.0f, 0.0f}, .img = ~(0ui32), .col = color};
        v[3] = vertex_t { .pos = b/ctx->screen_size - n * (line_width/ctx->screen_size * 0.5f), .tex = v2f{1.0f, 1.0f}, .img = ~(0ui32), .col = color};

        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;

        i[3] = v_start + 2;
        i[4] = v_start + 1;
        i[5] = v_start + 3;
    }

    inline void 
    draw_arc(
        ctx_t* ctx,
        v2f pos, f32 radius,
        f32 start, f32 end,
        f32 line_width, u32 color, u32 res = 10
    ) {
        range_u32(i, 1, res) {
            const f32 ae = math::constants::tau32 * (start + end * f32(i-1)/f32(res-1));
            const f32 as = math::constants::tau32 * (start + end * f32(i)/f32(res-1));

            const v2f p0 = pos + v2f{glm::cos(ae), glm::sin(ae)} * radius;
            const v2f p1 = pos + v2f{glm::cos(as), glm::sin(as)} * radius;

            draw_line(ctx, p0, p1, line_width, color);
        }
    }

    inline void 
    draw_ellipse_arc(
        ctx_t* ctx,
        v2f pos, v2f radius,
        f32 start, f32 end,
        f32 line_width, u32 color, u32 res = 10
    ) {
        range_u32(i, 1, res) {
            const f32 ae = math::constants::tau32 * (start + end * f32(i-1)/f32(res-1));
            const f32 as = math::constants::tau32 * (start + end * f32(i)/f32(res-1));

            const v2f p0 = pos + v2f{glm::cos(ae), glm::sin(ae)} * radius;
            const v2f p1 = pos + v2f{glm::cos(as), glm::sin(as)} * radius;

            draw_line(ctx, p0, p1, line_width, color);
        }
    }

    inline void 
    draw_ellipse_arc_with_basis(
        ctx_t* ctx,
        v2f pos, v2f radius, v2f b0, v2f b1,
        f32 start, f32 end,
        f32 line_width, u32 color, u32 res = 10
    ) {
        range_u32(i, 1, res) {
            const f32 ae = math::constants::tau32 * (start + end * f32(i-1)/f32(res-1));
            const f32 as = math::constants::tau32 * (start + end * f32(i)/f32(res-1));

            const v2f p0 = pos + b0 * glm::cos(ae) * radius.x + b1 * glm::sin(ae) * radius.y;
            const v2f p1 = pos + b0 * glm::cos(as) * radius.x + b1 * glm::sin(as) * radius.y;

            draw_line(ctx, p0, p1, line_width, color);
        }
    }

    inline void
    draw_round_rect(
        ctx_t* ctx,
        math::aabb_t<v2f> box,
        f32 radius,
        u32 color,
        u64 num_segments = 8
    ) {
        const auto hw = box.size().x * 0.5f;
        const auto hh = box.size().y * 0.5f;
        const auto half_extents = box.size().y * 0.5f;
        // Calculate the corner positions of the rounded rectangle
        
        glm::vec2 p1(-hw + radius, hh - radius);
        glm::vec2 p2(hw - radius, hh - radius);
        glm::vec2 p3(hw - radius, -hh + radius);
        glm::vec2 p4(-hw + radius, -hh + radius);
        const f32 pi_32 = (glm::pi<f32>());
        float theta = pi_32 * 0.5f;
        float delta_theta = pi_32 * 2.0f / num_segments;

        // Calculate the number of vertices and indices
        const size_t num_vertices = (num_segments) * 6;
        const size_t num_indices = num_segments * 12;
        const u32 start_index = safe_truncate_u64(ctx->vertices->count);

        // Resize the vertex and index vectors to hold the generated data
        vertex_t* vertices = ctx->vertices->allocate(num_vertices);
        u32* indices = ctx->indices->allocate(num_indices);

        // Generate the vertex data for the rounded rectangle
        for (int i = 0; i < num_segments; i++) {
            const glm::vec2 v1(glm::cos(theta), glm::sin(theta));
            const glm::vec2 v2(glm::cos(theta + delta_theta * 0.5f), glm::sin(theta + delta_theta * 0.5f));
            const glm::vec2 v3(glm::cos(theta + delta_theta), glm::sin(theta + delta_theta));
            const glm::vec2 t1 = box.center() + p1 + v1 * radius;
            const glm::vec2 t2 = box.center() + p2 + v2 * radius;
            const glm::vec2 t3 = box.center() + p2 + v3 * radius;
            const glm::vec2 t4 = box.center() + p3 + v3 * radius;
            const glm::vec2 t5 = box.center() + p4 + v3 * radius;
            const glm::vec2 t6 = box.center() + p4 + v1 * radius;
            
            const size_t index = i * 6;
            vertices[index + 0].pos = t1 / ctx->screen_size;
            vertices[index + 1].pos = t2 / ctx->screen_size;
            vertices[index + 2].pos = t3 / ctx->screen_size;
            vertices[index + 3].pos = t4 / ctx->screen_size;
            vertices[index + 4].pos = t5 / ctx->screen_size;
            vertices[index + 5].pos = t6 / ctx->screen_size;
            theta += delta_theta;

            loop_iota_u64(j, 6) {
                vertices[index + j].tex = v2f{0.0f};
                vertices[index + j].img = ~0ui32;
                vertices[index + j].col = color;
            }

            const size_t tri_index = i * 12;
            
            indices[tri_index + 0] = (start_index + 6 * i);
            indices[tri_index + 1] = (start_index + 6 * i + 1);
            indices[tri_index + 2] = (start_index + 6 * i + 2);
            indices[tri_index + 3] = (start_index + 6 * i);
            indices[tri_index + 4] = (start_index + 6 * i + 2);
            indices[tri_index + 5] = (start_index + 6 * i + 3);
            indices[tri_index + 6] = (start_index + 6 * i);
            indices[tri_index + 7] = (start_index + 6 * i + 3);
            indices[tri_index + 8] = (start_index + 6 * i + 4);
            indices[tri_index + 9] = (start_index + 6 * i);
            indices[tri_index + 10] = (start_index + 6 * i + 4);
            indices[tri_index + 11] = (start_index + 6 * i + 5);
        }


 
    }   
    
    inline void
    draw_rect(
        ctx_t* ctx,
        math::aabb_t<v2f> box,
        std::span<u32, 4> colors
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count);
        const u32 i_start = safe_truncate_u64(ctx->indices->count);

        const v2f p0 = box.min;
        const v2f p1 = v2f{box.min.x, box.max.y};
        const v2f p2 = v2f{box.max.x, box.min.y};
        const v2f p3 = box.max;

        vertex_t* v = ctx->vertices->allocate(4);
        u32* i = ctx->indices->allocate(6);
        v[0] = vertex_t { .pos = p0 / ctx->screen_size, .tex = v2f{0.0f, 1.0f}, .img = ~(0ui32), .col = colors[0]};
        v[1] = vertex_t { .pos = p1 / ctx->screen_size, .tex = v2f{0.0f, 0.0f}, .img = ~(0ui32), .col = colors[1]};
        v[2] = vertex_t { .pos = p2 / ctx->screen_size, .tex = v2f{1.0f, 0.0f}, .img = ~(0ui32), .col = colors[2]};
        v[3] = vertex_t { .pos = p3 / ctx->screen_size, .tex = v2f{1.0f, 1.0f}, .img = ~(0ui32), .col = colors[3]};

        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;

        i[3] = v_start + 2;
        i[4] = v_start + 1;
        i[5] = v_start + 3;
    }

    inline void
    draw_rect(
        ctx_t* ctx,
        math::aabb_t<v2f> box,
        u32 color
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count);
        const u32 i_start = safe_truncate_u64(ctx->indices->count);

        const v2f p0 = box.min;
        const v2f p1 = v2f{box.min.x, box.max.y};
        const v2f p2 = v2f{box.max.x, box.min.y};
        const v2f p3 = box.max;

        vertex_t* v = ctx->vertices->allocate(4);
        u32* i = ctx->indices->allocate(6);
        v[0] = vertex_t { .pos = p0 / ctx->screen_size, .tex = v2f{0.0f, 1.0f}, .img = ~(0ui32), .col = color};
        v[1] = vertex_t { .pos = p1 / ctx->screen_size, .tex = v2f{0.0f, 0.0f}, .img = ~(0ui32), .col = color};
        v[2] = vertex_t { .pos = p2 / ctx->screen_size, .tex = v2f{1.0f, 0.0f}, .img = ~(0ui32), .col = color};
        v[3] = vertex_t { .pos = p3 / ctx->screen_size, .tex = v2f{1.0f, 1.0f}, .img = ~(0ui32), .col = color};

        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;

        i[3] = v_start + 2;
        i[4] = v_start + 1;
        i[5] = v_start + 3;
    }

    inline void
    panel_render(
        ctx_t* ctx,
        panel_t* panel
    ) {
        if (panel->next) { panel_render(ctx, panel->next); }

        if (panel->text_widgets) {
            text_render(ctx, panel, panel->text_widgets);
        }
        draw_rect(ctx, *panel, panel->theme.bg_color);
    }

    namespace im {
        struct ui_id_t {
            u64 owner;
            u64 id;
            u64 index;

            ui_id_t& operator=(u64 id_) {
                id = id_;
                owner = 0;
                index = 0;
                return *this;
            }
        };

        struct state_t {
            ctx_t& ctx;

            v2f draw_cursor{0.0f}; 
            v2f gizmo_mouse_start{};
            v3f gizmo_world_start{};
            v2f gizmo_axis_dir{};
            v3f gizmo_axis_wdir{};

            theme_t theme;

            ui_id_t hot;
            ui_id_t active;

            panel_t panel;

            bool next_same_line{false};
            v2f saved_cursor;
        };

        inline bool
        want_mouse_capture(state_t& imgui) {
            return imgui.hot.id != 0;
        }

        inline void
        clear(state_t& imgui) {
            imgui.draw_cursor = v2f{0.0f};
            // imgui.active = 0;
        }

        inline void
        same_line(state_t& imgui) {
            imgui.next_same_line = true;
            imgui.saved_cursor = imgui.draw_cursor;
        }

        inline bool
        begin_panel(
            state_t& imgui, 
            string_t name,
            v2f pos = v2f{0.0f},
            v2f size = v2f{-1.0f}
        ) {
            const sid_t pnl_id = sid(name.sv());

            if (pos.x >= 0.0f && pos.y >= 0.0f) { 
                imgui.draw_cursor = pos+1.0f;
            }
            imgui.saved_cursor = imgui.draw_cursor;

            imgui.panel.min = imgui.draw_cursor;
            if (size.x > 0.0f && size.y > 0.0f) {
                imgui.panel.expand(imgui.draw_cursor + size);
            }

            imgui.panel.name = pnl_id;
            
            return true;
        }

        inline bool
        begin_panel_3d(
            state_t& imgui, 
            string_t name,
            m44 vp,
            v3f pos
        ) {
            const v3f ndc = math::world_to_screen(vp, pos);
            math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{1.0f}};

            if (viewable.contains(ndc)) {
                const v2f screen = v2f{ndc} * imgui.ctx.screen_size;
                return begin_panel(imgui, name, screen);
            }
            return false;
        }

        inline void
        end_panel(
            state_t& imgui
        ) {
            imgui.panel.expand(imgui.draw_cursor);

            constexpr f32 border_size = 1.0f;
            constexpr f32 corner_radius = 16.0f;

            draw_rect(&imgui.ctx, imgui.panel, imgui.theme.bg_color);
            imgui.panel.min -= v2f{border_size};
            imgui.panel.max += v2f{border_size};
            draw_rect(&imgui.ctx, imgui.panel, imgui.theme.fg_color);

            imgui.panel = {};
        }

        inline void
        draw_gizmo(
            state_t& imgui,
            v3f* pos,
            const m44& vp
        ) {
            const v3f ndc = math::world_to_screen(vp, *pos);
            math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{1.0f}};

            if (viewable.contains(ndc)) {
                const v2f screen = v2f{ndc} * imgui.ctx.screen_size;
                
                auto v_up         = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::up)};
                auto v_forward    = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::forward)};
                auto v_right      = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::right)};
                auto v_pos        = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos)};
                
                const f32 line_width = 3.0f;

                const v2f rd = v_right - v_pos;
                const v2f ud = v_up - v_pos;
                const v2f fd = v_forward - v_pos;
                

                draw_circle(&imgui.ctx, v_pos, 3.0f,     color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_up, 3.0f,     color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_right, 3.0f,  color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_forward, 3.0f,color::rgba::light_gray);

                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{0.75f}, ud, fd, 0.0f, 0.25f, 3.0f, color::rgba::red, 64);
                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{0.75f}, rd, fd, 0.0f, 0.25f, 3.0f, color::rgba::green, 64);
                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{0.75f}, rd, ud, 0.0f, 0.25f, 3.0f, color::rgba::blue, 64);

                draw_circle(&imgui.ctx, v_up, 5.0f, color::rgba::green);
                draw_circle(&imgui.ctx, v_right, 5.0f, color::rgba::red);
                draw_circle(&imgui.ctx, v_forward, 5.0f, color::rgba::blue);

                draw_line(&imgui.ctx, v_pos, v_up, line_width, color::rgba::green);
                draw_line(&imgui.ctx, v_pos, v_right, line_width, color::rgba::red);
                draw_line(&imgui.ctx, v_pos, v_forward, line_width, color::rgba::blue);

            }
        }
        inline void
        gizmo(
            state_t& imgui,
            v3f* pos,
            const m44& vp
        ) {
            draw_gizmo(imgui, pos, vp);

            const v3f ndc = math::world_to_screen(vp, *pos);
            math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{1.0f}};

            if (viewable.contains(ndc)) {
                const v2f screen = v2f{ndc} * imgui.ctx.screen_size;
                
                const auto v_up         = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::up)};
                const auto v_forward    = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::forward)};
                const auto v_right      = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::right)};
                const auto v_pos        = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos)};

                const v2f rd = v_right - v_pos;
                const v2f ud = v_up - v_pos;
                const v2f fd = v_forward - v_pos;
                

                math::circle_t pc{v_pos, 8.0f};
                math::circle_t rc{v_right, 8.0f};
                math::circle_t uc{v_up, 8.0f};
                math::circle_t fc{v_forward, 8.0f};
                const auto [mx,my] = imgui.ctx.input->mouse.pos;
                const v2f mouse{mx,my};
                const v2f mdelta{-(imgui.gizmo_mouse_start-mouse)};
                u64 giz_id = 0;

                const auto is_gizmo_id =[](u64 id){
                    switch(id) {
                        case "gizmo_pos"_sid: 
                        case "gizmo_right"_sid: 
                        case "gizmo_up"_sid: 
                        case "gizmo_forward"_sid:
                            return true;
                        default:
                            return false;
                    }                        
                };

                if (math::intersect(pc, mouse)) {giz_id = "gizmo_pos"_sid;} else
                if (math::intersect(rc, mouse)) {giz_id = "gizmo_right"_sid;} else
                if (math::intersect(uc, mouse)) {giz_id = "gizmo_up"_sid;} else
                if (math::intersect(fc, mouse)) {giz_id = "gizmo_forward"_sid;}

                const bool clicked = imgui.ctx.input->mouse.buttons[0];
                if (is_gizmo_id(imgui.active.id)) {
                    if (is_gizmo_id(imgui.hot.id)) {
                        if (imgui.gizmo_mouse_start != v2f{0.0f}) {
                            const v2f naxis = glm::normalize(imgui.gizmo_axis_dir);
                            const f32 scale = glm::length(imgui.gizmo_axis_dir);
                            *pos = imgui.gizmo_world_start + imgui.gizmo_axis_wdir * glm::dot(mdelta/scale, naxis);
                            const f32 proj = glm::dot(mdelta, naxis);

                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start, 3.0f, color::rgba::light_gray);
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start, 5.0f, color::to_color32(imgui.gizmo_axis_wdir));
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, 3.0f, color::rgba::light_gray);
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, 5.0f, color::to_color32(imgui.gizmo_axis_wdir));
                            
                            draw_line(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, imgui.gizmo_mouse_start, 3.0f, color::to_color32(imgui.gizmo_axis_wdir * 0.7f));
                            draw_line(&imgui.ctx, v_pos - naxis * 10000.0f, v_pos + naxis * 10000.0f, 2.0f, color::rgba::light_gray);

                        } else {
                            imgui.gizmo_world_start = *pos;
                            switch(giz_id) {
                                case 0: break;
                                case "gizmo_pos"_sid: {
                                    imgui.gizmo_mouse_start = v_pos;
                                    imgui.gizmo_axis_dir = (rd);
                                    break;
                                }
                                case "gizmo_right"_sid: {
                                    imgui.gizmo_axis_dir = (rd);
                                    imgui.gizmo_axis_wdir = axis::right;
                                    imgui.gizmo_mouse_start = v_right;
                                    break;
                                }
                                case "gizmo_up"_sid: {
                                    imgui.gizmo_axis_dir = (ud);
                                    imgui.gizmo_axis_wdir = axis::up;
                                    imgui.gizmo_mouse_start = v_up;
                                    break;
                                }
                                case "gizmo_forward"_sid: {
                                    imgui.gizmo_axis_dir = (fd);
                                    imgui.gizmo_axis_wdir = axis::forward;
                                    imgui.gizmo_mouse_start = v_forward;
                                    break;
                                }
                                case_invalid_default;
                            }
                        }
                    }
                    if (!clicked) {
                        imgui.active = 0;
                        imgui.gizmo_mouse_start = v2f{0.0f};
                        imgui.gizmo_axis_dir = v2f{0.0f};
                        imgui.gizmo_axis_wdir = v3f{0.0f};
                    }
                }
                if (imgui.hot.id == giz_id) {
                    if (clicked) {
                        imgui.active = giz_id;
                    }
                } else if (is_gizmo_id(imgui.hot.id)) {
                    if (!clicked) {
                        imgui.hot = 0;
                        imgui.gizmo_mouse_start = v2f{0.0f};
                        imgui.gizmo_axis_dir = v2f{0.0f};
                        imgui.gizmo_axis_wdir = v3f{0.0f};
                    }
                }

                if (giz_id) {
                    imgui.hot = giz_id;
                }
            }
        }
        inline void
        color_edit(
            state_t& imgui,
            color32* color,
            v2f size = v2f{64.0f}
        ) {
            const u64 clr_id = (u64)color;
            v3f color_hsv = glm::hsvColor(color::to_color3(*color));
            
            v2f tmp_cursor = imgui.draw_cursor;
            tmp_cursor.x += imgui.theme.padding;

            math::aabb_t<v2f> box;
            box.expand(tmp_cursor);
            box.expand(tmp_cursor + size);

            math::aabb_t<v2f> hue_box;

            color32 rect_colors[4]= {
                color::rgba::white,
                color::rgba::black,
                color::to_color32(glm::rgbColor(v3f{color_hsv.x, 1.0f, 1.0f})),
                color::rgba::black,
            };

            draw_rect(&imgui.ctx, box, std::span{rect_colors});

            const size_t res = 4;
            loop_iota_u64(i, res) {
                math::aabb_t<v2f> hue_seg;
                hue_seg.expand(box.min + box.size() * v2f{1.0f, f32(i)/f32(res)} + v2f{imgui.theme.padding, 0.0f});
                hue_seg.expand(box.min + box.size() * v2f{1.0f, f32(i+1)/f32(res)} + v2f{8.0f, 0.0f} + v2f{imgui.theme.padding,0.0f});

                hue_box.expand(hue_seg);

                const f32 start_angle = f32(i) * (360.0f / f32(res));
                const f32 next_angle = f32(i+1) * (360.0f / f32(res));
                const v3f start_color = glm::rgbColor(v3f{start_angle, 1.0f, 1.0f});
                const v3f next_color = glm::rgbColor(v3f{next_angle, 1.0f, 1.0f});

                color32 hue_colors[4]= {
                    color::to_color32(start_color),
                    color::to_color32(next_color),
                    color::to_color32(start_color),
                    color::to_color32(next_color),
                };
                
                draw_rect(&imgui.ctx, hue_seg, std::span{hue_colors});
            }

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            const f32 col_s = glm::max(x - box.min.x, 0.0f) / box.size().x;
            const f32 col_v = glm::max(y - box.min.y, 0.0f) / box.size().y;
            const f32 hue_prc = glm::max(y - hue_box.min.y, 0.0f) / hue_box.size().y;

            if (imgui.active.id == clr_id) {
                if (imgui.hot.id == clr_id) {
                    if (box.contains(v2f{x,y})) {
                        *color = color::to_color32(glm::rgbColor(v3f{color_hsv.x, col_s, 1.0f-col_v}));
                    } else if (hue_box.contains(v2f{x,y})) {
                        *color = color::to_color32(glm::rgbColor(v3f{hue_prc*360.0f, color_hsv.y, color_hsv.z}));
                    }
                }
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = 0;
                }
            } else if (imgui.hot.id == clr_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = clr_id;
                }
            }

            if (box.contains(v2f{x,y}) || hue_box.contains(v2f{x,y})) {
                imgui.hot = clr_id;
            } else if (imgui.hot.id == clr_id) {
                imgui.hot.id = 0;
            }

            imgui.draw_cursor.y += box.size().y + imgui.theme.padding * 2.0f;
        }

        inline void
        float_slider(
            state_t& imgui,
            f32* val,
            f32 min = 0.0f,
            f32 max = 1.0f,
            v2f size = v2f{64.0f, 16.0f}
        ) {
            const u64 sld_id = (u64)val;

            v2f tmp_cursor = imgui.draw_cursor;
            tmp_cursor.x += 8.0f;
            math::aabb_t<v2f> box;
            math::aabb_t<v2f> prc_box;
            const f32 prc = glm::clamp((*val - min) / (max - min), 0.0f, 1.0f);
            box.expand(tmp_cursor);
            box.expand(tmp_cursor + size);

            prc_box.expand(tmp_cursor + 2.0f);
            prc_box.expand(prc_box.min + (size - 4.0f) * v2f{prc, 1.0f});
            imgui.panel.expand(box.max + imgui.theme.padding);

            tmp_cursor = box.center() - font_get_size(imgui.ctx.font, fmt_sv("{:.2f}", *val)) * 0.5f;
            string_render(&imgui.ctx, fmt_sv("{:.2f}", *val), tmp_cursor, imgui.theme.text_color);
            draw_rect(&imgui.ctx, prc_box, imgui.active.id == sld_id ? imgui.theme.active_color : imgui.theme.fg_color);
            draw_rect(&imgui.ctx, box, imgui.theme.bg_color);
            box.max += v2f{1.0f};
            box.min -= v2f{1.0f};
            draw_rect(&imgui.ctx, box, imgui.theme.fg_color);

            imgui.draw_cursor.y += box.size().y + imgui.theme.padding;

            const auto [x,y] = imgui.ctx.input->mouse.pos;

            // expand all the way to get prc box size
            prc_box.expand(prc_box.min + (size - 4.0f) * v2f{1.0f});
            const f32 m_prc = glm::max(x - prc_box.min.x, 0.0f) / prc_box.size().x;

            if (imgui.active.id == sld_id) {
                if (imgui.hot.id == sld_id) {
                    *val = m_prc * (max - min) + min;
                    *val = glm::clamp(*val, min, max);
                }
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = 0;
                }
            } else if (imgui.hot.id == sld_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = sld_id;
                }
            }

            if (box.contains(v2f{x,y})) {
                imgui.hot = sld_id;
            } else if (imgui.hot.id == sld_id) {
                imgui.hot.id = 0;
            }
        }

        inline bool
        text(
            state_t& imgui, 
            string_t text,
            bool* toggle_state = 0         
        ) {
            const sid_t txt_id = sid(text.sv());
            bool result = toggle_state ? *toggle_state : false;

            constexpr f32 text_pad = 4.0f;
            const auto font_size = font_get_size(imgui.ctx.font, text);
            imgui.panel.expand(imgui.draw_cursor + font_size + text_pad * 2.0f);
            v2f temp_cursor = imgui.draw_cursor;
            const f32 start_x = imgui.draw_cursor.x;
            temp_cursor.x += text_pad;

            math::aabb_t<v2f> text_box;
            text_box.expand(temp_cursor);
            text_box.expand(temp_cursor + font_size);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (text_box.contains(v2f{x,y})) {
                imgui.hot = txt_id;
            } else if (imgui.hot.id == txt_id) {
                imgui.hot.id = 0;
            }

            if (imgui.active.id == txt_id) {
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    if (imgui.hot.id == txt_id) {
                        result = !result;
                        if (toggle_state) {
                            *toggle_state = result;
                        }
                    }
                    imgui.active = 0;
                }
            }
            if (imgui.hot.id == txt_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = txt_id;
                }
            }

            string_render(&imgui.ctx, text, temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            // draw_rect(&imgui.ctx, text_box, gfx::color::rgba::purple);
            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.draw_cursor.x += font_size.x;
            } else {
                imgui.draw_cursor.y = temp_cursor.y - font_size.y + imgui.theme.padding;
                imgui.draw_cursor.x = imgui.saved_cursor.x;
            }
            return result;
        }

        inline void
        histogram(
            state_t& imgui, 
            f32* values,
            size_t value_count,
            f32 max, 
            v2f size = v2f{64.0f, 16.0f},
            gfx::color32 color = gfx::color::rgba::yellow
        ) {
            if (!value_count) {
                return;
            }
            const u64 hst_id = (uintptr_t)values;

            const f32 start_x = imgui.draw_cursor.x;
            math::aabb_t<v2f> bg_box;
            bg_box.min = imgui.draw_cursor;
            bg_box.max = imgui.draw_cursor + size;

            const v2f box_size{size.x / (f32)value_count, size.y};

            range_u64(i, 0, value_count) {
                f32 perc = f32(i)/f32(value_count);
                math::aabb_t<v2f> box;
                box.min = imgui.draw_cursor + v2f{box_size.x*i, 0.0f};
                box.max = imgui.draw_cursor + v2f{box_size.x*(i+1), size.y*(1.0f-(values[i]/max))};
                draw_rect(&imgui.ctx, box, imgui.theme.fg_color);
            }
            draw_rect(&imgui.ctx, bg_box, color);
            
            if (imgui.next_same_line) {
                imgui.draw_cursor.x += size.x + imgui.theme.padding;
                imgui.next_same_line = false;
            } else {
                imgui.draw_cursor.y += size.y + imgui.theme.padding;
                imgui.draw_cursor.x = imgui.saved_cursor.x;
            }
            imgui.panel.expand(bg_box.max + v2f{imgui.theme.padding});
        }

        inline void
        checkbox(
            state_t& imgui, 
            string_t name,
            bool* var,
            v2f size = v2f{16.0f}
        ) {
            const sid_t btn_id = sid(name.sv());
            math::aabb_t<v2f> box;
            box.min = imgui.draw_cursor;
            box.max = imgui.draw_cursor + v2f{size};

            if (imgui.active.id == btn_id) {
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    if (imgui.hot.id == btn_id) {
                        *var = !*var;
                    }
                    imgui.active = 0;
                }
            } else if (imgui.hot.id == btn_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = btn_id;
                }
            }

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (box.contains( v2f{x,y} )) {
                // todo check  not active?
                imgui.hot = btn_id;
            } else {
                imgui.hot = 0;
            }
            
            draw_rect(&imgui.ctx, box, imgui.theme.border_color);
            box.min += v2f{1.0f};
            box.max -= v2f{1.0f};
            draw_rect(&imgui.ctx, box, *var ? imgui.theme.bg_color : imgui.theme.disabled_color);
            
            imgui.draw_cursor.y += size.y + imgui.theme.padding;

        }

        inline bool
        button(
            state_t& imgui, 
            string_t name, 
            v2f size = v2f{16.0f}
        ) {
            const sid_t btn_id = sid(name.sv());
            math::aabb_t<v2f> box;
            box.min = imgui.draw_cursor + v2f{4.0f, 0.0f};
            box.max = imgui.draw_cursor + v2f{size};

            bool result = false;

            if (imgui.active.id == btn_id) {
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    if (imgui.hot.id == btn_id) {
                        result = true;
                    }
                    imgui.hot = imgui.active = 0;
                }
            } else if (imgui.hot.id == btn_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = btn_id;
                }
            }

            const v2f text_size = font_get_size(imgui.ctx.font, name);
            box.expand(box.min + text_size + imgui.theme.margin * 2.0f);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (box.contains( v2f{x,y} )) {
                if (imgui.active.id == 0) {
                    imgui.hot = btn_id;
                }
            } else if (imgui.hot.id == btn_id) {
                imgui.hot = 0;
            }

            imgui.panel.expand(box.max + imgui.theme.padding);
            v2f same_line = box.min + imgui.theme.margin;
            string_render(&imgui.ctx, name, same_line, imgui.theme.text_color);
            draw_rect(&imgui.ctx, box, imgui.hot.id == btn_id ? imgui.theme.active_color : imgui.theme.fg_color);
            
            imgui.draw_cursor.y += size.y + imgui.theme.padding;

            return result;
        }

    };
}; // namespace gui

#include "stb/stb_truetype.h"

struct font_t {
    struct glyph_t {
        char c{};
        math::aabb_t<v2f> screen{};
        math::aabb_t<v2f> texture{};
    };
    f32 size{12.0f};
    u32 pixel_count{512>>1};

    u8* ttf_buffer{0};
    u8* bitmap{0};
    stbtt_bakedchar cdata[96];
};

inline void
font_load(
    arena_t* arena,
    font_t* font,
    std::string_view path,
    float size
) {
    font->size = size;

    font->bitmap = (u8*)arena_alloc(arena, font->pixel_count*font->pixel_count);
    size_t stack_mark = arena_get_mark(arena);

        font->ttf_buffer = (u8*)arena_alloc(arena, 1<<20);
        
        FILE* file;
        fopen_s(&file, path.data(), "rb");

        fread(font->ttf_buffer, 1, 1<<20, file);
        stbtt_BakeFontBitmap(font->ttf_buffer, 0, size, font->bitmap, font->pixel_count, font->pixel_count, 32, 96, font->cdata);
                
        fclose(file);

    arena_set_mark(arena, stack_mark);
}

inline font_t::glyph_t
font_get_glyph(font_t* font, f32 x, f32 y, char c) {

    math::aabb_t<v2f> screen{};
    math::aabb_t<v2f> texture{};
    
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(font->cdata, font->pixel_count, font->pixel_count, c - 32, &x,&y, &q, 1);

    texture.expand(v2f{q.s0, q.t0});
    texture.expand(v2f{q.s1, q.t1});

    screen.expand(v2f{q.x0, q.y0});
    screen.expand(v2f{q.x1, q.y1});

    return font_t::glyph_t {
        c, 
        screen,
        texture
    };
}

struct vertex_t;

inline v2f 
font_get_size(
    font_t* font,
    std::string_view text
) {
    f32 start_x = 0;
    v2f size{0.0f};
    v2f cursor{0.0f};

    cursor.y += font_get_glyph(font, cursor.x, cursor.y, ';').screen.size().y + 1;

    for (const char c : text) {
        if (c > 32 && c < 128) {
            const auto glyph = font_get_glyph(font, cursor.x, cursor.y, c);

            cursor.x += glyph.screen.size().x + 1;

            size = glm::max(size, cursor);
        } else if (c == '\n') {
            cursor.y += font_get_glyph(font, cursor.x, cursor.y, '_').screen.size().y + 1;
            cursor.x = start_x;
        } else if (c == ' ') {
            cursor.x += font_get_glyph(font, cursor.x, cursor.y, '_').screen.size().x + 1;
        } else if (c == '\t') {
            cursor.x += (font_get_glyph(font, cursor.x, cursor.y, '_').screen.size().x + 1) * 5;
        }
    }
    return size;
}

inline void
font_render(
    arena_t* arena,
    font_t* font,
    std::string_view text,
    v2f& cursor,
    v2f screen_size,
    utl::pool_t<gui::vertex_t>* vertices,
    utl::pool_t<u32>* indices,
    color32 text_color = color::rgba::cream
) {
    assert(screen_size.x && screen_size.y);
    f32 start_x = cursor.x;
    cursor.y += font_get_glyph(font, cursor.x, cursor.y, ';').screen.size().y + 1;

    for (const char c : text) {
        if (c > 32 && c < 128) {
            const auto glyph = font_get_glyph(font, cursor.x, cursor.y, c);

            const u32 v_start = safe_truncate_u64(vertices->count);
            gui::vertex_t* v = vertices->allocate(4);            
            u32* i = indices->allocate(6);            

            const v2f p0 = v2f{glyph.screen.min.x, glyph.screen.min.y} / screen_size;
            const v2f p1 = v2f{glyph.screen.min.x, glyph.screen.max.y} / screen_size;
            const v2f p2 = v2f{glyph.screen.max.x, glyph.screen.min.y} / screen_size;
            const v2f p3 = v2f{glyph.screen.max.x, glyph.screen.max.y} / screen_size;
            
            const v2f c0 = v2f{glyph.texture.min.x, glyph.texture.min.y};
            const v2f c1 = v2f{glyph.texture.min.x, glyph.texture.max.y};
            const v2f c2 = v2f{glyph.texture.max.x, glyph.texture.min.y};
            const v2f c3 = v2f{glyph.texture.max.x, glyph.texture.max.y};

            v[0] = gui::vertex_t{ .pos = p0, .tex = c0, .img = 0|BIT(30), .col = text_color};
            v[1] = gui::vertex_t{ .pos = p1, .tex = c1, .img = 0|BIT(30), .col = text_color};
            v[2] = gui::vertex_t{ .pos = p2, .tex = c2, .img = 0|BIT(30), .col = text_color};
            v[3] = gui::vertex_t{ .pos = p3, .tex = c3, .img = 0|BIT(30), .col = text_color};

            i[0] = v_start + 0;
            i[1] = v_start + 2;
            i[2] = v_start + 1;

            i[3] = v_start + 1;
            i[4] = v_start + 2;
            i[5] = v_start + 3;

            cursor.x += glyph.screen.size().x + 1;
        } else if (c == '\n') {
            cursor.y += font_get_glyph(font, cursor.x, cursor.y, '_').screen.size().y + 1;
            cursor.x = start_x;
        } else if (c == ' ') {
            cursor.x += font_get_glyph(font, cursor.x, cursor.y, '_').screen.size().x + 1;
        } else if (c == '\t') {
            cursor.x += (font_get_glyph(font, cursor.x, cursor.y, '_').screen.size().x + 1) * 5;
        }
    }
    cursor.y += font_get_glyph(font, cursor.x, cursor.y, ';').screen.size().y + 1;
    cursor.x = start_x; 
}

struct vertex_t {
    v3f pos;
    v3f nrm;
    color3 col;
    v2f tex;
};

struct skinned_vertex_t {
    v3f position;
    v3f normal;
    v2f tex_coord;
    // v3f tangent;
    // v3f bitangent;
    u32 bone_id[4];
    v4f weight{0.0f};
};

using index32_t = u32;

struct mesh_t {
    vertex_t*   vertices{nullptr};
    u32         vertex_count{0};
    index32_t*  indices{nullptr};
    u32         index_count{0};
};

struct mesh_view_t {
    u32 vertex_start{};
    u32 vertex_count{};
    u32 index_start{};
    u32 index_count{};
    u32 instance_start{};
    u32 instance_count{1};

    math::aabb_t<v3f> aabb{};
};

struct mesh_list_t {
    gfx::mesh_view_t* meshes{nullptr};
    size_t count{0};
};

// there should only be one running at a time

}; // namespace gfx

namespace utl {

struct profile_t {
    std::string_view name;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    // bool stopped = false;

    profile_t(std::string_view p_name) 
        : name(p_name) {
        start = std::chrono::high_resolution_clock::now();
    }
    
    ~profile_t() {
        // if (stopped) return;
        const auto stop = std::chrono::high_resolution_clock::now();
        const auto delta = stop - start;
        const auto m_time_delta = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
        const auto u_time_delta = std::chrono::duration_cast<std::chrono::microseconds>(delta).count();
        const auto n_time_delta = std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
        gen_profile(name, " {}{}", 
            m_time_delta ? m_time_delta : u_time_delta ? u_time_delta : n_time_delta,
            m_time_delta ? "ms" : u_time_delta ? "us" : "ns");
    }
    
    auto end() {
        // stopped = true;
        auto stop = std::chrono::high_resolution_clock::now();
        const auto n_time_delta = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        return n_time_delta;
    }
};



namespace rng {

//constexpr 
inline uint64_t fnv_hash_u64(uint64_t val) {
#if defined(GAME_USE_SIMD) && 0
    const u64 start = 14695981039346656037ull;
    __m256i hash = _mm256_set1_epi64x(start);
    const __m256i zero = _mm256_set1_epi64x(0);
    const __m256i val256 = _mm256_set1_epi64x(val);
    const __m256i prime = _mm256_set1_epi64x(1099511628211ull);
    const __m128i prime128 = _mm_set1_epi64x(1099511628211ull);
    __m256i v0 = _mm256_set_epi64x(0*8,1*8,2*8,3*8);
    __m256i v1 = _mm256_set_epi64x(4*8,5*8,6*8,7*8);

    v0 = _mm256_srlv_epi64(v0, val256);
    v1 = _mm256_srlv_epi64(v1, val256);

    v0 = _mm256_unpacklo_epi8(v0, zero);
    v1 = _mm256_unpacklo_epi8(v1, zero);

    hash = _mm256_xor_si256(hash, v0);
    hash = _mm256_mul_epu32(hash, prime);

    hash = _mm256_xor_si256(hash, v1);
    hash = _mm256_mul_epu32(hash, prime);

    __m128i lo = _mm256_extracti128_si256(hash, 0);
    __m128i hi = _mm256_extracti128_si256(hash, 1);

    lo = _mm_xor_si128(lo, hi);
    lo = _mm_mul_epu32(lo, prime128);
    lo = _mm_xor_si128(lo, _mm_srli_si128(lo, 8));
    lo = _mm_mul_epu32(lo, prime128);
    lo = _mm_xor_si128(lo, _mm_srli_si128(lo, 4));
    lo = _mm_mul_epu32(lo, prime128);
    lo = _mm_xor_si128(lo, _mm_srli_si128(lo, 2));
    lo = _mm_mul_epu32(lo, prime128);
    lo = _mm_xor_si128(lo, _mm_srli_si128(lo, 1));

    return _mm_cvtsi128_si64(lo);
    return start;
#else 
    u64 hash = 14695981039346656037ull;
    for (u64 i = 0; i < 8; i+=4) {
        const u64 v0 = ((val >> (8 * (i+0))) & 0xff);
        const u64 v1 = ((val >> (8 * (i+1))) & 0xff);
        const u64 v2 = ((val >> (8 * (i+2))) & 0xff);
        const u64 v3 = ((val >> (8 * (i+3))) & 0xff);
        hash = hash ^ v0;
        hash = hash * 1099511628211ull;
        hash = hash ^ v1;
        hash = hash * 1099511628211ull;
        hash = hash ^ v2;
        hash = hash * 1099511628211ull;
        hash = hash ^ v3;
        hash = hash * 1099511628211ull;
    }
    return hash;
#endif
}

struct xoshiro256_random_t {
    uint64_t state[4];
    constexpr static uint64_t max = std::numeric_limits<uint64_t>::max();

    void randomize() {
        constexpr u64 comp_time_entropy = sid(__TIME__) ^ sid(__DATE__);
        const u64 this_entropy = utl::rng::fnv_hash_u64((std::uintptr_t)this);
        const u64 pid_entropy = utl::rng::fnv_hash_u64(RAND_GETPID);
        
        if constexpr (sizeof(time_t) >= 8) {
            state[3] = static_cast<uint64_t>(time(0)) ^ comp_time_entropy;
            state[2] = (static_cast<uint64_t>(time(0)) << 17) ^ this_entropy;
            state[1] = (static_cast<uint64_t>(time(0)) >> 21) ^ (static_cast<uint64_t>(~time(0)) << 13);
            state[0] = (static_cast<uint64_t>(~time(0)) << 23) ^ pid_entropy;
        }
        else {
            state[3] = static_cast<uint64_t>(time(0) | (~time(0) << 32));
            state[2] = static_cast<uint64_t>(time(0) | (~time(0) << 32));
            state[1] = static_cast<uint64_t>(time(0) | (~time(0) << 32));
            state[0] = static_cast<uint64_t>(time(0) | (~time(0) << 32));
        }
    };

    uint64_t rand() {
        uint64_t *s = state;
        uint64_t const result = rol64(s[1] * 5, 7) * 9;
        uint64_t const t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;
        s[3] = rol64(s[3], 45);

        return result;
    }

    uint64_t rol64(uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }
};

struct xor64_random_t {
    uint64_t state;
    constexpr static uint64_t max = std::numeric_limits<uint64_t>::max();

    void randomize() {
        if constexpr (sizeof(time_t) >= 8) {
            constexpr u64 comp_time_entropy = sid(__TIME__) ^ sid(__DATE__);
            const u64 this_entropy = utl::rng::fnv_hash_u64((std::uintptr_t)this);
            const u64 pid_entropy = utl::rng::fnv_hash_u64(RAND_GETPID);
        
            state = static_cast<uint64_t>(time(0)) ^ comp_time_entropy ^ this_entropy ^ pid_entropy;
            state = utl::rng::fnv_hash_u64(state);
        }
        else {
            state = static_cast<uint64_t>(time(0) | (~time(0) << 32));
        }
    };

    uint64_t rand(){
        auto x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return state = x;
    }
};

struct xor32_random_t {
    uint32_t state;
    constexpr static uint32_t max = std::numeric_limits<uint32_t>::max();

    void randomize() {
        constexpr u64 comp_time_entropy = sid(__TIME__) ^ sid(__DATE__);
        const u64 this_entropy = utl::rng::fnv_hash_u64((std::uintptr_t)this);
        const u64 pid_entropy = utl::rng::fnv_hash_u64(RAND_GETPID);
    
        state = static_cast<uint32_t>(time(0)) ^ (u32)(comp_time_entropy ^ this_entropy ^ pid_entropy);
        state = (u32)utl::rng::fnv_hash_u64(state);
    };

    uint32_t rand(){
        auto x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state = x;
    }
};

template <typename Generator>
struct random_t {
    Generator rng;

    random_t(const decltype(Generator::state)& x = {}) noexcept {
        seed(x);
    }

    void randomize() {
        rng.randomize();
    }

    void seed(const decltype(Generator::state)& x) {
        if constexpr (std::is_integral_v<decltype(Generator::state)>) {
            if (x == 0) {
                randomize();
            } else {
                rng.state = x;
            }
        } else {
            randomize();
        }
    }

    auto rand() {
        return rng.rand();
    }

    float randf() {
        return float(rng.rand()) / float(Generator::max);
    }

    float randn() {
        return randf() * 2.0f - 1.0f;
    }

    template <typename T>
    T randv() {
        auto v = T{1.0f};
        for (decltype(T::length()) i = 0; i < T::length(); i++) {
            v[i] = randf();
        }
        return v;
    }

    template <typename T>
    T randnv() {
        auto v = T{1.0f};
        for (decltype(T::length()) i = 0; i < T::length(); i++) {
            v[i] = randn();
        }
        return v;
    }

    template <typename T>
    auto choice(const T& indexable) -> const typename T::value_type& {
        return indexable[rng.rand()%indexable.size()];
    }

    v3f aabb(const math::aabb_t<v3f>& box) {
        return v3f{
            (this->randf() * (box.max.x - box.min.x)) + box.min.x,
            (this->randf() * (box.max.y - box.min.y)) + box.min.y,
            (this->randf() * (box.max.z - box.min.z)) + box.min.z
        };
    }
};

struct random_s {
    inline static random_t<xor64_random_t> state{0};

    template <typename T>
    static auto choice(const T& indexable) -> const typename T::value_type& {
        return state.choice(indexable);
    }

    static void randomize() {
        state.randomize();
    }
    //random float
    static float randf() {
        return state.randf();
    }
    static uint64_t rand() {
        return state.rand();
    }
    // random normal
    static float randn() {
        return state.randn();
    }
    template <typename AABB>
    static auto aabb(const AABB& box) -> decltype(box.min) {
        return state.aabb(box);
    }
};

}; // namespace rng

namespace noise {

inline f32 
hash21(v2f in) {
    return glm::fract(
        glm::sin(glm::dot(in, v2f{12.9898f, 78.233f})
        * 43758.5453123f)
    );
}

inline f32 
noise21(v2f in) {
    const v2f i = glm::floor(in);
    const v2f f = glm::fract(in);

    const f32 a = hash21(i + v2f{0.0f, 0.0f});
    const f32 b = hash21(i + v2f{1.0f, 0.0f});
    const f32 c = hash21(i + v2f{0.0f, 1.0f});
    const f32 d = hash21(i + v2f{1.0f, 1.0f});

    const v2f u = f*f*(3.0f-2.0f*f);

    return glm::mix(a, b, u.x) +
        (c - a) * u.y * (1.0f - u.x) +
        (d - b) * u.x * u.y;
}

};

// todo(zack): add way to make different sized ones
static constexpr u64 hash_size = 0xffff;
using str_hash_t = u64[hash_size];
static constexpr u64 invalid_hash = ~0ui64;


inline void
str_hash_create(str_hash_t hash) {
    for (size_t i = 0; i < hash_size; i++) {
        hash[i] = invalid_hash;
    }
}

inline void
str_hash_add(
    str_hash_t hash, 
    std::string_view name, 
    u64 mesh_id,
    str_hash_t* meta = 0
) {
    sid_t str_id = sid(name);

    str_id = utl::rng::fnv_hash_u64(str_id);

    const size_t hash_code = str_id;

    size_t hash_index = hash_code % hash_size;
    
    // probe
    if (hash[hash_index] != invalid_hash) {
        if (meta) {
            while (meta[0][hash_index] != hash_code) {
                hash_index = (hash_index + 1) % hash_size;
            }
            if (meta[0][hash_index] == invalid_hash) {
                meta[0][hash_index] = hash_code;
                hash[hash_index] = mesh_id;
                return;
            }
        }
        assert(0 && "Collision not handled");
    }
    if (meta) {
        meta[0][hash_index] = hash_code;
    }
    hash[hash_index] = mesh_id;
}

inline u64
str_hash_find(str_hash_t hash, std::string_view name, str_hash_t* meta = 0) {
    sid_t str_id = sid(name);

    str_id = utl::rng::fnv_hash_u64(str_id);

    const size_t hash_code = str_id;

    size_t hash_index = hash_code % hash_size;
    
    if (hash[hash_index] != invalid_hash) {
        if (meta) {
            while (meta[0][hash_index] != hash_code) {
                hash_index = (hash_index + 1) % hash_size;
            }
            if (meta[0][hash_index] == invalid_hash) {
                return invalid_hash;
            }
        }
        return hash[hash_index];
    }

    return invalid_hash;
}

template <typename T>
struct deque {
private:
    T* head_{nullptr};
    T* tail_{nullptr};
    size_t size_{0};
public:

    // only call if you know what you are doing
    void clear() noexcept {
        head_ = tail_ = nullptr;
        size_ = 0;
    }

    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] bool is_empty() const noexcept {
        return head_ == nullptr;
    }

    T* front() noexcept {
        return head_;
    }

    const T* front() const noexcept {
        return head_;
    }

    T* back() noexcept {
        return tail_;
    }

    const T* back() const noexcept {
        return tail_;
    }

    T* operator[](size_t i) const {
        assert(i < size_);
        T* n = head_;
        for (size_t j = 0; j < i; j++) {
            n = n->next;
        }
        return n;
    }

    // this has a bug/endless loop possible

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

        for (T* n{head_->next}; n; n = n->next) {
            if (n == val) {
                if (val->next) {
                    val->next->prev = val->prev;
                }
                if (val->prev) {
                    val->prev->next = val->next;
                }

                val->next = val->prev = nullptr;
                size_--;
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
        size_--;
        old->next = old->prev = nullptr;
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
        size_--;
        old->next = old->prev = nullptr;
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
        size_++;
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
        size_++;
    }
};

template <typename T>
struct free_list_t {
    utl::deque<T> deque{};
    T* first_free{nullptr};
    size_t min_stack_pos{~static_cast<size_t>(0)};
    size_t count{0};
    size_t free_count{0};

    void clear() noexcept {
        free_count += count;
        count = 0;
        T* last = deque.back();
        if (last) {
            last->next = first_free;
            first_free = deque.front();
            deque.clear();
        }
    }

    T* alloc(arena_t* arena) {
        count += 1;
        if (first_free) { 
            T* node {nullptr};  
            node_pop(node, first_free);
            
            new (node) T();          

            deque.push_back(node);
            
            free_count -= 1;
            return node;
        } else {
            T* node = (T*)arena_alloc(arena, sizeof(T));
            new (node) T();
            
            deque.push_back(node);

            return node;
        }
    }

    void free_index(size_t _i) noexcept {
        T* c = deque.front();
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

        if (deque.front() == o) { // if node to free is first
            T* t{nullptr};
            // node_pop(t, head);

            node_push(t, first_free);
            o->~T();
            o = nullptr;
            count -= 1;
            free_count += 1;
            return;
        }
        T* c = deque.front(); // find node and free it, while keeping list connected
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


inline auto 
make_v3f(const auto& v) -> v3f {
    return {v.X, v.Y, v.Z};
}

namespace anim {

using bone_id_t = u32;

template <typename T>
struct keyframe {
    f32 time;
    T value;
};

template <typename T>
using anim_pool_t = pool_t<keyframe<T>>;

struct xform_t {
    v3f         position;
    glm::quat   orientation;
    v3f         scale;
};

struct bone_t {
    const char* name;
    u64 parent_index;
    xform_t*    xforms;
    u64     xform_count;
};

#define MAX_ANIMATION_BONES 128

struct animation_t {
    f32 duration{0.0f};
    i32 ticks_per_second{24};

    bone_t bones[MAX_ANIMATION_BONES];
    m44     matrices[MAX_ANIMATION_BONES];
};


template <typename T>
inline void
anim_pool_add_time_value(anim_pool_t<T>* pool, f32 time, T val) {
    auto* time_value = pool->allocate(1);
    *time_value = keyframe<T>{
        .time = time,
        .value = val
    };
}

template <typename T>
inline T
anim_pool_get_time_value(anim_pool_t<T>* pool, f32 time) {
    if (pool->count == 0) {
        return T{0.0f};
    }

    if (time <= 0.0f) {
        return (*pool)[0].value;
    } else if (time >= (*pool)[pool->count-1].time) {
        return (*pool)[pool->count-1].value;
    }

    for (size_t i = 0; i < pool->count - 1; i++) {
        if (pool->data()[i].time <= time && time <= pool->data()[i+1].time) {
            const auto d = (pool->data()[i+1].time - pool->data()[i].time);
            const auto p = time - pool->data()[i].time;
            if constexpr (std::is_same_v<T, glm::quat>) {
                return glm::slerp(pool[0][i], pool[0][i+1], p/d);
            } else {
                return glm::mix(
                    pool->data()[i].value,
                    pool->data()[i+1].value,
                    p/d
                );
            }
        }
    }

    return T{};
}


}; // namespace anim


template <typename T>
struct offset_pointer_t {
    i64 offset{0};

    const T* get() const {
        return offset ? (T*)((std::byte*)(&offset) + offset) : nullptr;
    }

    T* get() {
        return offset ? (T*)((std::byte*)(&offset) + offset) : nullptr;
    }

    [[nodiscard]] bool is_null() const noexcept {
        return offset == 0;
    }

    T* operator->() {
		return get();
    }
    
    const T* operator->() const {
		return get();
    }

    T& operator*() {
		return *get();
	}

    const T& operator*() const {
		return *get();
	}

    offset_pointer_t<T>& operator=(T* ptr) {
        offset = (std::byte*)(ptr) - (std::byte*)(&offset);
        return *this;
    }
    offset_pointer_t<T>& operator=(const T* ptr) {
        offset = (std::byte*)(ptr) - (std::byte*)(&offset);
        return *this;
    }
    offset_pointer_t<T>(const T* ptr)
        : offset{(std::byte*)(ptr) - (std::byte*)(&offset)}
    {
    }
    offset_pointer_t<T>(T* ptr)
        : offset{(std::byte*)(ptr) - (std::byte*)(&offset)}
    {
    }
    explicit offset_pointer_t(i64 o) : offset{o} {};
    // explicit offset_pointer_t(const offset_pointer_t<T>& o) : offset{o.offset} {};
    offset_pointer_t() = default;
    virtual ~offset_pointer_t() = default;

    offset_pointer_t(const offset_pointer_t<T>& o) {
        operator=(o.get());
    }
    offset_pointer_t(offset_pointer_t<T>&& o) {
        operator=(o.get());
    }
    offset_pointer_t& operator=(const offset_pointer_t<T>& o) {
        operator=(o.get());
        return *this;
    }
    offset_pointer_t& operator=(offset_pointer_t<T>&& o) {
        operator=(o.get());
        return *this;
    }
};

template <typename T>
struct offset_array_t {
    offset_pointer_t<T> data{};
    size_t count{0};

    offset_array_t() = default;
    explicit offset_array_t(arena_t* arena, size_t p_count) 
        : data{arena_alloc_ctor<T>(arena, p_count)}, count{p_count}
    {
    }

    explicit offset_array_t(T* p_data, size_t p_count) 
        : data{p_data}, count{p_count}
    {
    }

    const T& operator[](size_t i) const {
        return data.get()[i];
    }
    
    T& operator[](size_t i) {
        return data.get()[i];
    }
};

struct memory_blob_t {
    std::byte* data{0};

    std::byte* read_data() {
        return data + read_offset;
    }

    // in bytes
    size_t allocation_offset{0};
    size_t serialize_offset{0};

    size_t read_offset{0};

    explicit memory_blob_t(std::byte* storage)
    : data{storage}
    {
    }

    explicit memory_blob_t(arena_t* arena)
    : data{arena->start}
    {
    }

    void advance(const size_t bytes) noexcept {
        read_offset += bytes;
    }

    template <typename T>
    void allocate() {
        allocation_offset += sizeof(T);
    }

    void allocate(size_t bytes) {
        allocation_offset += bytes;
    }

    i64 get_relative_data_offset( void* o_data, size_t offset ) {
        // data_memory points to the newly allocated data structure to be used at runtime.
        const i64 data_offset_from_start = ( i64 )(data - ( std::byte* )o_data );
        const i64 data_offset = offset + data_offset_from_start;
        return data_offset;
    }


    template <typename T>
    void serialize(
        arena_t* arena, 
        const offset_pointer_t<T>& ptr
    ) {
        // allocate(sizeof(T));
        i64 data_offset = 8;// allocation_offset - serialize_offset;

        // gen_info("blob", "data_offset: {}", data_offset);
        
        serialize(arena, data_offset);

        u64 cached_serialize_offset = serialize_offset;

        // serialize_offset += 8;//allocation_offset;

        if (ptr.offset) {
            serialize(arena, *ptr.get());
        }

        // serialize_offset = cached_serialize_offset;
    }

    template <typename T>
    void serialize(arena_t* arena, const T& obj) {
        // note(zack): probably dont need to actually pass in arena, but it is safer(?) and more clear
        arena_alloc(arena, sizeof(T));

        std::memcpy(&data[serialize_offset], (std::byte*)&obj, sizeof(T));

        allocate<T>();
        serialize_offset += sizeof(T);
    }

    template <>
    void serialize(arena_t* arena, const std::string_view& obj) {
        // note(zack): probably dont need to actually pass in arena, but it is safer(?) and more clear
        serialize(arena, obj.size());

        arena_alloc(arena, obj.size());

        std::memcpy(&data[serialize_offset], (std::byte*)obj.data(), obj.size());

        allocate(obj.size());
        serialize_offset += obj.size();
    }

    template <typename T>
    void serialize(
        arena_t* arena, 
        const offset_array_t<T>& array
    ) {
        // allocate(8);
        //allocate(sizeof(T) * array.count);
        i64 data_offset = 8;// allocation_offset - serialize_offset;

        // gen_info("blob", "data_offset: {}", data_offset);
        
        serialize(arena, array.count);
        serialize(arena, data_offset);

        u64 cached_serialize_offset = serialize_offset;

        // serialize_offset += 8;// allocation_offset;
        
        for (u64 i = 0; i < array.count; ++i) {
            serialize(arena, array[i]);
        }
        
        // serialize_offset = cached_serialize_offset;
    }

    void reset() {
        allocation_offset = 0;
        serialize_offset = 0;
        read_offset = 0;
    }
    
    template <typename T>
    void deserialize(offset_pointer_t<T>* ptr) {
        ptr->offset = get_relative_data_offset(ptr, read_offset);
        
        read_offset += sizeof(T) + 8;
    }

    template <typename T>
    void deserialize(offset_array_t<T>* array) {
        array->count = deserialize<u64>();
        array->data.offset = get_relative_data_offset(&array->data, read_offset);
        
        read_offset += array->count * sizeof(T) + 8;
    }

    template <typename T>
    T deserialize() {
        const auto t_offset = read_offset;
        
        advance(sizeof(T));

        return *(T*)(data + t_offset);
    }   
    
    template<>
    std::string deserialize() {
        const size_t length = deserialize<u64>();
        std::string value((char*)&data[read_offset], length);
        advance(length);
        return value;

    }
};

namespace res {

namespace magic {
    constexpr u64 meta = 0xfeedbeeff04edead;
    constexpr u64 vers = 0x2;
    constexpr u64 mesh = 0x1212121212121212;
    constexpr u64 text = 0x1212121212121213;
    constexpr u64 table_start = 0x7abe17abe1;
};

struct resource_t {
    u64 size{0};
    std::byte* data{0};
};

struct resource_table_entry_t {
    string_t name;
    u64 file_type{0};
    u64 size{0};
};

struct pack_file_t {
    u64 meta{0};
    u64 vers{0};

    u64 file_count{0};
    u64 resource_size{0};

    u64 table_start{0};

    resource_table_entry_t* table{0};

    resource_t* resources{0};
};

inline pack_file_t* 
load_pack_file(
    arena_t* arena,
    arena_t* string_arena,
    std::string_view path
) {
    std::ifstream file{path.data(), std::ios::binary};

    if(!file.is_open()) {
        gen_error("res", "Failed to open file");
        return 0;
    }

    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::byte* data = arena_alloc(arena, size);
    file.read((char*)data, size);
    memory_blob_t loader{data};
    
    // eat meta info
    const auto meta = loader.deserialize<u64>();
    const auto vers = loader.deserialize<u64>();

#if !NDEBUG
    assert(meta == magic::meta);
    assert(vers == magic::vers);
#endif

    pack_file_t* packed_file = arena_alloc_ctor<pack_file_t>(arena, 1);

    packed_file->file_count = loader.deserialize<u64>();
    packed_file->resource_size = loader.deserialize<u64>();

#if NDEBUG
    loader.deserialize<u64>();
#else
    assert(loader.deserialize<u64>()== magic::table_start);
#endif

    // need to load strings into string_t
    packed_file->table = arena_alloc_ctor<resource_table_entry_t>(arena, packed_file->file_count);
    size_t table_size = sizeof(resource_table_entry_t) * packed_file->file_count;
    loader.deserialize<u64>();
    for (size_t i = 0; i < packed_file->file_count; i++) {
        resource_table_entry_t* entry = packed_file->table + i;
        std::string file_name = loader.deserialize<std::string>();
        entry->name.own(string_arena, file_name.c_str());
        entry->file_type = loader.deserialize<u64>();
        entry->size = loader.deserialize<u64>();
    }
    
    packed_file->resources = arena_alloc_ctor<resource_t>(arena, packed_file->file_count);
    loader.deserialize<u64>();
    for (size_t i = 0; i < packed_file->file_count; i++) {
        size_t resource_size = packed_file->resources[i].size = loader.deserialize<u64>();
        packed_file->resources[i].data = arena_alloc(arena, packed_file->resources[i].size);
        std::memcpy(packed_file->resources[i].data, loader.read_data(), resource_size);
        loader.advance(resource_size);
    }

    gen_info("res", "Loaded Resource File: {}", path);
    gen_info("res", "Asset File contains: {} files", packed_file->file_count);
    for (size_t i = 0; i < packed_file->file_count; i++) {
        gen_info("res", "\tfile: {} - {}", packed_file->table[i].name.c_str(), packed_file->table[i].size);
    }

    return packed_file;
}

size_t pack_file_find_file(
    pack_file_t* pack_file,
    std::string_view file_name
) {
    for (size_t i = 0; i < pack_file->file_count; i++) {
        if (pack_file->table[i].name.c_str() == file_name) {
            return i;//pack_file->table[i].offset;
        }
    }
    gen_warn("pack_file", "Failed to find file: {}", file_name);
    return ~0ui32;
}

u64 pack_file_get_file_size(
    pack_file_t* pack_file,
    std::string_view file_name
) {
    for (size_t i = 0; i < pack_file->file_count; i++) {
        if (pack_file->table[i].name.c_str() == file_name) {
            return pack_file->table[i].size;
        }
    }
    gen_warn("pack_file", "Failed to find file: {}", file_name);
    return 0;
}

std::byte* pack_file_get_file(
    pack_file_t* pack_file,
    size_t index
) {
    return pack_file->resources[index].data;
}

std::byte* pack_file_get_file_by_name(
    pack_file_t* pack_file,
    std::string_view file_name
) {
    const auto file_id = pack_file_find_file(pack_file, file_name);
    if (~0ui32 == file_id) {
        return nullptr;
    }
    return pack_file->resources[file_id].data;
}

}; // namespace res 



}; // namespace utl

