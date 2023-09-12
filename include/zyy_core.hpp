#pragma once

// note(zack): this file is the sole interface between the app and the platform layer

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/format.h>

#define fmt_str(...) (fmt::format(__VA_ARGS__))
// #define fmt_sv(...) (std::string_view{fmt::format(__VA_ARGS__)})
#define fmt_sv fmt::format
#define println(...) do { fmt::print(__VA_ARGS__); } while(0)
#define zyy_info(cat, str, ...) do { fmt::print(fg(fmt::color::white) | fmt::emphasis::bold, fmt_str("[info][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define zyy_warn(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::yellow) | fmt::emphasis::bold, fmt_str("[warn][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define zyy_error(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::red) | fmt::emphasis::bold, fmt_str("[error][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define zyy_profile(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::blue) | fmt::emphasis::bold, fmt_str("[profile][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)

#if defined(GAME_USE_SIMD)
#include <immintrin.h>
#endif

#define WIN32_MEAN_AND_LEAN
#define NOMINMAX

#if defined(RAND_GETPID)

#elif defined(_WIN64) || defined(_WIN32)
    #include <process.h>


#if ZYY_INTERNAL

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
    

bool check_for_debugger()
{
    return IsDebuggerPresent();
}

#else

bool check_for_debugger()
{
    return false;
}

#endif
    
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

using namespace std::string_view_literals;

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

using b8 = u8;
using b16 = u16;
using b32 = u32;
using b64 = u64;
using umm = size_t;


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
using v3u = glm::uvec3;
using v4i = glm::ivec4;

using quat = glm::quat;
using m22 = glm::mat2x2;
using m33 = glm::mat3x3;
using m34 = glm::mat3x4;
using m43 = glm::mat4x3;
using m44 = glm::mat4x4;


template<>
struct fmt::formatter<v3f>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(v3f const& v, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "[{:.2f}, {:.2f}, {:.2f}]", v.x, v.y, v.z);
    }
};

// why do I have two of these?
template<typename T>
struct fmt::formatter<glm::vec<3, T>>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(glm::vec<3, T> const& v, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}, {}]", v.x, v.y, v.z);
    }
};


namespace axis {
    constexpr inline static v3f up          {0.0f, 1.0f, 0.0f};
    constexpr inline static v3f down        {0.0f,-1.0f, 0.0f};
    constexpr inline static v3f right       {1.0f, 0.0f, 0.0f};
    constexpr inline static v3f left        {-1.0f,0.0f, 0.0f};
    constexpr inline static v3f forward     {0.0f, 0.0f,-1.0f};
    constexpr inline static v3f backward    {0.0f, 0.0f, 1.0f};
};

namespace planes {
    constexpr inline static v3f xy {axis::right + axis::up};
    constexpr inline static v3f yz {axis::backward + axis::up};
    constexpr inline static v3f xz {axis::right + axis::backward};

    constexpr inline static v3f yx {axis::right + axis::up};
    constexpr inline static v3f zy {axis::backward + axis::up};
    constexpr inline static v3f zx {axis::right + axis::backward};
};

struct arena_settings_t {
    size_t alignment{0};
    size_t minimum_block_size{0};
    u8     dynamic{0};
};

struct arena_t {
    std::byte* start{0};
    size_t top{0};
    size_t size{0};

    arena_settings_t settings;
};

struct frame_arena_t {
    arena_t arena[2];
    u32     active{0};

    arena_t& get() {
        return arena[active%2];
    }
    arena_t& last() {
        return arena[!(active%2)];
    }
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
#define align_2n(val, size) ((val + size-1) & ~(size-1))


#define to_secs(x) (f32(x/1000))
// #define co_lerp(coro, val, start, end, duration) do { if (coro->start_time == 0) { coro->start_time = coro->now; } val = glm::mix(start, end, (coro->now - coro->start_time)/duration); co_yield_until(coro,  coro->now > (coro->start_time) + (f32)duration); coro->start_time = 0; } while(0)

struct coroutine_t {
	const f32& now;
	void* data;
	u64 line;
	f32 start_time;
    u32 running{1};
    u8* stack{0};
    u64 stack_size{0};
    u64 stack_top{0};
};

#define co_begin(coro) switch (coro->line) {case 0: coro->line = 0; coro->running=1;
#define co_yield(coro) do { coro->line = __LINE__; return; case __LINE__:;} while(0)
#define co_yield_until(coro, condition) while (!(condition)) { co_yield(coro); }
#define co_wait(coro, duration) do {if (coro->start_time == 0) \
    { coro->start_time = coro->now; } \
    co_yield_until(coro, coro->now > (coro->start_time) + (f32)duration);\
    coro->start_time = 0; } while (0)
#define co_end(coro) do { coro->line = __LINE__; coro->running = 0; coro->stack = 0;coro->stack_size = coro->stack_top = 0; } while (0); }
#define co_reset(coro) do { coro->line = 0; } while (0); }
#define co_set_label(coro, label) case label:
#define co_goto_label(coro, label) do { coro->line = (label); return; } while(0)
#define co_lerp(coro, value, start, end, duration, func) \
do { \
    coro->start_time = coro->now;\
    while (coro->now < coro->start_time + (duration)) { \
        (value) = (func)((start), (end), ((coro->now - coro->start_time)/(duration))); \
        co_yield(coro); \
    } \
    (value) = (end); \
} while (0)

// #define co_is_finished(coro) (coro->line == -1)
#define loop_iota_u64(itr, stop) for (size_t itr = 0; itr < stop; itr++)
#define range_u32(itr, start, stop) for (u32 itr = (start); itr < (stop); itr++)
#define range_u64(itr, start, stop) for (size_t itr = (start); itr < (stop); itr++)
#define range_f32(itr, start, stop) for (f32 itr = (start); itr < (stop); itr++)
#define loop_iota_i32(itr, stop) for (int itr = 0; itr < stop; itr++)

struct closure_t {
    void (*func)(void*){0};
    void* data{0};

    operator bool() const {
        return func != nullptr;
    }

    template <typename ... Args>
    void operator()(Args ... args) {
        dispatch(std::forward<Args>(args)...);
    }

    template <typename ... Args>
    void dispatch(Args&& ... args) {
        ((void(*)(void*, Args...))func)(data, std::forward<Args>(args)...);
    }

    template <typename ReturnType, typename ... Args>
    ReturnType dispatch_request(Args&& ... args) {
        return ((ReturnType(*)(void*, Args...))func)(data, std::forward<Args>(args)...);
    }
};

inline u32
safe_truncate_u64(u64 value) {
    assert(value <= 0xFFFFFFFF);
    const u32 result = static_cast<u32>(value);
    return result;
}

constexpr u64 
BIT(u64 x) {
	return 1ui64 << x;
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

        // preserved, everything above here is reset every frame    
        f32 delta[2];
        i32 scroll[2];
    } mouse;
    
    f32 render_time;
    f32 render_dt;

    char keyboard_input() noexcept {
        const bool shift = keys[key_id::LEFT_SHIFT] || keys[key_id::RIGHT_SHIFT];
        const char* num_lut=")!@#$%^&*(";
        range_u64(c, key_id::SPACE, key_id::GRAVE_ACCENT+1) {
            if (pressed.keys[c]) { 
                pressed.keys[c] = 0;
                if (shift) {
                    if (c >= key_id::NUM_0 && c <= key_id::NUM_9) {
                        return num_lut[c-key_id::NUM_0];
                    }
                    return (char)c;
                } else {
                    return (char)std::tolower((char)c); 
                }
            }
        }
        if (pressed.keys[key_id::ENTER]) return -1;
        return 0;
    }

};

inline void
app_input_reset(app_input_t* input) {
    std::memset(input, 0, sizeof(app_input_t) - 6 * sizeof(i32));
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


#if ZYY_INTERNAL


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

    u16         set_breakpoint{0};
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
    void*       user_data{0};
    const char* fmt_str{"{}"};
};

#define MAX_DEBUG_EVENT_COUNT (16*65536)
#define MAX_DEBUG_RECORD_COUNT (256)
#define MAX_USER_DATA_SIZE (megabytes(64))

#include <atomic>
struct debug_table_t {
    std::atomic<u64> event_index{0};
    debug_event_t   events[MAX_DEBUG_EVENT_COUNT];

    u64             record_count{0};
    debug_record_t  records[MAX_DEBUG_RECORD_COUNT];

    // std::atomic<u64> user_top{0};
    // std::byte        user_data[MAX_USER_DATA_SIZE];

};

extern debug_table_t gs_debug_table;

inline void
record_debug_event(int record_index, debug_event_type event_type) {
    u64 index = gs_debug_table.event_index;
    gs_debug_table.event_index = (gs_debug_table.event_index + 1) % MAX_DEBUG_EVENT_COUNT;
    assert(index < MAX_DEBUG_EVENT_COUNT);
    debug_event_t* event = gs_debug_table.events + index;
    event->clock_count = get_nano_time();
    event->core_index = 0; // todo
    event->thread_id = 0; // todo
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

        if (record->set_breakpoint && check_for_debugger()) {
            __debugbreak();
        }
    }

    ~timed_block_t() {
        record->history[record->hist_id] = record->cycle_count = get_nano_time() - record->cycle_count;
        record->hist_id = (record->hist_id + 1) % array_count(record->history);

        record_debug_event(counter, DebugEventType_EndBlock);
    }
};

#else 

#define TIMED_BLOCK(Name)
#define TIMED_FUNCTION

#endif

#include "zyy_physics.hpp"

struct platform_api_t {
    using allocate_memory_function = void*(*)(umm);
    using free_memory_function = void(*)(void*);

    allocate_memory_function allocate;
    free_memory_function free;

    using message_box_function = i32(*)(const char*);
    message_box_function message_box{0};

    struct audio_closures_t {
        closure_t load_sound;
        closure_t play_sound;
        closure_t stop_sound;
    } audio;
};

extern platform_api_t Platform;

struct game_memory_t {
    platform_api_t platform;

    arena_t arena;

    app_config_t config;
    app_input_t input;

    b32 running{true};

    physics::api_t* physics{nullptr};
};

using app_func_t = void(__cdecl *)(game_memory_t*);

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

struct window_t {
    u32 size[2];

    void* native_handle{nullptr};
    void* api_handle{nullptr};

    app_func_t on_update{nullptr};
    app_func_t on_render{nullptr};

    v2f sizef() const {
        return v2f{(f32)size[0],(f32)size[1]};
    }
};

// nodes

#define node_next(node) (node = node->next)
#define node_push(node, list) do { node->next = list; list = node; } while(0)
#define node_pop(node, list) do { node = list; list = list->next; } while(0)
#define node_last(list) do { while (list->next && list=list->next);} while(0)

#define node_for(type, node, itr) for(type* itr = (node); (itr); node_next(itr))

#define dlist_insert(sen, ele) \
    (ele)->next = (sen)->next;\
    (ele)->prev = (sen);\
    (ele)->next->prev = (ele);\
    (ele)->prev->next = (ele);

#define dlist_insert_as_last(sen, ele) \
    (ele)->next = (sen);\
    (ele)->prev = (sen)->prev;\
    (ele)->next->prev = (ele);\
    (ele)->prev->next = (ele);

#define dlist_init(sen) \
    (sen)->next = (sen);\
    (sen)->prev = (sen);

#define freelist_allocate(result, freelist, alloc_code)\
    (result) = (freelist);\
    if (result) { freelist = (result)->next_free;} else { result = alloc_code;}
#define freelist_deallocate(ptr, freelist) \
    if (ptr) { (ptr)->next_free = (freelist); (freelist) = (ptr); }

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

namespace utl {

std::string_view get_extension(std::string_view str) {
    return str.substr(str.find_last_of('.') + 1);
}

bool has_extension(std::string_view str, std::string_view ext) {
    return get_extension(str) == ext;
}

struct spinlock_t {
  std::atomic<bool> lock_ = {0};

  void lock() noexcept {
    for (;;) {
      // Optimistically assume the lock is free on the first try
      if (!lock_.exchange(true, std::memory_order_acquire)) {
        return;
      }
      // Wait for lock to be released without generating cache misses
      while (lock_.load(std::memory_order_relaxed)) {
        // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
        // hyper-threads
        _mm_pause();
      }
    }
  }

  bool try_lock() noexcept {
    // First do a relaxed load to check if lock is free in order to prevent
    // unnecessary cache misses if someone does while(!try_lock())
    return !lock_.load(std::memory_order_relaxed) &&
           !lock_.exchange(true, std::memory_order_acquire);
  }

  void unlock() noexcept {
    lock_.store(false, std::memory_order_release);
  }
};

inline constexpr u64
memdif(const u8* dst, const u8* src, size_t size) {
    // return std::memcmp(dst, src, size); // not constexpr
    u64 dif = 0;
    for (size_t i = 0; i < size; i++) {
        dif += dst[i] != src[i];
    }
    return dif;
}

inline void
memzero(void* buffer, umm size) {
    std::memset(buffer, 0, size);
}

}; // namespace utl


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

inline void
arena_begin_sweep(arena_t* arena) {
    arena->top = 0;
}

inline void arena_sweep_keep(arena_t* arena, std::byte* one_past_end) {
    umm size = one_past_end - arena->start;
    arena->top = std::max(size, arena->top);
}

inline std::byte*
arena_get_top(arena_t* arena) {
    return arena->start + arena->top;
}

#define arena_display_info(arena, name) \
    (fmt_sv("{}: {} {} / {} {} - {:.2}%", \
    (name), \
    (arena)->top > gigabytes(1) ? (arena)->top/gigabytes(1) : (arena)->top > megabytes(1) ? (arena)->top/megabytes(1) : (arena)->top > kilobytes(1) ? (arena)->top / kilobytes(1) : (arena)->top,\
    (arena)->top > gigabytes(1) ? "Gb" : (arena)->top > megabytes(1) ? "Mb" : (arena)->top > kilobytes(1) ? "Kb" : "B",\
    (arena)->size > gigabytes(1) ? (arena)->size/gigabytes(1) : (arena)->size > megabytes(1) ? (arena)->size/megabytes(1) : (arena)->size > kilobytes(1) ? (arena)->size / kilobytes(1) : (arena)->size,\
    (arena)->size > gigabytes(1) ? "Gb" : (arena)->size > megabytes(1) ? "Mb" : (arena)->size > kilobytes(1) ? "Kb" : "B",\
    ((f64((arena)->top) / f64((arena)->size)) * 100.0)))
    
template <typename T>
inline T*
arena_alloc(arena_t* arena) {
    std::byte* p_start = arena->start + arena->top;
    arena->top += sizeof(T);

    new (p_start) T();

    assert(arena->top <= arena->size && "Arena overflow");

    return (T*)p_start;
}

template <typename T>
inline T*
arena_alloc(arena_t* arena, size_t count) {
    T* first = arena_alloc<T>(arena);

    for(size_t i = 1; i < count; i++) {
        arena_alloc<T>(arena);
    }

    return first;
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

arena_t* co_stack(coroutine_t* coro, frame_arena_t& frame_arena) {
    // if (coro->running == false) return 0;
    if (coro->stack) {
        auto* stack = (u8*)arena_alloc(&frame_arena.get(), coro->stack_size);
        std::memcpy(stack, coro->stack, coro->stack_size);
        coro->stack = stack;
        coro->stack_size = coro->stack_top = 0;
        return 0;
    } else {
        coro->stack_top = 0;
        coro->stack_size = 0;
        coro->stack = (u8*)arena_get_top(&frame_arena.get());
        return &frame_arena.get();
    }    
}

template <typename T>
T* co_push_stack_(coroutine_t* coro, arena_t* arena, umm count = 1) {
    // if (coro->running == false) return 0;
    T* data = (T*)(coro->stack + coro->stack_top);
    coro->stack_top += sizeof(T) * count;
    coro->stack_size += sizeof(T) * count;
    if (arena) { arena_alloc<T>(arena, count); }
    return data;
}

#define co_push_stack(coro, stack, type) co_push_stack_<type>(coro, stack) 
#define co_push_array_stack(coro, stack, type, count) co_push_stack_<type>(coro, stack, count) 


namespace utl {

template <typename T>
struct dynarray_t {
    dynarray_t(const dynarray_t& o) = delete;
    dynarray_t(dynarray_t&& o) = delete;
    dynarray_t& operator=(const dynarray_t& o) = delete;
    dynarray_t& operator=(dynarray_t&& o) = delete;
    constexpr dynarray_t() = default;
    constexpr dynarray_t(size_t size, const T& value = T()) {
        resize(size, value);
    }
    constexpr dynarray_t(std::span<T> in) {
        data = new T[in.size()];
        count = capacity = in.size();
        std::copy(data, in.data(), sizeof(T)*count);
    }

    T* begin() {
        return data;
    }

    T* end() {
        return data + capacity;
    }

    T* cbegin() const {
        return data;
    }

    T* cend() const {
        return data;
    }

    T& front() {
        assert(data&&count);
        return *data;
    }

    T& back() {
        assert(data&&count);
        return *(data+count-1);
    }

    bool is_empty() const {
        return count==0;
    }

    void clear() {
        for(size_t i = 0; i < count; i++) {
            (data+i)->~T();
        }
        count = 0;
    }

    void reserve(size_t elements) {
        if (data) {
            T* t = new T[elements];
            if (count) {
                std::copy(data, data+count, t);
            }
            delete [] data;
            data = t;
            capacity = elements;
        } else {
            data = new T[elements];
            capacity = elements;
        }
    }

    void pop() {
        assert(count);
        back().~T();
        count--;
    }

    T& push_back(const T& x) {
        if (capacity==0) {
            reserve(2);
        } else if (count>=capacity) {
            reserve(capacity<<1);
        }

        return data[count++] = x;
    }

    bool find(const T& o, size_t* index) const {
        for (size_t i = 0; i < count; i++) {
            if (data[i]==o) {
                *index = i;
                return true;
            }
        }
        return false;
    }

    void remove(size_t index) {
        assert(index < count);
        (data+index)->~T();
        data[index] = (data[count--]);
    }

    void resize(size_t size, const T& value = T()) {
        if (size > capacity) {
            reserve(size);
        }
        if (size < count) {
            range_u64(i, size, count-1) {
                data[i].~T();
            }
            count = size;
        } else if (size > count) {
            for (size_t i = count; i < size; i++) {
                new (data+i) T(value);
            }
            count = size;
        }
    }

    size_t size() const noexcept {
        return count;
    }

    size_t buffer_size() const noexcept {
        return capacity;
    }

    const T& operator[](size_t index) const {
        return data[index];
    }

    T& operator[](size_t index) {
        return data[index];
    }

    std::span<T> view() const noexcept {
        return std::span<T>{data, count};
    }

    std::span<T> view_all() const noexcept {
        return std::span<T>{data, capacity};
    }

    constexpr ~dynarray_t() noexcept {
        delete [] data;
    }

private:
    size_t count{0};
    size_t capacity{0};
    T* data{0};
};

template <typename T>
struct pool_t : public arena_t {
    size_t count() const {
        return top/sizeof(T);
    }
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
        return (T*)arena_alloc(this, sizeof(T) * p_count);
    }

    void free(T* ptr) {
        ptr->~T();
        std::swap(*ptr, back());
        count--;
    }

    // note(zack): calls T dtor
    void clear() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < count; i++) {
                (data() + i)->~T();
            }
        }
        arena_clear(this);
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
    STRING_VIEW,
    VOID_STAR,
    ARRAY,
    UNKNOWN,
};

namespace reflect {
    struct type_info_t {
        sid_t       type_id{};
        size_t      size{};
        const char* name{};
    };

    struct type_t;
    struct object_t {
        const type_t* type{0};
        std::byte*   data{0};
        size_t       count{0};
        type_tag     tag{type_tag::UNKNOWN};
    };

    
    struct property_t {
        const type_t* type{0};
        std::string_view name{};
        std::size_t size{0};
        std::size_t offset{0};

        constexpr bool valid() {
            return name.empty() == false;
        }

        template<typename Obj, typename Value>
        void set_value(Obj& obj, Value v) const noexcept {
            std::memcpy((std::byte*)&obj + offset, &v, sizeof(Value));
        }
        
        template<typename Value>
        void set_value_raw(std::byte* obj, Value v) const noexcept {
            std::memcpy(obj + offset, &v, sizeof(Value));
        }

        
        // read value from obj into memory
        object_t get_value_raw(std::byte* obj, std::byte* memory) const noexcept {
            object_t var{type, memory};
            std::memcpy(var.data, obj + offset, size);
            return var;
        }

        template<typename Obj>
        object_t get_value(Obj& obj, std::byte* memory) const noexcept {
            object_t var{type, memory};
            std::memcpy(var.data, ((std::byte*)&obj + offset), size);
            return var;
        }
    };
    struct method_t {
        using function_ptr_t = void (*)(void);
        
        const type_t* function_type{0};
        std::string_view name{};
        std::size_t arg_count{0};
        function_ptr_t method{0};

        template <typename RT, typename O, typename ... Args>
        RT invoke(object_t obj, Args&& ... args) {
            using cast_fn_t = RT(*)(O, Args...);
            cast_fn_t cast = reinterpret_cast<cast_fn_t>(method);
            return cast((O*)obj.data, std::forward<Args>(args)...);
        }
    };

    struct type_t {
        std::string_view name;
        size_t size;
        type_tag tags;

        size_t property_count{};
        property_t properties[32];
        size_t method_count{};
        method_t methods[32];

        constexpr type_t& add_property(const property_t& prop) {
            assert(property_count < array_count(properties));
            properties[property_count++] = prop;
            return *this;
        }
        constexpr type_t& add_dyn_property(const property_t& prop) {
            assert(property_count < array_count(properties));
            properties[property_count++] = prop;
            properties[property_count++].offset = size;
            size += prop.size;
            return *this;
        }
        constexpr type_t& add_method(method_t method) {
            assert(method_count < array_count(methods));
            methods[method_count++] = method;
            return *this;
        }
        
        constexpr property_t get_property(std::string_view prop) const {
            range_u64(i, 0, property_count) {
                if (prop == properties[i].name) {
                    return properties[i];
                }
            }
            return {};
        }
        constexpr method_t get_method(std::string_view method) const {
            range_u64(i, 0, method_count) {
                if (method == methods[i].name) {
                    return methods[i];
                }
            }
            return {};
        }

        constexpr bool operator==(const type_t& o) const {
            return name == o.name;
        }

        template <typename T>
        static constexpr type_tag get_tags() {
            type_tag tag;
            if constexpr (std::is_same_v<T, u8> ) {tag=type_tag::UINT8;} else
            if constexpr (std::is_same_v<T, u16>) {tag=type_tag::UINT16;} else
            if constexpr (std::is_same_v<T, u32>) {tag=type_tag::UINT32;} else
            if constexpr (std::is_same_v<T, u64>) {tag=type_tag::UINT64;} else
            if constexpr (std::is_same_v<T, i8>) {tag=type_tag::INT8;} else
            if constexpr (std::is_same_v<T, i16>) {tag=type_tag::INT16;} else
            if constexpr (std::is_same_v<T, i32>) {tag=type_tag::INT32;} else
            if constexpr (std::is_same_v<T, i64>) {tag=type_tag::INT64;} else
            if constexpr (std::is_same_v<T, f32>) {tag=type_tag::FLOAT32;} else
            if constexpr (std::is_same_v<T, f64>) {tag=type_tag::FLOAT64;} else
            if constexpr (std::is_same_v<T, std::string_view>) {tag=type_tag::STRING_VIEW;} else
            if constexpr (std::is_same_v<T, std::string>) {tag=type_tag::STRING_VIEW;} else
            if constexpr (std::is_same_v<T, char*>) {tag=type_tag::STRING;} else
            if constexpr (std::is_same_v<T, const char*>) {tag=type_tag::STRING;} else
            if constexpr (std::is_same_v<T, const char* const>) {tag=type_tag::STRING;} else
            if constexpr (std::is_pointer_v<T>) {tag=type_tag::VOID_STAR;} else
            if constexpr (std::is_array_v<T>) {tag=type_tag::ARRAY;} else
            {tag=type_tag::UNKNOWN;} 
            return tag;
        }
    };

    template <typename T>
    struct type : std::false_type {
        using type_of = T;
        static constexpr type_t info{};
    };

#define REFLECT_TYPE_INFO(ttype) .name = #ttype, .size = sizeof(ttype), .tags = type_t::get_tags<ttype>()
#define REFLECT_TYPE(ttype) template<> struct reflect::type<ttype> : std::true_type { static constexpr reflect::type_t info = reflect::type_t
#define REFLECT_TYPE_(ttype) REFLECT_TYPE(ttype) {REFLECT_TYPE_INFO(ttype)}
#define REFLECT_TRY_TYPE(ttype) reflect::type<ttype>{}?&reflect::type<ttype>::info:nullptr
#define REFLECT_FUNC(ttype, func, args) add_method(reflect::method_t{REFLECT_TRY_TYPE(decltype(func)), #func, args, (void (*)(void))&func})
#define REFLECT_PROP(ttype, prop) add_property(reflect::property_t{reflect::type<decltype(ttype::prop)>{}?&reflect::type<decltype(ttype::prop)>::info:nullptr, #prop, sizeof(ttype::prop), offsetof(ttype, prop)})
#define REFLECT_DYN_PROP(ttype, prop, size, offset) add_dynamic_property(reflect::property_t{reflect::type<decltype(ttype::prop)>{}?&reflect::type<decltype(ttype::prop)>::info:nullptr, #prop, size})

};
using reflect::type_info_t;
template<>
struct fmt::formatter<reflect::object_t> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(reflect::object_t const& o, FormatContext& ctx) {
        switch (o.type->tags) {
            case type_tag::UINT8:         return fmt::format_to(ctx.out(), "{}", *(u8*)o.data);
            case type_tag::UINT16:        return fmt::format_to(ctx.out(), "{}", *(u16*)o.data);
            case type_tag::UINT32:        return fmt::format_to(ctx.out(), "{}", *(u32*)o.data);
            case type_tag::UINT64:        return fmt::format_to(ctx.out(), "{}", *(u64*)o.data);
            case type_tag::INT8:          return fmt::format_to(ctx.out(), "{}", *(i8*)o.data);
            case type_tag::INT16:         return fmt::format_to(ctx.out(), "{}", *(i16*)o.data);
            case type_tag::INT32:         return fmt::format_to(ctx.out(), "{}", *(i32*)o.data);
            case type_tag::INT64:         return fmt::format_to(ctx.out(), "{}", *(i64*)o.data);
            case type_tag::FLOAT32:       return fmt::format_to(ctx.out(), "{}", *(f32*)o.data);
            case type_tag::FLOAT64:       return fmt::format_to(ctx.out(), "{}", *(f64*)o.data);
            case type_tag::STRING:        return fmt::format_to(ctx.out(), "{}", std::string_view{(const char*)o.data, o.count});
            case type_tag::STRING_VIEW:   return fmt::format_to(ctx.out(), "{}", *(std::string_view*)o.data);
            case type_tag::VOID_STAR:     return fmt::format_to(ctx.out(), "obj: {}", (void*)o.data);
            default:                      return fmt::format_to(ctx.out(), "<unknown>");
        }
    }
};


REFLECT_TYPE_(bool);};
REFLECT_TYPE_(f32);};
REFLECT_TYPE_(u64);};

REFLECT_TYPE(v3f) {
    REFLECT_TYPE_INFO(v3f)
    }
    .REFLECT_PROP(v3f, x)
    .REFLECT_PROP(v3f, y)
    .REFLECT_PROP(v3f, z);
};

#define GEN_TYPE_ID(type) (sid(#type))
#define GEN_TYPE_INFO(type) (type_info_t{sid(#type), sizeof(type), #type})
#define GEN_REFLECT_VAR(type, var) size_t offset{offsetof(type, var)}; const char* name{#var};
#define GEN_REFLECT_INFO(type, var, ...) struct VAR_CAT_(type, VAR_CAT(var, _info)) { GEN_REFLECT_VAR(type, var) }

struct any_t {
    

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


namespace swizzle {

    constexpr v2f xx(v2f v) { return v2f{v.x,v.x}; }
    constexpr v2f yx(v2f v) { return v2f{v.y,v.x}; }
    constexpr v2f yy(v2f v) { return v2f{v.y,v.y}; }
    
    constexpr v2f xy(v3f v) { return v2f{v.x,v.y}; }
    constexpr v2f xx(v3f v) { return v2f{v.x,v.x}; }
    constexpr v2f yx(v3f v) { return v2f{v.y,v.x}; }
    constexpr v2f yy(v3f v) { return v2f{v.y,v.y}; }
    constexpr v2f zy(v3f v) { return v2f{v.z,v.y}; }
    constexpr v2f yz(v3f v) { return v2f{v.y,v.z}; }
    constexpr v2f xz(v3f v) { return v2f{v.x,v.z}; }
    constexpr v2f zx(v3f v) { return v2f{v.z,v.x}; }
};

namespace math {
    constexpr v2f from_polar(f32 angle, f32 radius = 1.0f) noexcept {
        return v2f{glm::cos(angle), glm::sin(angle)} * radius;
    }

    template <typename T>
    constexpr T remap01(T a, T b, T x) {
        return (x-a)/(b-a);
    }

    constexpr v2f octahedral_mapping(v3f co) {
        using namespace swizzle;
        // projection onto octahedron
        co /= glm::dot( v3f(1.0f), glm::abs(co) );

        // out-folding of the downward faces
        if ( co.y < 0.0f ) {
            co = v3f(1.0f - glm::abs(zx(co)) * glm::sign(xz(co)), co.z);
        }

        // mapping to [0;1]ˆ2 texture space
        return xy(co) * 0.5f + 0.5f;
    }

    constexpr v3f octahedral_unmapping(v2f co) {
        using namespace swizzle;
        co = co * 2.0f - 1.0f;

        v2f abs_co = glm::abs(co);
        v3f v = v3f(co, 1.0f - (abs_co.x + abs_co.y));

        if ( abs_co.x + abs_co.y > 1.0f ) {
            v = v3f{(glm::abs(yx(co)) - 1.0f) * -glm::sign(co), v.z};
        }

        return v;
    }

    constexpr v3f get_spherical(f32 yaw, f32 pitch) {
        return v3f{
            glm::cos((yaw)) * glm::cos((pitch)),
            glm::sin((pitch)),
            glm::sin((yaw)) * glm::cos((pitch))
        };
    }

    v3f world_to_screen(const m44& proj, const m44& view, const v4f& viewport, const v3f& p) noexcept {
        return glm::project(p, view, proj, viewport);
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

    v3f world_normal_to_screen(const m44& vp, const v3f& p) noexcept {
        return world_to_screen(vp, v4f{p, 0.0f});
    }

    bool fcmp(f32 a, f32 b, f32 eps = 1e-6) {
        return std::fabsf(a-b) < eps;
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

        v3f normal() const {
            return glm::normalize(glm::cross(e1(), e2()));
        }

        v3f e1() const {
            return  p[1]-p[0];
        }
        v3f e2() const {
            return  p[2]-p[0];
        }

        v3f bary(v2f uv) {
            return p[0] + e1() * uv.x + e2() * uv.y;
        }
    };

    struct plane_t {
        v3f n{};
        f32 d{};

        static plane_t 
        from_triangle(const triangle_t& tri) {
            plane_t p;
            p.n = tri.normal();
            p.d = glm::dot(tri.p[0], p.n);
        }

        f32 distance(v3f p) const {
            return glm::dot(n, p) - d;
        }
    };

    struct sphere_t {
        v3f origin{0.0f};
        f32 radius{0.0f};
    };

    struct circle_t {
        v2f origin{0.0f};
        f32 radius{0.0f};
    };

    
    struct polygon_t {
        v3f* points{};
        size_t count{0}, capacity{0};

        explicit polygon_t(v3f* memory, size_t capacity_)
            : points{memory}, capacity{capacity_}
        {}

        v3f sample(f32 t) {
            assert(count>2);
            size_t index = size_t(t * f32(count));
            f32 p1 = f32(index/(count));
            f32 p2 = f32((index+1)/(count));
            f32 d = p2-p1;
            t = (t-p1)/d;
            return points[index] + t * points[index+1];
        }

        polygon_t& add(v3f p) {
            assert(count < capacity);
            points[count++] = p;

            return *this;
        }

        polygon_t& insert(size_t i, v3f p) {
            assert(i<=count);
            assert(i<capacity);
            size_t r{count}, w{count+1};
            while(i&&w!=i) { 
                points[w--] = points[r--]; 
            }
            points[i] = p;
            count++;
            return *this;
        }

        polygon_t& remove(size_t i) {
            assert(i<count);
            assert(i<capacity);
            size_t r{i+1}, w{i};
            while(i&&i!=capacity&&w!=count) points[w++] = points[r++];
            count--;
            return *this;
        }
    };

    bool intersect(circle_t c, v2f p) {
        return glm::distance2(c.origin, p) < c.radius * c.radius;
    }

    bool intersect(sphere_t a, v3f b) {
        return glm::distance2(a.origin, b) < a.radius * a.radius;
    }

    f32 distance(circle_t c, v2f p) {
        return glm::distance(c.origin, p) - c.radius;
    }

    f32 distance(sphere_t a, v3f p) {
        return glm::distance(a.origin, p) - a.radius;
    }

    f32 distance(plane_t pl, v3f p) {
        return pl.distance(p);
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

        v3f at(f32 t) const {
            return origin + direction * t;
        }
    };
    
template <typename T = v3f>
struct aabb_t {
    // aabb_t(){}
    // aabb_t(const T& o) {
    //     expand(o);
    // }

    // aabb_t(const T& min_, const T& max_) noexcept
    //     : min{min_}, max{max_}
    // {
    // }

    T min{std::numeric_limits<float>::max()};
    T max{-std::numeric_limits<float>::max()};

    T size() const {
        return max - min;
    }

    T center() const {
        return (max+min)/2.0f;
    }

    T sample(f32 a) const {
        return size() * a + min;
    }

    void add(T amount) {
        aabb_t<T> t;
        t.expand(min + amount);
        t.expand(max + amount);
        *this = t;
    }

    void scale(T amount) {
        aabb_t<T> t;
        t.expand(center() + size() * 0.5f * amount);
        t.expand(center() - size() * 0.5f * amount);
        *this = t;
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

    aabb_t<T> operator+(const T& v) const {
        return aabb_t<T>{min+v,max+v};
    }
    
};

static bool
intersect(aabb_t<v2f> rect, v2f p) {
    return rect.contains(p);
}

inline static bool
intersect(const ray_t& ray, const triangle_t& tri, f32& t) noexcept {
    const auto& [v0, v1, v2] = tri.p;
    const v3f v0v1 = v1 - v0;
    const v3f v0v2 = v2 - v0;
    const v3f n = glm::cross(v0v1, v0v2);
    const f32 area = glm::length(n);
    const f32 NoR = glm::dot(ray.direction, n);
    if (glm::abs(NoR) < 0.00001f) {
        return false;
    }
    const f32 d = -glm::dot(n, v0);

    t = -(glm::dot(n,ray.origin) + d) / NoR;
    if (t < 0.0f) {
        return false;
    }
    
    const v3f p = ray.origin + ray.direction * t;
    v3f c;

    // edge 0
    v3f edge0 = v1 - v0; 
    v3f vp0 = p - v0;
    c = glm::cross(edge0, vp0);
    if (glm::dot(n, c) < 0.0f) {return false;} // P is on the right side
 
    // edge 1
    v3f edge1 = v2 - v1; 
    v3f vp1 = p - v1;
    c = glm::cross(edge1, vp1);
    if (glm::dot(n, c) < 0.0f) {return false;} // P is on the right side
  
    // edge 2
    v3f edge2 = v0 - v2; 
    v3f vp2 = p - v2;
    c = glm::cross(edge2, vp2);
    if (glm::dot(n, c) < 0.0f) {return false;} // P is on the right side

    return true; 
}


bool intersect_aabb_slab( const ray_t& ray, float t, const v3f bmin, const v3f bmax )
{
    float tx1 = (bmin.x - ray.origin.x) / ray.direction.x, tx2 = (bmax.x - ray.origin.x) / ray.direction.x;
    float tmin = glm::min( tx1, tx2 ), tmax = glm::max( tx1, tx2 );
    float ty1 = (bmin.y - ray.origin.y) / ray.direction.y, ty2 = (bmax.y - ray.origin.y) / ray.direction.y;
    tmin = glm::max( tmin, glm::min( ty1, ty2 ) ), tmax = glm::min( tmax, glm::max( ty1, ty2 ) );
    float tz1 = (bmin.z - ray.origin.z) / ray.direction.z, tz2 = (bmax.z - ray.origin.z) / ray.direction.z;
    tmin = glm::max( tmin, glm::min( tz1, tz2 ) ), tmax = glm::min( tmax, glm::max( tz1, tz2 ) );
    return tmax >= tmin && tmin < t && tmax > 0;
}

inline hit_result_t          //<u64> 
intersect(const ray_t& ray, const aabb_t<v3f>& aabb) {
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
	
	transform_t(const v3f& position, const quat& rotation) : origin{position} {
        set_rotation(rotation);
    }
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
	transform_t& translate(const v3f& delta) {
        origin += delta;
        return *this;
    }
	transform_t& scale(const v3f& delta) {
        basis = glm::scale(m44{basis}, delta);
        return *this;
    }
	transform_t& rotate(const v3f& axis, f32 rads) {
        set_rotation((get_orientation() * glm::angleAxis(rads, axis)));

        // m44 rot = m44(1.0f);
        // rot = glm::rotate(rot, delta.z, { 0,0,1 });
        // rot = glm::rotate(rot, delta.y, { 0,1,0 });
        // rot = glm::rotate(rot, delta.x, { 1,0,0 });
        // basis = (basis) * m33(rot);
        return *this;
    }
	transform_t& rotate_quat(const glm::quat& delta) {
        set_rotation(get_orientation() * delta);
        return *this;
    }
    
    glm::quat get_orientation() const {
        return glm::normalize(glm::quat_cast(basis));
    }

	transform_t inverse() const {
        transform_t transform;
        transform.basis = glm::transpose(basis);
        transform.origin = transform.basis * -origin;
        return transform;
    }

	void look_at(const v3f& target, const v3f& up = axis::up) {
        set_rotation(glm::quatLookAt(target-origin, up));
    }

    constexpr transform_t& set_scale(const v3f& scale) {
        for (int i = 0; i < 3; i++) {
            basis[i] = glm::normalize(basis[i]) * scale[i];
        }
        return *this;
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
// #pragma pack(push, 1)
struct vertex_t {
    v3f pos;
    v3f nrm;
    v3f col;
    v2f tex;
};
// #pragma pack(pop)

struct skinned_vertex_t {
    v3f pos;
    v3f nrm;
    v2f tex;
    // v3f tangent;
    // v3f bitangent;
    u32 bone_id[4];
    v4f weight{0.0f};
};

struct instance_draw_t {
    u32 object_id{};
    u32 batch_id{};
};

struct indirect_draw_t {
    u32     vertex_count{};
    u32     instance_count{};
    u32     first_vertex{};
    u32     first_instance{};
};

struct indirect_indexed_draw_t {
    u32     index_count{};
    u32     instance_count{};
    u32     first_index{};
    i32     vertex_offset{};

    u32     first_instance{};
    u32     albedo_id{};
    u32     normal_id{};
    u32     object_id{};
};

using index32_t = u32;

struct mesh_t {
    vertex_t*   vertices{nullptr};
    u32         vertex_count{0};
    index32_t*  indices{nullptr};
    u32         index_count{0};
};

struct material_info_t {
    char name[64]{};
    v4f color;
    f32 roughness{0.5f};
    f32 metallic{0.0f};
    f32 emission{0.0f};

    u64 albedo_id{};
    u64 normal_id{};
    char albedo[128]{};
    char normal[128]{};

    material_info_t& operator=(const material_info_t& o) {
        if (this == &o) { return *this; }
        
        std::memcpy(this, &o, sizeof(material_info_t));

        return *this;
    }
};

struct mesh_view_t {
    u32 vertex_start{};
    u32 vertex_count{};
    u32 index_start{};
    u32 index_count{};
    u32 instance_start{};
    u32 instance_count{1};

    material_info_t material{};

    math::aabb_t<v3f> aabb{};


    // for RTX
    u32 blas{0};
    // u32 hit{0};
};

struct mesh_list_t {
    gfx::mesh_view_t* meshes{nullptr};
    size_t count{0};
    math::aabb_t<v3f> aabb{};


    // mesh_list_t* next_lod{0};
};

// there should only be one running at a time
struct mesh_builder_t {
    utl::pool_t<gfx::vertex_t>& vertices;
    utl::pool_t<u32>& indices;

    mesh_builder_t(
        utl::pool_t<gfx::vertex_t>& vertices_,
        utl::pool_t<u32>& indices_
    ) : vertices{vertices_}, indices{indices_} {
        vertex_start = safe_truncate_u64(vertices_.count());
        index_start = safe_truncate_u64(indices_.count());
    }

    u32 vertex_start{0};
    u32 vertex_count{0};
    u32 index_start{0};
    u32 index_count{0};

    gfx::mesh_list_t
    build(arena_t* arena) {
        gfx::mesh_list_t m;
        m.count = 1;
        m.meshes = arena_alloc_ctor<gfx::mesh_view_t>(arena, 1);
        m.meshes[0].vertex_start = vertex_start;
        m.meshes[0].vertex_count = vertex_count;
        m.meshes[0].index_start = index_start;
        m.meshes[0].index_count = index_count;
        return m;
    }

    void rewrite() {
        assert(vertex_count);
        vertices.clear();
        indices.clear();
    }

    mesh_builder_t& add_vertex(
        v3f pos,
        v2f uv = v2f{0.0f, 0.0f},
        v3f nrm = axis::up,
        v3f col = v3f{1.0f}
    ) {
        *vertices.allocate(1) = gfx::vertex_t{.pos = pos, .nrm = nrm, .col = col, .tex = uv};
        vertex_count++;
        return *this;
    }

    mesh_builder_t& add_vertex(gfx::vertex_t&& vertex) {
        *vertices.allocate(1) = std::move(vertex);
        vertex_count++;
        return *this;
    }
    mesh_builder_t& add_index(u32 index) {
        *indices.allocate(1) = index;
        index_count++;
        return *this;
    }


    mesh_builder_t& add_triangles(math::triangle_t* tris, u64 count) {
        loop_iota_u64(i, count) {
            auto* v = vertices.allocate(3);

            const auto [t0, t1, t2] = tris[i].p;

            const v3f norm = glm::normalize(glm::cross(t1-t0,t2-t0));

            v[0].pos = t0;
            v[1].pos = t1;
            v[2].pos = t2;

            loop_iota_u64(j, 3) {
                v[j].nrm = norm;
                v[j].col = norm;
                v[j].tex = v2f{0.0};
            }
        }

        vertex_count += safe_truncate_u64(count) * 3;
        return *this;
    }

    mesh_builder_t& add_box(math::aabb_t<v3f> box) {
        math::triangle_t tris[6];
        const auto s = box.size();
        const auto p = box.min;
        const auto sx = s.x;
        const auto sy = s.y;
        const auto sz = s.z;
        const auto sxv = v3f{s.x, 0.0f, 0.0f};
        const auto syv = v3f{0.0f, s.y, 0.0f};
        const auto szv = v3f{0.0f, 0.0f, s.z};
        u64 i = 0;
        tris[i++].p = {p, p + sxv, p + sxv + syv};
        tris[i++].p = {p, p + syv + sxv, p + syv};

        tris[i++].p = {p + szv + sxv, p + sxv + syv + szv, p + szv};
        tris[i++].p = {p + syv + sxv, p + sxv + syv + szv, p + szv};
        
        tris[i++].p = {p, p + szv + syv, p + szv};
        tris[i++].p = {p, p + szv + syv, p + szv};
        
        add_triangles(tris, 6);



        return *this;
    }
    mesh_builder_t& add_quad(gfx::vertex_t vertex[4]) {
        u32 v_start = safe_truncate_u64(vertices.count());
        gfx::vertex_t* v = vertices.allocate(6);
        v[0] = vertex[0];
        v[1] = vertex[1];
        v[2] = vertex[2];
        
        v[3] = vertex[2];
        v[4] = vertex[1];
        v[5] = vertex[3];

        vertex_count += 6;

        return *this;
    }
};

struct bvh_node_t {
    v3f min;
    u32 left_first;

    v3f max;
    u32 tri_count;
};

struct bvh_tree_t {
    bvh_node_t* nodes{0};
    u64 node_count{0};
    u64 root{0};
    u64 nodes_used{1};
    vertex_t* vertices{0};
    u64 vertex_count{0};
    u32* indices{0};
    u64 index_count{0};
    u32* tri_remap{0};
};

void subdivide(bvh_tree_t* tree, u64 id) {

}

void update_node_bounds(bvh_tree_t* tree, u64 id) {
    auto* node = &tree->nodes[id];
    node->min = v3f{1e30f};
    node->max = v3f{-1e30f};
    range_u64(i, node->left_first, node->tri_count) {
        auto* t = tree->indices + i * 3;
        auto* v = tree->vertices;
        node->min = glm::min(v[t[0]].pos, node->min);
        node->max = glm::max(v[t[0]].pos, node->max);
        node->min = glm::min(v[t[1]].pos, node->min);
        node->max = glm::max(v[t[1]].pos, node->max);
        node->min = glm::min(v[t[2]].pos, node->min);
        node->max = glm::max(v[t[2]].pos, node->max);
    }
}

void build_bvh(
    bvh_tree_t* tree
) {
    auto* root = &tree->nodes[tree->root];
    root->left_first = 0;
    root->tri_count = safe_truncate_u64(tree->index_count/3);

    update_node_bounds(tree, tree->root);
    subdivide(tree, tree->root);
}


using color4 = v4f;
using color3 = v3f;
using color32 = u32;
struct font_t;

static constexpr u32 material_lit = (u32)BIT(0);
static constexpr u32 material_triplanar = (u32)BIT(1);
static constexpr u32 material_billboard = (u32)BIT(2);

struct material_t {
    v4f albedo{};

    f32 ao{0.5f};
    f32 emission{};
    f32 metallic{};
    f32 roughness{};

    u32 flags{material_lit};     // for material effects
    u32 opt_flags{}; // for performance
    u32 albedo_id{0};
    u32 normal_id{0};

    f32 scale{1.0f};
    u32 padding[3];

    static constexpr material_t plastic(const v4f& color, f32 roughness = 0.8f, u32 albedo_id = 0, u32 normal_id = 0) {
        return material_t {
            .albedo = color,
            .ao = 0.6f,
            .emission = 0.0f,
            .metallic = 0.0f,
            .roughness = roughness,
            .albedo_id = albedo_id,
            .normal_id = normal_id,
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

struct shader_description_t {
    std::string_view filename{};
    u32                 stage{0};
    u32           next_stages{0};
    u32    push_constant_size{0};
    // Note(Zack): Add stuff for descriptor sets

    struct uniform_t {
        std::string_view name{};
        std::string_view type{};
        void*            data{};
    };

    uniform_t uniforms[32];
    size_t    uniform_count{};

    constexpr shader_description_t& set_stage(u32 s) {
        stage = s;
        return *this;
    }

    constexpr shader_description_t& add_next_stage(u32 s) {
        next_stages |= s;
        return *this;
    }

    template <typename T>
    constexpr shader_description_t& add_push_constant() {
        add_push_constant(sizeof(T));
    }

    constexpr shader_description_t& add_push_constant(u32 size) {
        push_constant_size += size;
        assert(push_constant_size < (sizeof(m44)<<1));
        return *this;
    }

    constexpr shader_description_t& add_uniform(std::string_view name, std::string_view type) {
        assert(uniform_count < array_count(uniforms));
        uniforms[uniform_count++] = uniform_t{
            .name = name,
            .type = type
        };
        return *this;
    }
};

struct trail_renderer_t {
    inline static constexpr size_t MAX_POINTS = 32;

    v3f points[MAX_POINTS]{};
    f32 point_life[MAX_POINTS]{0.0f};
    size_t point_head{0};

    color4 start_color{1.0f};
    color4 end_color{1.0f};

    f32 min_distance{0.1f};
    f32 life_time{1.0f};

    bool emitting{true};
    // bool billboard{true};

private:
    v3f _last_position;

    void emit(v3f position) {
        points[point_head] = position;
        point_head = (point_head + 1) % MAX_POINTS;
    }

    size_t last_index(size_t i){
        return i ? i-1 : MAX_POINTS-1;
    }

    v3f last_point(size_t i) {
        return points[last_index(i)];
    }
    f32 last_life(size_t i) {
        return point_life[last_index(i)];
    }

public:
    void tick(f32 dt, v3f position) {
        if (emitting) {
            if (point_life[point_head] > 0.0f) {
                _last_position = points[point_head];
                while (glm::distance(_last_position, position) > min_distance) {
                    v3f delta = glm::normalize(position - _last_position);
                    emit(_last_position + delta * min_distance);
                    _last_position = points[point_head];
                }
            }
        }

        range_u64(i, 0, MAX_POINTS) {
            if ((point_life[i] -= dt) > 0.0f) {

            }
        }
    }

    void make_quad(
        v3f a, v3f b, 
        f32 height, 
        std::span<v3f> vertices
    ) {
        const auto up = axis::up * height;

        const auto p0 = a + up;
        const auto p1 = a - up;
        const auto p2 = b + up;
        const auto p3 = b - up;

        vertices[0] = p0;
        vertices[1] = p1;
        vertices[2] = p2;
        vertices[3] = p0;
        vertices[4] = p3;
        vertices[5] = p2;
    }

    size_t write_vertices(std::span<v3f> vertices) {
        size_t write_position = 0;
        range_u64(_i, 0, MAX_POINTS-1) {
            if (write_position > vertices.size()) {
                zyy_error(__FUNCTION__, "Not enough vertices supplied: {} were given", vertices.size());
                return write_position > 5 ? write_position - 6 : 0;
            }
            u64 i = (point_head + _i) % MAX_POINTS;
            if (point_life[i] > 0.0f && last_life(i) > 0.0f) {
                // @hardcoded
                make_quad(points[i], last_point(i), 0.2f, vertices.subspan(write_position, write_position + 6));
                write_position += 6;
            }
        }

        return write_position;
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
        static constexpr auto reddish = "#fa2222ff"_c4;
        static constexpr auto green = "#00ff00ff"_c4;
        static constexpr auto blue  = "#0000ffff"_c4;
        static constexpr auto purple= "#ff00ffff"_c4;
        static constexpr auto cyan  = "#00ffffff"_c4;
        static constexpr auto yellow= "#ffff00ff"_c4;
        static constexpr auto yellowish= "#fafa22ff"_c4;
        static constexpr auto orange  = "#faa522ff"_c4;
        static constexpr auto sand  = "#C2B280ff"_c4;
        static constexpr auto dirt  = "#9b7653ff"_c4;
    };

    namespace v3 {
        static constexpr auto clear = "#00000000"_c3;
        static constexpr auto black = "#000000ff"_c3;
        static constexpr auto white = "#ffffffff"_c3;
        static constexpr auto ray_white   = "#f1f1f1ff"_c3;
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

        gfx::font_t* dyn_font[32];

        u64 frame{0};
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
        f32     shadow_distance{1.0f};
        color32 shadow_color{color::rgba::black};

        f32 padding{1.0f};
        f32 margin{1.0f};
        f32 border_radius{6.0f};
    };

    struct text_t;
    struct image_t;
    struct button_t;

    struct panel_t : public math::aabb_t<v2f> {
        sid_t name;
        v2f draw_cursor;
        v2f saved_cursor;
    };
    
    inline void
    ctx_clear(
        ctx_t* ctx,
        utl::pool_t<vertex_t>* vertices,
        utl::pool_t<u32>* indices
    ) {
        // ctx->frame++; // remove this
        ctx->vertices = vertices;
        ctx->indices = indices;
        ctx->vertices->clear();
        ctx->indices->clear();
    }

    inline void
    string_render(
        ctx_t* ctx,
        std::string_view text,
        v2f* position,
        const color32& text_color = color::rgba::white,
        font_t* font = 0
    ) {
        font_render(0, font ? font : ctx->font, 
            text, 
            *position,
            ctx->screen_size,
            ctx->vertices,
            ctx->indices,
            text_color
        );
    }

    inline void
    string_render(
        ctx_t* ctx,
        std::string_view text,
        const v2f& position,
        const color32& text_color = color::rgba::white,
        font_t* font = 0
    ) {
        v2f cursor = position;
        string_render(
            ctx,
            text,
            &cursor,
            text_color,
            font
        );
    }

    inline void 
    draw_circle(
        ctx_t* ctx,
        v2f pos, f32 radius,
        u32 color, u32 res = 16, f32 perc = 1.0f, f32 start = 0.0f
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count());

        vertex_t* center = ctx->vertices->allocate(1);

        center->pos = pos / ctx->screen_size;
        center->col = color;
        center->img = ~(0ui32);

        vertex_t* lv{0};
        range_u32(i, 0, res+1) {
            const auto a = start * math::constants::tau32 + perc * math::constants::tau32 * f32(i)/f32(res);
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
        u32* tris = ctx->indices->allocate(3);
        *tris++ = v_start;
        *tris++ = v_start + res;
        *tris++ = v_start + 1;
    }


    // in screen space
    inline void
    draw_line(
        ctx_t* ctx,
        v2f a, v2f b,
        f32 line_width,
        u32 color
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count());
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
    draw_quad_outline(
        ctx_t* ctx,
        const v2f* points,
        f32 line_width,
        u32 color
    ) {
        draw_line(ctx, points[0], points[1], line_width, color);
        draw_line(ctx, points[1], points[2], line_width, color);
        draw_line(ctx, points[2], points[3], line_width, color);
        draw_line(ctx, points[3], points[0], line_width, color);
    }

    inline void
    draw_quad_outline(
        ctx_t* ctx,
        const v3f* points,
        const m44& vp,
        f32 line_width,
        u32 color
    ) {
        draw_line(ctx, swizzle::xy(math::world_to_screen(vp, points[0])) * ctx->screen_size, swizzle::xy(math::world_to_screen(vp, points[1])) * ctx->screen_size, line_width, color);
        draw_line(ctx, swizzle::xy(math::world_to_screen(vp, points[1])) * ctx->screen_size, swizzle::xy(math::world_to_screen(vp, points[2])) * ctx->screen_size, line_width, color);
        draw_line(ctx, swizzle::xy(math::world_to_screen(vp, points[2])) * ctx->screen_size, swizzle::xy(math::world_to_screen(vp, points[3])) * ctx->screen_size, line_width, color);
        draw_line(ctx, swizzle::xy(math::world_to_screen(vp, points[3])) * ctx->screen_size, swizzle::xy(math::world_to_screen(vp, points[0])) * ctx->screen_size, line_width, color);
    }

    inline void
    draw_aabb(
        ctx_t* ctx,
        math::aabb_t<v3f> aabb,
        const m44& vp,
        f32 line_width,
        u32 color
    ) {
        const v3f size = aabb.size();
        const v3f sxxx = v3f{size.x,0,0};
        const v3f syyy = v3f{0,size.y,0};
        const v3f szzz = v3f{0,0,size.z};

        math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{1.0f}};

        if (viewable.contains(math::world_to_screen(vp, aabb.min)) == false) {return;}
        if (viewable.contains(math::world_to_screen(vp, aabb.center())) == false) {return;}
        if (viewable.contains(math::world_to_screen(vp, aabb.max)) == false) {return;}

        // bottom
        const v3f q1[4]{aabb.min, aabb.min + sxxx, aabb.min + sxxx + szzz, aabb.min + szzz};
        // top
        const v3f q2[4]{aabb.min + syyy, aabb.min + syyy + sxxx, aabb.min + syyy + sxxx + szzz, aabb.min + syyy + szzz};
        draw_quad_outline(ctx, q1, vp, line_width, color);
        draw_quad_outline(ctx, q2, vp, line_width, color);

        range_u64(i, 0, 4) {
            draw_line(ctx, swizzle::xy(math::world_to_screen(vp, q1[i])) * ctx->screen_size, swizzle::xy(math::world_to_screen(vp, q2[i])) * ctx->screen_size, line_width, color);
        }
    }

    inline void
    draw_rect(
        ctx_t* ctx,
        math::aabb_t<v2f> box,
        std::span<u32, 4> colors
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count());
        const u32 i_start = safe_truncate_u64(ctx->indices->count());

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
        const u32 v_start = safe_truncate_u64(ctx->vertices->count());
        const u32 i_start = safe_truncate_u64(ctx->indices->count());

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

    //textured
    inline void
    draw_rect(
        ctx_t* ctx,
        math::aabb_t<v2f> box,
        u32 tex, math::aabb_t<v2f> uvs
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count());
        const u32 i_start = safe_truncate_u64(ctx->indices->count());

        const v2f p0 = box.min;
        const v2f p1 = v2f{box.min.x, box.max.y};
        const v2f p2 = v2f{box.max.x, box.min.y};
        const v2f p3 = box.max;

        const v2f uv0 = uvs.min;
        const v2f uv1 = v2f{uvs.min.x, uvs.max.y};
        const v2f uv2 = v2f{uvs.max.x, uvs.min.y};
        const v2f uv3 = uvs.max;

        vertex_t* v = ctx->vertices->allocate(4);
        u32* i = ctx->indices->allocate(6);
        v[0] = vertex_t { .pos = p0 / ctx->screen_size, .tex = uv0, .img = tex, .col = color::rgba::white};
        v[1] = vertex_t { .pos = p1 / ctx->screen_size, .tex = uv1, .img = tex, .col = color::rgba::white};
        v[2] = vertex_t { .pos = p2 / ctx->screen_size, .tex = uv2, .img = tex, .col = color::rgba::white};
        v[3] = vertex_t { .pos = p3 / ctx->screen_size, .tex = uv3, .img = tex, .col = color::rgba::white};

        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;

        i[3] = v_start + 2;
        i[4] = v_start + 1;
        i[5] = v_start + 3;
    }

    inline void
    draw_round_rect(
        ctx_t* ctx,
        math::aabb_t<v2f> box,
        f32 radius,
        u32 color,
        u32 num_segments = 20
    ) {
        math::aabb_t<v2f> inner{box.min+radius, box.max-radius};


        //top
        draw_rect(ctx, math::aabb_t<v2f>{box.min + v2f{radius,0}, inner.max-v2f{0,inner.size().y}}, color);
        //bot
        draw_rect(ctx, math::aabb_t<v2f>{inner.min + v2f{0,inner.size().y}, inner.max+v2f{0,radius}}, color);
        //left
        draw_rect(ctx, math::aabb_t<v2f>{box.min + v2f{0,radius}, inner.max-v2f{inner.size().x,0}}, color);
        //right
        draw_rect(ctx, math::aabb_t<v2f>{inner.max - v2f{0,inner.size().y}, inner.max+v2f{radius,0}}, color);

        draw_rect(ctx, inner, color);
        
        draw_circle(ctx, inner.min, radius, color, num_segments);
        draw_circle(ctx, inner.min + v2f{0,inner.size().y}, radius, color, num_segments);
        draw_circle(ctx, inner.max, radius, color, num_segments);
        draw_circle(ctx, inner.min + v2f{inner.size().x,0}, radius, color, num_segments);
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

            f32 gizmo_scale_start{};
            v2f draw_cursor{0.0f}; 
            v2f gizmo_mouse_start{};
            v3f gizmo_world_start{};
            v2f gizmo_axis_dir{};
            v3f gizmo_axis_wdir{};

            theme_t theme;

            ui_id_t hot;
            ui_id_t active;

            panel_t panels[32];
            panel_t* panel{this->panels};

            bool next_same_line{false};
            v2f saved_cursor;
        };

        inline void
        indent(state_t& imgui, v2f offset) {
            imgui.draw_cursor += offset;
        }

        inline bool
        want_mouse_capture(state_t& imgui) {
            return imgui.hot.id != 0;
        }

        inline void
        clear(state_t& imgui) {
            range_u64(i, 0, array_count(imgui.panels)) {
                // imgui.panels[i] = {};
                imgui.panels[i].draw_cursor = v2f{0.0f};
                // imgui.panels[i].max = imgui.panels[i].min;
            }
            // imgui.panel->draw_cursor = v2f{0.0f};
            // imgui.active = 0;
        }

        inline void
        same_line(state_t& imgui) {
            imgui.next_same_line = true;
            imgui.panel->saved_cursor = imgui.panel->draw_cursor;
        }

        inline bool
        begin_panel(
            state_t& imgui, 
            string_t name,
            v2f pos = v2f{0.0f},
            v2f size = v2f{-1.0f}
        ) {
            const sid_t pnl_id = sid(name.sv());

            imgui.panel++;
            assert((void*)imgui.panel != (void*)&imgui.panel && "End of nested panels");

            if (pos.x >= 0.0f && pos.y >= 0.0f) { 
                imgui.panel->draw_cursor = pos+1.0f;
            }
            imgui.panel->saved_cursor = imgui.panel->draw_cursor;

            imgui.panel->max = imgui.panel->min = imgui.panel->draw_cursor;
            if (size.x > 0.0f && size.y > 0.0f) {
                imgui.panel->expand(imgui.panel->draw_cursor + size);
            }

            imgui.panel->name = pnl_id;
            
            return true;
        }

        inline bool
        begin_panel_3d(
            state_t& imgui, 
            string_t name,
            m44 vp,
            v3f pos
        ) {
            const v3f spos = math::world_to_screen(vp, pos);
            math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{1.0f}};

            if (viewable.contains(spos)) {
                const v2f screen = v2f{spos} * imgui.ctx.screen_size;
                return begin_panel(imgui, name, screen);
            }
            return false;
        }

        inline void
        end_panel(
            state_t& imgui
        ) {
            imgui.panel->expand(imgui.panel->draw_cursor);

            constexpr f32 border_size = 1.0f;
            f32 corner_radius = imgui.theme.border_radius;

            draw_round_rect(&imgui.ctx, *imgui.panel, corner_radius, imgui.theme.bg_color);
            imgui.panel->min -= v2f{border_size};
            imgui.panel->max += v2f{border_size};
            draw_round_rect(&imgui.ctx, *imgui.panel, corner_radius, imgui.theme.fg_color);

            auto [x,y] = imgui.ctx.input->mouse.pos;
            if (imgui.hot.id == 0) {
                if (math::intersect(*imgui.panel, v2f{x,y})) {
                    imgui.hot.id = imgui.panel->name;
                }
            } else if (imgui.hot.id == imgui.panel->name) {
                if (!math::intersect(*imgui.panel, v2f{x,y})) {
                    imgui.hot.id = 0;
                }
            }
            // imgui.panel->draw_cursor = v2f{0.0f};
            imgui.panel--;
        }

        inline panel_t
        get_last_panel(
            state_t& imgui
        ) {
            return imgui.panel[1];
        }

        inline bool
        draw_circle(
            state_t& imgui,
            v2f pos, f32 radius,
            color32 color, u32 res = 16
        ) {
            draw_circle(&imgui.ctx, pos, radius, color, res);
            const auto [x,y] = imgui.ctx.input->mouse.pos;
            const bool clicked = imgui.ctx.input->mouse.buttons[0];
            return math::intersect(math::circle_t{pos, radius}, v2f{x,y}) && clicked;
        }

        inline bool
        draw_hiding_circle_3d(
            state_t& imgui,
            const m44& vp,
            v3f pos, f32 radius,
            color32 color, f32 show_distance, u32 res = 16
        ) {
            const v3f spos = math::world_to_screen(vp, pos);
            math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{1.0f}};

            if (viewable.contains(spos)) {
                const v3f offset = math::world_to_screen(vp, pos + axis::up * radius);
                const v2f screen = v2f{spos} * imgui.ctx.screen_size;
                const f32 scr_radius = glm::distance(v2f{offset} * imgui.ctx.screen_size, screen);
                auto [x,y] = imgui.ctx.input->mouse.pos;
                if (math::intersect(math::circle_t{screen, scr_radius + show_distance}, v2f{x,y})) {
                    draw_circle(&imgui.ctx, screen, scr_radius, color, res);
                }
                return math::intersect(math::circle_t{screen, scr_radius}, v2f{x,y});
            } else {
                return false;
            }
        }
        
        inline bool
        draw_circle_3d(
            state_t& imgui,
            const m44& vp,
            v3f pos, f32 radius,
            color32 color, u32 res = 16
        ) {
            return draw_hiding_circle_3d(imgui, vp, pos, radius, color, 100000000.0f, res);
        }

        inline void
        draw_gizmo(
            state_t& imgui,
            v3f* pos,
            const m44& vp
        ) {
            const v3f spos = math::world_to_screen(vp, *pos);
            math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{1.0f}};

            if (viewable.contains(spos)) {
                const v2f screen = v2f{spos} * imgui.ctx.screen_size;
                
                auto v_up         = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::up)};
                auto v_forward    = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::forward)};
                auto v_right      = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::right)};
                auto v_pos        = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos)};
                
                const f32 line_width = 3.0f;

                const v2f rd = v_right - v_pos;
                const v2f ud = v_up - v_pos;
                const v2f fd = v_forward - v_pos;
                
                constexpr f32 clw = 4.0f; // color_line_width
                constexpr f32 abw = 6.0f; // axis bubble width
                constexpr f32 bbw = 2.0f; // bubble border width
                constexpr f32 dlw = 2.0f; // disabled line width
                constexpr f32 rad = 0.75f;// rotation axis distance

                draw_circle(&imgui.ctx, v_pos,      abw - bbw, color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_up,       abw - bbw, color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_right,    abw - bbw, color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_forward,  abw - bbw, color::rgba::light_gray);

                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, ud, fd, 0.0f, 0.25f, clw, color::rgba::red, 32);
                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, rd, fd, 0.0f, 0.25f, clw, color::rgba::green, 32);
                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, rd, ud, 0.0f, 0.25f, clw, color::rgba::blue, 32);

                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, ud, fd, 0.25f, 1.0f, dlw, color::rgba::light_gray, 32);
                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, rd, fd, 0.25f, 1.0f, dlw, color::rgba::light_gray, 32);
                draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, rd, ud, 0.25f, 1.0f, dlw, color::rgba::light_gray, 32);

                draw_circle(&imgui.ctx, v_up,       abw, color::rgba::green);
                draw_circle(&imgui.ctx, v_right,    abw, color::rgba::red);
                draw_circle(&imgui.ctx, v_forward,  abw, color::rgba::blue);

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

            const v3f spos = math::world_to_screen(vp, *pos);
            math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{1.0f}};

            const auto is_gizmo_id = [](u64 id){
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

            if (viewable.contains(spos)) {
                // gizmo points in screen space
                const auto v_up         = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::up)};
                const auto v_forward    = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::forward)};
                const auto v_right      = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + axis::right)};
                const auto v_pos        = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos)};

                // gizmo delta
                const v2f rd = v_right - v_pos;
                const v2f ud = v_up - v_pos;
                const v2f fd = v_forward - v_pos;
                
                const math::circle_t pc{v_pos, 8.0f};
                const math::circle_t rc{v_right, 8.0f};
                const math::circle_t uc{v_up, 8.0f};
                const math::circle_t fc{v_forward, 8.0f};
                const auto [mx,my] = imgui.ctx.input->mouse.pos;
                const v2f mouse{mx,my};
                const v2f mdelta{-(imgui.gizmo_mouse_start-mouse)};


                u64 giz_id = is_gizmo_id(imgui.active.id) ? imgui.active.id : 0;

                // if (math::intersect(pc, mouse)) {giz_id = "gizmo_pos"_sid;} else
                if (is_gizmo_id(imgui.active.id) == false) {

                    if (math::intersect(rc, mouse)) {giz_id = "gizmo_right"_sid;} else
                    if (math::intersect(uc, mouse)) {giz_id = "gizmo_up"_sid;} else
                    if (math::intersect(fc, mouse)) {giz_id = "gizmo_forward"_sid;}

                }
                const bool clicked = imgui.ctx.input->pressed.mouse_btns[0];
                const bool released = imgui.ctx.input->released.mouse_btns[0];
                if (is_gizmo_id(imgui.active.id)) {
                    if (is_gizmo_id(imgui.hot.id)) {
                        if (imgui.gizmo_mouse_start != v2f{0.0f}) {
                            const v2f naxis = glm::normalize(imgui.gizmo_axis_dir);
                            const f32 proj = glm::dot(mdelta, naxis);
                            const f32 scale = imgui.gizmo_scale_start;
                            *pos = imgui.gizmo_world_start + imgui.gizmo_axis_wdir * glm::dot(mdelta, naxis/scale);

                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start, 6.0f, color::rgba::light_gray);
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start, 8.0f, color::to_color32(imgui.gizmo_axis_wdir));
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, 6.0f, color::rgba::light_gray);
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, 8.0f, color::to_color32(imgui.gizmo_axis_wdir));
                            
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
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case "gizmo_up"_sid: {
                                    imgui.gizmo_axis_dir = (ud);
                                    imgui.gizmo_axis_wdir = axis::up;
                                    imgui.gizmo_mouse_start = v_up;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case "gizmo_forward"_sid: {
                                    imgui.gizmo_axis_dir = (fd);
                                    imgui.gizmo_axis_wdir = axis::forward;
                                    imgui.gizmo_mouse_start = v_forward;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case_invalid_default;
                            }
                        }
                    }
                    if (released) {
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
                    if (released) {
                        imgui.hot = 0;
                        imgui.gizmo_mouse_start = v2f{0.0f};
                        imgui.gizmo_axis_dir = v2f{0.0f};
                        imgui.gizmo_axis_wdir = v3f{0.0f};
                    }
                }
                if (giz_id) {
                    imgui.hot = giz_id;
                }
            } else {
                if (is_gizmo_id(imgui.hot.id) || is_gizmo_id(imgui.active.id)) {
                    imgui.hot = imgui.active = 0;
                    imgui.gizmo_mouse_start = v2f{0.0f};
                    imgui.gizmo_axis_dir = v2f{0.0f};
                    imgui.gizmo_axis_wdir = v3f{0.0f};
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
            
            v2f tmp_cursor = imgui.panel->draw_cursor;
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

            imgui.panel->draw_cursor.y += box.size().y + imgui.theme.padding * 2.0f;
        }

        inline void
        image(
            state_t& imgui,
            u32 bind_index,
            math::aabb_t<v2f> screen,
            math::aabb_t<v2f> uv = math::aabb_t<v2f>{v2f{0.0f}, v2f{1.0f}}
        ) {
            draw_rect(&imgui.ctx, screen, bind_index, uv);
        }
 
        inline void
        float_slider_id(
            state_t& imgui,
            u64 id,
            f32* val,
            f32 min = 0.0f,
            f32 max = 1.0f,
            v2f size = v2f{64.0f, 16.0f}
        ) {
            const u64 sld_id = id;

            v2f tmp_cursor = imgui.panel->draw_cursor;
            v2f save_draw = tmp_cursor;
            tmp_cursor.x += 8.0f;
            math::aabb_t<v2f> box;
            math::aabb_t<v2f> prc_box;
            const f32 prc = glm::clamp((*val - min) / (max - min), 0.0f, 1.0f);
            box.expand(tmp_cursor);
            box.expand(tmp_cursor + size);

            prc_box.expand(tmp_cursor + 2.0f);
            prc_box.expand(prc_box.min + (size - 4.0f) * v2f{prc, 1.0f});
            imgui.panel->expand(box.max + imgui.theme.padding);

            tmp_cursor = box.center() - font_get_size(imgui.ctx.font, fmt_sv("{:.2f}", *val)) * 0.5f;
            string_render(&imgui.ctx, fmt_sv("{:.2f}", *val), &tmp_cursor, imgui.theme.text_color);
            draw_rect(&imgui.ctx, prc_box, imgui.active.id == sld_id ? imgui.theme.active_color : imgui.theme.fg_color);
            draw_rect(&imgui.ctx, box, imgui.theme.bg_color);
            box.max += v2f{1.0f};
            box.min -= v2f{1.0f};
            draw_rect(&imgui.ctx, box, imgui.theme.fg_color);

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

            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += box.size().x + imgui.theme.padding;
            } else {
                imgui.panel->draw_cursor.y += box.size().y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->min.x + imgui.theme.padding;
            }
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
            float_slider_id(imgui, sld_id, val, min, max, size);
        }
       

        inline bool
        text(
            state_t& imgui, 
            std::string_view text,
            bool* toggle_state = 0         
        ) {
            const sid_t txt_id = sid(text);
            bool result = toggle_state ? *toggle_state : false;

            constexpr f32 text_pad = 4.0f;
            const auto font_size = font_get_size(imgui.ctx.font, text);
            imgui.panel->expand(imgui.panel->draw_cursor + font_size + text_pad * 2.0f);
            v2f temp_cursor = imgui.panel->draw_cursor;
            const f32 start_x = imgui.panel->draw_cursor.x;
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

            auto shadow_cursor = temp_cursor + imgui.theme.shadow_distance;
            string_render(&imgui.ctx, text, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            string_render(&imgui.ctx, text, &shadow_cursor, imgui.theme.shadow_color);
            // draw_rect(&imgui.ctx, text_box, gfx::color::rgba::purple);
            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += font_size.x;
            } else {
                imgui.panel->draw_cursor.y = temp_cursor.y - font_size.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->min.x + imgui.theme.padding;
            }
            return result;
        }

        inline bool
        text_edit(
            state_t& imgui, 
            std::string_view text,
            size_t* position,
            sid_t txt_id_ = 0,
            bool* toggle_state = 0         
        ) {
            const sid_t txt_id = txt_id_ ? txt_id_ : sid(text);
            bool result = toggle_state ? *toggle_state : false;

            constexpr f32 text_pad = 4.0f;
            const auto min_size = font_get_size(imgui.ctx.font, "Hello world");
            const auto font_size = font_get_size(imgui.ctx.font, text);
            imgui.panel->expand(imgui.panel->draw_cursor + font_size + text_pad * 2.0f);
            v2f temp_cursor = imgui.panel->draw_cursor;
            const f32 start_x = imgui.panel->draw_cursor.x;
            temp_cursor.x += text_pad + imgui.theme.padding;

            math::aabb_t<v2f> text_box;
            text_box.expand(temp_cursor);
            text_box.expand(temp_cursor + min_size);
            text_box.expand(temp_cursor + font_size);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (text_box.contains(v2f{x,y})) {
                imgui.hot = txt_id;
            }

            if (imgui.active.id == txt_id) {
                auto key = imgui.ctx.input->keyboard_input();
                if (key > 0) {                    
                    const_cast<char&>(text[(*position)++]) = key;
                    imgui.hot = txt_id;
                } else if ((*position) && imgui.ctx.input->pressed.keys[key_id::BACKSPACE]) {
                    (*position)--;
                    const_cast<char&>(text[(*position)]) = '\0';
                    imgui.hot = txt_id;
                }
                if (imgui.ctx.input->pressed.keys[key_id::ENTER]) {
                    imgui.active = 0;
                    result = true;
                }
            }
            if (imgui.hot.id == txt_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = txt_id;
                }
            }
            auto shadow_cursor = temp_cursor + imgui.theme.shadow_distance;
            string_render(&imgui.ctx, text, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            string_render(&imgui.ctx, text, &shadow_cursor, imgui.theme.shadow_color);

            if (imgui.hot.id != txt_id && *position == 0) {
                draw_rect(&imgui.ctx, text_box, imgui.theme.fg_color);
            }
            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += font_size.x;
            } else {
                imgui.panel->draw_cursor.y = temp_cursor.y - font_size.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
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

            math::aabb_t<v2f> bg_box;
            const auto start = bg_box.min = imgui.panel->draw_cursor + imgui.theme.padding;
            bg_box.max = start + size;

            const v2f box_size{size.x / (f32)value_count, size.y};

            range_u64(i, 0, value_count) {
                f32 perc = f32(i)/f32(value_count);
                math::aabb_t<v2f> box;
                box.min = start + v2f{box_size.x*i, 0.0f};
                box.max = start + v2f{box_size.x*(i+1), size.y*(1.0f-(values[i]/max))};
                draw_rect(&imgui.ctx, box, imgui.theme.fg_color);
            }
            draw_rect(&imgui.ctx, bg_box, color);
            
            if (imgui.next_same_line) {
                imgui.panel->draw_cursor.x += size.x + imgui.theme.padding;
                imgui.next_same_line = false;
            } else {
                imgui.panel->draw_cursor.y += size.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }
            imgui.panel->expand(bg_box.max + v2f{imgui.theme.padding});
        }

        inline void
        checkbox(
            state_t& imgui, 
            bool* var,
            sid_t id = 0,
            v2f size = v2f{16.0f}
        ) {
            const sid_t btn_id = id ? id : (sid_t)var;
            math::aabb_t<v2f> box;
            v2f temp_cursor = imgui.panel->draw_cursor + v2f{imgui.theme.padding};
            box.min = temp_cursor;
            box.max = temp_cursor + v2f{size};

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
            }

            // draw_rect(&imgui.ctx, box, imgui.theme.border_color);
            draw_circle(&imgui.ctx, box.center(), box.size().x * 0.5f, *var ? imgui.theme.active_color : imgui.theme.disabled_color);
            box.min -= v2f{1.0f};
            box.max += v2f{1.0f};
            draw_circle(&imgui.ctx, box.center(), box.size().x * 0.5f, imgui.hot.id == btn_id ? imgui.theme.active_color : imgui.theme.border_color);
            // draw_rect(&imgui.ctx, box, *var ? imgui.theme.active_color : imgui.theme.disabled_color);
            
            if (imgui.next_same_line) {
                imgui.panel->draw_cursor.x += size.x + imgui.theme.padding;
                imgui.next_same_line = false;
            } else {
                imgui.panel->draw_cursor.y += size.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }
        }

        inline void
        bitflag(
            state_t& imgui,
            std::span<std::string_view> names,
            u64* flag
        ) {
            bool b[64];
            size_t i{0};
            for (const auto& name: names) {
                b[i] = (*flag) & (1ui64<<i);

                same_line(imgui);
                checkbox(imgui, &b[i]);
                b[i] ^= text(imgui, fmt_sv(" - {}", name));
                // *flag ^= (b[i] << i);
                if (b[i]) {
                    *flag |= (1ui64 << i); // Set the corresponding bit to 1
                } else {
                    *flag &= ~(1ui64 << i); // Set the corresponding bit to 0
                }

                i++;
            }
        }

        inline bool
        button(
            state_t& imgui, 
            std::string_view name,
            gfx::color32 color,
            gfx::color32 text_color,
            sid_t btn_id_ = 0,
            v2f size = v2f{16.0f},
            u8 font_size = 16
        ) {
            const sid_t btn_id = btn_id_ ? btn_id_ : sid(name);
            math::aabb_t<v2f> box;
            box.min = imgui.panel->draw_cursor + v2f{4.0f, 0.0f};
            box.max = imgui.panel->draw_cursor + v2f{size};

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

            const v2f text_size = font_get_size(imgui.ctx.dyn_font[font_size], name);
            box.expand(box.min + text_size + imgui.theme.margin * 2.0f);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (box.contains( v2f{x,y} )) {
                if (imgui.active.id == 0) {
                    imgui.hot = btn_id;
                }
            } else if (imgui.hot.id == btn_id) {
                imgui.hot = 0;
            }

            imgui.panel->expand(box.max + imgui.theme.padding);
            v2f same_line = box.min + imgui.theme.margin;
            string_render(&imgui.ctx, name, same_line, text_color, imgui.ctx.dyn_font[font_size]);
            draw_rect(&imgui.ctx, box, imgui.hot.id == btn_id ? imgui.theme.active_color : color);
            
            if (imgui.next_same_line) {
                imgui.panel->draw_cursor.x += box.size().x + imgui.theme.padding;
                imgui.next_same_line = false;
            } else {
                imgui.panel->draw_cursor.y += box.size().y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }

            return result;
        }

        
        inline bool
        button(
            state_t& imgui, 
            std::string_view name,
            sid_t btn_id_ = 0,
            v2f size = v2f{16.0f},
            u8 font_size = 16
        ) {
            return button(imgui, name, imgui.theme.fg_color, imgui.theme.text_color, btn_id_, size, font_size);
        }
        
        inline void
        vec3(
            state_t& imgui, 
            const v3f& data,
            f32 min = 0.0f,
            f32 max = 1.0f
        ) {

            const auto theme = imgui.theme;

            imgui.theme.fg_color = gfx::color::rgba::red;
            same_line(imgui);
            indent(imgui, v2f{28.0f,0.0f});
            float_slider(imgui, (f32*)&data.x, min, max);

            imgui.theme.fg_color = gfx::color::rgba::green;
            same_line(imgui);
            indent(imgui, v2f{28.0f,0.0f});
            indent(imgui, v2f{28.0f,0.0f});
            float_slider(imgui, (f32*)&data.y, min, max);

            imgui.theme.fg_color = gfx::color::rgba::blue;
            indent(imgui, v2f{28.0f,0.0f});
            indent(imgui, v2f{28.0f,0.0f});
            indent(imgui, v2f{28.0f,0.0f});
            float_slider(imgui, (f32*)&data.z, min, max);

            imgui.theme = theme;
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
    u32 id{0};
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
        if (c == 0) break;
        if (c > 32 && c < 128) {
            const auto glyph = font_get_glyph(font, cursor.x, cursor.y, c);

            const u32 v_start = safe_truncate_u64(vertices->count());
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

            const u32 font_id = font->id;
            const u32 texture_bit = (u32)BIT(30);

            v[0] = gui::vertex_t{ .pos = p0, .tex = c0, .img = font_id|texture_bit, .col = text_color};
            v[1] = gui::vertex_t{ .pos = p1, .tex = c1, .img = font_id|texture_bit, .col = text_color};
            v[2] = gui::vertex_t{ .pos = p2, .tex = c2, .img = font_id|texture_bit, .col = text_color};
            v[3] = gui::vertex_t{ .pos = p3, .tex = c3, .img = font_id|texture_bit, .col = text_color};

            i[0] = v_start + 0;
            i[1] = v_start + 2;
            i[2] = v_start + 1;

            i[3] = v_start + 1;
            i[4] = v_start + 2;
            i[5] = v_start + 3;

            cursor.x += glyph.screen.size().x + 1;
        } else if (c == '\n') {
            cursor.y += font_get_glyph(font, cursor.x, cursor.y, 'G').screen.size().y + 1;
            cursor.x = start_x;
        } else if (c == ' ') {
            cursor.x += font_get_glyph(font, cursor.x, cursor.y, 'G').screen.size().x + 1;
        } else if (c == '\t') {
            cursor.x += (font_get_glyph(font, cursor.x, cursor.y, 'G').screen.size().x + 1) * 4;
        }
    }
    if (text.size()) {
        cursor.y += font_get_glyph(font, cursor.x, cursor.y, 'G').screen.size().y + 1;
    }
    cursor.x = start_x; 
}



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
        zyy_profile(name, " {}{}", 
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
        
        state[3] = static_cast<uint64_t>(time(0)) ^ comp_time_entropy;
        state[2] = (static_cast<uint64_t>(time(0)) << 17) ^ this_entropy;
        state[1] = (static_cast<uint64_t>(time(0)) >> 21) ^ (static_cast<uint64_t>(~time(0)) << 13);
        state[0] = (static_cast<uint64_t>(~time(0)) << 23) ^ pid_entropy;
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
            // const u64 pid_entropy = utl::rng::fnv_hash_u64(RAND_GETPID);
        
            state = static_cast<uint64_t>(time(0)) + 7 * comp_time_entropy + 23 * this_entropy;
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

    auto norm_dist() {
        const auto theta = math::constants::tau32 * randf();
        const auto rho = glm::sqrt(-2.0f * glm::log(randf()));
        return rho * glm::cos(theta);
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

    f32 range(const math::aabb_t<f32>& range) {
        return this->randf() * range.size() + range.min;
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
    static v3f randv() {
        return {randf(), randf(), randf()};
    }
    static v3f randnv() {
        return {randn(), randn(), randn()};
    }
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
        glm::sin(glm::dot(in, v2f{12.9898f, 78.233f}))
        * 43758.5453123f
    );
}

inline f32 hash(v2f p) {
    p  = 50.0f*glm::fract( p*0.3183099f + v2f(0.71f,0.113f));
    return -1.0f+2.0f*glm::fract( p.x*p.y*(p.x+p.y) );
}

inline f32 
noise21(v2f in) {
    const v2f i = glm::floor(in);
    const v2f f = glm::fract(in);

    const f32 a = hash(i + v2f{0.0f, 0.0f});
    const f32 b = hash(i + v2f{1.0f, 0.0f});
    const f32 c = hash(i + v2f{0.0f, 1.0f});
    const f32 d = hash(i + v2f{1.0f, 1.0f});

    const v2f u = f*f*(3.0f-2.0f*f);

    return glm::mix(a, b, u.x) +
        (c - a) * u.y * (1.0f - u.x) +
        (d - b) * u.x * u.y;
}

f32 value_noise( v2f p ) {
    v2f i = glm::floor( p );
    v2f f = glm::fract( p );
	
	v2f u = f*f*(3.0f-2.0f*f);

    return glm::mix( glm::mix( hash( i + v2f(0.0,0.0) ), 
                     hash( i + v2f(1.0,0.0) ), u.x),
                glm::mix( hash( i + v2f(0.0,1.0) ), 
                     hash( i + v2f(1.0,1.0) ), u.x), u.y);
}

f32 fbm(v2f uv) {
    m22 m = m22( 1.6f, 1.2f, -1.2f,  1.6f);
    f32 f  = 0.5000f*noise21( uv ); uv = m*uv;
    f += 0.2500f*noise21( uv ); uv = m*uv;
    f += 0.1250f*noise21( uv ); uv = m*uv;
    f += 0.0625f*noise21( uv ); uv = m*uv;
    return f;
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

        // zyy_info("blob", "data_offset: {}", data_offset);
        
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

    // template <typename T>
    // void serialize(arena_t* arena, const std::optional<T>& opt) {
    //     serialize(arena, u8{opt?1ui8:0ui8});
    //     if (opt) {
    //         serialize(arena, *opt);
    //     }
    // }

    // template <typename T>
    // void serialize(arena_t* arena, const std::optional<T>& opt) {
    //     serialize(arena, u8{opt?1:0});
    //     if (opt) {
    //         serialize(arena, *opt);
    //     }
    // }

    template <>
    void serialize(arena_t* arena, const std::string_view& obj) {
        // note(zack): probably dont need to actually pass in arena, but it is safer(?) and more clear
        serialize(arena, obj.size());

        arena_alloc(arena, obj.size());

        std::memcpy(&data[serialize_offset], (std::byte*)obj.data(), obj.size());

        allocate(obj.size());
        serialize_offset += obj.size();
    }

    template <>
    void serialize(arena_t* arena, const string_t& str) {
        serialize(arena, str.sv());
    }

    template <typename T>
    void serialize(
        arena_t* arena, 
        const offset_array_t<T>& array
    ) {
        // allocate(8);
        //allocate(sizeof(T) * array.count);
        i64 data_offset = 8;// allocation_offset - serialize_offset;

        // zyy_info("blob", "data_offset: {}", data_offset);
        
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

        // return *(T*)(data+t_offset);

        T t;
        std::memcpy(&t, (T*)(data + t_offset), sizeof(T));

        return t;
        // return std::bit_cast<T>(*(T*)(data + t_offset));
    }   
    
    template<>
    std::string deserialize() {
        const size_t length = deserialize<u64>();
        std::string value((char*)&data[read_offset], length);
        advance(length);
        return value;
    }

    template<>
    string_t deserialize() {
        const size_t length = deserialize<u64>();
        string_t value(std::string_view{(char*)&data[read_offset], length});
        advance(length);
        return value;
    }

    // template<typename T>
    // std::optional<T> deserialize() {
    //     const u8 has_value = deserialize<u8>();
    //     if (has_value) {
    //         return (std::optional<T>)deserialize<T>();
    //     }
    // }
};

namespace res {

namespace magic {

constexpr auto make_magic(const char key[8]) {
    u64 res=0;
    for(size_t i=0;i<4;i++) { res = (res<<8) | key[i];}
    return res;
}
    constexpr u64 meta = 0xfeedbeeff04edead;
    constexpr u64 vers = 0x2;
    constexpr u64 mesh = 0x1212121212121212;
    constexpr u64 text = 0x1212121212121213;
    constexpr u64 skel = 0x1212691212121241;
    constexpr u64 anim = 0x1212691212121269;
    constexpr u64 mate = make_magic("MATERIAL");
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
        zyy_error("res", "Failed to open file");
        return 0;
    }

    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::byte* data = new std::byte[size];
    defer {
        delete [] data;
    };
    file.read((char*)data, size);
    memory_blob_t loader{data};
    
    // eat meta info
    const auto meta = loader.deserialize<u64>();
    const auto vers = loader.deserialize<u64>();

    assert(meta == magic::meta);
    assert(vers == magic::vers);

    pack_file_t* packed_file = arena_alloc_ctor<pack_file_t>(arena, 1);

    packed_file->file_count = loader.deserialize<u64>();
    packed_file->resource_size = loader.deserialize<u64>();

    const auto table_start = loader.deserialize<u64>();
    assert(table_start == magic::table_start);

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

    // zyy_info("res", "Loaded Resource File: {}", path);

    return packed_file;
}

void pack_file_print(
    pack_file_t* packed_file
) {
    zyy_info("res", "Asset File contains: {} files", packed_file->file_count);
    for (size_t i = 0; i < packed_file->file_count; i++) {
        zyy_info("res", "\tfile: {} - {}", packed_file->table[i].name.c_str(), packed_file->table[i].size);
    }
}

size_t pack_file_find_file(
    pack_file_t* pack_file,
    std::string_view file_name
) {
    for (size_t i = 0; i < pack_file->file_count; i++) {
        if (pack_file->table[i].name.c_str() == file_name) {
            return i;
        }
    }
    zyy_warn("pack_file", "Failed to find file: {}", file_name);
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
    zyy_warn("pack_file", "Failed to find file: {}", file_name);
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


namespace tween {

template <typename T>
inline T lerp(T a, T b, f32 t) {
    return b * t + a * (1.0f - t);
}
template <typename T>
inline T lerp_dt(T a, T b, f32 s, f32 dt) {
    return lerp(b, a, std::pow(1.0f - s, dt));
}
template <typename T>
inline T damp(T a, T b, f32 lambda, f32 dt) {
    return lerp<T>(a, b, 1.0f - std::exp(-lambda * dt));
}

template <typename T>
inline T bilinear(T a, T b, T c, T d, f32 u, f32 v) {
    return glm::mix(
        glm::mix(a, b, v),
        glm::mix(c, d, v),
        u
    );
}

template <typename T>
T smoothdamp(
    T current, T target, T& velocity, 
    f32 smooth_time, f32 max_speed, f32 dt
) {
    smooth_time = glm::max(0.0001f, smooth_time);
    f32 omega = 2.0f / smooth_time;

    f32 x = omega * dt;
    f32 exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    auto change = current - target;
    auto original = target;
    auto maxChange = max_speed * smooth_time;
    change = glm::clamp(change, -maxChange, maxChange);
    target = current - change;
    auto temp = (velocity + omega * change) * dt;
    velocity = (velocity - omega * temp) * exp;
    auto output = target + (change + temp) * exp;

    if(original - current > 0.0f == output > original) {
        output = original;
        velocity = (output - original) / dt;
    }
    
    return output;
}

constexpr f32 smoothstep_cubic(f32 x) noexcept {
    return x*x*(3.0f-2.0f*x);
}

constexpr f32 smoothstep_quartic(f32 x) noexcept {
    return x*x*(2.0f*x*x);
}

constexpr f32 smoothstep_quintic(f32 x) noexcept {
    return x*x*x*(x*(x*6.0f-15.0f)+10.0f);
}

template <typename T>
constexpr T smoothstep_cubic(T a, T b, f32 t) noexcept {
    return a + (b-a) * smoothstep_cubic(t);
}

template <typename T>
constexpr T smoothstep_quartic(T a, T b, f32 t) noexcept {
    return a + (b-a) * smoothstep_quartic(t);
}
template <typename T>
constexpr T smoothstep_quintic(T a, T b, f32 t) noexcept {
    return a + (b-a) * smoothstep_quintic(t);
}

f32 in_out_sine(f32 x) noexcept {
    return -(glm::cos(math::constants::pi32 * x) - 1.0f) * 0.5f;
}

constexpr f32 in_elastic(f32 x) noexcept {
    const auto c4 = (2.0f * math::constants::pi32) / 3.0f;
    return x == 0.0f ? 0.0f : x == 1.0f ? 1.0f : -glm::pow(2.0f, 10.0f * x - 10.0f) * glm::sin((x * 10.0f - 10.75f) * c4);

}

constexpr f32 out_elastic(f32 x) noexcept {
    const auto c4 = (2.0f * math::constants::pi32) / 3.0f;
    return 
        x == 0.0f 
        ? 0.0f 
        : x == 1.0f 
        ? 1.0f 
        : glm::pow(2.0f, -10.0f * x) * glm::sin((x * 10.0f - 0.75f) * c4) + 1.0f;
}

constexpr f32 
in_out_elastic(f32 x) noexcept {
    const auto c5 = (2.0f * math::constants::pi32) / 4.5f;
    return x == 0.0f 
            ? 0.0f 
            : x == 1.0f 
            ? 1.0f
            : x < 0.5f
            ? -(glm::pow(2.0f, 20.0f * x - 10.0f) * glm::sin((20.0f * x - 11.125f) * c5)) * 0.5f 
            : (glm::pow(2.0f, -20.0f * x + 10.0f) * glm::sin((20.0f * x - 11.125f) * c5)) * 0.5f + 1.0f;
}

template <typename T>
auto generic(auto&& fn) {
    return [=](T a, T b, f32 x) {
        return a + (b-a) * fn(x);
    };
}

};
