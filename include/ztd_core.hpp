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

#define ztd_info(cat, str, ...) do { fmt::print(fg(fmt::color::white) | fmt::emphasis::bold, fmt_str("[info][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define ztd_warn(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::yellow) | fmt::emphasis::bold, fmt_str("[warn][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define ztd_error(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::red) | fmt::emphasis::bold, fmt_str("[error][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)
#define ztd_profile(cat, str, ...) do { fmt::print(stderr, fg(fmt::color::blue) | fmt::emphasis::bold, fmt_str("[profile][{}]: {}\n", cat, str), __VA_ARGS__); } while(0)



#define array_count(arr) (sizeof((arr)) / (sizeof((arr)[0])))
// #define array_count(arr) ((u32)std::size((arr)))
#define kilobytes(x) (x*1024ull)
#define megabytes(x) (kilobytes(x)*1024ull)
#define gigabytes(x) (megabytes(x)*1024ull)
#define terabytes(x) (gigabytes(x)*1024ull)
#define align16(val) ((val + 15) & ~15)
#define align4(val) ((val + 3) & ~3)
#define align_2n(val, size) ((val + size-1) & ~(size-1))

#define loop_iota_u64(itr, stop) for (size_t itr = 0; itr < stop; itr++)
#define range_u32(itr, start, stop) for (u32 itr = (start); itr < (stop); itr++)
#define range_u64(itr, start, stop) for (size_t itr = (start); itr < (stop); itr++)
#define range_f32(itr, start, stop) for (f32 itr = (start); itr < (stop); itr++)
#define loop_iota_i32(itr, stop) for (int itr = 0; itr < stop; itr++)

#if defined(GAME_USE_SIMD)
#include <immintrin.h>
#endif

#define WIN32_MEAN_AND_LEAN
#define NOMINMAX

#if defined(RAND_GETPID)

#elif defined(_WIN64) || defined(_WIN32)
    #include <process.h>


#if ZTD_INTERNAL

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#undef near
#undef far

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
#include <cstdio>
#include <span>
#include <array>
#include <fstream>
#include <chrono>
#include <functional>
#include <ranges>
#include <mutex>
#include <charconv>
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
using umm = std::uintptr_t;
using imm = std::ptrdiff_t;

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
using v2u = glm::uvec2;
using v3u = glm::uvec3;
using v4u = glm::uvec4;

using quat = glm::quat;
using m22 = glm::mat2x2;
using m33 = glm::mat3x3;
using m34 = glm::mat3x4;
using m43 = glm::mat4x3;
using m44 = glm::mat4x4;

using string_id_t = u64;
using sid_t = string_id_t;

constexpr static const quat quat_identity{1.0f, 0.0f, 0.0f, 0.0f};
static const m33  mat3_identity{1.0f};
static const m44  mat4_identity{1.0f};

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

#include "ztd_math.hpp"
#include "ztd_rng.hpp"

template <typename T, size_t N>
struct stack_buffer : public std::array<T, N>
{
    using std::array<T,N>::array;

    u64 _top{0};

    u64 count() const {
        return _top;
    }

    constexpr u64 capacity() const {
        return N;
    }

    void clear() {
        while(count()) {
            pop();
        }
    }

    b32 is_full() const {
        return _top == N;
    }

    b32 empty() const {
        return _top == 0;
    }

    std::span<T> view() {
        return std::span<T>{this->data(), _top};
    }

    void push(const T& data) {
        assert(count() < capacity());
        this->data()[_top++] = (data);
    }

    // noop when empty
    void pop() {
        // assert(count());
        if (_top) this->data()[--_top].~T();
    }

    T& top() {
        assert(count());
        return this->data()[_top-1];
    }

    operator T*() {
        return this->data();
    }
};

template <size_t N, typename CharType = char>
struct stack_string 
{
    CharType buffer[N] = {};
    
    constexpr stack_string() = default;
    constexpr stack_string(std::string_view str) {
        size_t s = std::min(N-1, str.size());

        for (size_t i = 0; i < s; ++i) {
            buffer[i] = str[i];
        }
        buffer[s] = '\0'; // Ensure null-terminated.
    }

    constexpr const CharType* data() const {
        return buffer;
    }
    constexpr CharType* data() {
        return buffer;
    }
    // constexpr const CharType* data() const {
    //     return buffer;
    // }
    
    constexpr std::string_view sv() const {
        return view();
    }
    constexpr std::string_view view() const {
        return std::string_view{buffer};
    }

    constexpr auto size() const {
        return std::string_view{buffer}.size();
    }

    constexpr size_t capacity() const {
        return N;
    }

    constexpr bool empty() const {
        return buffer[0] == 0;
    }

    constexpr bool operator==(std::string_view o) const {
        return std::string_view{buffer} == o;
    }
    constexpr bool operator!=(std::string_view o) const {
        return !this->operator==(o);
    }

    constexpr stack_string<N, CharType>& operator+=(const char* str) {
        size_t len = std::strlen(str);
        size_t current_size = size();

        if (current_size + len < N) {
            for (size_t i = 0; i < len; ++i) {
                buffer[current_size + i] = str[i];
            }

            current_size += len;
            buffer[current_size] = '\0'; // Ensure null-terminated.
        }

        return *this;
    }

    constexpr stack_string<N, CharType>& operator+=(const stack_string<N, CharType>& other) {
        size_t len = other.size();
        if (this->size() + len < N) {
            std::strncat(buffer, other.buffer, len);
        }
        return *this;
    }

    constexpr stack_string<N, CharType>& operator=(std::string_view str) {
        size_t s = str.size();
        assert(s < N);

        if (s < N) {
            for (size_t i = 0; i < s; ++i) {
                buffer[i] = str[i];
            }
            buffer[s] = '\0';
        }

        return *this;
    }

    constexpr stack_string<N, CharType>& operator=(const char* str) {
        size_t s = std::strlen(str);
        assert(s < N);

        if (s < N) {
            for (size_t i = 0; i < s; ++i) {
                buffer[i] = str[i];
            }
            buffer[s] = '\0';
        }

        return *this;
    }

    constexpr CharType& operator[](std::size_t i) {
        return buffer[i];
    }
    constexpr const CharType& operator[](std::size_t i) const {
        return buffer[i];
    }

    constexpr void clear() {
        std::memset(buffer, 0, N);
    }

    constexpr operator const char*() const {
        return buffer;
    }

    constexpr operator bool() const {
        return buffer[0] != 0;
    }

    constexpr std::span<CharType, N> span() {
        return std::span<CharType, N>{buffer};
    }
};


template<>
struct fmt::formatter<v2f>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(v2f const& v, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "[{:.2f}, {:.2f}]", v.x, v.y);
    }
};

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

template<>
struct fmt::formatter<quat>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(quat const& v, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "[{:.2f}, {:.2f}, {:.2f}, {:.2f}]", v.x, v.y, v.z, v.w);
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

struct arena_settings_t {
    size_t alignment{16};
    size_t minimum_block_size{kilobytes(1)}; // tune this

    using allocation_function = void*(*)(size_t);
    using free_function = void(*)(void*);

    allocation_function allocate{0};
    free_function       free{0};

    u8     fixed{0};
};

struct arena_block_footer_t {
    std::byte* base{0};
    size_t size{0};
    size_t used{0};
};

struct allocation_tag_t {
    const char* type_name;
    const char* file_name;
    const char* function_name;
    u64 line_number;

    u64 count;
    u64 type_size;
    u64 block_id; // might not be needed
    allocation_tag_t* next;

    u64 pad[40] = {};

    void poison() {
        range_u64(i, 0, array_count(pad)) {
            pad[i] = ~0ui64;
        }
    }

    b32 overflowed() const {
        range_u64(i, 0, array_count(pad)) {
            if (pad[i] != ~0) return true;
        }
        return false;
    }
};

struct arena_t {
    std::byte* start{0};
    size_t top{0};
    size_t size{0};

    u32 block_count{0};

    allocation_tag_t* tags{0};

    arena_settings_t settings{};
};

struct temporary_arena_t {
    allocation_tag_t* last_tag{0};
    arena_t* arena{0};
    std::byte* base{0};
    umm used{0};
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

template<typename Fn>
struct _auto_deferer_t {
    std::function<void(void)> fn;

    _auto_deferer_t(Fn _fn) 
        : fn{_fn}
    {
    }

    ~_auto_deferer_t() {
        fn();
    }
};

#define VAR_CAT(a,b) a ## b
#define VAR_CAT_(a,b) VAR_CAT(a, b)
#define defer _auto_deferer_t VAR_CAT_(_res_auto_defer_, __LINE__) = [&]()


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
    BACKTICK =96, /* ` */
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

        v2f pos2() const noexcept {
            return v2f{pos[0], pos[1]};
        }
        v2f delta2() const noexcept {
            return v2f{delta[0], delta[1]};
        }

        // preserved, everything above here is reset every frame    
        f32 delta[2];
        v2f scroll={};
    } mouse;

    struct text_input_callback_t {
        b32 fire   = 0;
        b32 active = 0;
        u32 key    = 0;
    } text{};

    f32 render_time;
    f32 render_dt;

    char keyboard_input() noexcept {
        text.active = 1;
        return text.fire ? text.fire = 0, (char)text.key : 0;

        const bool shift = keys[key_id::LEFT_SHIFT] || keys[key_id::RIGHT_SHIFT];

        range_u64(c, key_id::SPACE, key_id::BACKTICK+1) {
            if (pressed.keys[c]) { 
                pressed.keys[c] = 0;
                if (pressed.keys[key_id::BACKSLASH] && shift) return '|';
                if (shift) {
                    if (c >= key_id::NUM_0 && c <= key_id::NUM_9) {
                        return ")!@#$%^&*("[c-key_id::NUM_0];
                    }
                    if (c == key_id::MINUS) {
                        return '_';
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
    std::memset(input, 0, offsetof(app_input_t, mouse.delta));
    //input->text.fire = 0;
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
    u64 VERSION{0};
    u8 channels{100};
    u8 master_volumn{100};
    u8 sfx_volumn{100};
    u8 music_volumn{100};

    u8 mute{0};
};

 
namespace utl {
    struct closure_t;
};


#if ZTD_INTERNAL


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
    u64         history[1024];

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

        if (record->set_breakpoint && check_for_debugger()) [[unlikely]] {
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

#include "ztd_physics.hpp"

struct platform_api_t {
    using allocate_memory_function = void*(*)(umm);
    using free_memory_function = void(*)(void*);

    using unload_library_function = void(*)(void*);
    using load_library_function = void*(*)(const char*);
    using load_module_function = void*(*)();
    using load_proc_function = void*(*)(void*, const char*);

    allocate_memory_function allocate;
    free_memory_function free;

    unload_library_function unload_library;
    load_library_function load_library;
    load_module_function load_module;
    load_proc_function load_proc;

    template <typename ReturnType>
    ReturnType load_function(void* lib, const char* name) {
        return reinterpret_cast<ReturnType>(load_proc(lib, name));
    }

    using message_box_function = i32(*)(const char*);
    message_box_function message_box{0};

    const char** arguments{0};
    int argument_count{0};

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

    void* game_state{0};

    void* imgui_context{0};

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
#define node_prev(node) (node = node->prev)
#define node_push(node, list) do { node->next = list; list = node; } while(0)
#define node_pop(node, list) do { node = list; list = list->next; } while(0)
#define node_last(list) do { while (list->next && list=list->next);} while(0)

#define node_for(type, node, itr) for(type* itr = (node); (itr); node_next(itr))

u32 node_count(auto* node) {
    u32 result = 0;
    node_for(auto, node, n) {
        result += 1;
    }
    return result;
}

#define dlist_for(sen, ele) \
    for (auto* (ele) = (sen).next;  \
        (ele) != &(sen);            \
        node_next((ele)))
#define dlist_remove(ele) \
    (ele)->prev->next = (ele)->next; \
    (ele)->next->prev = (ele)->prev;


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

#define dlist_range(ele, name) for (auto* (name) = (ele)->next; (name) != (ele); node_next((name))) 

#define dlist_empty(ele) ((ele)->next == (ele))

u32 dlist_count(auto& head) {
    u32 result = 0;
    for (auto* n = head.next;
        n != &head;
        n = n->next
    ) {
        result += 1;
    }
    return result;
}

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

template <typename T>
std::optional<T> cond(b32 x, T&& v) {
    return x ? std::optional<T>{v} : std::nullopt;
}

std::string_view trim_filename(std::string_view str, char slash = '\\') {
    return str.substr(str.find_last_of(slash) + 1);
}


using string_pair = std::pair<std::string_view, std::string_view>;

// returns [before, after]
// returns [text, ""] if not found
constexpr string_pair cut_left(std::string_view text, std::string_view delim) {
    auto pos = text.find_first_of(delim);
    if (pos != std::string_view::npos) {
        return {text.substr(0, pos), text.substr(pos+1)};
    } else {
        return {text, ""sv};
    }
}


std::string_view cut(std::string_view str, char start, char end, u32 start_add = 0) {
    auto beg = str.substr(str.find_first_of(start) + 1 + start_add);
    if (beg.empty() == false) {
        auto end_pos = beg.substr(0, beg.find_first_of(end));
        return end_pos;
    }
    return {};
}

std::string_view get_extension(std::string_view str) {
    return str.substr(str.find_last_of('.') + 1);
}

bool has_extension(std::string_view str, std::string_view ext) {
    return get_extension(str) == ext;
}

struct timer_t {
    f32 _time{1.0f};
    f32 _timer{0.0f};

    static timer_t create(f32 t, b32 one_shot = 1) {
        timer_t result = {};
        result.start(t, one_shot);
        return result;
    }

    void start(f32 t, b32 one_shot = 1) {
        _timer = _time = t;
        flags = one_shot ? timer_flags::one_shot : timer_flags::repeat;
    }
    
    b32 tick(f32 dt) {
        if (_time && (_timer -= dt) <= 0.0f) {
            if (flags == timer_flags::repeat) {
                _timer = _time;
            } else {
                _time = 0.0f;
            }
            return 1;
        }
        return 0;
    }

    enum struct timer_flags : u32 { 
        one_shot, repeat
    } flags = timer_flags::one_shot;
};

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

void*
copy(void* dst, const void* src, umm size) {    
    return std::memmove(dst, src, size);
    return std::memcpy(dst, src, size);
}

template <typename T, typename S>
T*
copy(T* dst, const S* src) {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_copyable_v<S>);
    return (utl::copy(dst, src, std::min(sizeof T, sizeof S)), dst);
}

constexpr void*
ccopy(char* dst, const char* src, umm size) {
    for (umm i = 0; i < size; i++) {
        *(dst+i) = *(src + i);
    }
    return dst;
}

} // namespace utl


inline arena_t
arena_create(void* p, size_t bytes) noexcept {
    arena_t arena{};
    arena.start = (std::byte*)p;
    arena.size = bytes;
    arena.top = 0;
    arena.block_count = 0;
    arena.settings.alignment = 0;
    arena.settings.fixed = 1;
    arena.settings.minimum_block_size = 0;
    arena.tags = 0;
    return arena;
}

inline arena_t
arena_create(umm minimum_block_size = 0) noexcept {
    arena_t arena = {};
    if (minimum_block_size) {
        arena.settings.minimum_block_size = minimum_block_size;
    } else {
        arena.settings.minimum_block_size = kilobytes(4);
    }
    arena.settings.alignment = 16;
    return arena;
}

static void 
arena_add_tag(arena_t* arena, allocation_tag_t* tag) {
    if (arena->settings.fixed) {
        tag->block_id = 0;
    } else {
        assert(arena->block_count);
        tag->block_id = arena->block_count - 1;
        tag->poison();
    }
    node_push(tag, arena->tags);
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


inline void arena_sweep_keep(arena_t* arena, std::byte* one_past_end) {
    assert(one_past_end > arena->start);
    assert(arena->start + arena->size > one_past_end);
    umm size = one_past_end - arena->start;
    arena->top = std::max(size, arena->top);
}

inline std::byte*
arena_get_top(arena_t* arena) {
    return arena->start + arena->top;
}

// footer goes on end
arena_block_footer_t*
arena_get_footer(arena_t* arena) {
    return (arena_block_footer_t*)(arena->start + arena->size);
}

#define arena_display_info(arena, name) \
    (fmt_sv("{}[{} Blocks]: {} {} / {} {} - {:.2}%", \
    (name), \
    (arena)->block_count,\
    (arena)->top > gigabytes(1) ? (arena)->top/gigabytes(1) : (arena)->top > megabytes(1) ? (arena)->top/megabytes(1) : (arena)->top > kilobytes(1) ? (arena)->top / kilobytes(1) : (arena)->top,\
    (arena)->top > gigabytes(1) ? "Gb" : (arena)->top > megabytes(1) ? "Mb" : (arena)->top > kilobytes(1) ? "Kb" : "B",\
    (arena)->size > gigabytes(1) ? (arena)->size/gigabytes(1) : (arena)->size > megabytes(1) ? (arena)->size/megabytes(1) : (arena)->size > kilobytes(1) ? (arena)->size / kilobytes(1) : (arena)->size,\
    (arena)->size > gigabytes(1) ? "Gb" : (arena)->size > megabytes(1) ? "Mb" : (arena)->size > kilobytes(1) ? "Kb" : "B",\
    ((f64((arena)->top) / f64((arena)->size)) * 100.0)))
    

inline std::byte*
push_bytes(arena_t* arena, size_t bytes) {
    if (!arena->settings.fixed) {
        if (arena->top + bytes >= arena->size) [[unlikely]] {
            umm block_size = std::max(bytes + sizeof(arena_block_footer_t), arena->settings.minimum_block_size);
            block_size = std::max(block_size, kilobytes(64));
            
            // arena->settings.minimum_block_size = std::max(block_size, arena->settings.minimum_block_size);

            // if ((arena->block_count % 10) == 1) {
            //     arena->settings.minimum_block_size *= 2;
            // }

            arena_block_footer_t footer;
            footer.base = arena->start;
            footer.size = arena->size;
            footer.used = arena->top;

            if (!arena->settings.allocate) {
                assert(Platform.allocate);
                arena->settings.allocate = Platform.allocate;
                arena->settings.free = Platform.free;
            }

            arena->start = (std::byte*)arena->settings.allocate(block_size);
            assert(arena->start);
            arena->top = 0;
            arena->size = block_size - sizeof(arena_block_footer_t);

            if (footer.base) {
                *arena_get_footer(arena) = footer;
            }

            arena->block_count++;
        }
    }

    assert(arena->start);
    if (arena->settings.alignment) {
        arena->top = align_2n(arena->top, arena->settings.alignment);
    }

    std::byte* start = arena->start + arena->top;

    arena->top += bytes;
    assert(arena->top <= arena->size && "Arena overflow");

    return start;
}

static allocation_tag_t*
get_allocation_tag(void* ptr) {
    auto* tag = ((allocation_tag_t*)ptr)-1;
    // if (tag->magic == std::string_view{"ATAG"}) {
    return tag;
    // }
    // return 0;
}

#if ZTD_INTERNAL
    #define tag_struct(src, type, arena, ...) \
        arena_add_tag((arena), push_struct<allocation_tag_t>((arena), 1, allocation_tag_t{#type, __FILE__, __FUNCTION__, (u64)__LINE__, 1, sizeof(type)}));\
        src = push_struct<type>((arena), 1, __VA_ARGS__)
#else 
    #define tag_struct(src, type, arena, ...) \
        src = push_struct<type>((arena), 1, __VA_ARGS__)
#endif


#if ZTD_INTERNAL
    #define tag_array(src, type, arena, count, ...) \
        arena_add_tag((arena), push_struct<allocation_tag_t>((arena), 1, allocation_tag_t{#type, __FILE__, __FUNCTION__, (u64)__LINE__, (count), sizeof(type)}));\
        src = push_struct<type>((arena), (count), __VA_ARGS__)
#else 
    #define tag_array(src, type, arena, count, ...) \
        src = push_struct<type>((arena), (count), __VA_ARGS__)
#endif

template <typename T>
inline T*
push_struct(arena_t* arena, size_t count = 1) {
    // static_assert(std::is_trivially_copyable_v<T>);
    T* t = reinterpret_cast<T*>(push_bytes(arena, sizeof(T) * count));
    for (size_t i = 0; i < count; i++) {
        new (t + i) T();
    }
    return t;
}

template <typename T, typename ... Args>
inline T*
push_struct(arena_t* arena, size_t count, Args&& ... args) {
    T* t = reinterpret_cast<T*>(push_bytes(arena, sizeof(T) * count));
    for (size_t i = 0; i < count; i++) {
        new (t + i) T(std::forward<Args>(args)...);
    }
    return t;
}

template <typename T>
inline T*
bootstrap_arena_(umm offset, umm minimum_block_size = 0) {
    arena_t arena = arena_create(std::max(sizeof(T) + sizeof(arena_block_footer_t) + sizeof(allocation_tag_t), minimum_block_size));
    tag_struct(auto* obj, T, &arena);
    // auto* obj = push_struct<T>(&arena);
    *((arena_t*)((u8*)obj + offset)) = arena;
    return obj;
}

#define bootstrap_arena(type, ...) bootstrap_arena_<type>(offsetof(type, arena), __VA_ARGS__)


// todo add file and line for tagging
arena_t 
arena_sub_arena(arena_t* arena, size_t size) {
    using subarena_t = std::byte;
    tag_array(auto* bytes, subarena_t, arena, size);
    return arena_create(bytes, size);
}

inline frame_arena_t
arena_create_frame_arena(
    arena_t* arena, 
    size_t bytes
) {
    frame_arena_t frame{};
    frame.arena[0] = arena_sub_arena(arena, bytes);
    frame.arena[1] = arena_sub_arena(arena, bytes);
    return frame;
}

inline void
arena_begin_sweep(arena_t* arena) {
    assert(arena->block_count == 0);
    arena->top = 0;
}

arena_t arena_get_block(arena_t* arena, u64 block_index) {
    arena_t result = *arena;
    assert(block_index < arena->block_count);

    range_u64(i, 1, arena->block_count - block_index) {
        auto* footer = arena_get_footer(&result);
        result.start = footer->base;
        result.top = footer->used;
        result.size = footer->size;
    }

    return result;
}

void
arena_free_block(arena_t* arena) {
    assert(arena->block_count);
    auto* block = (void*)arena->start;

    auto* footer = arena_get_footer(arena);

    arena->start = footer->base;
    arena->top = footer->used;
    arena->size = footer->size;

    assert(arena->settings.free);

    arena->block_count--;

    arena->settings.free(block);
}

inline void
arena_clear(arena_t* arena) {
    arena->tags = 0;
    // ztd_warn(__FUNCTION__, "Arena cleared: {}", (void*)arena);
    if (arena->settings.fixed) {
        arena->top = 0;
    } else {
        // @Explain
        // bootstrapped arena will be invalidated if they are freed
        u64 block_count = arena->block_count;
        while(block_count) { 
            block_count = arena->block_count - 1;
            arena_free_block(arena);
        }
    }
}



temporary_arena_t
begin_temporary_memory(arena_t* arena) {
    temporary_arena_t temporary{};
    temporary.arena = arena;
    temporary.base = arena->start;
    temporary.used = arena->top;
    temporary.last_tag = arena->tags;
    return temporary;
};

void 
end_temporary_memory(temporary_arena_t memory) {
    // return;
    auto* arena = memory.arena;
    while(arena->start != memory.base) {
        arena_free_block(arena);
    }
    arena->tags = memory.last_tag;
    arena->top = memory.used;
    // arena->temporary_count--;
}

arena_t* co_stack(coroutine_t* coro, frame_arena_t& frame_arena) {
    // if (coro->running == false) return 0;
    if (coro->stack) {
        auto* stack = (u8*)push_bytes(&frame_arena.get(), coro->stack_size);
        utl::copy(stack, coro->stack, coro->stack_size);
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
    if (arena) { push_struct<T>(arena, count); }
    return data;
}

#define co_push_stack(coro, stack, type) co_push_stack_<type>(coro, stack) 
#define co_push_array_stack(coro, stack, type, count) co_push_stack_<type>(coro, stack, count) 

namespace utl {
    // Todo(Zack): Merge blocks
struct allocator_t {
    struct memory_block_t {
        void* start = 0;
        u64   size  = 0;
        memory_block_t* next = this;
        memory_block_t* prev = this;
    };

    arena_t arena{};
    arena_t* block_arena{&arena};
    memory_block_t used_blocks{};
    memory_block_t free_blocks{};

    virtual ~allocator_t() = default;

    // void* allocate(u64 size, u64 line_number, const char* filename, const char* type_name, const char* function_name) {
    // todo add source location
    void* allocate(u64 size) {
        // ztd_info(__FUNCTION__, "{} free blocks", dlist_count(free_blocks));
        dlist_range(&free_blocks, block) {
            if (block->size < size) { continue; }
            // if (block->size < size || size*2 < block->size) { continue; }

            if (block->size == size || arena.settings.fixed) {
                dlist_remove(block);

                dlist_insert_as_last(&used_blocks, block);

                utl::memzero(block->start, block->size);

                return block->start;
            } else {
#if 1

                if (arena.settings.alignment) {
                    // todo
                    // assert(((u64)block->start & (arena.settings.alignment-1)) == 0);
                    // auto aligned_size = align_2n(size, arena.settings.alignment);
                    // new_block->start = (u8*)block->start + aligned_size;
                    // // new_block->start = (void*)align_2n((u64)new_block->start, arena.settings.alignment);
                    // // i64 offset = (u8*)new_block->start - (u8*)block->start;
                    // // assert(offset > 0);
                    // assert(aligned_size < block->size);

                    // new_block->size = block->size - aligned_size;
                    // block->size = size;

                    // utl::memzero(new_block->start, new_block->size);

                    // assert(new_block->size);
                } else {
                    tag_struct(auto* new_block, memory_block_t, block_arena);
                    new_block->start = (u8*)block->start + size;
                    assert(block->size > size);
                    new_block->size = block->size - size;
                    
                    block->size = size;
                    assert(block->size);
                    assert(new_block->size);
                    dlist_insert_as_last(&free_blocks, new_block);
                }
#endif
                
                dlist_remove(block);

                dlist_insert_as_last(&used_blocks, block);

                utl::memzero(block->start, size);

                return block->start;
            }
        }
        // if we reach here, theres no free blocks that fit

        tag_struct(auto* new_block, memory_block_t, block_arena);            
        std::byte* data;
        if (arena.settings.fixed) {
            data = push_bytes(&arena, size); // TODO(zack): breaks mesh allocators if you tag
        } else {
            using allocator_memory_t = std::byte;
            tag_array(data, allocator_memory_t, &arena, size);
        }

        utl::memzero(data, size);

        new_block->start = data;
        new_block->size  = size;

        dlist_insert_as_last(&used_blocks, new_block);

        return new_block->start;
    }

    void free(void* ptr) {
        // remove from used blocks
        dlist_range(&used_blocks, block) {
            if (block->start == ptr) {
                dlist_remove(block);
                dlist_insert_as_last(&free_blocks, block);
                return;
            }
        }
        // should not reach here
        assert(0);
    }
};

}

template <typename T>
struct buffer 
{
    T* data = 0;
    u64 count = 0;

    T* reallocate(arena_t* arena, u64 new_count) {
        tag_array(T* new_data, T, arena, new_count);
        return new_data;
    }

    T* reallocate(utl::allocator_t* allocator, u64 size) {
        auto* new_data = (T*)allocator->allocate(sizeof(T)*size);
        return new_data;
    }

    void reserve(utl::allocator_t* allocator, u64 size) {
        if (data) allocator->free(data);
        data = allocator->allocate(size*sizeof(T));
        count = size;
    }

    void reserve(arena_t* arena, u64 size) {
        data = reallocate(arena, size);
        count = size;
    }

    void sort(auto&& fn) {
        if (data) {
            std::sort(data, data + count, fn);
        }
    }

    template<typename Allocator>
    void push(Allocator* allocator, std::string_view t) {
        auto* new_data = reallocate(allocator, count+t.size()/sizeof(T));
        if (data) {
            std::memcpy(new_data, data, count*sizeof(T));
            if constexpr (std::is_same_v<Allocator, utl::allocator_t>) {
                allocator->free(data);
            }
        }
        data = new_data;
        std::memcpy(data+count, t.data(), t.size());
        count += t.size() / sizeof(T);


    }

    template<typename Allocator>
    void push(Allocator* allocator, std::span<T> v) {
        auto* new_data = reallocate(allocator, count+v.size());
        if (data) {
            std::memcpy(new_data, data, count*sizeof(T));
            if constexpr (std::is_same_v<Allocator, utl::allocator_t>) {
                allocator->free(data);
            }
        }
        data = new_data;
        std::memcpy(data+count, v.data(), v.size() * sizeof(T));
        count += v.size();
    }

    template<typename Allocator>
    void push(Allocator* allocator, const T& o) {
        auto* new_data = reallocate(allocator, count+1);
        if (data) {
            std::memcpy(new_data, data, count*sizeof(T));
            if constexpr (std::is_same_v<Allocator, utl::allocator_t>) {
                allocator->free(data);
            }
        }
        data = new_data;
        data[count++] = o;
    }

    std::span<T> view() const {
        return std::span{data, count};
    }

    std::string_view sv() const {
        return std::string_view{data, count};
    }

    auto& operator[](u64 index) {
        assert(index < count);
        return data[index];
    }

    bool operator==(const buffer<T>& o) const {
        if (this == &o) {
            return true;
        }
        if (count != o.count) {
            return false;
        }
        range_u64(i, 0, count) {
            if (data[i] != o.data[i]) return false;
        }
        return true;
    }
};

using string_buffer = buffer<char>;

template<>
struct std::hash<string_buffer>
{
    std::size_t operator()(string_buffer s) const noexcept
    {
        std::size_t h1 = std::hash<std::string_view>{}(s.sv());
        
        return h1;
    }
};


namespace utl {
    
template <typename Key, typename Value>
struct hash_trie_t {
    hash_trie_t<Key, Value>* child[4]={};
    Key key={};
    Value value={};
};

template <typename Key, typename Value>
Value* hash_get(hash_trie_t<Key,Value>** map, Key key, arena_t* arena=0, b32* had = 0) {
    for (u64 h = std::hash<Key>{}(key); *map; h>>=2) {
        if (key == (*map)->key) {
            if (had) {
                *had = 1;
            }
            return &(*map)->value;
        }
        map = &(*map)->child[h>>62];
    }
    if (!arena) {
        return 0;
    }
    if (had) {
        *had = 0;
    }
    *map = push_struct<hash_trie_t<Key, Value>>(arena);
    **map = {};
    // (*map)->value = {};
    (*map)->key = key;
    return &(*map)->value;
}

template <typename Key, typename Value>
void hash_foreach(hash_trie_t<Key,Value>* map, std::function<void(typename Key,typename Value*)> func) {
    if (!map) {
        return;
    }
    func(map->key, &map->value);
    for (u64 i = 0; i < array_count(map->child); i++) {
        if (map->child[i]) {
            hash_foreach(map->child[i], func);
        }
    }
}

struct string_builder_t {
    temporary_arena_t memory{};

    char* string{0};
    u64   size{0};

    static string_builder_t begin(arena_t* arena) {
        string_builder_t builder {
            .memory = begin_temporary_memory(arena)
        };

        return builder;
    }

    void end(arena_t* arena) {
        tag_array(char* str, char, arena, size);

        utl::copy(str, string, size);

        end_temporary_memory(memory);
    }

    void add(std::string_view str) {
        auto* new_str = push_bytes(memory.arena, size + str.size());
        if (size) {
            utl::copy(new_str, string, size);
        }
        utl::copy(new_str + size, str.data(), str.size());

        size += str.size();
    }
};

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

#define pool_display_info(pool, name) \
    (fmt_sv("{}: {} / {} - {:.2}%", \
    (name), \
    (pool)->top, \
    (pool)->capacity, \
    ((f64((pool)->top) / f64((pool)->capacity)) * 100.0)))

struct pool_base_t {
    size_t top = 0;
    size_t capacity{0};
};

template <typename T>
struct pool_t : pool_base_t {
    T* start = 0;

    size_t count() const {
        return top;
    }

    size_t size_bytes() const {
        return sizeof(T) * capacity;
    }

    pool_t() {
        capacity = 0;
        start = 0;
        top = 0;
    }

    explicit pool_t(T* data, size_t _cap) 
    {
        top = 0;
        capacity = _cap;
        start = data;
    }
    
    T& operator[](size_t id) {
        assert(id < capacity);
        return data()[id];
    }

    T& back() {
        return operator[](count()-1);
    }

    T& front() {
        return operator[](0);
    }

    T* data() {
        return start;
    }

    T* create(size_t p_count) {
        T* o = allocate(p_count);
        for (size_t i = 0; i < p_count; i++) {
            new (o+i) T();
        }
        return o;
    }

    T* allocate(size_t _count) {
        T* o = start+top;
        top+=_count;
        return o;
    }

    void free(T* ptr) {
        ptr->~T();
        std::swap(*ptr, back());
        top--;
    }

    // note(zack): calls T dtor
    void clear() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < count(); i++) {
                (data() + i)->~T();
            }
        }
        top = 0;
        // arena_clear(this);
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
        data = (char*)push_bytes(arena, size+1);
        utl::copy(data, str.data(), size);
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

namespace reflect {

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
            utl::copy((std::byte*)&obj + offset, &v, sizeof(Value));
        }
        
        template<typename Value>
        void set_value_raw(std::byte* obj, Value v) const noexcept {
            utl::copy(obj + offset, &v, sizeof(Value));
        }

        
        // read value from obj into memory
        object_t get_value_raw(std::byte* obj, std::byte* memory) const noexcept {
            object_t var{type, memory};
            utl::copy(var.data, obj + offset, size);
            return var;
        }

        template<typename Obj>
        object_t get_value(Obj& obj, std::byte* memory) const noexcept {
            object_t var{type, memory};
            utl::copy(var.data, ((std::byte*)&obj + offset), size);
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

        constexpr auto hash() const {
            return sid(name);
        }

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

} // namespace reflect

using reflect::type_info_t;
template<>
struct fmt::formatter<reflect::object_t> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(reflect::object_t const& o, FormatContext& ctx) {
        using reflect::type_tag;

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
            case type_tag::VOID_STAR:     return fmt::format_to(ctx.out(), "{}: {}", o.type->name, (void*)o.data);
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

    reflect::type_tag tag{reflect::type_tag::UNKNOWN};

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

    #undef ANY_T_RETURN_TYPE

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

namespace packing {
    v2u split_u64(u64 x) {
        return v2u{x&0xffffffff, (x>>32)&0xffffffff};
    }

    u64 pack(u32 a, u32 b) {
        u64 result = u64(a) | (u64(b)<<32);
        return result;
    }

    u32 pack(u8 r, u8 g, u8 b, u8 a) {
        u32 result = u64(a) | (u64(b)<<32);
        return result;
    }

    const std::array<u8, 4> unpack_u32(u32 c) {
        return {
            (c >> 0 ) & 0xff,
            (c >> 8 ) & 0xff,
            (c >> 16) & 0xff,
            (c >> 24) & 0xff
        };
    }

    const u32 pack_u32(u8 r, u8 g, u8 b, u8 a) {
        return 
            (r << 0 ) |
            (g << 8 ) |
            (b << 16) |
            (a << 24) ;
    }

    u32 pack1212to24(u16 a, u16 b) {
        u32 result = u32(a) | (u32(b)<<12);
        return result & (0x00ffffff);
    }

    u32 pack_normal(v3f n) {
        u32 r=0;
        r |= u32((n.x*0.5f+0.5f)*255.0f)<<0;
        r |= u32((n.y*0.5f+0.5f)*255.0f)<<8;
        r |= u32((n.z*0.5f+0.5f)*255.0f)<<16;
        return r;
    }

    v3f unpack_normal(u32 p) {
        v3f n;
        n.x = f32((p>>0)&0xff)/255.0f*2.0f-1.0f;
        n.y = f32((p>>8)&0xff)/255.0f*2.0f-1.0f;
        n.z = f32((p>>16)&0xff)/255.0f*2.0f-1.0f;
        return n;
    }

    v2u u32tov2u(u32 x) {
        return v2u(u16(x&0xffff), u16((x>>16)&0xffff));
    } 

    u32 pack_rgbe(v3f rgb) {
        const f32 max_val = std::bit_cast<f32>(0x477f8000ui32);
        const f32 min_val = std::bit_cast<f32>(0x37800000ui32);
        rgb = glm::clamp(rgb, v3f{0.0f}, v3f{max_val});

        float max_channel = glm::max(glm::max(min_val, rgb.r), glm::max(rgb.g, rgb.b));

        float bias = std::bit_cast<f32>((std::bit_cast<u32>(max_channel) + 0x07804000ui32) & 0x7f800000ui32);

        v3u RGB;
        RGB.x = std::bit_cast<u32>(rgb.x+bias);
        RGB.y = std::bit_cast<u32>(rgb.y+bias);
        RGB.z = std::bit_cast<u32>(rgb.z+bias);
        u32 e = (std::bit_cast<u32>(bias) << 4) + 0x10000000ui32;

        return e | RGB.b << 18 | RGB.g << 9 | (RGB.r & 0x1ffui32);
    }

    v3f unpack_rgbe(u32 p) {
        v3f rgb{v3u{p&0x1ff,(p>>9)&0x1ff,(p>>18)&0x1ff}};
        return glm::ldexp(rgb, v3i{(p>>27)-24});
    }

};

#if defined(ZTD_GRAPHICS)

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
        
        utl::copy(this, &o, sizeof(material_info_t));

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

    math::rect3d_t aabb{};


    // for RTX
    u32 blas{0};
    // u32 hit{0};
};

struct mesh_list_t {
    gfx::mesh_view_t* meshes{nullptr};
    size_t count{0};
    math::rect3d_t aabb{};

    std::string_view name{};

    // mesh_list_t* next_lod{0};
};

enum struct blend_mode {
    none,
    alpha_blend, 
    additive
};

enum struct render_command_type {
    draw_mesh,
    clear_texture,
    set_blend,
};

struct clear_texture_command_t {
    u64 texture_id{0};
};

struct draw_mesh_command_t {
    m44 transform;

    u64 mesh_id{0};
    u32 material_id{0};
    u32 albedo_id{~0ui32};
    u32 gfx_id{0};
    u64 gfx_count{0};

    u32 instance_offset{0};
    u32 instance_count{1};

    b32 rtx_on{1};
};

struct set_blend_command_t {
    blend_mode blend{blend_mode::alpha_blend};
};

struct batched_draw_t {
    void* buffer{0}; // api buffer
    u32   offset{0};
    u32   count {0};
};

struct render_command_t {
    render_command_type type{render_command_type::draw_mesh};

    union {
        draw_mesh_command_t draw_mesh{};
        set_blend_command_t blend_function;
        // clear_texture_command_t clear_texture;
    };

    void set_blend(blend_mode mode) {
        type = render_command_type::set_blend;
        blend_function = {};
        blend_function.blend = mode;
    }
};

struct render_group_t {
    buffer<render_command_t> commands{};
    u64                      size{0};
    
    buffer<batched_draw_t>   draw_batches{};
    
    render_command_t& push_command() {
        assert(size < commands.count);
        return commands[size++];
    }
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
    u32 index_start{0};

    gfx::mesh_list_t
    build(arena_t* arena) {
        gfx::mesh_list_t m;
        m.count = 1;
        m.meshes = push_struct<gfx::mesh_view_t>(arena, 1);
        m.meshes[0].vertex_start = vertex_start;
        m.meshes[0].vertex_count = safe_truncate_u64(vertices.count()) - vertex_start;
        m.meshes[0].index_start = index_start;
        m.meshes[0].index_count = safe_truncate_u64(indices.count()) - index_start;
        return m;
    }

    mesh_builder_t& add_vertex(
        v3f pos,
        v2f uv = v2f{0.0f, 0.0f},
        v3f nrm = axis::up,
        v3f col = v3f{1.0f}
    ) {
        *vertices.allocate(1) = gfx::vertex_t{.pos = pos, .nrm = nrm, .col = col, .tex = uv};
        return *this;
    }

    mesh_builder_t& add_vertex(gfx::vertex_t&& vertex) {
        *vertices.allocate(1) = std::move(vertex);
        return *this;
    }
    mesh_builder_t& add_index(u32 index) {
        *indices.allocate(1) = index;
        return *this;
    }

    mesh_builder_t& add_triangles(math::triangle_t* tris, u64 count) {
        loop_iota_u64(i, count) {
            u32 v_start = (u32)vertices.count();
            auto* v = vertices.allocate(3);

            const auto [t0, t1, t2] = tris[i].p;

            const v3f norm = glm::normalize(glm::cross(t1-t0,t2-t0));

            v[0].pos = t0;
            v[1].pos = t1;
            v[2].pos = t2;

            loop_iota_u64(j, 3) {
                v[j].nrm = norm;
                v[j].col = v3f{1.0f};
                v[j].tex = v2f{0.0f, 0.0f};
            }

            auto* t = indices.allocate(3);
            t[0] = v_start;
            t[1] = v_start+1;
            t[2] = v_start+2;
        }
        return *this;
    }

    mesh_builder_t& add_box(math::rect3d_t box) {
        
        const auto p0 = box.sample(v3f{0,0,0}); // min
        const auto p1 = box.sample(v3f{1,0,0}); // min right
        const auto p2 = box.sample(v3f{0,1,0}); // min up
        const auto p3 = box.sample(v3f{0,0,1}); // min forward

        const auto p4 = box.sample(v3f{1,0,1}); // min across
        const auto p5 = box.sample(v3f{1,1,0}); // min up right
        const auto p6 = box.sample(v3f{1,1,1}); // max
        const auto p7 = box.sample(v3f{0,1,1}); // min up forward

        math::triangle_t triangles[12]={
            math::triangle_t{{p0, p1, p3}},
            math::triangle_t{{p1, p3, p4}},
            math::triangle_t{{p0, p3, p7}},
            math::triangle_t{{p0, p7, p2}},
            math::triangle_t{{p0, p1, p5}},
            math::triangle_t{{p0, p5, p2}},
            math::triangle_t{{p6, p7, p2}},
            math::triangle_t{{p6, p2, p5}},
            math::triangle_t{{p6, p5, p1}},
            math::triangle_t{{p6, p1, p4}},
            math::triangle_t{{p4, p3, p7}},
            math::triangle_t{{p4, p7, p6}},
        };

        add_triangles(triangles, 12);

        return *this;
    }
    
    mesh_builder_t& add_box(math::rect3d_t box, const m44& transform) {
        
        const auto p0 = v3f{transform * v4f{box.sample(v3f{0,0,0}), 1.0f}}; // min
        const auto p1 = v3f{transform * v4f{box.sample(v3f{1,0,0}), 1.0f}}; // min right
        const auto p2 = v3f{transform * v4f{box.sample(v3f{0,1,0}), 1.0f}}; // min up
        const auto p3 = v3f{transform * v4f{box.sample(v3f{0,0,1}), 1.0f}}; // min forward
        const auto p4 = v3f{transform * v4f{box.sample(v3f{1,0,1}), 1.0f}}; // min across
        const auto p5 = v3f{transform * v4f{box.sample(v3f{1,1,0}), 1.0f}}; // min up right
        const auto p6 = v3f{transform * v4f{box.sample(v3f{1,1,1}), 1.0f}}; // max
        const auto p7 = v3f{transform * v4f{box.sample(v3f{0,1,1}), 1.0f}}; // min up forward

        math::triangle_t triangles[12]={
            math::triangle_t{{p0, p1, p3}},
            math::triangle_t{{p1, p3, p4}},
            math::triangle_t{{p0, p3, p7}},
            math::triangle_t{{p0, p7, p2}},
            math::triangle_t{{p0, p1, p5}},
            math::triangle_t{{p0, p5, p2}},
            math::triangle_t{{p6, p7, p2}},
            math::triangle_t{{p6, p2, p5}},
            math::triangle_t{{p6, p5, p1}},
            math::triangle_t{{p6, p1, p4}},
            math::triangle_t{{p4, p3, p7}},
            math::triangle_t{{p4, p7, p6}},
        };

        add_triangles(triangles, 12);

        return *this;
    }
    
};

using color4 = v4f;
using color3 = v3f;
using color32 = u32;
struct font_t;

static constexpr u32 material_lit          = (u32)BIT(0);
static constexpr u32 material_triplanar    = (u32)BIT(1);
static constexpr u32 material_billboard    = (u32)BIT(2);
static constexpr u32 material_wind         = (u32)BIT(3);
static constexpr u32 material_water        = (u32)BIT(4);
static constexpr u32 material_sprite_sheet = (u32)BIT(5);

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
    f32 reflectivity{0.0f};
    u32 sprite_info{0}; // 4 packed 8bits [index, dimx, dimy]
    u32 pad;

    void set_sprite_index(u8 index) {
        auto [_index, dimx, dimy, _] = packing::unpack_u32(sprite_info);
        set_sprite_info(index, dimx, dimy);
    }

    void set_sprite_info(u8 index, u8 dimx, u8 dimy) {
        sprite_info = packing::pack_u32(index, dimx, dimy, 0);
    }

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
        return add_push_constant(sizeof(T));
    }

    constexpr shader_description_t& add_push_constant(u32 size) {
        push_constant_size += size;
        assert(push_constant_size <= 128);
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
                ztd_error(__FUNCTION__, "Not enough vertices supplied: {} were given", vertices.size());
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
    f32 depth,
    v2f screen_size,
    utl::pool_t<gui::vertex_t>* vertices,
    utl::pool_t<u32>* indices,
    color32 text_color);


namespace color {

    enum struct color_variant_type {
        hex,
        uniform,
        range,
        gradient,
        curve,
        SIZE
    };

    struct gradient_t {
        u32  count{0};
        v4f* colors{0};
        f32* positions{0}; // 0..1

        b32 is_sorted() {
            range_u32(i, 1, count) {
                if (positions[i-1] > positions[i]) {
                    return false;
                }
            }
            return true;
        }

        f32 nearest_distance(f32 t) {
            f32 min_distance = 1.01f;
            range_u32(i, 0, count) {
                const auto distance = fabsf(t - positions[i]);
                if (distance < min_distance) {
                    min_distance = distance;
                }
            }
            return min_distance;
        }

        u32 closest(f32 t) {
            u32 result = count + 1;
            f32 min_distance = 1.01f;
            range_u32(i, 0, count) {
                const auto distance = fabsf(t - positions[i]);
                if (distance < min_distance) {
                    result = i;
                    min_distance = distance;
                }
            }
            return result;
        }

        void remove(u32 i) {
            if (i < count) {
                std::swap(colors[i], colors[count-1]);
                std::swap(positions[i], positions[count-1]);
                count--;
            }
        }

        v4f sample(f32 t) {
            if (count == 0) return v4f{1.0f, 0.0f, 1.0f, 1.0f};

            if (t < positions[0]) {
                return colors[0];
            }

            range_u32(i, 1, count) {
                if ((positions[i-1] < t && positions[i] >= t)) {
                    auto lt = (t - positions[i-1]) / (positions[i] - positions[i-1]);
                        // return glm::mix(math::sqr(colors[i-1]), math::sqr(colors[i]), glm::clamp(lt, 0.0f, 1.0f));
                    return glm::sqrt(glm::mix(math::sqr(colors[i-1]), math::sqr(colors[i]), glm::clamp(lt, 0.0f, 1.0f)));
                    return glm::mix((colors[i-1]), (colors[i]), glm::clamp(lt, 0.0f, 1.0f));
                }
            }
            
            if (t > positions[count-1]) {
                return colors[count-1];
            }
            return v4f{1.0f, 0.0f, 1.0f, 1.0f};
            // return colors[0];
        }

        void add(arena_t* arena, v4f color, f32 position) {
            add(arena, &color, &position, 1);
        }

        void add(arena_t* arena, v4f* cs, f32* ps, u32 size) {
            tag_array(auto* new_colors, v4f, arena, count+size);
            tag_array(auto* new_positions, f32, arena, count+size);

            if (count > 0) {
                utl::copy(new_colors, colors, sizeof(v4f) * count);
                utl::copy(new_positions, positions, sizeof(f32) * count);
            }

            utl::copy(new_colors + count, cs, sizeof(v4f) * size);
            utl::copy(new_positions + count, ps, sizeof(f32) * size);
            
            colors = new_colors;
            positions = new_positions;

            count += size;
        }

        void sort() {
            if (count == 0) return;

            range_u32(w, 0, count-1) {
                u32 min{w};
                range_u32(r, w+1, count) {
                    if (positions[r] < positions[min]) {
                        min = r;
                    }
                }

                if (min != w) {
                    std::swap(positions[w], positions[min]);
                    std::swap(colors[w], colors[min]);
                }
            }
        }
    };

    const std::array<u8, 4> unpack_color32(color32 c) {
        return packing::unpack_u32(c);
    }

    const color32 pack_color32(u8 r, u8 g, u8 b, u8 a) {
        return packing::pack_u32(r, g, b, a);
    }

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
    
    constexpr color32 modulate(color32 c, f32 f, std::optional<f32> a = std::nullopt) {
        return to_color32(to_color4(c)*v4f{f,f,f,a?*a:f});
    }
    constexpr color32 modulate(color32 c, v4f v) {
        return to_color32(to_color4(c)*v);
    }

    constexpr color32 modulate(color32 c, v3f v) {
        return to_color32(to_color3(c)*v);
    }

    constexpr color32 gamma(color32 c) {
        auto v = to_color3(c);
        return to_color32(v*v);
    }

    constexpr color32 flatten_color(color32 c) {
        return to_color32(glm::mix(to_color3(c), v3f{0.5f}, 0.5f) ); 
    }

    constexpr color32 lerp(color32 a, color32 b, f32 t) {
        return to_color32(glm::mix(to_color4(a),to_color4(b),t));
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
        static constexpr auto clear_alpha = "#000000ff"_rgba;
        static constexpr auto black = "#000000ff"_rgba;
        static constexpr auto dark_gray = "#111111ff"_rgba;
        static constexpr auto darkish_gray = "#5a5a5aff"_rgba;
        static constexpr auto ue5_bg = "#262626ff"_rgba;
        static constexpr auto ue5_grid = "#2e2e2eff"_rgba;
        static constexpr auto ue5_dark_grid = dark_gray;
        static constexpr auto gray = "#808080ff"_rgba;
        static constexpr auto light_gray  = "#d3d3d3ff"_rgba;
        static constexpr auto white = "#ffffffff"_rgba;
        static constexpr auto cream = gamma("#fafafaff"_rgba);
        static constexpr auto red   = "#ff0000ff"_rgba;
        static constexpr auto reddish = "#fa2222ff"_rgba;
        static constexpr auto green = "#00ff00ff"_rgba;
        static constexpr auto dark_red = gamma("#282021ff"_rgba);
        static constexpr auto light_green = "#587068ff"_rgba;
        static constexpr auto dark_green = "#282921ff"_rgba;
        static constexpr auto blue  = "#0000ffff"_rgba;
        static constexpr auto light_blue  = "#2222f2ff"_rgba;
        static constexpr auto yellow= "#ffff00ff"_rgba;
        static constexpr auto cyan = "#00ffffff"_rgba;
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
        static constexpr auto red     = "#ff0000ff"_c4;
        static constexpr auto reddish = "#ff2222ff"_c4;
        static constexpr auto dark_red = "#3d1111ff"_c4;
        static constexpr auto green = "#00ff00ff"_c4;
        static constexpr auto blue  = "#0000ffff"_c4;
        static constexpr auto purple= "#ff00ffff"_c4;
        static constexpr auto dark_purple = "#150c25ff"_c4;
        static constexpr auto darker_purple = "#0a0612ff"_c4;
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

    struct color_variant_t {
        color_variant_type _type{color_variant_type::uniform};
        union {
            v4f uniform{1.0f};
            u32 hex;
            gradient_t gradient;
            math::aabb_t<v4f> range;
            // math::curve_t curve;
        };

        v4f sample(f32 t) {
            switch(_type) {
                case color_variant_type::uniform:
                    return uniform;
                case color_variant_type::hex:
                    return to_color4(hex);
                case color_variant_type::range:
                    return range.sample(v4f{t});
                case color_variant_type::gradient:
                    return gradient.sample(t);
                case_invalid_default;
            }
            return v4f{1.0f, 0.0f, 1.0f, 1.0f};
        }

        color_variant_type set_type(color_variant_type t) {
            if (_type == t) {
                return t;
            }
            switch(_type = t) {
                case color_variant_type::uniform:
                    uniform = v4f{1.0f};
                    break;
                case color_variant_type::hex:
                    hex = rgba::white;
                    break;
                case color_variant_type::range:
                    range = {};
                    break;
                case color_variant_type::gradient:
                    gradient = {};
                    break;
                case_invalid_default;
            }
            return t;
        }
    };

}; // namespace color


namespace gui {
    struct vertex_t {
        v3f pos;
        v2f tex;
        u32 nrm;
        u32 img;
        u32 col;
    };

    v3f ui_2d(v2f i) {
        return v3f{i,0.0f};
    }
    
    struct draw_command_t {
        enum struct draw_type {
            scissor, draw
        } type;

        union {
            math::rect2d_t clip;
            struct {
                u32 index_start{0};
                u32 vertex_start{0};
                u32 index_count{0};
                u32 vertex_count{0}; // for padding and stats
            } draw;
        };

        draw_command_t* next{};
        draw_command_t* prev{};

        draw_command_t() {
            dlist_init(this);
        }
    };

    using vertex_selector_t = buffer<vertex_t>;

    void apply_vertex_transform(vertex_selector_t vertices, const m44& transform) {
        range_u64(i, 0, vertices.count) { 
            vertices[i].pos = transform * v4f{vertices[i].pos, 1.0f};
        }
    }

    v3f selection_center(vertex_selector_t vertices) {
        v3f result = {};
        range_u64(i, 0, vertices.count) { 
            result += vertices[i].pos;
        }
        return result / f32(vertices.count);
    }

    void rotate_selection_center(vertex_selector_t selector, f32 radians) {
        auto center = selection_center(selector);

        math::transform_t transform{};
        transform.origin = center;
        transform.set_rotation(axis::backward, radians);

        apply_vertex_transform(selector, transform.to_matrix() * math::transform_t{-center}.to_matrix());
    }

    struct text_input_state_t {
        char* data = 0;
        u64   text_position = 0;
        u64   text_size = 0;
        u64   max_size = 0;
    };

    void text_insert_key(text_input_state_t& text_input, char key) {
        if (key > 0) {
            if (text_input.text_size + 1 < text_input.max_size) {
                if (text_input.text_position == text_input.text_size) {
                    text_input.data[text_input.text_position++] = key;
                } else {
                    utl::copy(text_input.data + text_input.text_position + 1, text_input.data + text_input.text_position, text_input.text_size - text_input.text_position);
                    text_input.data[text_input.text_position++] = key;
                }
                text_input.text_size++;
            }
        }
    }

    void text_paste(text_input_state_t& text_input, std::string_view str, b32 space) {
        utl::memzero(text_input.data, text_input.max_size);
        utl::copy(text_input.data, str.data(), str.size());

        text_input.text_size = text_input.text_position
            = str.size();

        if (space) {
            text_input.data[text_input.text_position++] = ' ';
            text_input.text_size++;
        }
    }

    void text_cursor_left(text_input_state_t& text_input) {
        if (text_input.text_position) {
            text_input.text_position--;
        }
    }

    void text_cursor_right(text_input_state_t& text_input) {
        if (text_input.text_position < text_input.text_size) {
            text_input.text_position++;
        }
    }

    void text_clear(text_input_state_t& text_input) {
        utl::memzero(text_input.data, text_input.max_size);

        text_input.text_position = text_input.text_size 
            = 0;
    }

    void text_delete(text_input_state_t& text_input, b32 ctrl) {
        if (ctrl) {
            utl::memzero(text_input.data, text_input.text_position + 1);

            utl::copy(text_input.data, text_input.data + text_input.text_position, text_input.text_size - text_input.text_position);

            text_input.text_size -= text_input.text_position;
            text_input.text_position = 0;
        } else if (text_input.text_position > 0) {
            if (text_input.text_position == text_input.text_size) {
                text_input.text_position -= 1;
                text_input.data[text_input.text_position] = 0;
            } else {
                text_input.text_position -= 1;
                utl::copy(text_input.data + text_input.text_position, text_input.data + text_input.text_position + 1, text_input.text_size - text_input.text_position + 1);
            }
            text_input.text_size -= 1;
        }
    }

    b32 text_input(app_input_t* input, text_input_state_t& text_input, std::optional<std::string_view> placeholder = std::nullopt) {
        auto key = input->keyboard_input();
        auto entered = input->pressed.keys[key_id::ENTER];
        auto deleted = input->pressed.keys[key_id::BACKSPACE];
        auto ctrl  = input->keys[key_id::LEFT_CONTROL];
        auto left  = input->pressed.keys[key_id::LEFT];
        auto right = input->pressed.keys[key_id::RIGHT];
        auto tab   = input->pressed.keys[key_id::TAB];

        if (tab && placeholder) {
            text_paste(text_input, *placeholder, true);
        }
        if (left) { 
            text_cursor_left(text_input);
        } else if (right) {
            text_cursor_right(text_input);
        }

        text_insert_key(text_input, key);
        
        if (deleted) {
            text_delete(text_input, ctrl);
        }
        return entered;
    }

    struct ctx_t {
        utl::pool_t<vertex_t>* vertices;
        utl::pool_t<u32>* indices;
        gfx::font_t* font;
        app_input_t* input;
        v2f screen_pos{0.0f};
        v2f screen_size{};

        f32 depth{0.0f};
        f32 draw_z{0.5f};

        void depth_down() {
            // draw_z += 0.00001f;
        }

        vertex_selector_t begin_vertex_selection() {
            vertex_selector_t result = {};
            auto vertex_index = vertices->count();
            result.data = &(*vertices)[vertex_index];
            result.count = vertex_index;

            return result;
        }

        void end_vertex_selection(vertex_selector_t& selection) {
            auto vertex_index = vertices->count();
            selection.count = vertex_index - selection.count;
            assert(selection.count);
        }

        math::rect2d_t screen_rect() const {
            return math::rect2d_t{
                screen_pos,
                screen_pos + screen_size
            };
        }

        v3f ui_2d(v2f x) {
            return v3f{x.x, x.y, draw_z};
        }

        gfx::font_t* dyn_font[32];

        u64 frame{0};

        draw_command_t scissor(math::rect2d_t rect) {
            draw_command_t command = {};

            command.type = draw_command_t::draw_type::scissor;
            command.clip = rect;

            return command;
        }

        draw_command_t begin_draw() {
            draw_command_t command = {};

            // draw_z = depth;
            // depth += 0.01;

            command.type = draw_command_t::draw_type::draw;
            command.draw.index_start = safe_truncate_u64(indices->count());
            command.draw.vertex_start = safe_truncate_u64(vertices->count());

            return command;
        }

        void begin_draw(arena_t* arena, auto* head) {
            tag_struct(auto* draw_command, draw_command_t, arena);

            *draw_command = begin_draw();

            dlist_insert_as_last(head, draw_command);
        }

        void end_draw(draw_command_t* command) {
            assert(command);
            assert(command->type == draw_command_t::draw_type::draw);
            
            command->draw.index_count = safe_truncate_u64(indices->count()) - command->draw.index_start;
            command->draw.vertex_count = safe_truncate_u64(vertices->count()) - command->draw.vertex_start;
        }

        void set_draw_depth(draw_command_t* command, f32 d) {
            assert(command);
            assert(command->type == draw_command_t::draw_type::draw);

            range_u64(o, 0, command->draw.index_count) {
                auto i = command->draw.index_start + o;
                vertices->data()[i].pos.z = d;
            }
        }
    };


    enum eTypeFlag {
        eTypeFlag_Theme,
        eTypeFlag_Panel,
        eTypeFlag_Text,

        eTypeFlag_None,
    };

    // have multiple theme types? ex. button_theme_t, window_theme_t
    struct theme_t {
        u64 VERSION{0};
        color32 fg_color{color::rgba::gray};
        color32 bg_color{color::rgba::black};
        color32 text_color{color::rgba::cream};
        color32 active_color{color::rgba::yellow};
        color32 disabled_color{};
        color32 border_color{};
        f32     border_thickness{1.0f};
        f32     shadow_distance{2.0f};
        color32 shadow_color{color::rgba::black};

        f32 padding{1.0f};
        f32 hpad{1.0f};
        f32 margin{1.0f};
        f32 border_radius{6.0f};
        f32 title_height{8.0f};
    };

    
    struct text_t;
    struct image_t;
    struct button_t;
    
    struct panel_t : public math::rect2d_t {
        panel_t* next{0};
        panel_t* prev{0};

        b32 clipping  = 0;
        math::rect2d_t clip;

        b32 validate_position(v2f p) {
            if (clipping) {
                return clip.contains(p);
            }
            return 1;
        }

        stack_buffer<math::rect2d_t, 64> saved{};
        stack_buffer<f32, 64> scroll_buffer{};

        void save() {
            saved.push(*this);
            scroll_buffer.push(draw_cursor.y);
        }

        f32 restore() {
            min = saved.top().min;
            max = saved.top().max;

            f32 r = scroll_buffer.top();

            scroll_buffer.pop();
            saved.pop();
            return r;
        }

        void update_cursor_max(math::rect2d_t r) {
            update_cursor_max(r.min);
            update_cursor_max(r.max);
        }
        void update_cursor_max(v2f p) {
            max_draw = glm::max(p, max_draw);
            expand(p);
        }

        sid_t name;
        v2f draw_cursor;
        v2f max_draw;
        v2f saved_cursor;


        draw_command_t draw_commands{};

        explicit panel_t() {
            dlist_init(this);
        }
    };

    inline void
    ctx_clear(
        ctx_t* ctx,
        utl::pool_t<vertex_t>* vertices,
        utl::pool_t<u32>* indices
    ) {
        // ctx->frame++; // remove this
        // ctx->draw_z =
        // ctx->depth = 0.0f;
        ctx->vertices = vertices;
        ctx->indices = indices;
        ctx->vertices->clear();
        ctx->indices->clear();
    }

    inline v2f
    draw_string(
        ctx_t* ctx,
        std::string_view text,
        v2f* position,
        const color32& text_color = color::rgba::white,
        font_t* font = 0,
        color32 shadow = 0
    ) {
        if (shadow) {
            auto start = *position;
            start += v2f{2.0f};
            font_render(0, font ? font : ctx->font, 
                text, 
                start,
                ctx->draw_z,
                ctx->screen_size,
                ctx->vertices,
                ctx->indices,
                shadow
            );
        }
        font_render(0, font ? font : ctx->font, 
            text, 
            *position,
            ctx->draw_z,
            ctx->screen_size,
            ctx->vertices,
            ctx->indices,
            text_color
        );
        ctx->depth_down();
        return font_get_size(font ? font : ctx->font, text);
    }

    inline v2f
    draw_string(
        ctx_t* ctx,
        std::string_view text,
        const v2f& position,
        const color32& text_color = color::rgba::white,
        font_t* font = 0,
        color32 shadow = 0
    ) {
        v2f cursor = position;
        return draw_string(
            ctx,
            text,
            &cursor,
            text_color,
            font,
            shadow
        );
    }

    inline void 
    draw_circle(
        ctx_t* ctx,
        v2f pos, f32 radius,
        u32 color, u32 res = 16, f32 perc = 1.0f, f32 start = 0.0f
    ) {
        // return;
        const u32 v_start = safe_truncate_u64(ctx->vertices->count());
        vertex_t* center = ctx->vertices->allocate(1);

        center->pos = ctx->ui_2d(pos);
        center->col = color;
        center->img = ~(0ui32);

        while(res%3) {
            res++;
        }

        range_u32(i, 0, res+1) {
            const auto a = start * math::constants::tau32 + perc * math::constants::tau32 * f32(i)/f32(res);
            vertex_t* v = ctx->vertices->allocate(1);

            v->pos = center->pos + ctx->ui_2d(v2f{glm::cos(a), glm::sin(a)} * radius);
            v->pos.z = ctx->draw_z;
            v->col = color;
            v->img = ~(0ui32);

            if (i > 0) {
                u32* tris = ctx->indices->allocate(3);
                tris[0] = v_start; // center 
                tris[1] = v_start + i - 1; // this vertex
                tris[2] = v_start + i; // last vertex
            }
        }
            
        u32* tris = ctx->indices->allocate(3);
        tris[0] = v_start;
        tris[1] = v_start + res;
        tris[2] = v_start + 1;

        ctx->depth_down();
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
        const v2f d{(b-a)};
        const v2f ud{glm::normalize(d)};
        const v2f n{v2f{-ud.y, ud.x}};

        vertex_t* v = ctx->vertices->allocate(4);
        u32* i = ctx->indices->allocate(6);
        v[0] = vertex_t { .pos = ctx->ui_2d(a + n * (line_width * 0.5f)), .tex = v2f{0.0f, 1.0f}, .nrm=0, .img = ~(0ui32), .col = color};
        v[1] = vertex_t { .pos = ctx->ui_2d(a - n * (line_width * 0.5f)), .tex = v2f{0.0f, 0.0f}, .nrm=0, .img = ~(0ui32), .col = color};
        v[2] = vertex_t { .pos = ctx->ui_2d(b + n * (line_width * 0.5f)), .tex = v2f{1.0f, 0.0f}, .nrm=0, .img = ~(0ui32), .col = color};
        v[3] = vertex_t { .pos = ctx->ui_2d(b - n * (line_width * 0.5f)), .tex = v2f{1.0f, 1.0f}, .nrm=0, .img = ~(0ui32), .col = color};

        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;

        i[3] = v_start + 2;
        i[4] = v_start + 1;
        i[5] = v_start + 3;

        ctx->depth_down();
    }

    void
    draw_curve(
        ctx_t* ctx,
        std::span<v2f,4> p, 
        math::rect2d_t rect,
        u32 res,
        f32 line_width,
        u32 color
    ) {
        math::cat_curve_t c;
        c.p[0] = p[0];
        c.p[1] = p[1];
        c.p[2] = p[2];
        c.p[3] = p[3];
        for (u32 i = 0; i < res-1; i++) {
            f32 t0 = f32(i)/f32(res-1);
            f32 t1 = f32(i+1)/f32(res-1);
            auto p0 = math::hermite(t0, p[0], p[1], p[2], p[3]) * rect.size() + rect.min;
            auto p1 = math::hermite(t1, p[0], p[1], p[2], p[3]) * rect.size() + rect.min;
            // auto p0 = c.sample(t0) * rect.size() + rect.min;
            // auto p1 = c.sample(t1) * rect.size() + rect.min;
            draw_line(ctx, p0, p1, line_width, color);
        }
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
    draw_wire_sphere(
        ctx_t* ctx,
        v3f pos,
        f32 radius,
        const m44& vp,
        f32 line_width = 3.0f,
        u32 color = gfx::color::rgba::white,
        u32 res = 32
    ) {
        const v3f spos = math::world_to_screen(vp, pos);
        math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};

        if (viewable.contains(spos)) {
            const v2f screen  = v2f{spos} * ctx->screen_size;
            auto w_up         = pos + axis::up * radius;
            auto w_forward    = pos + axis::forward * radius;
            auto w_right      = pos + axis::right * radius;
            auto v_up         = ctx->screen_size * v2f{math::world_to_screen(vp, w_up)};
            auto v_forward    = ctx->screen_size * v2f{math::world_to_screen(vp, w_forward)};
            auto v_right      = ctx->screen_size * v2f{math::world_to_screen(vp, w_right)};
            auto v_pos        = ctx->screen_size * v2f{math::world_to_screen(vp, pos)};
            
            const v2f rd = v_right - v_pos;
            const v2f ud = v_up - v_pos;
            const v2f fd = v_forward - v_pos;
            
            draw_ellipse_arc_with_basis(ctx, v_pos, v2f{1.0f}, ud, fd, 0.0f, 1.0f, line_width, color, res);
            draw_ellipse_arc_with_basis(ctx, v_pos, v2f{1.0f}, rd, fd, 0.0f, 1.0f, line_width, color, res);
            draw_ellipse_arc_with_basis(ctx, v_pos, v2f{1.0f}, rd, ud, 0.0f, 1.0f, line_width, color, res);
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
        math::rect3d_t aabb,
        const m44& vp,
        f32 line_width,
        u32 color
    ) {
        const v3f size = aabb.size();
        const v3f sxxx = v3f{size.x,0,0};
        const v3f syyy = v3f{0,size.y,0};
        const v3f szzz = v3f{0,0,size.z};

        math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};

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

    void
    draw_triangle(
        ctx_t* ctx,
        math::triangle_t triangle,
        std::span<u32, 3> colors,
        std::span<v2f, 3> uv,
        u32 img = ~0ui32
    ) {
        const u32 v_start = safe_truncate_u64(ctx->vertices->count());
        const u32 i_start = safe_truncate_u64(ctx->indices->count());

        vertex_t* v = ctx->vertices->allocate(3);
        u32* i = ctx->indices->allocate(3);

        auto normal = packing::pack_normal(triangle.normal());

        v[0] = gfx::gui::vertex_t{ .pos = triangle.p[0], .tex = uv[0], .nrm = normal, .img = img, .col = colors[0]};
        v[1] = gfx::gui::vertex_t{ .pos = triangle.p[1], .tex = uv[1], .nrm = normal, .img = img, .col = colors[1]};
        v[2] = gfx::gui::vertex_t{ .pos = triangle.p[2], .tex = uv[2], .nrm = normal, .img = img, .col = colors[2]};
        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;
    }

    void
    draw_triangle(
        ctx_t* ctx,
        math::triangle_t triangle,
        u32 color = gfx::color::rgba::white
    ) {
        v2f uv[3]{v2f{0.0f}, v2f{0.0f}, v2f{0.0f}};
        u32 c[3]{color,color,color};
        draw_triangle(ctx, triangle, std::span{c}, std::span{uv} );
    }

    void
    draw_quad(
        ctx_t* ctx,
        std::span<v3f, 4> points,
        u32 color
    ) {
        math::triangle_t t0{ .p = {
                points[0],
                points[1],
                points[2]
            }
        };
        math::triangle_t t1{ .p = {
                points[0],
                points[2],
                points[3]
            }
        };

        draw_triangle(ctx, t0, color);
        draw_triangle(ctx, t1, color);
    }


    void draw_cube(
        ctx_t* ctx,
        const math::rect3d_t& box,
        u32 color = gfx::color::rgba::white
    ) {
        v2f uv[3]{v2f{0.0f}, v2f{0.0f}, v2f{0.0f}};
        u32 c[3]{color,color,color};
        
        const auto p0 = box.sample(v3f{0,0,0}); // min
        const auto p1 = box.sample(v3f{1,0,0}); // min right
        const auto p2 = box.sample(v3f{0,1,0}); // min up
        const auto p3 = box.sample(v3f{0,0,1}); // min forward

        const auto p4 = box.sample(v3f{1,0,1}); // min across
        const auto p5 = box.sample(v3f{1,1,0}); // min up right
        const auto p6 = box.sample(v3f{1,1,1}); // max
        const auto p7 = box.sample(v3f{0,1,1}); // min up forward

        draw_triangle(ctx, math::triangle_t{{p0, p1, p3}}, std::span{c}, std::span{uv} );
        draw_triangle(ctx, math::triangle_t{{p1, p3, p4}}, std::span{c}, std::span{uv} );
        
        draw_triangle(ctx, math::triangle_t{{p0, p3, p7}}, std::span{c}, std::span{uv} );
        draw_triangle(ctx, math::triangle_t{{p0, p7, p2}}, std::span{c}, std::span{uv} );

        draw_triangle(ctx, math::triangle_t{{p0, p1, p5}}, std::span{c}, std::span{uv} );
        draw_triangle(ctx, math::triangle_t{{p0, p5, p2}}, std::span{c}, std::span{uv} );

        draw_triangle(ctx, math::triangle_t{{p6, p7, p2}}, std::span{c}, std::span{uv} );
        draw_triangle(ctx, math::triangle_t{{p6, p2, p5}}, std::span{c}, std::span{uv} );
        
        draw_triangle(ctx, math::triangle_t{{p6, p5, p1}}, std::span{c}, std::span{uv} );
        draw_triangle(ctx, math::triangle_t{{p6, p1, p4}}, std::span{c}, std::span{uv} );

        draw_triangle(ctx, math::triangle_t{{p4, p3, p7}}, std::span{c}, std::span{uv} );
        draw_triangle(ctx, math::triangle_t{{p4, p7, p6}}, std::span{c}, std::span{uv} );
    }

    void draw_mesh(
        ctx_t* ctx,
        gfx::mesh_list_t* mesh_list,
        gfx::vertex_t* vertices,
        u32* indices,
        const m44& transform,
        u32 color
    ) {
        for(size_t m=0; m<mesh_list->count;m++) {
            auto* mesh = mesh_list->meshes+m;
            for (u32 t=mesh->index_start;t<mesh->index_start+mesh->index_count;t+=3) {
                math::triangle_t tri{
                    .p = {
                        v3f{transform * v4f{vertices[mesh->vertex_start+indices[t+0]].pos, 1.0f}},
                        v3f{transform * v4f{vertices[mesh->vertex_start+indices[t+1]].pos, 1.0f}},
                        v3f{transform * v4f{vertices[mesh->vertex_start+indices[t+2]].pos, 1.0f}},
                    }
                };
                draw_triangle(ctx, tri, color);
            }
        }
    }

    // [min, {min.x, max.y}, {max.x, min.y}, max]
    inline void
    draw_rect(
        ctx_t* ctx,
        math::rect2d_t box,
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
        v[0] = vertex_t { .pos = ctx->ui_2d(p0), .tex = v2f{0.0f, 1.0f}, .nrm = 0, .img = ~(0ui32), .col = colors[0]};
        v[1] = vertex_t { .pos = ctx->ui_2d(p1), .tex = v2f{0.0f, 0.0f}, .nrm = 0, .img = ~(0ui32), .col = colors[1]};
        v[2] = vertex_t { .pos = ctx->ui_2d(p2), .tex = v2f{1.0f, 0.0f}, .nrm = 0, .img = ~(0ui32), .col = colors[2]};
        v[3] = vertex_t { .pos = ctx->ui_2d(p3), .tex = v2f{1.0f, 1.0f}, .nrm = 0, .img = ~(0ui32), .col = colors[3]};

        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;

        i[3] = v_start + 2;
        i[4] = v_start + 1;
        i[5] = v_start + 3;

        ctx->depth_down();
    }

    inline void
    draw_rect(
        ctx_t* ctx,
        math::rect2d_t box,
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
        v[0] = vertex_t { .pos = ctx->ui_2d(p0), .tex = v2f{0.0f, 1.0f}, .nrm=0, .img = ~(0ui32), .col = color};
        v[1] = vertex_t { .pos = ctx->ui_2d(p1), .tex = v2f{0.0f, 0.0f}, .nrm=0, .img = ~(0ui32), .col = color};
        v[2] = vertex_t { .pos = ctx->ui_2d(p2), .tex = v2f{1.0f, 0.0f}, .nrm=0, .img = ~(0ui32), .col = color};
        v[3] = vertex_t { .pos = ctx->ui_2d(p3), .tex = v2f{1.0f, 1.0f}, .nrm=0, .img = ~(0ui32), .col = color};

        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;

        i[3] = v_start + 2;
        i[4] = v_start + 1;
        i[5] = v_start + 3;

        ctx->depth_down();
    }

    //textured
    inline void
    draw_rect(
        ctx_t* ctx,
        math::rect2d_t box,
        u32 tex, math::rect2d_t uvs, u32 color = color::rgba::white
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
        v[0] = vertex_t { .pos = ctx->ui_2d(p0), .tex = uv0, .nrm = 0, .img = tex, .col = color};
        v[1] = vertex_t { .pos = ctx->ui_2d(p1), .tex = uv1, .nrm = 0, .img = tex, .col = color};
        v[2] = vertex_t { .pos = ctx->ui_2d(p2), .tex = uv2, .nrm = 0, .img = tex, .col = color};
        v[3] = vertex_t { .pos = ctx->ui_2d(p3), .tex = uv3, .nrm = 0, .img = tex, .col = color};

        i[0] = v_start + 0;
        i[1] = v_start + 1;
        i[2] = v_start + 2;

        i[3] = v_start + 2;
        i[4] = v_start + 1;
        i[5] = v_start + 3;

        ctx->depth_down();
    }

    inline void
    draw_rect_outline(
        ctx_t* ctx,
        math::rect2d_t box,
        u32 color,
        u32 outline_color,
        f32 thickness
    ) {
        draw_rect(ctx, box, outline_color); 
        draw_rect(ctx, box.pad(thickness), color);
    }

    inline void
    draw_round_rect(
        ctx_t* ctx,
        math::rect2d_t box,
        f32 radius,
        u32 color,
        u32 num_segments = 20
    ) {
        box.set_min_size(v2f{radius*2.0f});
        math::rect2d_t inner{box.min+radius, box.max-radius};
        
        draw_circle(ctx, inner.min, radius, color, num_segments);
        draw_circle(ctx, inner.min + v2f{0,inner.size().y}, radius, color, num_segments);
        draw_circle(ctx, inner.max, radius, color, num_segments);
        draw_circle(ctx, inner.min + v2f{inner.size().x,0}, radius, color, num_segments);

        draw_rect(ctx, math::rect2d_t{box.min + v2f{radius,0}, inner.max+v2f{0,radius}}, color);
        draw_rect(ctx, math::rect2d_t{box.min + v2f{0,radius}, inner.max+v2f{radius,0}}, color);

        // draw_circle(ctx, inner.min, radius, color, num_segments, 0.25f, 0.5f);
        // draw_circle(ctx, inner.min + v2f{0,inner.size().y}, radius, color, num_segments, 0.25f, 0.25f);
        // draw_circle(ctx, inner.max, radius, color, num_segments, 0.25f, 0.0f);
        // draw_circle(ctx, inner.min + v2f{inner.size().x,0}, radius, color, num_segments, 0.25f, 0.750f);
    }

    inline void
    draw_leftround_rect(
        ctx_t* ctx,
        math::rect2d_t box,
        f32 radius,
        u32 color,
        u32 num_segments = 20
    ) {
        box.set_min_size(v2f{radius*2.0f});
        math::rect2d_t inner{box.min+radius, box.max-radius};
        
        draw_circle(ctx, inner.min, radius, color, num_segments);
        draw_circle(ctx, inner.min + v2f{0,inner.size().y}, radius, color, num_segments);
        // draw_circle(ctx, inner.max, radius, color, num_segments);
        // draw_circle(ctx, inner.min + v2f{inner.size().x,0}, radius, color, num_segments);

        draw_rect(ctx, math::rect2d_t{box.min + v2f{radius,0}, box.max}, color);
        draw_rect(ctx, math::rect2d_t{box.min + v2f{0,radius}, box.min+v2f{radius,inner.size().y}}, color);

        // draw_circle(ctx, inner.min, radius, color, num_segments, 0.25f, 0.5f);
        // draw_circle(ctx, inner.min + v2f{0,inner.size().y}, radius, color, num_segments, 0.25f, 0.25f);
        // draw_circle(ctx, inner.max, radius, color, num_segments, 0.25f, 0.0f);
        // draw_circle(ctx, inner.min + v2f{inner.size().x,0}, radius, color, num_segments, 0.25f, 0.750f);
    }

    inline void
    draw_round_rect_outline(
        ctx_t* ctx,
        math::rect2d_t box,
        f32 radius,
        u32 color,
        u32 outline_color,
        f32 thickness,
        u32 num_segments = 20
    ) {
        draw_round_rect(ctx, box, radius, outline_color, num_segments);
        draw_round_rect(ctx, box.pad(thickness), radius, color, num_segments);
    }

    enum struct progress_bar_style : u32 {
        none, rounded, rounded_outline,
        leftround, rightround,
    };

    inline void
    draw_progress_bar(
        ctx_t* ctx,
        math::rect2d_t box,
        f32 percent,
        u32 bar_color,
        u32 bg_color,
        u32 outline_color,
        f32 thickness,
        progress_bar_style style = progress_bar_style::none,
        f32 border_radius = 4.0f,
        u32 num_segments = 20
    ) {
        assert(percent >= 0.0f && percent <= 1.0f);
        switch (style) {
            case progress_bar_style::none: {
                draw_round_rect(ctx, box.pad(thickness), border_radius, bg_color, num_segments);
            } break;
            case progress_bar_style::rightround:
            case progress_bar_style::leftround:
            case progress_bar_style::rounded:
            case progress_bar_style::rounded_outline: {
                draw_round_rect_outline(ctx, box, border_radius, bg_color, outline_color, thickness, num_segments);
            } break;
            case_invalid_default;
        }

        box.max.x = box.min.x + box.size().x * percent;

        auto half_circle_percentage = border_radius / box.size().x;

        switch (style) {
            case progress_bar_style::none:
            case progress_bar_style::rounded: {
                draw_round_rect(ctx, box.pad(thickness), border_radius, bar_color, num_segments);
            } break;
            case progress_bar_style::rounded_outline: {
                draw_round_rect_outline(ctx, box.pad(thickness), border_radius, bar_color, outline_color, thickness, num_segments);
            } break;
            case progress_bar_style::leftround: {
                if (percent < 1.0f - half_circle_percentage) {
                    draw_leftround_rect(ctx, box.pad(thickness), border_radius, bar_color, num_segments);
                } else {
                    draw_round_rect(ctx, box.pad(thickness), border_radius, bar_color, num_segments);
                }
            } break;
            case_invalid_default;
        }
    }

    namespace theme_mask_t {
        enum ENUM : u64 {
            none, 
            no_border = 1
        };
    };

    namespace cut_string_result_t {
        enum ENUM : u32 {
            none, hot, active, fire
        };
    };

    cut_string_result_t::ENUM cut_string(
        ctx_t* c, 
        math::rect2d_t& r,
        std::string_view text,
        gfx::color32 text_color,
        gfx::color32 fg_color = 0,
        gfx::color32 border_color = 0,
        f32 border_size = 2.0f,
        v2f padding = v2f{2.0f},
        gfx::font_t* font = 0,
        gfx::color32 shadow = 0,
        math::rect2d_t* outrect = 0,
        math::rect2d_t* thisrect = 0
    ) {
        math::rect2d_t label;
        math::rect2d_t _tr;

        auto s0 = font_get_size(font, text);
        std::tie(label, r) = math::cut_top(r, s0.y + padding.y);

        // if (outrect)                                                                        
        std::tie(label, outrect?*outrect:_tr) = math::cut_left(label, s0.x + padding.x);

        if (thisrect) {
            *thisrect = label;
        }
        
        if (fg_color && border_color) {
            draw_rect_outline(c, label, fg_color, border_color, border_size);
        }
        draw_string(c, text, label.min, text_color, font, shadow);

        u32 mask = ((!!label.contains(c->input->mouse.pos2())) << 1) | !!c->input->released.mouse_btns[0];
        using namespace cut_string_result_t;
        switch(mask) {
            case 0: return none;
            case 1: return active;
            case 2: return hot;
            case 3: return fire;
        };
        return none;
    }

    cut_string_result_t::ENUM cut_string(
        ctx_t* c, 
        math::rect2d_t& r,
        std::string_view text,
        const theme_t& theme,
        u32 mask = 0,
        gfx::font_t* font = 0,
        math::rect2d_t* outrect = 0,
        math::rect2d_t* thisrect = 0
    ) {
        assert(font);
        return cut_string(c, r, text, theme.text_color, theme.fg_color, !(mask & 1) ? theme.border_color : 0, theme.border_thickness, v2f{theme.hpad, theme.padding}, font, theme.shadow_color, outrect, thisrect);
        // return cut_string(c, r, text, theme.text_color, theme.fg_color, theme.border_color, theme.border_thickness, v2f{theme.hpad, theme.padding}, font, theme.shadow_color, outrect);
    }
    

    b32 text_box(
        ctx_t* c, 
        text_input_state_t& text_in,
        // math::rect2d_t& r,
        const math::rect2d_t& text_box,
        gfx::font_t* font,
        gfx::color32 text_color,
        gfx::color32 text_box_color,
        b32 active,
        std::optional<std::string_view> placeholder = std::nullopt,
        gfx::color32 guess_text_color = gfx::color::rgba::yellow
    ) {
        draw_rect(c, text_box, text_box_color);

        if (placeholder) {
            if (!active) {
                guess_text_color = gfx::color::modulate(guess_text_color, v4f{0.7f});
            }
            draw_string(c, *placeholder, text_box.min, guess_text_color);
        }
        draw_string(c, text_in.data, text_box.min, text_color);

        auto pos_size = gfx::font_get_size(c->font, std::string_view{text_in.data, text_in.text_position});
        auto [before_cursor, after_cursor] = math::cut_left(text_box, pos_size.x);

        if (active) {
            math::rect2d_t cursor;
            cursor.min = math::top_right(before_cursor);
            cursor.max = before_cursor.max;
            cursor.min.x -= 3.0f;
            draw_rect(c, cursor, gfx::color::rgba::yellow);
        }

        return active ? text_input(c->input, text_in, placeholder) : false;
    }

    
    b32 float_box(
        ctx_t* c, 
        const math::rect2d_t& rect,
        gfx::font_t* font,
        gfx::color32 text_color,
        gfx::color32 text_box_color,
        b32 active,
        f32* value
    ) {
        local_persist char text[64] = {};
        local_persist text_input_state_t text_in {
            .data = text,
            .max_size = array_count(text)
        };

        if (active) {
            text_paste(text_in, fmt_sv("{}", *value), 0);
        }

        auto r = text_box(c, text_in, rect, font, text_color, text_box_color, active);

        if (r) {
            std::from_chars(text_in.data, text_in.data + text_in.text_size + 1, *value);
        }
        return r;
    }

    namespace im {
        constexpr u64 id_ignore_clear_flag = 0b1;
        struct ui_id_t {
            u64 owner;
            u64 id;
            u64 index;
            u64 flags{id_ignore_clear_flag};

            ui_id_t& operator=(u64 id_) {
                id = id_;
                owner = 0;
                index = 0;
                flags = id_ignore_clear_flag;
                return *this;
            }
        };

        struct drag_event_t {
            void* user_data{0};
            umm user_data_size{0};
            const char* type_name=0;

            void* dropped_user_data{0};
            umm dropped_user_data_size{0};
            const char* dropped_type_name=0;

            u64 widget_id{0};
            u64 dropped_id{0};

            f32 start_time{0.0f};
            v2f start_pos{};
        };

        struct obstruction_t {
            math::rect2d_t rect;

            u64 panel_id{};
            utl::hash_trie_t<u64, b32>* children = 0;

            b32 clip(v2f p) {
                return !rect.contains(p);
            }

            void add(arena_t* arena, u64 id) {
                *utl::hash_get(&children, id, arena) = 1;
            }

            b32 has(u64 id) {
                b32 has = 0;
                auto* x = utl::hash_get(&children, id, 0, &has);
                if (has) {
                    return 1;
                }
                return 0;
            }
        };

        struct state_t {
            ctx_t& ctx;

            arena_t perm_arena = {};
            arena_t frame_arena = {};
            std::optional<drag_event_t> drag_event = std::nullopt;

            utl::hash_trie_t<u64, obstruction_t>* obstructions = 0;

            obstruction_t* get_rect(u64 id, math::rect2d_t r) {
                b32 had = 0;
                auto* obstruction = utl::hash_get(&obstructions, id, &perm_arena, &had);
                if (!had) {
                    obstruction->panel_id = id;
                    obstruction->rect = r;
                }

                return obstruction;
            }

            v2f window_drag_start{0.0f};
            v2f drag_start{};
            f32 float_drag_start_value{};

            f32 gizmo_scale_start{};
            v2f draw_cursor{0.0f}; 
            v2f gizmo_mouse_start{};
            v3f gizmo_world_start{};
            m33 gizmo_basis_start{};
            f32 gizmo_basis_scale{};
            v2f gizmo_axis_dir{};
            v3f gizmo_axis_wdir{};

            u32 selected_index = 0xfffffff; // gradient_editor

            b32 dragging_enabled = false;
            void* drag_user_data = 0; // used to pass data from user, set before draggable ui
            size_t drag_user_data_size = 0;
            const char* drag_user_type_name = 0;

            theme_t theme;

            ui_id_t stealing;
            ui_id_t hot;
            ui_id_t active;

            // panels that have been started with begin, but not ended
            panel_t active_panels{};

            // all of the panels this frame
            panel_t panels{};
            panel_t* panel{0};

            draw_command_t commands{};
            stack_buffer<math::rect2d_t, 64> scissor_stack{};

            b32 validate_position(v2f p) {
                return panel->validate_position(p);
            }

            void end_scissor(draw_command_t* head = 0) {
                if (!head) head = &commands;
                scissor_stack.pop();
                if (scissor_stack.empty() == false) {
                    scissor(scissor_stack.top(), head, 0);
                } else {
                    scissor(ctx.screen_rect(), head, 0);
                }
            }

            // you must end drawing before calling this
            void scissor(math::rect2d_t rect, draw_command_t* head = 0, b32 push_stack = 1) {
                if (!head) head = &commands;
                if (scissor_stack.empty() == false) {
                    for (const auto& scissor: scissor_stack.view()) {
                        // rect = scissor.clip(rect);
                        rect = rect.clip(scissor);
                    }

                    if (rect.size().x <= 0.0f || rect.size().y <= 0.0f) {
                        rect.min = {};
                        rect.max = {};
                    }
                }

                tag_struct(auto* command, draw_command_t, &frame_arena);
                *command = ctx.scissor(rect);
                if (push_stack) {
                    scissor_stack.push(rect);
                }

                dlist_insert_as_last(head, command);
            }

            void begin_free_drawing() {
                ctx.begin_draw(&frame_arena, &commands);
            }

            void end_free_drawing() {
                ctx.end_draw(commands.prev);
            }

            bool next_same_line{false};
            b32 saw_hot{0};
            b32 saw_active{0};
            v2f saved_cursor;

            u64 verify_id(u64 ptr, b32 add_to_panel = 0) {
                u64 id = (ptr);
                if (add_to_panel) {
                    auto* x = utl::hash_get(&obstructions, panel->name);
                    if (x) {
                        x->add(&perm_arena, id);
                    }
                }
                saw_hot |= hot.id == id;
                saw_active |= active.id == id;
                return id;
            }

            f32 time() const {
                return ctx.input->time;
            }

            f32 anim(f32 s = 1.0f) const {
                return cosf(time()*s)*0.5f+0.5f;
            }
        };

        struct rect_status_t {
            b32 hot;
            b32 active;
            b32 clicked;
            v2f mouse_pos;
        };

        void
        begin_drag(state_t& imgui) {
            assert(imgui.dragging_enabled == 0);
            // if (imgui.ctx.input->mouse.buttons[0] == false) {
            // }
            imgui.dragging_enabled = 1;
        }
        
        template<typename T>
        void drag_user_data(state_t& imgui, T& data, const char* data_type_name) {
            imgui.drag_user_data = &data;
            imgui.drag_user_data_size = sizeof(T);
            imgui.drag_user_type_name = data_type_name;
        }

        bool released_drag(state_t& imgui) {
            if (imgui.ctx.input->released.mouse_btns[0] && imgui.drag_event && imgui.drag_event->widget_id) {
                return true;
            }
            return imgui.drag_event && imgui.drag_event->dropped_id != 0;
        }

        bool is_dragging(state_t& imgui) {
            return !!imgui.drag_event && imgui.dragging_enabled;
        }


        // call after released event handler
        void
        end_drag(state_t& imgui) {
            if (imgui.ctx.input->released.mouse_btns[0]) {
                imgui.drag_event = std::nullopt;
            }
            imgui.dragging_enabled = 0;            
        }

        inline void
        space(state_t& imgui, f32 offset) {
            imgui.panel->draw_cursor.y += offset;
        }

        inline void
        indent(state_t& imgui, v2f offset) {
            imgui.panel->draw_cursor += offset;
        }

        inline void
        indent(state_t& imgui, f32 offset) {
            imgui.panel->draw_cursor.x += offset;
        }

        inline void
        horizontal_center(state_t& imgui) {
            indent(imgui, imgui.panel->size().x*0.5f);
        }

        inline void
        horizontal_text_center(state_t& imgui, std::string_view text) {
            auto size = font_get_size(imgui.ctx.font, text);
            indent(imgui, std::max(0.0f, (imgui.panel->size().x - size.x) * 0.5f));
        }

        inline bool
        want_mouse_capture(state_t& imgui) {
            return imgui.hot.id != 0 || imgui.active.id != 0;
        }

        inline void
        clear(state_t& imgui) {
            if (imgui.stealing.id) {
                imgui.hot = imgui.active = imgui.stealing;
            }
            if (imgui.hot.flags == id_ignore_clear_flag) {
                imgui.hot.flags &= ~0b1;
            } else {
                if (imgui.saw_hot == 0) {
                    imgui.hot = 0;
                }
            }
            if (imgui.active.flags == id_ignore_clear_flag) {
                imgui.active.flags &= ~0b1;
            } else {
                if (imgui.saw_active == 0) {
                    imgui.active = 0;
                }
            }
            imgui.saw_hot = 0;
            imgui.saw_active = 0;
            // range_u64(i, 0, array_count(imgui.panels)) {
                // imgui.panels[i].draw_cursor = v2f{0.0f};
            // }

            dlist_init(&imgui.active_panels);
            dlist_init(&imgui.panels);
            dlist_init(&imgui.commands);

            // puts("clearing");
            arena_clear(&imgui.frame_arena);

            imgui.scissor_stack.clear();
            imgui.end_scissor();
        }

        inline void
        same_line(state_t& imgui) {
            imgui.next_same_line = true;
            imgui.panel->saved_cursor = imgui.panel->draw_cursor;
        }

        inline bool
        same_line(state_t& imgui, bool b) {
            if (b) {
                same_line(imgui);
            }
            return true;
        }

        math::rect2d_t get_draw_rect(state_t& imgui, v2f size) {
            return {
                imgui.panel->draw_cursor,
                imgui.panel->draw_cursor + size,
            };
        }

        void set_draw_cursor(state_t& imgui, v2f p) {
            imgui.panel->min.x = p.x;
            imgui.panel->draw_cursor = p + imgui.theme.padding;
        }

        // bug with scissor rect overflow
        void begin_scroll_rect(
            state_t& imgui,
            f32* scroll,
            v2f* size_
        ) {
            *size_ = glm::max(*size_, v2f{16.0f});
            auto size = *size_;
            const u64 scl_id = imgui.verify_id((u64)scroll);

            math::rect2d_t scissor{
                imgui.panel->draw_cursor,
                imgui.panel->draw_cursor + size
            };

            scissor.pad(-imgui.theme.border_thickness);
            draw_rect(&imgui.ctx, scissor, imgui.theme.border_color);
            scissor.pad(imgui.theme.border_thickness);
            draw_rect(&imgui.ctx, scissor, imgui.theme.bg_color);

            imgui.ctx.end_draw(imgui.panel->draw_commands.prev);
            imgui.scissor(scissor, &imgui.panel->draw_commands);
            imgui.panel->clip = scissor;
            imgui.panel->clipping = 1;
            
            imgui.ctx.begin_draw(&imgui.frame_arena, &imgui.panel->draw_commands);

            auto [x,y] = imgui.ctx.input->mouse.pos;

            imgui.panel->save();
            imgui.panel->min = scissor.min;
            imgui.panel->max = scissor.max;
            // auto is_active = imgui.active.id == scl_id;
            // auto is_hot = imgui.hot.id == scl_id;
            
            // imgui.panel->scroll_start = imgui.panel->draw_cursor.y;
            imgui.panel->draw_cursor.y -= *scroll;
        }

        void end_scroll_rect(
            state_t& imgui,
            f32* scroll,
            v2f* size_
        ) {
            auto size = *size_;
            const u64 scl_id = imgui.verify_id((u64)scroll);
            
            imgui.ctx.end_draw(imgui.panel->draw_commands.prev);

            imgui.end_scissor(&imgui.panel->draw_commands);
            imgui.panel->clipping = 0;

            imgui.ctx.begin_draw(&imgui.frame_arena, &imgui.panel->draw_commands);

            f32 scroll_start = imgui.panel->scroll_buffer.top();

            imgui.panel->draw_cursor.y += *scroll;
            auto draw_delta = 
                imgui.panel->draw_cursor.y - scroll_start + imgui.theme.padding;

            auto visible_prc = std::clamp(size.y / draw_delta, 0.0f, 1.0f);

            imgui.panel->draw_cursor.y = scroll_start;
            auto draw_width = imgui.panel->max_draw.x - imgui.panel->draw_cursor.x;

            math::rect2d_t scissor{
                imgui.panel->draw_cursor,
                imgui.panel->draw_cursor + size
            };
            auto scroll_bar_size = 0.05f * scissor.size().x;
            scroll_bar_size = std::min(scroll_bar_size, 16.0f);
            
            size_->x = std::max(size_->x, draw_width);

            imgui.panel->restore();
            imgui.panel->max_draw = scissor.min;
            //imgui.panel->max_draw.y = imgui.panel->max.y;
            
            imgui.panel->update_cursor_max(scissor.max + imgui.theme.padding);
            
            // auto [scroll_bar, rest] = math::cut_right(scissor, scroll_bar_size);
            // auto [drag_col, rest2] = math::cut_right(rest, scroll_bar_size);
            // auto [drag_box, rest3] = math::cut_bottom(drag_col, scroll_bar_size);
            
            auto [right_panel, rest] = math::cut_right(scissor, scroll_bar_size);
            right_panel.add(v2f{-4.0f, 0.0f});
            auto [drag_box, scroll_bar] = math::cut_bottom(right_panel, scroll_bar_size);
            // auto [drag_col, rest2] = math::cut_right(rest, scroll_bar_size);

            auto [x,y] = imgui.ctx.input->mouse.pos;
            auto clicked = imgui.ctx.input->mouse.buttons[0];

            // if (visible_prc == 1.0f) {
                // drag_box.add(v2f{scroll_bar.size().x, 0.0f});
            //     scroll_bar.min = scroll_bar.max;
            // }
            v2f mouse{x,y};
            if (right_panel.contains(mouse)) {
                imgui.hot = scl_id;
            }
            if (imgui.hot.id == scl_id) {
                if (clicked) {
                    if (scroll_bar.contains(mouse)) {

                    } else if (drag_box.contains(mouse)) {
                        imgui.active = scl_id;
                    }
                } else {
                    imgui.active = 0;
                }
            }
            if (imgui.active.id == scl_id) {
                size_->x += imgui.ctx.input->mouse.delta[0];
                size_->y += imgui.ctx.input->mouse.delta[1];
            }

            // auto is_active = imgui.active.id == scl_id;
            // auto is_hot = imgui.hot.id == scl_id;

            // hover whole rect and mouse scroll
            if (scissor.contains(v2f{x,y})) {
                auto& dw = imgui.ctx.input->mouse.scroll.y;
                local_persist f32 scroll_velocity;

                scroll_velocity -= dw * 5.0f;

                *scroll += scroll_velocity;

                // scroll_velocity *= 0.0f; // @frame independent
                scroll_velocity *= 0.9f; // @frame independent
                
                dw = 0.0f;
            }

            auto dy = glm::max(0.01f, (draw_delta-size.y));

            auto scroll_prc = (*scroll) / dy;
            auto scroll_ball_prc = (*scroll) / (draw_delta);

            scroll_ball_prc = glm::clamp(scroll_ball_prc, 0.0f, 1.0f - visible_prc);
            scroll_prc = glm::clamp(scroll_prc, 0.0f, 1.0f);

            math::rect2d_t scroll_ball{
                scroll_bar.sample(v2f{0.0f, scroll_ball_prc}),
                scroll_bar.sample(v2f{1.0f, scroll_ball_prc + visible_prc}),
            };

            draw_rect(&imgui.ctx, drag_box, imgui.theme.fg_color);

            draw_rect(&imgui.ctx, scroll_bar, color::flatten_color(imgui.theme.fg_color));
            draw_rect(&imgui.ctx, scroll_ball, imgui.theme.fg_color);

            *scroll = (scroll_prc) * dy;
            
            imgui.panel->draw_cursor.y += size.y + imgui.theme.padding;
        }

        math::rect2d_t
        render_panel(
            state_t& imgui,
            math::rect2d_t rect,
            b32 draw = 1
        ) {
            const auto padding = imgui.theme.padding;

            auto [title_bar, _tr] = math::cut_top(rect, imgui.theme.title_height + padding);
            title_bar.pull(v2f{-padding});

            if (!draw) return title_bar;

            auto [exit_button, _er] = math::cut_right(title_bar, imgui.theme.title_height + padding);
            exit_button.pull(v2f{-padding});
            
            math::rect2d_t close_button = exit_button;
            close_button.add(-exit_button.size() * v2f{1.0f, 0.0f} + v2f{-imgui.theme.padding, 0.0f});

            auto flattened_bg = gfx::color::flatten_color(imgui.theme.bg_color);
            auto close_symbol = close_button;

            close_symbol.set_size(v2f{close_button.size().x - 2.0f, 2.0f});
            close_symbol.add(v2f{0.0f, close_button.size().y * 0.25f});

            if (imgui.hot.id == imgui.panel->name) {
                flattened_bg = gfx::color::modulate(flattened_bg, 1.5f);
            }
            
            f32 border_size = imgui.theme.border_thickness;
            f32 corner_radius = imgui.theme.border_radius;

            rect.pull(v2f{border_size});
            draw_round_rect(&imgui.ctx, rect, corner_radius, imgui.theme.fg_color);
            rect.pull(v2f{-border_size});
            draw_round_rect(&imgui.ctx, rect, corner_radius, imgui.theme.bg_color);


            draw_rect(&imgui.ctx, title_bar, imgui.theme.fg_color);
            title_bar.pad(2.0f);
            draw_rect(&imgui.ctx, title_bar, flattened_bg);
            
            draw_rect(&imgui.ctx, close_symbol, imgui.theme.fg_color);
            
            // close_button.pull(v2f{-2.0f});
            // draw_rect(&imgui.ctx, close_button, flattened_bg);
            // close_button.pull(v2f{2.0f});
            // draw_rect(&imgui.ctx, close_button, imgui.theme.fg_color);

            // draw_rect(&imgui.ctx, exit_button, color::rgba::red);
            draw_line(&imgui.ctx, exit_button.min, exit_button.max, 2.0f, color::rgba::red);
            draw_line(&imgui.ctx, math::top_right(exit_button), math::bottom_left(exit_button), 2.0f, color::rgba::red);
            

            return title_bar;
        }

        inline void
        end_panel(
            state_t& imgui,
            v2f* pos,
            v2f* size
        ) {
            imgui.ctx.end_draw(imgui.panel->draw_commands.prev);
            imgui.ctx.begin_draw(&imgui.frame_arena, &imgui.panel->draw_commands);

            // imgui.panel->update_cursor_max(imgui.panel->draw_cursor);
            math::rect2d_t panel = *imgui.panel;
            panel.max = imgui.panel->max_draw;
            *size = panel.size();

            if (imgui.ctx.input->keys[key_id::LEFT_ALT]) {
                draw_circle(&imgui.ctx, imgui.panel->draw_cursor, 4.0f, color::rgba::purple);
                draw_circle(&imgui.ctx, imgui.panel->max_draw, 4.0f, color::rgba::yellow);
            }

            const auto padding = imgui.theme.padding;

            auto title_bar = render_panel(imgui, *imgui.panel, 0);
            
            auto [x,y] = imgui.ctx.input->mouse.pos;
            auto clicked = !!imgui.ctx.input->mouse.buttons[0];

            auto txt_id = imgui.panel->name;

            if (imgui.active.id == txt_id) {
                if (!clicked) {
                    imgui.active = 0;
                }
            } else if (imgui.hot.id == txt_id) {
                if (clicked) {
                    imgui.active = txt_id;
                    imgui.window_drag_start = imgui.panel->min - v2f{x,y};
                }
            }
            
            if (math::intersect(title_bar, v2f{x,y})) {
                imgui.hot.id = imgui.panel->name;
            } else if (imgui.active.id != imgui.panel->name && imgui.hot.id == imgui.panel->name) {
                imgui.hot.id = 0;
            }
        
            // *size = imgui.panel->size();
            // imgui.panel->draw_cursor = v2f{0.0f};
            // imgui.panel--;
            panel_t* next_panel = 0;
            if (imgui.panel->prev != &imgui.active_panels) {
                next_panel = imgui.panel->prev;

                imgui.ctx.begin_draw(&imgui.frame_arena, &next_panel->draw_commands);
            }
            imgui.ctx.end_draw(imgui.panel->draw_commands.prev);
            dlist_remove(imgui.panel);
            dlist_insert_as_last(&imgui.panels, imgui.panel);
            imgui.panel = next_panel;
        }

        struct panel_result_t {
            bool maximized;
            bool want_to_close;

            operator bool() {
                return maximized;
            }
        };

        struct panel_state_t {
            v2f pos;
            v2f size;
            b32 open;
        };

        inline void
        end_panel(
            state_t& imgui,
            panel_state_t* state
        ) {
            end_panel(imgui, &state->pos, &state->size);
        }

        inline panel_result_t
        begin_panel(
            state_t& imgui, 
            string_t name,
            v2f* pos_,
            v2f* size_,
            b32* open
        ) {
            panel_result_t result{};
            result.want_to_close = false;
            auto theme = imgui.theme;
            auto padding = theme.padding;

            const v2f text_size = font_get_size(imgui.ctx.font, name.sv());
            const v2f title_bar_size_est = text_size + v2f{text_size.y * 2.0f + padding * 6.0f, padding*2.0f};
            *size_ = glm::max(*size_, title_bar_size_est);

            imgui.theme.title_height = text_size.y + theme.padding * 2.0f;
            
            auto size = *size_;
            // const sid_t pnl_id = imgui.verify_id(sid(name.sv()));
            const sid_t pnl_id = imgui.verify_id((u64)pos_);

            if (imgui.panel) {
                imgui.ctx.end_draw(imgui.panel->draw_commands.prev);
            }

            tag_struct(imgui.panel, panel_t, &imgui.frame_arena);
            dlist_insert_as_last(&imgui.active_panels, imgui.panel);

            imgui.ctx.begin_draw(&imgui.frame_arena, &imgui.panel->draw_commands);
            
            // imgui.panel++;
            // assert((void*)imgui.panel != (void*)&imgui.panel && "End of nested panels");

            if (imgui.active.id == pnl_id) {
                pos_->x = imgui.ctx.input->mouse.pos[0] + imgui.window_drag_start.x;
                pos_->y = imgui.ctx.input->mouse.pos[1] + imgui.window_drag_start.y;
            }

            auto pos = *pos_;

            imgui.panel->min = pos;
            imgui.panel->max = pos+size;

            imgui.get_rect(pnl_id, *imgui.panel);

            auto [title_bar, _tr] = math::cut_top(*imgui.panel, imgui.theme.title_height + padding);
            title_bar.pull(v2f{-padding});

            auto [exit_button, _er] = math::cut_right(title_bar, imgui.theme.title_height + padding);
            exit_button.pull(v2f{-padding});
            
            math::rect2d_t close_button = exit_button;
            close_button.add(-exit_button.size() * v2f{1.0f, 0.0f} + v2f{-imgui.theme.padding, 0.0f});

            const auto [mx,my] = imgui.ctx.input->mouse.pos;
            const auto clicked = !!imgui.ctx.input->pressed.mouse_btns[0];

            if (imgui.hot.id == pnl_id) {
                if (clicked && math::intersect(close_button, v2f{mx,my})) {
                    *open = !*open;
                }
                if (clicked && math::intersect(exit_button, v2f{mx,my})) {
                    result.want_to_close = true;
                }
            }

            if (pos.x >= 0.0f && pos.y >= 0.0f) { 
                imgui.panel->draw_cursor = pos+imgui.theme.padding;
            }

            imgui.panel->draw_cursor = title_bar.min;

            render_panel(imgui, *imgui.panel, 1);

            if (imgui.ctx.input->keys[key_id::LEFT_ALT]) {
                draw_string(&imgui.ctx, fmt_sv("{}: {}", name.sv(), imgui.panel->min), &imgui.panel->draw_cursor, theme.text_color);
            } else {
                draw_string(&imgui.ctx, name.sv(), &imgui.panel->draw_cursor, theme.text_color);
            }

            imgui.panel->max_draw = {};
            imgui.panel->draw_cursor.x = pos.x+theme.padding;
            // imgui.panel->max_draw.y = 
            imgui.panel->draw_cursor.y = title_bar.max.y + theme.padding;
            imgui.panel->saved_cursor = imgui.panel->draw_cursor;

            // imgui.panel->min = pos;
            // imgui.panel->max = imgui.panel->min + size;

            if (size.x > 0.0f && size.y > 0.0f) {
                // dont use update_cursor_max here
                imgui.panel->expand(imgui.panel->min + size);
            }
            imgui.panel->expand(title_bar.max);

            imgui.panel->name = pnl_id;

            result.maximized = *open;

        
            if (result.maximized == false) {
                end_panel(imgui, pos_, size_);
                *size_ = {};
            }

            return result;
        }

        inline panel_result_t
        begin_panel(
            state_t& imgui, 
            string_t name,
            panel_state_t* state
        ) {
            return begin_panel(imgui, name, &state->pos, &state->size, &state->open);
        }

        inline panel_result_t
        begin_panel_3d(
            state_t& imgui, 
            string_t name,
            m44 vp,
            v3f pos
        ) {
            const v3f spos = math::world_to_screen(vp, pos);
            math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};
            b32 open=1;

            if (viewable.contains(spos)) {
                v2f screen = v2f{spos} * imgui.ctx.screen_size;
                v2f size = {};
                return begin_panel(imgui, name, &screen, &size, &open);
            }
            return {};
        }

        inline panel_t*
        get_last_panel(
            state_t& imgui
        ) {
            return imgui.panels.prev;
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
            math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};

            imgui.begin_free_drawing();
            defer {
                imgui.end_free_drawing();
            };

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
            const m44& vp,
            b32 draw_rings = 1,
            b32 draw_axis = 1,
            m33 basis = m33{1.0f}
        ) {
            const v3f spos = math::world_to_screen(vp, *pos);
            math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};

            imgui.begin_free_drawing();
            defer {
                imgui.end_free_drawing();
            };

            if (viewable.contains(spos)) {
                const v2f screen = v2f{spos} * imgui.ctx.screen_size;
                auto w_up         = *pos + (basis[1]);
                auto w_forward    = *pos + (basis[2]);
                auto w_right      = *pos + (basis[0]);
                auto v_up         = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, w_up)};
                auto v_forward    = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, w_forward)};
                auto v_right      = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, w_right)};
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

                draw_circle(&imgui.ctx, v_up,       abw, color::rgba::green);
                draw_circle(&imgui.ctx, v_right,    abw, color::rgba::red);
                draw_circle(&imgui.ctx, v_forward,  abw, color::rgba::blue);

                if (draw_axis) {
                    draw_line(&imgui.ctx, v_pos, v_up, line_width, color::rgba::green);
                    draw_line(&imgui.ctx, v_pos, v_right, line_width, color::rgba::red);
                    draw_line(&imgui.ctx, v_pos, v_forward, line_width, color::rgba::blue);
                }

                draw_circle(&imgui.ctx, v_pos,      abw - bbw, color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_up,       abw - bbw, color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_right,    abw - bbw, color::rgba::light_gray);
                draw_circle(&imgui.ctx, v_forward,  abw - bbw, color::rgba::light_gray);

                if (draw_rings) {
                    draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, ud, fd, 0.25f, 1.0f, dlw, color::rgba::light_gray, 32);
                    draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, rd, fd, 0.25f, 1.0f, dlw, color::rgba::light_gray, 32);
                    draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, rd, ud, 0.25f, 1.0f, dlw, color::rgba::light_gray, 32);

                    draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, ud, fd, 0.0f, 0.25f, clw, color::rgba::red, 32);
                    draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, rd, fd, 0.0f, 0.25f, clw, color::rgba::green, 32);
                    draw_ellipse_arc_with_basis(&imgui.ctx, v_pos, v2f{rad}, rd, ud, 0.0f, 0.25f, clw, color::rgba::blue, 32);
                } else if (draw_axis) {
                    auto axis_cube = [](auto p, auto d = 0.05f) {
                        math::rect3d_t cube{};
                        cube.expand(p - v3f{d});
                        cube.expand(p + v3f{d});
                        return cube;
                    };
                    auto axis_rod = [](auto w, auto p, auto d = 0.01f) {
                        math::rect3d_t cube{};
                        cube.expand(w);
                        cube.expand(p);
                        cube.set_min_size(v3f{d});
                        return cube;
                    };
                    // draw_cube(&imgui.ctx, axis_cube(w_up, .1f), color::rgba::green);
                    // draw_cube(&imgui.ctx, axis_cube(w_right, .1f), color::rgba::red);
                    // draw_cube(&imgui.ctx, axis_cube(w_forward, .1f), color::rgba::blue);

                    // draw_cube(&imgui.ctx, axis_rod(*pos, w_up, 0.05f), color::rgba::green);
                    // draw_cube(&imgui.ctx, axis_rod(*pos, w_right, 0.05f), color::rgba::red);
                    // draw_cube(&imgui.ctx, axis_rod(*pos, w_forward, 0.05f), color::rgba::blue);
                }

                
            }
        }

        struct gizmo_result_t {
            b32 released = false;
            v3f start_pos{};

        };

        enum struct gizmo_mode {
            local, global
        };

        inline gizmo_result_t
        gizmo(
            state_t& imgui,
            v3f* pos,
            const m44& vp,
            f32 snapping = 0.0f,
            m33 basis = m33{1.0f},
            gizmo_mode mode = gizmo_mode::global
        ) {
            if (mode == gizmo_mode::local) {
                basis = m33{1.0f};
            }

            const m33 drawing_basis = math::normalize(basis);

            gizmo_result_t result = {};
            draw_gizmo(imgui, pos, vp, 1, 1, drawing_basis);

            const v3f spos = math::world_to_screen(vp, *pos);
            math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};

            const auto is_gizmo_id = [](u64 id){
                switch(id) {
                    case "gizmo_xy"_sid: 
                    case "gizmo_xz"_sid: 
                    case "gizmo_yz"_sid: 
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
                const auto v_up         = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + drawing_basis[1])};
                const auto v_forward    = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + drawing_basis[2])};
                const auto v_right      = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + drawing_basis[0])};
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
                
                if constexpr (false)
                {
                    const f32 center = 0.5f;
                    const f32 quad_size = 0.15f;
                    const f32 quad_min = center - quad_size;
                    const f32 quad_max = center + quad_size;

                    v3f xy_quad[4] = {
                        *pos + (drawing_basis[0] + drawing_basis[1]) * quad_min, 
                        *pos + drawing_basis[0] * quad_min + drawing_basis[1] * quad_max,
                        *pos + (drawing_basis[0] + drawing_basis[1]) * quad_max, 
                        *pos + drawing_basis[0] * quad_max + drawing_basis[1] * quad_min,
                    };
                    draw_quad(&imgui.ctx, xy_quad, gfx::color::rgba::yellow);

                    v3f xz_quad[4] = {
                        *pos + (drawing_basis[0] + drawing_basis[2]) * quad_min, 
                        *pos + drawing_basis[0] * quad_min + drawing_basis[2] * quad_max,
                        *pos + (drawing_basis[0] + drawing_basis[2]) * quad_max, 
                        *pos + drawing_basis[0] * quad_max + drawing_basis[2] * quad_min,
                    };
                    draw_quad(&imgui.ctx, xz_quad, gfx::color::rgba::purple);

                    v3f zy_quad[4] = {
                        *pos + (drawing_basis[2] + drawing_basis[1]) * quad_min, 
                        *pos + drawing_basis[2] * quad_min + drawing_basis[1] * quad_max,
                        *pos + (drawing_basis[2] + drawing_basis[1]) * quad_max, 
                        *pos + drawing_basis[2] * quad_max + drawing_basis[1] * quad_min,
                    };
                    draw_quad(&imgui.ctx, zy_quad, gfx::color::rgba::cyan);
                }

                u64 giz_id = is_gizmo_id(imgui.active.id) ? imgui.active.id : 0;

                // if (math::intersect(pc, mouse)) {giz_id = "gizmo_pos"_sid;} else
                if (is_gizmo_id(imgui.active.id) == false) {

                    if (math::intersect(rc, mouse)) {giz_id = "gizmo_right"_sid;} else
                    if (math::intersect(uc, mouse)) {giz_id = "gizmo_up"_sid;} else
                    if (math::intersect(fc, mouse)) {giz_id = "gizmo_forward"_sid;}
                }
                imgui.verify_id(giz_id);

                const bool clicked = imgui.ctx.input->pressed.mouse_btns[0];
                const bool released = imgui.ctx.input->released.mouse_btns[0];
                if (is_gizmo_id(imgui.active.id)) {
                    if (is_gizmo_id(imgui.hot.id)) {
                        if (imgui.gizmo_mouse_start != v2f{0.0f}) {
                            const v2f naxis = glm::normalize(imgui.gizmo_axis_dir);
                            const f32 proj = glm::dot(mdelta, naxis);
                            const f32 scale = imgui.gizmo_scale_start;
                            auto p = imgui.gizmo_world_start + imgui.gizmo_axis_wdir * glm::dot(mdelta, naxis/scale);
                            *pos = snapping != 0.0f ? glm::floor(p/snapping+0.5f)*snapping : p;

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
                                    imgui.gizmo_axis_wdir = glm::normalize(basis[0]);
                                    imgui.gizmo_mouse_start = v_right;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case "gizmo_up"_sid: {
                                    imgui.gizmo_axis_dir = (ud);
                                    imgui.gizmo_axis_wdir = glm::normalize(basis[1]);
                                    imgui.gizmo_mouse_start = v_up;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case "gizmo_forward"_sid: {
                                    imgui.gizmo_axis_dir = (fd);
                                    imgui.gizmo_axis_wdir = glm::normalize(basis[2]);
                                    imgui.gizmo_mouse_start = v_forward;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case_invalid_default;
                            }
                        }
                    }
                    if (released) {
                        result.released = 1;      
                        result.start_pos = imgui.gizmo_world_start;
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
                        result.released = 1;
                        result.start_pos = imgui.gizmo_world_start;
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
                    result.released = 1;
                    result.start_pos = imgui.gizmo_world_start;
                    imgui.hot = imgui.active = 0;
                    imgui.gizmo_mouse_start = v2f{0.0f};
                    imgui.gizmo_axis_dir = v2f{0.0f};
                    imgui.gizmo_axis_wdir = v3f{0.0f};
                }
            }

            return result;
        }

        inline gizmo_result_t
        gizmo_scale(
            state_t& imgui,
            v3f* pos,
            m33* basis,
            const m44& vp,
            f32 snapping = 0.0f
        ) {
            gizmo_result_t result = {};
            draw_gizmo(imgui, pos, vp, 0, 1, math::normalize(*basis));

            const v3f spos = math::world_to_screen(vp, *pos);
            math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};

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
                const auto v_up         = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + glm::normalize((*basis)[1]))};
                const auto v_forward    = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + glm::normalize((*basis)[2]))};
                const auto v_right      = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + glm::normalize((*basis)[0]))};
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
                imgui.verify_id(giz_id);

                const bool clicked = imgui.ctx.input->pressed.mouse_btns[0];
                const bool released = imgui.ctx.input->released.mouse_btns[0];
                if (is_gizmo_id(imgui.active.id)) {
                    if (is_gizmo_id(imgui.hot.id)) {
                        if (imgui.gizmo_mouse_start != v2f{0.0f}) {
                            const v2f naxis = glm::normalize(imgui.gizmo_axis_dir);
                            const f32 proj = glm::dot(mdelta, naxis);
                            const f32 scale = imgui.gizmo_scale_start;
                            
                            auto s = (imgui.gizmo_basis_scale + glm::dot(mdelta, naxis/scale)); 
                            if (snapping != 0.0f) s = glm::floor(s / snapping + 0.5f) * snapping;
                            if (imgui.gizmo_axis_wdir == axis::right) {
                                (*basis)[0] = imgui.gizmo_world_start * s;
                            }
                            if (imgui.gizmo_axis_wdir == axis::up) {
                                (*basis)[1] = imgui.gizmo_world_start * s;
                            }
                            if (imgui.gizmo_axis_wdir == axis::forward) {
                                (*basis)[2] = -imgui.gizmo_world_start * s;
                            }
                            // *basis = glm::scale(m44{*basis}, imgui.gizmo_axis_wdir * proj);

                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start, 6.0f, color::rgba::light_gray);
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start, 8.0f, color::to_color32(imgui.gizmo_axis_wdir));
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, 6.0f, color::rgba::light_gray);
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, 8.0f, color::to_color32(imgui.gizmo_axis_wdir));
                            
                            draw_line(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, imgui.gizmo_mouse_start, 3.0f, color::to_color32(imgui.gizmo_axis_wdir * 0.7f));
                            draw_line(&imgui.ctx, v_pos - naxis * 10000.0f, v_pos + naxis * 10000.0f, 2.0f, color::rgba::light_gray);

                        } else {
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
                                    imgui.gizmo_basis_scale = glm::length((*basis)[0]);
                                    // imgui.gizmo_axis_wdir = glm::normalize((*basis)[0]);
                                    imgui.gizmo_mouse_start = v_right;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case "gizmo_up"_sid: {
                                    imgui.gizmo_axis_dir = (ud);
                                    imgui.gizmo_axis_wdir = axis::up;
                                    imgui.gizmo_basis_scale = glm::length((*basis)[1]);
                                    // imgui.gizmo_axis_wdir = glm::normalize((*basis)[1]);
                                    imgui.gizmo_mouse_start = v_up;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case "gizmo_forward"_sid: {
                                    imgui.gizmo_axis_dir = (fd);
                                    imgui.gizmo_axis_wdir = axis::forward;
                                    imgui.gizmo_basis_scale = glm::length((*basis)[2]);
                                    // imgui.gizmo_axis_wdir = glm::normalize((*basis)[2]);
                                    imgui.gizmo_mouse_start = v_forward;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case_invalid_default;
                            }
                            imgui.gizmo_world_start = glm::normalize(*basis * imgui.gizmo_axis_wdir);
                            imgui.gizmo_basis_scale = glm::length(*basis * imgui.gizmo_axis_wdir);
                            imgui.gizmo_basis_start = *basis;
                        }
                    }
                    if (released) {
                        result.released = 1;      
                        result.start_pos = imgui.gizmo_world_start;
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
                        result.released = 1;
                        result.start_pos = imgui.gizmo_world_start;
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
                    result.released = 1;
                    result.start_pos = imgui.gizmo_world_start;
                    imgui.hot = imgui.active = 0;
                    imgui.gizmo_mouse_start = v2f{0.0f};
                    imgui.gizmo_axis_dir = v2f{0.0f};
                    imgui.gizmo_axis_wdir = v3f{0.0f};
                }
            }

            return result;
        }

        inline gizmo_result_t
        gizmo_rotate(
            state_t& imgui,
            v3f* pos,
            m33* basis,
            const m44& vp,
            f32 snapping
        ) {
            gizmo_result_t result = {};
            draw_gizmo(imgui, pos, vp, 1, 0, math::normalize(*basis));

            const v3f spos = math::world_to_screen(vp, *pos);
            math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};

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
                const auto v_up         = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + glm::normalize((*basis)[1]))};
                const auto v_forward    = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + glm::normalize((*basis)[2]))};
                const auto v_right      = imgui.ctx.screen_size * v2f{math::world_to_screen(vp, *pos + glm::normalize((*basis)[0]))};
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
                imgui.verify_id(giz_id);

                const bool clicked = imgui.ctx.input->pressed.mouse_btns[0];
                const bool released = imgui.ctx.input->released.mouse_btns[0];
                if (is_gizmo_id(imgui.active.id)) {
                    if (is_gizmo_id(imgui.hot.id)) {
                        if (imgui.gizmo_mouse_start != v2f{0.0f}) {
                            const v2f naxis = glm::normalize(imgui.gizmo_axis_dir);
                            const f32 proj = glm::dot(mdelta, naxis);
                            const f32 scale = imgui.gizmo_scale_start;

                            math::transform_t t{};
                            // t.basis = *basis;
                            auto angle = glm::dot(mdelta * 10.0F, naxis/scale);
                            //angle = glm::radians(angle);
                            if (snapping != 0.0f) {
                                angle = glm::floor(angle / snapping + 0.5f) * snapping;
                            }
                            angle = glm::radians(angle);
                            
                            // t.basis = imgui.gizmo_basis_start;
                            t.set_rotation(imgui.gizmo_axis_wdir, angle);
                            // t.rotate(imgui.gizmo_axis_wdir, angle);
                            *basis = t.basis * imgui.gizmo_basis_start;

                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start, 6.0f, color::rgba::light_gray);
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start, 8.0f, color::to_color32(imgui.gizmo_axis_wdir));
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, 6.0f, color::rgba::light_gray);
                            draw_circle(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, 8.0f, color::to_color32(imgui.gizmo_axis_wdir));
                            
                            draw_line(&imgui.ctx, imgui.gizmo_mouse_start + naxis * proj, imgui.gizmo_mouse_start, 3.0f, color::to_color32(imgui.gizmo_axis_wdir * 0.7f));
                            draw_line(&imgui.ctx, v_pos, v_pos + naxis * 10000.0f, 2.0f, color::rgba::light_gray);

                        } else {
                            imgui.gizmo_basis_start = *basis;
                            switch(giz_id) {
                                case 0: break;
                                case "gizmo_pos"_sid: {
                                    imgui.gizmo_mouse_start = v_pos;
                                    imgui.gizmo_axis_dir = (rd);
                                    break;
                                }
                                case "gizmo_right"_sid: {
                                    imgui.gizmo_axis_dir = (rd);
                                    // imgui.gizmo_axis_wdir = axis::right;
                                    imgui.gizmo_axis_wdir = (*basis)[0];
                                    imgui.gizmo_mouse_start = v_right;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case "gizmo_up"_sid: {
                                    imgui.gizmo_axis_dir = (ud);
                                    // imgui.gizmo_axis_wdir = axis::up;
                                    imgui.gizmo_axis_wdir = (*basis)[1];
                                    imgui.gizmo_mouse_start = v_up;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case "gizmo_forward"_sid: {
                                    imgui.gizmo_axis_dir = (fd);
                                    // imgui.gizmo_axis_wdir = axis::forward;
                                    imgui.gizmo_axis_wdir = (*basis)[2];
                                    imgui.gizmo_mouse_start = v_forward;
                                    imgui.gizmo_scale_start = glm::length(imgui.gizmo_axis_dir); 
                                    break;
                                }
                                case_invalid_default;
                            }
                            imgui.gizmo_axis_wdir = glm::normalize(imgui.gizmo_axis_wdir);
                        }
                    }
                    if (released) {
                        result.released = 1;      
                        result.start_pos = imgui.gizmo_world_start;
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
                        result.released = 1;
                        result.start_pos = imgui.gizmo_world_start;
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
                    result.released = 1;
                    result.start_pos = imgui.gizmo_world_start;
                    imgui.hot = imgui.active = 0;
                    imgui.gizmo_mouse_start = v2f{0.0f};
                    imgui.gizmo_axis_dir = v2f{0.0f};
                    imgui.gizmo_axis_wdir = v3f{0.0f};
                }
            }

            return result;
        }

        inline void
        point_edit(
            state_t& imgui,
            v2f* point, 
            math::rect2d_t rect, // where on screen to draw
            math::rect2d_t range,
            color32 color,
            color32 outline_color = gfx::color::rgba::black,
            f32 radius = 8.0f
        ) {
            const u64 pnt_id = imgui.verify_id((u64)point);

            const auto t = (*point - range.min) / range.size();
            
            math::circle_t circle;
            circle.origin = t * rect.size() + rect.min;
            circle.radius = radius;

            color = imgui.hot.id == pnt_id ? gfx::color::to_color32(glm::mix(
                gfx::color::to_color3(imgui.theme.active_color), 
                gfx::color::to_color3(color), 
                imgui.anim(5.0f))) : color;

            draw_circle(imgui, circle.origin, circle.radius, outline_color);
            draw_circle(imgui, circle.origin, circle.radius - 2.0f, color);

            auto [x,y] = imgui.ctx.input->mouse.pos;

            if (imgui.active.id == pnt_id) {
                if (imgui.hot.id == pnt_id) {
                    const auto mt = (v2f{x,y} - rect.min)/rect.size();
                    *point = mt * range.size() + range.min;
                }
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = 0;
                }
            } else if (imgui.hot.id == pnt_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = pnt_id;
                }
            }

            if (circle.contains(v2f{x,y})) {
                imgui.hot = pnt_id;
            } else if (imgui.hot.id == pnt_id && imgui.ctx.input->mouse.buttons[0] == false) {
                imgui.hot.id = 0;
            }
        }

        inline void
        derivative_edit(
            state_t& imgui,
            v2f point,
            v2f* derivative, 
            math::rect2d_t rect, // where on screen to draw
            math::rect2d_t range,
            color32 color,
            color32 outline_color = gfx::color::rgba::black,
            f32 radius = 8.0f
        ) {
            *derivative = point+*derivative;
            point_edit(imgui, derivative, rect, range, color, outline_color, radius);
            *derivative = *derivative-point;
        }

        inline void
        color_edit(
            state_t& imgui,
            color32* color,
            v2f size = v2f{128.0f}
        ) {
            const u64 clr_id = imgui.verify_id((u64)color);

            v3f color_hsv = glm::hsvColor(color::to_color3(*color));
            
            v2f tmp_cursor = imgui.panel->draw_cursor;
            // tmp_cursor.x += imgui.theme.padding;

            const f32 slider_pxl = 0.1f * size.x;
            const f32 value_circle_radius = 0.025f * size.x;

            math::rect2d_t r;
            r.expand(tmp_cursor);
            r.expand(tmp_cursor + size);
            r.pad(imgui.theme.padding);

            auto starting_rect = r;
            // draw_round_rect(&imgui.ctx, starting_rect, imgui.theme.border_radius, imgui.theme.border_color);

            r.pad(imgui.theme.border_thickness + imgui.theme.border_radius); // pad again for border

            math::rect2d_t hue_box;
            math::rect2d_t value_box;
            math::rect2d_t saturation_box;
            math::rect2d_t tr = r;
            std::tie(saturation_box, r) = math::cut_bottom(r, slider_pxl);
            std::tie(value_box, r)      = math::cut_bottom(r, slider_pxl);
            std::tie(hue_box, r)        = math::cut_right(r, slider_pxl);

            hue_box.max.y -= imgui.theme.padding;

            // value_box.max.x -= imgui.theme.padding;
            // saturation_box.max.x -= imgui.theme.padding;

            // saturation_box = value_box;
            // r.min = r.max - v2f{r.size().y};
            r.max = r.min + v2f{r.size().y};
            hue_box.add(math::width2 * -(hue_box.min.x - r.max.x - imgui.theme.padding));

            saturation_box.add(math::height2 * (imgui.theme.padding));
            // saturation_box.add(math::height2 * (value_box.size().y + imgui.theme.padding));
            // value_box.max.x =  
            // auto [divider, box] = math::cut_right(r, imgui.theme.padding);
            auto box = r;
            box.pad(imgui.theme.padding);
            // hue_box.pad(imgui.theme.padding);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            const auto mouse = v2f{x,y};
            const auto selection_color = color::rgba::white;

            color32 rect_colors[4]= {
                color::rgba::white,
                color::rgba::black,
                color::to_color32(glm::rgbColor(v3f{color_hsv.x, 1.0f, 1.0f})),
                color::rgba::black,
            };
            const size_t res = 8;
            range_u64(xx, 0, res) {
                f32 xps = f32(xx)/f32(res);
                f32 xpe = f32(xx+1)/f32(res);
                range_u64(yy, 0, res) {
                    f32 yps = f32(yy)/f32(res);
                    f32 ype = f32(yy+1)/f32(res);

                    math::rect2d_t seg;
                    seg.expand(box.sample(v2f{xps, yps}));
                    seg.expand(box.sample(v2f{xpe, ype}));
                
                    // [min, {min.x, max.y}, {max.x, min.y}, max]
                    color32 seg_colors[4]= {                        
                        // left
                        color::to_color32(glm::rgbColor(v3f{color_hsv.x, xps, 1.0f - yps})), // top
                        color::to_color32(glm::rgbColor(v3f{color_hsv.x, xps, 1.0f - ype})), // bottom
                        // right
                        color::to_color32(glm::rgbColor(v3f{color_hsv.x, xpe, 1.0f - yps})), // top 
                        color::to_color32(glm::rgbColor(v3f{color_hsv.x, xpe, 1.0f - ype})), // bottom
                    };

                    draw_rect(&imgui.ctx, seg, std::span{seg_colors});
                }
            }

            // draw_rect(&imgui.ctx, box, std::span{rect_colors});
            // draw_rect(&imgui.ctx, box, std::span{rect_colors});

            f32 bx = color_hsv.y * box.size().x;
            f32 by = (1.0f-color_hsv.z) * box.size().y;

            draw_circle(&imgui.ctx, box.min + v2f{bx,by}, value_circle_radius + imgui.theme.border_thickness, color::rgba::black);
            draw_circle(&imgui.ctx, box.min + v2f{bx,by}, value_circle_radius, selection_color);

            range_u64(i, 0, res) {
                math::rect2d_t hue_seg;
                math::rect2d_t val_seg;
                math::rect2d_t sat_seg;
                
                const f32 start_prc = f32(i) / f32(res);
                const f32 end_prc = f32(i+1) / f32(res);


                hue_seg.expand(hue_box.sample(math::height2 * start_prc));
                hue_seg.expand(hue_box.sample(v2f{1.0f, end_prc}));

                val_seg.expand(value_box.sample(math::width2 * start_prc));
                val_seg.expand(value_box.sample(v2f{end_prc, 1.0f}));
                sat_seg.expand(saturation_box.sample(math::width2 * start_prc));
                sat_seg.expand(saturation_box.sample(v2f{end_prc, 1.0f}));
                
                const f32 start_angle = start_prc * 360.0f;
                const f32 next_angle = end_prc * 360.0f;
                const v3f start_color = glm::rgbColor(v3f{start_angle, 1.0f, 1.0f});
                const v3f next_color = glm::rgbColor(v3f{next_angle, 1.0f, 1.0f});
                const v3f start_value = glm::rgbColor(v3f{color_hsv.x, 1.0f, start_prc});
                const v3f next_value = glm::rgbColor(v3f{color_hsv.x, 1.0f, end_prc});
                const v3f start_sat = glm::rgbColor(v3f{color_hsv.x, start_prc, 1.0f});
                const v3f next_sat = glm::rgbColor(v3f{color_hsv.x, end_prc, 1.0f});

                color32 hue_colors[4]= {
                    color::to_color32(start_color),
                    color::to_color32(next_color),
                    color::to_color32(start_color),
                    color::to_color32(next_color),
                };
                color32 value_colors[4]= {
                    color::to_color32(start_value),
                    color::to_color32(start_value),
                    color::to_color32(next_value),
                    color::to_color32(next_value),
                };
                color32 saturation_colors[4]= {
                    color::to_color32(start_sat),
                    color::to_color32(start_sat),
                    color::to_color32(next_sat),
                    color::to_color32(next_sat),
                };
                
                draw_rect(&imgui.ctx, hue_seg, std::span{hue_colors});
                draw_rect(&imgui.ctx, val_seg, std::span{value_colors});
                draw_rect(&imgui.ctx, sat_seg, std::span{saturation_colors});
            }

            f32 tx = hue_box.max.x + 2.0f;
            f32 ty = hue_box.min.y + (color_hsv.x/360.0f)*hue_box.size().y;

            f32 sx = saturation_box.min.x + (color_hsv.y*saturation_box.size().x);
            f32 sy = saturation_box.max.y + 2.0f;

            f32 vx = value_box.min.x + (color_hsv.z*value_box.size().x);
            f32 vy = value_box.max.y + 2.0f;

            draw_circle(&imgui.ctx, v2f{tx,ty}, 7.0f, color::rgba::black, 3, 1.0f, 0.5f);
            draw_circle(&imgui.ctx, v2f{tx,ty}, 5.0f, color::rgba::white, 3, 1.0f, 0.5f);

            draw_circle(&imgui.ctx, v2f{sx,sy}, 7.0f, color::rgba::black, 3, 1.0f, 0.75f);
            draw_circle(&imgui.ctx, v2f{sx,sy}, 5.0f, color::rgba::white, 3, 1.0f, 0.75f);

            draw_circle(&imgui.ctx, v2f{vx,vy}, 7.0f, color::rgba::black, 3, 1.0f, 0.75f);
            draw_circle(&imgui.ctx, v2f{vx,vy}, 5.0f, color::rgba::white, 3, 1.0f, 0.75f);


            const f32 col_s = glm::max(x - box.min.x, 0.0f) / box.size().x;
            const f32 col_v = glm::max(y - box.min.y, 0.0f) / box.size().y;
            const f32 hue_prc = glm::max(y - hue_box.min.y, 0.0f) / hue_box.size().y;
            const f32 value_prc = glm::max(x - value_box.min.x, 0.0f) / value_box.size().x;
            const f32 saturation_prc = glm::max(x - saturation_box.min.x, 0.0f) / saturation_box.size().x;

            if (imgui.active.id == clr_id) {
                if (imgui.hot.id == clr_id) {
                    if (box.contains(v2f{x,y})) {
                        *color = color::to_color32(glm::rgbColor(v3f{color_hsv.x, col_s, 1.0f-col_v}));
                    } else if (hue_box.contains(v2f{x,y})) {
                        *color = color::to_color32(glm::rgbColor(v3f{hue_prc*360.0f, color_hsv.y, color_hsv.z}));
                    } else if (saturation_box.contains(v2f{x,y})) {
                        // draw_rect(&imgui.ctx, saturation_box, selection_color);
                        // puts("sat");
                        *color = color::to_color32(glm::rgbColor(v3f{color_hsv.x, saturation_prc, color_hsv.z}));
                    } else if (value_box.contains(v2f{x,y})) {
                        *color = color::to_color32(glm::rgbColor(v3f{color_hsv.x, color_hsv.y, value_prc}));
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

            if (tr.contains(mouse) || saturation_box.contains(mouse)) {
                imgui.hot = clr_id;
            } else if (imgui.hot.id == clr_id) {
                imgui.hot.id = 0;
            }

            imgui.panel->draw_cursor.y = saturation_box.max.y + imgui.theme.padding * 2.0f;
            imgui.panel->expand(imgui.panel->draw_cursor);
        }

        inline void
        image(
            state_t& imgui,
            u32 bind_index,
            math::rect2d_t screen,
            math::rect2d_t uv = math::rect2d_t{v2f{0.0f}, v2f{1.0f}},
            u32 color = color::rgba::white
        ) {
            draw_rect(&imgui.ctx, screen, bind_index, uv, color);
        }
 
        inline void
        float_drag_id(
            state_t& imgui,
            u64 id,
            f32* val,
            f32 step = 0.1f,
            f32 min = std::numeric_limits<f32>::min(),
            f32 max = std::numeric_limits<f32>::max(),
            v2f size = v2f{64.0f, 16.0f}
        ) {
            const u64 drg_id = imgui.verify_id(id);

            v2f tmp_cursor = imgui.panel->draw_cursor;
            v2f save_draw = tmp_cursor;
            tmp_cursor.x += 8.0f;
            math::rect2d_t box;
            math::rect2d_t prc_box;

            const b32 is_clamped = (min != std::numeric_limits<f32>::min());
                        
            const f32 prc = 
                // is_clamped ?
                // glm::clamp((*val - min) / (max - min), 0.0f, 1.0f) : 
                1.0f;

            box.expand(tmp_cursor);
            box.expand(tmp_cursor + size);

            prc_box.expand(tmp_cursor + 2.0f);
            prc_box.expand(prc_box.min + (size - 4.0f) * v2f{prc, 1.0f});
            imgui.panel->update_cursor_max(box.max + imgui.theme.padding);

            tmp_cursor = box.center() - font_get_size(imgui.ctx.font, fmt_sv("{:.2f}", *val)) * 0.5f;

            draw_rect(&imgui.ctx, box, imgui.theme.fg_color);
            box.pad(-imgui.theme.border_thickness);
            draw_rect(&imgui.ctx, box, imgui.theme.bg_color);
            draw_rect(&imgui.ctx, prc_box, imgui.active.id == drg_id ? imgui.theme.active_color : imgui.theme.fg_color);
            draw_string(&imgui.ctx, fmt_sv("{:.2f}", *val), &tmp_cursor, imgui.theme.text_color);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            const v2f offset = v2f{x,y} - imgui.drag_start;
            // expand all the way to get prc box size
            prc_box.expand(prc_box.min + (size - 4.0f) * v2f{1.0f});

            const f32 dist_prc = glm::min(offset.x, offset.y);
            const b32 shift_held = imgui.ctx.input->keys[key_id::RIGHT_SHIFT] || imgui.ctx.input->keys[key_id::LEFT_SHIFT];
            f32 pixel_scale = shift_held ? 4.0f : 64.0f;

            const f32 m_prc = glm::length(offset) / pixel_scale * glm::sign(dist_prc);


            // if (imgui.stealing.id == drg_id) {
            //     imgui.active = imgui.stealing;
            // }

            if (imgui.active.id == drg_id) {
                {
                    auto tc = imgui.drag_start + v2f{2.0f};
                    draw_string(&imgui.ctx, fmt_sv("{:.2f}", *val), &tc, gfx::color::rgba::black);
                    tc = imgui.drag_start - v2f{1.0f};
                    draw_string(&imgui.ctx, fmt_sv("{:.2f}", *val), &tc, gfx::color::rgba::white);
                }
                draw_circle(&imgui.ctx, imgui.drag_start, 5.0f, gfx::color::rgba::black);
                draw_circle(&imgui.ctx, imgui.drag_start, 3.0f, gfx::color::rgba::white);
                draw_circle(&imgui.ctx, v2f{x,y}, 5.0f, gfx::color::rgba::black);
                draw_circle(&imgui.ctx, v2f{x,y}, 3.0f, gfx::color::rgba::white);
                draw_ellipse_arc_with_basis(&imgui.ctx, imgui.drag_start, v2f{glm::length(offset)}, v2f{1.0f,0.0f}, v2f{0.0f, 1.0f}, 0.0f, 1.0f, 4.0f, color::rgba::white, 32);

                // if (imgui.hot.id == drg_id) 
                {
                    auto add = glm::floor(m_prc) * step;
                    *val = imgui.float_drag_start_value + add;
                    if (is_clamped) {
                        *val = glm::clamp(*val, min, max);
                    }
                    // imgui.drag_start = v2f{x,y};
                    imgui.stealing = drg_id;
                }
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = 0;
                    imgui.stealing = 0;
                }
            } else if (imgui.hot.id == drg_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = drg_id;
                    imgui.drag_start = v2f{x,y};
                    imgui.float_drag_start_value = *val;
                }
            }

            if (box.contains(v2f{x,y})) {
                imgui.hot = drg_id;
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
        int_drag(
            state_t& imgui,
            i32* val,
            i32 step = 1,
            f32 min = std::numeric_limits<f32>::min(),
            f32 max = std::numeric_limits<f32>::max(),
            v2f size = v2f{64.0f, 16.0f}
        ) {
            f32 f = (f32)*val;
            float_drag_id(imgui, (u64)val, &f, (f32)step, min, max, size);
            *val = (i32)f;
        }

        inline void
        uint_drag(
            state_t& imgui,
            u32* val,
            i32 step = 1,
            f32 min = std::numeric_limits<f32>::min(),
            f32 max = std::numeric_limits<f32>::max(),
            v2f size = v2f{64.0f, 16.0f}
        ) {
            f32 f = (f32)*val;
            float_drag_id(imgui, (u64)val, &f, (f32)step, min, max, size);
            *val = (u32)(f+0.1f);
        }

        inline void
        float_drag(
            state_t& imgui,
            f32* val,
            f32 step = 0.1f,
            f32 min = std::numeric_limits<f32>::min(),
            f32 max = std::numeric_limits<f32>::max(),
            v2f size = v2f{64.0f, 16.0f}
        ) {
            float_drag_id(imgui, (u64)val, val, step, min, max, size);
        }

        inline void
        float2_drag(
            state_t& imgui,
            v2f* val,
            f32 step = 0.1f,
            f32 min = std::numeric_limits<f32>::min(),
            f32 max = std::numeric_limits<f32>::max(),
            v2f size = v2f{64.0f, 16.0f}
        ) {
            auto theme = imgui.theme;
            imgui.theme.text_color = gfx::color::rgba::black;
            
            same_line(imgui);
            imgui.theme.fg_color = gfx::color::rgba::red;
            float_drag_id(imgui, (u64)&val->x, &val->x, step, min, max, size);

            imgui.theme.fg_color = gfx::color::rgba::green;
            float_drag_id(imgui, (u64)&val->y, &val->y, step, min, max, size);

            imgui.theme = theme;
        }

        inline void
        float3_drag(
            state_t& imgui,
            v3f* val,
            f32 step = 0.1f,
            f32 min = std::numeric_limits<f32>::min(),
            f32 max = std::numeric_limits<f32>::max(),
            v2f size = v2f{64.0f, 16.0f}
        ) {
            auto theme = imgui.theme;
            imgui.theme.text_color = gfx::color::rgba::black;

            same_line(imgui);
            
            imgui.theme.fg_color = gfx::color::rgba::red;
            float_drag_id(imgui, (u64)&val->x, &val->x, step, min, max, size);

            same_line(imgui);
            
            imgui.theme.fg_color = gfx::color::rgba::green;
            float_drag_id(imgui, (u64)&val->y, &val->y, step, min, max, size);

            imgui.theme.fg_color = gfx::color::rgba::blue;
            float_drag_id(imgui, (u64)&val->z, &val->z, step, min, max, size);
            imgui.theme = theme;
        }

        inline void
        float4_drag(
            state_t& imgui,
            v4f* val,
            f32 step = 0.1f,
            f32 min = std::numeric_limits<f32>::min(),
            f32 max = std::numeric_limits<f32>::max(),
            v2f size = v2f{64.0f, 16.0f}
        ) {
            auto theme = imgui.theme;
            same_line(imgui);
            
            imgui.theme.fg_color = gfx::color::rgba::red;
            float_drag_id(imgui, (u64)&val->x, &val->x, step, min, max, size);

            same_line(imgui);

            imgui.theme.fg_color = gfx::color::rgba::green;
            float_drag_id(imgui, (u64)&val->y, &val->y, step, min, max, size);

            same_line(imgui);
            imgui.theme.fg_color = gfx::color::rgba::blue;
            float_drag_id(imgui, (u64)&val->z, &val->z, step, min, max, size);

            imgui.theme.fg_color = gfx::color::rgba::purple;
            float_drag_id(imgui, (u64)&val->w, &val->w, step, min, max, size);

            imgui.theme = theme;
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
            const u64 sld_id = imgui.verify_id(id);

            v2f tmp_cursor = imgui.panel->draw_cursor;
            v2f save_draw = tmp_cursor;
            tmp_cursor.x += 8.0f;
            math::rect2d_t box;
            math::rect2d_t prc_box;
            const f32 prc = glm::clamp((*val - min) / (max - min), 0.0f, 1.0f);
            box.expand(tmp_cursor);
            box.expand(tmp_cursor + size);

            prc_box.expand(tmp_cursor + 2.0f);
            prc_box.expand(prc_box.min + (size - 4.0f) * v2f{prc, 1.0f});
            imgui.panel->update_cursor_max(box.max + imgui.theme.padding);

            tmp_cursor = box.center() - font_get_size(imgui.ctx.font, fmt_sv("{:.2f}", *val)) * 0.5f;
            draw_rect(&imgui.ctx, box, imgui.theme.fg_color);
            box.pad(1.0f);
            draw_rect(&imgui.ctx, box, imgui.theme.bg_color);
            draw_rect(&imgui.ctx, prc_box, imgui.active.id == sld_id ? imgui.theme.active_color : imgui.theme.fg_color);
            // box.max += v2f{1.0f};
            // box.min -= v2f{1.0f};
            draw_string(&imgui.ctx, fmt_sv("{:.2f}", *val), &tmp_cursor, imgui.theme.text_color);

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
            const u64 sld_id = imgui.verify_id((u64)val);
            float_slider_id(imgui, sld_id, val, min, max, size);
        }

        enum struct text_activate_mode {
            click, hover
        };
       
        inline bool
        text(
            state_t& imgui, 
            std::string_view text,
            bool* toggle_state = 0,
            text_activate_mode mode = text_activate_mode::click
        ) {
            const sid_t txt_id = imgui.verify_id(sid(text));
            bool result = toggle_state ? *toggle_state : false;

            constexpr f32 text_pad = 4.0f;
            const auto font_size = font_get_size(imgui.ctx.font, text);

            imgui.panel->update_cursor_max(imgui.panel->draw_cursor + font_size + text_pad * 2.0f);

            v2f temp_cursor = imgui.panel->draw_cursor;
            const f32 start_x = imgui.panel->draw_cursor.x;
            temp_cursor.x += text_pad;

            math::rect2d_t text_box;
            text_box.expand(temp_cursor);
            text_box.expand(temp_cursor + font_size);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            v2f mp{x,y};
            if (imgui.validate_position(mp) && text_box.contains(v2f{x,y})) {
                imgui.hot = txt_id;
            } else if (imgui.hot.id == txt_id) {
                if (imgui.active.id == txt_id && imgui.dragging_enabled && !imgui.drag_event) {
                    drag_event_t drag_event{};
                    drag_event.start_time = imgui.time();
                    drag_event.start_pos = v2f{x,y};
                    drag_event.widget_id = txt_id;
                    drag_event.user_data = imgui.drag_user_data;
                    drag_event.user_data_size = imgui.drag_user_data_size;
                    drag_event.type_name = imgui.drag_user_type_name;
                    imgui.drag_event.emplace(drag_event);
                }
                imgui.hot.id = 0;
            }

            if (imgui.active.id == txt_id) {
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    if (imgui.hot.id == txt_id) {
                        result = !result;
                        if (toggle_state) {
                            *toggle_state = result;
                        }
                        if (imgui.drag_event) {
                            imgui.drag_event->dropped_id = txt_id;
                            imgui.drag_event->dropped_type_name = imgui.drag_user_type_name;
                            imgui.drag_event->dropped_user_data = imgui.drag_user_data;
                            imgui.drag_event->dropped_user_data_size = imgui.drag_user_data_size;
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

            auto save_temp = temp_cursor;
            if (imgui.drag_event && imgui.drag_event->widget_id == txt_id) {
                temp_cursor = v2f{x,y};
            }

            auto shadow_cursor = temp_cursor + imgui.theme.shadow_distance;
            draw_string(&imgui.ctx, text, &shadow_cursor, imgui.theme.shadow_color);
            draw_string(&imgui.ctx, text, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);

            if (imgui.drag_event && imgui.drag_event->widget_id == txt_id) {
                auto text_height = temp_cursor.y - y;
                temp_cursor = save_temp + v2f{0.0f, text_height};
            }

            // draw_rect(&imgui.ctx, text_box, gfx::color::rgba::purple);
            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += font_size.x + imgui.theme.padding;
            } else {
                imgui.panel->draw_cursor.y = temp_cursor.y - font_size.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->min.x + imgui.theme.padding;
            }
            
            // todo clean up hover and click seperation
            if (mode == text_activate_mode::hover) {
                return imgui.hot.id == txt_id;
            } else {
                return result;
            }
        }

        inline u64
        tabs(
            state_t& imgui, 
            std::span<std::string_view> titles,
            u64 current
        ) {
            const auto size = titles.size();
            u64 result = current;
            const auto theme = imgui.theme;
            for (u32 i = 0; i < size; i++) {
                if (titles[i] == "\n"sv) {
                    continue;
                }
                if (i+1 < size) {
                    if (!(i+2 < size && titles[i+1] == "\n"sv)) {
                        im::same_line(imgui);
                    } 
                }
                const auto id = sid(titles[i]);
                auto selected = current == id;
                if (selected) {
                    imgui.theme.text_color = imgui.theme.active_color;
                }

                if (im::text(imgui, titles[i])) {
                    result = !selected ? sid(titles[i]) : 0;
                }

                if (selected) {
                    imgui.theme = theme;
                }
            }
            return result;
        }

        inline bool
        uint_input(
            state_t& imgui, 
            u32* value,
            sid_t txt_id_ = 0,
            bool* toggle_state = 0         
        ) {
            const sid_t txt_id = imgui.verify_id(txt_id_ ? txt_id_ : (u64)value);
            bool result = toggle_state ? *toggle_state : false;

            local_persist char text[64] = {'0'};
            local_persist u64 position = 0;

            constexpr f32 text_pad = 4.0f;
            const auto min_size = font_get_size(imgui.ctx.font, "1000.0f");
            const auto font_size = font_get_size(imgui.ctx.font, imgui.active.id == txt_id ? text : fmt_sv("{}", *value));
            v2f temp_cursor = imgui.panel->draw_cursor;
            const f32 start_x = imgui.panel->draw_cursor.x;
            temp_cursor.x += text_pad + imgui.theme.padding;

            math::rect2d_t text_box;
            text_box.expand(temp_cursor);
            text_box.expand(temp_cursor + min_size);
            text_box.expand(temp_cursor + font_size + v2f{32.0f, 0.0f});
            imgui.panel->update_cursor_max(text_box.max);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (imgui.validate_position(v2f{x,y}) && text_box.contains(v2f{x,y})) {
                imgui.hot = txt_id;
            } else if (imgui.hot.id == txt_id) {
                imgui.hot = 0;
            }

            if (imgui.active.id == txt_id) {
                auto key = imgui.ctx.input->keyboard_input();
                if (key > 0 && key != '\n') {                    
                    text[position++] = key;
                    imgui.hot = txt_id;
                } else if ((position) && imgui.ctx.input->pressed.keys[key_id::BACKSPACE]) {
                    position--;
                    text[position] = '\0';
                    imgui.hot = txt_id;
                }
                if (imgui.ctx.input->pressed.keys[key_id::ENTER]) {
                    *value = (u32)std::atoi(text);
                    imgui.active = 0;
                    result = true;
                }
                if (position != 0) {
                    const auto bar_pad = v2f{4.0f, 0.0f};
                    draw_line(&imgui.ctx, temp_cursor + v2f{font_size.x, 0.0f} + bar_pad, temp_cursor + font_size + bar_pad, 2.0f, imgui.theme.fg_color);
                }
            }
            if (imgui.hot.id == txt_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = txt_id;
                    auto fstr = fmt_str("{}", *value);
                    utl::memzero(text, array_count(text));
                    utl::copy(text, fstr.c_str(), fstr.size());
                    position = fstr.size();
                }
            }
            
            auto shadow_cursor = temp_cursor + imgui.theme.shadow_distance;
            auto bg_color = imgui.theme.bg_color;
            if (imgui.hot.id == txt_id) {
                bg_color = color::lerp(bg_color, color::modulate(imgui.theme.fg_color, 0.3f), sin(imgui.ctx.input->time * 2.0f) * 0.5f + 0.5f);

            }

            text_box.pull(v2f{2.0f});
            draw_round_rect(&imgui.ctx, text_box, 4.0f, imgui.theme.fg_color, 10);
            text_box.pull(v2f{-2.0f});
            draw_round_rect(&imgui.ctx, text_box, 4.0f, bg_color, 10);

            if (imgui.active.id == txt_id) {
                draw_string(&imgui.ctx, text, &shadow_cursor, imgui.theme.shadow_color);
                draw_string(&imgui.ctx, text, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            } else {
                auto sval = fmt_str("{}", *value);
                draw_string(&imgui.ctx, sval, &shadow_cursor, imgui.theme.shadow_color);
                draw_string(&imgui.ctx, sval, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            }

            constexpr b32 show_clear = 1;
            if constexpr (show_clear) {
                auto tr = math::top_right(text_box);
                auto clear_space_top = tr - v2f{16.0f, -2.0f};
                auto clear_space_bottom = text_box.max - v2f{16.0f, 2.0f};
                auto clear_color = color::modulate(imgui.theme.fg_color, 0.5);
                auto cx0 = clear_space_top + v2f{3.0f, 1.0f};
                auto cx1 = cx0 + v2f{10.0f, 0.0f};
                auto cx2 = clear_space_bottom + v2f{3.0f, -1.0f};
                auto cx3 = cx2 + v2f{10.0f, 0.0f};
                draw_line(&imgui.ctx, clear_space_top, clear_space_bottom, 2.0f, clear_color);

                math::rect2d_t clear_box{
                    .min = cx0,
                    .max = cx3
                };

                if (imgui.validate_position(v2f{x,y}) && clear_box.contains(v2f{x,y})) { 
                    clear_color = color::modulate(imgui.theme.active_color, 0.5);
                    if (imgui.ctx.input->mouse.buttons[0] && position) {
                        *value = 0;
                    }
                }

                draw_line(&imgui.ctx, cx0, cx3, 1.0f, clear_color);
                draw_line(&imgui.ctx, cx1, cx2, 1.0f, clear_color);
            }

            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += font_size.x;
            } else {
                imgui.panel->draw_cursor.y = temp_cursor.y - font_size.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->min.x + imgui.theme.padding;
                // imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }
            return result;
        }

        inline bool
        float_input(
            state_t& imgui, 
            f32* value,
            sid_t txt_id_ = 0,
            bool* toggle_state = 0         
        ) {
            const sid_t txt_id = imgui.verify_id(txt_id_ ? txt_id_ : (u64)value);
            bool result = toggle_state ? *toggle_state : false;

            local_persist char text[64] = {'0'};
            local_persist u64 position = 0;

            constexpr f32 text_pad = 4.0f;
            const auto min_size = font_get_size(imgui.ctx.font, "1000.0f");
            const auto font_size = font_get_size(imgui.ctx.font, imgui.active.id == txt_id ? text : fmt_sv("{}", *value));
            v2f temp_cursor = imgui.panel->draw_cursor;
            const f32 start_x = imgui.panel->draw_cursor.x;
            temp_cursor.x += text_pad + imgui.theme.padding;

            math::rect2d_t text_box;
            text_box.expand(temp_cursor);
            text_box.expand(temp_cursor + min_size);
            text_box.expand(temp_cursor + font_size + v2f{32.0f, 0.0f});
            imgui.panel->update_cursor_max(text_box.max);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (imgui.validate_position(v2f{x,y}) && text_box.contains(v2f{x,y})) {
                imgui.hot = txt_id;
            } else if (imgui.hot.id == txt_id) {
                imgui.hot = 0;
            }

            if (imgui.active.id == txt_id) {
                auto key = imgui.ctx.input->keyboard_input();
                if (key > 0 && key != '\n') {                    
                    text[position++] = key;
                    imgui.hot = txt_id;
                } else if ((position) && imgui.ctx.input->pressed.keys[key_id::BACKSPACE]) {
                    position--;
                    text[position] = '\0';
                    imgui.hot = txt_id;
                }
                if (imgui.ctx.input->pressed.keys[key_id::ENTER]) {
                    std::string_view text_view{text};

                    std::from_chars(text_view.data(), text_view.data() + text_view.size(), *value);
                    // *value = (f32)std::atof(text);
                    imgui.active = 0;
                    result = true;
                }
                if (position != 0) {
                    const auto bar_pad = v2f{4.0f, 0.0f};
                    draw_line(&imgui.ctx, temp_cursor + v2f{font_size.x, 0.0f} + bar_pad, temp_cursor + font_size + bar_pad, 2.0f, imgui.theme.fg_color);
                }
            }
            if (imgui.hot.id == txt_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = txt_id;
                    auto fstr = fmt_str("{}", *value);
                    utl::memzero(text, array_count(text));
                    utl::copy(text, fstr.c_str(), fstr.size());
                    position = fstr.size();
                }
            }

            auto shadow_cursor = temp_cursor + imgui.theme.shadow_distance;
            auto bg_color = imgui.theme.bg_color;
            if (imgui.hot.id == txt_id) {
                bg_color = color::lerp(bg_color, color::modulate(imgui.theme.fg_color, 0.3f), sin(imgui.ctx.input->time * 2.0f) * 0.5f + 0.5f);

            }

            text_box.pull(v2f{2.0f});
            draw_round_rect(&imgui.ctx, text_box, 4.0f, imgui.theme.fg_color, 10);
            text_box.pull(v2f{-2.0f});
            draw_round_rect(&imgui.ctx, text_box, 4.0f, bg_color, 10);

            if (imgui.active.id == txt_id) {
                draw_string(&imgui.ctx, text, &shadow_cursor, imgui.theme.shadow_color);
                draw_string(&imgui.ctx, text, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            } else {
                auto sval = fmt_str("{}", *value);
                draw_string(&imgui.ctx, sval, &shadow_cursor, imgui.theme.shadow_color);
                draw_string(&imgui.ctx, sval, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            }
            

            
            constexpr b32 show_clear = 1;
            if constexpr (show_clear) {
                auto tr = math::top_right(text_box);
                auto clear_space_top = tr - v2f{16.0f, -2.0f};
                auto clear_space_bottom = text_box.max - v2f{16.0f, 2.0f};
                auto clear_color = color::modulate(imgui.theme.fg_color, 0.5);
                auto cx0 = clear_space_top + v2f{3.0f, 1.0f};
                auto cx1 = cx0 + v2f{10.0f, 0.0f};
                auto cx2 = clear_space_bottom + v2f{3.0f, -1.0f};
                auto cx3 = cx2 + v2f{10.0f, 0.0f};
                draw_line(&imgui.ctx, clear_space_top, clear_space_bottom, 2.0f, clear_color);

                math::rect2d_t clear_box{
                    .min = cx0,
                    .max = cx3
                };

                if (imgui.validate_position(v2f{x,y}) && clear_box.contains(v2f{x,y})) { 
                    clear_color = color::modulate(imgui.theme.active_color, 0.5);
                    if (imgui.ctx.input->mouse.buttons[0] && position) {
                        *value = 0.0f;
                    }
                }

                draw_line(&imgui.ctx, cx0, cx3, 1.0f, clear_color);
                draw_line(&imgui.ctx, cx1, cx2, 1.0f, clear_color);
            }

            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += text_box.size().x + imgui.theme.padding;
            } else {
                imgui.panel->draw_cursor.y = text_box.max.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->min.x + imgui.theme.padding;
                // imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }
            return result;
        }

        inline bool
        float2_input(
            state_t& imgui, 
            v2f* value,
            sid_t txt_id_ = 0,
            bool* toggle_state = 0         
        ) {
            auto theme = imgui.theme;

            imgui.theme.bg_color = gfx::color::rgba::red;
            same_line(imgui);

            auto r = float_input(imgui, &value->x, txt_id_, toggle_state);

            imgui.theme.bg_color = gfx::color::rgba::green;

            auto g = float_input(imgui, &value->y, txt_id_, toggle_state);

            imgui.theme = theme;

            return r && g;
        }

        inline bool
        float3_input(
            state_t& imgui, 
            v3f* value,
            sid_t txt_id_ = 0,
            bool* toggle_state = 0         
        ) {
            auto theme = imgui.theme;

            imgui.theme.bg_color = gfx::color::rgba::red;
            same_line(imgui);

            auto r = float_input(imgui, &value->x, txt_id_, toggle_state);

            imgui.theme.bg_color = gfx::color::rgba::green;
            same_line(imgui);

            auto g = float_input(imgui, &value->y, txt_id_, toggle_state);

            imgui.theme.bg_color = gfx::color::rgba::light_blue;

            auto b = float_input(imgui, &value->z, txt_id_, toggle_state);

            imgui.theme = theme;

            return r && g && b;
        }

        inline bool
        float4_input(
            state_t& imgui, 
            v4f* value,
            sid_t txt_id_ = 0,
            bool* toggle_state = 0         
        ) {
            auto theme = imgui.theme;

            imgui.theme.bg_color = gfx::color::rgba::red;
            same_line(imgui);

            auto r = float_input(imgui, &value->x, txt_id_, toggle_state);

            imgui.theme.bg_color = gfx::color::rgba::green;
            same_line(imgui);

            auto g = float_input(imgui, &value->y, txt_id_, toggle_state);

            imgui.theme.bg_color = gfx::color::rgba::light_blue;
            same_line(imgui);

            auto b = float_input(imgui, &value->z, txt_id_, toggle_state);

            imgui.theme.bg_color = gfx::color::rgba::purple;

            auto a = float_input(imgui, &value->w, txt_id_, toggle_state);

            imgui.theme = theme;

            return r && g && b;
        }

        inline bool
        text_edit(
            state_t& imgui, 
            std::span<char> text,
            size_t* position,
            sid_t txt_id_ = 0,
            bool* toggle_state = 0         
        ) {
            const sid_t txt_id = imgui.verify_id(txt_id_ ? txt_id_ : sid(text));
            bool result = toggle_state ? *toggle_state : false;

            constexpr f32 text_pad = 4.0f;
            const auto min_size = font_get_size(imgui.ctx.font, "Hello world");
            const auto font_size = font_get_size(imgui.ctx.font, std::string_view{text.data()});
            v2f temp_cursor = imgui.panel->draw_cursor;
            const f32 start_x = imgui.panel->draw_cursor.x;
            temp_cursor.x += text_pad + imgui.theme.padding;

            math::rect2d_t text_box;
            text_box.expand(temp_cursor);
            text_box.expand(temp_cursor + min_size);
            text_box.expand(temp_cursor + font_size + v2f{32.0f, 0.0f});
            imgui.panel->update_cursor_max(text_box.max);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (imgui.validate_position(v2f{x,y}) && text_box.contains(v2f{x,y})) {
                imgui.hot = txt_id;
            } else if (imgui.hot.id == txt_id) {
                imgui.hot = 0;
            }

            if (imgui.active.id == txt_id) {
                // imgui.hot = txt_id;
                local_persist text_input_state_t text_in;
                text_in.data = (char*)text.data();
                text_in.text_size = std::string_view{text_in.data}.size();
                text_in.max_size = text.size();
                text_in.text_position = *position;
                result = gfx::gui::text_box(&imgui.ctx, text_in, text_box, imgui.ctx.font, imgui.theme.text_color, imgui.theme.fg_color, 1);
                if(result) {
                    imgui.active = 0;
                }
                *position = text_in.text_position;
            } else {
                text_input_state_t text_in;
                text_in.data = (char*)text.data();
                text_in.text_size = std::string_view{text_in.data}.size();
                text_in.max_size = text.size();
                text_in.text_position = *position;
                gfx::gui::text_box(&imgui.ctx, text_in, text_box, imgui.ctx.font, imgui.theme.text_color, imgui.theme.fg_color, 0);
                *position = text_in.text_position;
            }
            // if (imgui.active.id == txt_id) {
            //     auto key = imgui.ctx.input->keyboard_input();
            //     if (key > 0) {                    
            //         const_cast<char&>(text[(*position)++]) = key;
            //         imgui.hot = txt_id;
            //     } else if ((*position) && imgui.ctx.input->pressed.keys[key_id::BACKSPACE]) {
            //         (*position)--;
            //         const_cast<char&>(text[(*position)]) = '\0';
            //         imgui.hot = txt_id;
            //     }
            //     if (imgui.ctx.input->pressed.keys[key_id::ENTER]) {
            //         imgui.active = 0;
            //         result = true;
            //     }
            // }
            if (imgui.ctx.input->mouse.buttons[0]) {
                if (imgui.hot.id == txt_id) {
                    imgui.active = txt_id;
                } else {
                    imgui.active = 0;
                }
            }

            // if (*position != 0) {
            //     const auto bar_pad = v2f{4.0f, 0.0f};
            //     draw_line(&imgui.ctx, temp_cursor + v2f{font_size.x, 0.0f} + bar_pad, temp_cursor + font_size + bar_pad, 2.0f, imgui.theme.fg_color);
            // }

            
            auto bg_color = imgui.theme.bg_color;
            if (imgui.hot.id == txt_id) {
                bg_color = color::lerp(bg_color, color::modulate(imgui.theme.fg_color, 0.3f), sin(imgui.ctx.input->time * 2.0f) * 0.5f + 0.5f);
            }
            
            // text_box.pull(v2f{2.0f});
            // draw_round_rect(&imgui.ctx, text_box, 4.0f, imgui.theme.fg_color, 10);
            // text_box.pull(v2f{-2.0f});
            // draw_round_rect(&imgui.ctx, text_box, 4.0f, bg_color, 10);
            
            // auto shadow_cursor = temp_cursor + imgui.theme.shadow_distance;
            // draw_string(&imgui.ctx, text, &shadow_cursor, imgui.theme.shadow_color);
            // draw_string(&imgui.ctx, text, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            temp_cursor.y += min_size.y;
            
            constexpr b32 show_clear = 1;
            if constexpr (show_clear) {
                auto tr = math::top_right(text_box);
                auto clear_space_top = tr - v2f{16.0f, -2.0f};
                auto clear_space_bottom = text_box.max - v2f{16.0f, 2.0f};
                auto clear_color = color::modulate(imgui.theme.fg_color, 0.5);
                auto cx0 = clear_space_top + v2f{3.0f, 1.0f};
                auto cx1 = cx0 + v2f{10.0f, 0.0f};
                auto cx2 = clear_space_bottom + v2f{3.0f, -1.0f};
                auto cx3 = cx2 + v2f{10.0f, 0.0f};
                draw_line(&imgui.ctx, clear_space_top, clear_space_bottom, 2.0f, clear_color);

                math::rect2d_t clear_box{
                    .min = cx0,
                    .max = cx3
                };

                if (imgui.validate_position(v2f{x,y}) && clear_box.contains(v2f{x,y})) { 
                    clear_color = color::modulate(imgui.theme.active_color, 0.5);
                    if (imgui.ctx.input->mouse.buttons[0] && *position) {
                        utl::memzero((void*)text.data(), text.size());
                        *position = 0;
                    }
                }

                draw_line(&imgui.ctx, cx0, cx3, 1.0f, clear_color);
                draw_line(&imgui.ctx, cx1, cx2, 1.0f, clear_color);
            }


            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += font_size.x;
            } else {
                imgui.panel->draw_cursor.y = temp_cursor.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->min.x + imgui.theme.padding;
                // imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }
            return result;
        }

        inline bool
        text_edit2(
            state_t& imgui, 
            std::string_view text,
            size_t* position,
            sid_t txt_id_ = 0,
            bool* toggle_state = 0         
        ) {
            const sid_t txt_id = imgui.verify_id(txt_id_ ? txt_id_ : sid(text));
            bool result = toggle_state ? *toggle_state : false;

            constexpr f32 text_pad = 4.0f;
            const auto min_size = font_get_size(imgui.ctx.font, "Hello world");
            const auto font_size = font_get_size(imgui.ctx.font, text);
            v2f temp_cursor = imgui.panel->draw_cursor;
            const f32 start_x = imgui.panel->draw_cursor.x;
            temp_cursor.x += text_pad + imgui.theme.padding;

            math::rect2d_t text_box;
            text_box.expand(temp_cursor);
            text_box.expand(temp_cursor + min_size);
            text_box.expand(temp_cursor + font_size + v2f{32.0f, 0.0f});
            imgui.panel->update_cursor_max(text_box.max);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (imgui.validate_position(v2f{x,y}) && text_box.contains(v2f{x,y})) {
                imgui.hot = txt_id;
            } else if (imgui.hot.id == txt_id) {
                imgui.hot = 0;
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

            if (*position != 0) {
                const auto bar_pad = v2f{4.0f, 0.0f};
                draw_line(&imgui.ctx, temp_cursor + v2f{font_size.x, 0.0f} + bar_pad, temp_cursor + font_size + bar_pad, 2.0f, imgui.theme.fg_color);
            }

            
            auto bg_color = imgui.theme.bg_color;
            if (imgui.hot.id == txt_id) {
                bg_color = color::lerp(bg_color, color::modulate(imgui.theme.fg_color, 0.3f), sin(imgui.ctx.input->time * 2.0f) * 0.5f + 0.5f);
            }
            
            text_box.pull(v2f{2.0f});
            draw_round_rect(&imgui.ctx, text_box, 4.0f, imgui.theme.fg_color, 10);
            text_box.pull(v2f{-2.0f});
            draw_round_rect(&imgui.ctx, text_box, 4.0f, bg_color, 10);
            
            auto shadow_cursor = temp_cursor + imgui.theme.shadow_distance;
            draw_string(&imgui.ctx, text, &shadow_cursor, imgui.theme.shadow_color);
            draw_string(&imgui.ctx, text, &temp_cursor, imgui.hot.id == txt_id ? imgui.theme.active_color : imgui.theme.text_color);
            
            constexpr b32 show_clear = 1;
            if constexpr (show_clear) {
                auto tr = math::top_right(text_box);
                auto clear_space_top = tr - v2f{16.0f, -2.0f};
                auto clear_space_bottom = text_box.max - v2f{16.0f, 2.0f};
                auto clear_color = color::modulate(imgui.theme.fg_color, 0.5);
                auto cx0 = clear_space_top + v2f{3.0f, 1.0f};
                auto cx1 = cx0 + v2f{10.0f, 0.0f};
                auto cx2 = clear_space_bottom + v2f{3.0f, -1.0f};
                auto cx3 = cx2 + v2f{10.0f, 0.0f};
                draw_line(&imgui.ctx, clear_space_top, clear_space_bottom, 2.0f, clear_color);

                math::rect2d_t clear_box{
                    .min = cx0,
                    .max = cx3
                };

                if (imgui.validate_position(v2f{x,y}) && clear_box.contains(v2f{x,y})) { 
                    clear_color = color::modulate(imgui.theme.active_color, 0.5);
                    if (imgui.ctx.input->mouse.buttons[0] && *position) {
                        utl::memzero((void*)text.data(), text.size());
                        *position = 0;
                    }
                }

                draw_line(&imgui.ctx, cx0, cx3, 1.0f, clear_color);
                draw_line(&imgui.ctx, cx1, cx2, 1.0f, clear_color);
            }


            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += font_size.x;
            } else {
                imgui.panel->draw_cursor.y = temp_cursor.y - font_size.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->min.x + imgui.theme.padding;
                // imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
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
            const u64 hst_id = imgui.verify_id((u64)values);

            math::rect2d_t bg_box;
            const auto start = bg_box.min = imgui.panel->draw_cursor + imgui.theme.padding;
            bg_box.max = start + size;

            const v2f box_size{size.x / (f32)value_count, size.y};

            draw_rect(&imgui.ctx, bg_box, color);

            range_u64(i, 0, value_count) {
                f32 perc = f32(i)/f32(value_count);
                math::rect2d_t box;
                box.min = start + v2f{box_size.x*i, 0.0f};
                box.max = start + v2f{box_size.x*(i+1), size.y*(1.0f-(values[i]/max))};
                draw_rect(&imgui.ctx, box, imgui.theme.fg_color);
            }
            
            if (imgui.next_same_line) {
                imgui.panel->draw_cursor.x = bg_box.max.x + imgui.theme.padding;
                imgui.next_same_line = false;
            } else {
                imgui.panel->draw_cursor.y = bg_box.max.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }
            imgui.panel->update_cursor_max(bg_box.max + v2f{imgui.theme.padding});
        }
        
        void 
        float_edit(
            state_t& imgui,
            f32* val
        ) {
            local_persist char text_buffer[256];
            local_persist size_t write=0;
            u64 id = imgui.verify_id((u64)val);

            if (imgui.active.id != id) {
                utl::memzero(text_buffer, array_count(text_buffer));
                write = 0;
            }

            const auto t = fmt_str("{}", *val);
            utl::copy(text_buffer, t.data(), t.size());

            text_edit(imgui, text_buffer, &write, id);
            
            *val = (f32)std::atof(text_buffer);
        }

        inline void
        checkbox(
            state_t& imgui, 
            bool* var,
            sid_t id = 0,
            v2f size = v2f{16.0f}
        ) {
            const u64 btn_id = imgui.verify_id(id ? id : (u64)var);
            math::rect2d_t box;
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
            box.pad(-imgui.theme.border_thickness);
            draw_circle(&imgui.ctx, box.center(), box.size().x * 0.5f, imgui.hot.id == btn_id ? imgui.theme.active_color : imgui.theme.border_color);
            box.pad(imgui.theme.border_thickness);
            draw_circle(&imgui.ctx, box.center(), box.size().x * 0.5f, *var ? imgui.theme.active_color : imgui.theme.disabled_color);
            // draw_rect(&imgui.ctx, box, *var ? imgui.theme.active_color : imgui.theme.disabled_color);
            
            if (imgui.next_same_line) {
                imgui.panel->draw_cursor.x += size.x + imgui.theme.padding;
                imgui.next_same_line = false;
            } else {
                imgui.panel->draw_cursor.y += size.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }
        }

        inline u64
        enumeration(
            state_t& imgui,
            std::span<std::string_view> names,
            u64 current
        ) {
            auto theme = imgui.theme;
            for (u64 i = 0; i < names.size(); i++){
                if (i == current) {
                    imgui.theme.text_color = imgui.theme.active_color;
                }
                if (text(imgui, names[i])) {
                    imgui.theme.text_color = theme.text_color;
                    return i;
                }
                if (i == current) {
                    imgui.theme.text_color = theme.text_color;
                }
            }
            return current;
        }

        template <typename T>
        inline T
        enumeration(
            state_t& imgui,
            std::span<std::string_view> names,
            T current
        ) {
            return (T)enumeration(imgui, names, (u64)current);
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
            u64 btn_id_ = 0,
            v2f size = v2f{16.0f},
            u8 font_size = 16
        ) {
            const u64 btn_id = imgui.verify_id(btn_id_ ? btn_id_ : sid(name));
            math::rect2d_t box;
            box.min = imgui.panel->draw_cursor;
            box.max = imgui.panel->draw_cursor + size;

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
            box.expand(box.min + text_size + math::width2 * text_size + imgui.theme.padding * 2.0f);

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            if (box.contains( v2f{x,y} )) {
                if (imgui.active.id == 0) {
                    imgui.hot = btn_id;
                }
            } else if (imgui.hot.id == btn_id) {
                imgui.hot = 0;
            }

            imgui.panel->update_cursor_max(box.max + imgui.theme.padding);

            v2f same_line = box.sample(math::half2) - text_size * math::half2;
            // string_render(&imgui.ctx, name, same_line, text_color, imgui.ctx.dyn_font[font_size]);
            // draw_rect(&imgui.ctx, box, imgui.hot.id == btn_id ? imgui.theme.active_color : color);

            // draw_rect(&imgui.ctx, box, imgui.theme.border_color);
            draw_rect(&imgui.ctx, box, imgui.hot.id == btn_id ? imgui.theme.active_color : color);
            // draw_round_rect(&imgui.ctx, box, imgui.theme.border_radius, imgui.theme.border_color, 10);
            // box.pull(v2f{-1.0f});
            // draw_round_rect(&imgui.ctx, box, imgui.theme.border_radius, imgui.hot.id == btn_id ? imgui.theme.active_color : color, 10);

            draw_string(&imgui.ctx, name, same_line, text_color, imgui.ctx.font);
            
            if (imgui.next_same_line) {
                imgui.panel->draw_cursor.x = box.max.x + imgui.theme.padding;
                imgui.next_same_line = false;
            } else {
                imgui.panel->draw_cursor.y = box.max.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->saved_cursor.x;
            }

            return result;
        }
        
        inline bool
        button(
            state_t& imgui, 
            std::string_view name,
            u64 btn_id_ = 0,
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

            same_line(imgui);
            
            imgui.theme.fg_color = gfx::color::rgba::red;
            float_slider(imgui, (f32*)&data.x, min, max);

            same_line(imgui);
            imgui.theme.fg_color = gfx::color::rgba::green;
            float_slider(imgui, (f32*)&data.y, min, max);

            imgui.theme.fg_color = gfx::color::rgba::blue;
            float_slider(imgui, (f32*)&data.z, min, max);

            imgui.theme = theme;
        }

        void color_picker(
            state_t& imgui,
            v4f* color,
            math::rect2d_t rect
        ) {
            local_persist bool color_mode = true;
            
            if (text(imgui, color_mode ? "Box" : "Wheel")) {
                color_mode = !color_mode;
            }
            
            if (color_mode) {
                auto color_uint = gfx::color::to_color32(*color);
                imgui.panel->draw_cursor.x = rect.min.x;
                if (rect.size().x < rect.size().y) {
                    rect.set_size(v2f{rect.size().x});
                } else {
                    rect.set_size(v2f{rect.size().y});
                }
                color_edit(imgui, &color_uint, rect.size());
                *color = gfx::color::to_color4(color_uint);
            } else {
                const auto padding = imgui.theme.padding;
                const auto right_panel_prc = 0.2f;
                const auto right_panel_pxl = rect.size().x * right_panel_prc;

                // rect.add(imgui.panel->draw_cursor);

                auto [main, panel] = math::cut_right(rect, right_panel_pxl);
                main.pull(v2f{-padding});
                panel.pull(v2f{-padding});

                draw_rect(&imgui.ctx, main, imgui.theme.fg_color);
                draw_rect(&imgui.ctx, panel, imgui.theme.fg_color);
                draw_rect(&imgui.ctx, rect, imgui.theme.bg_color);

                imgui.panel->update_cursor_max(rect);
            }
        }

        math::rect2d_t
        subwindow(
            state_t& imgui,
            v2f size,
            color32 color
        ) {
            math::rect2d_t result;

            assert(0);
            return result;
        }

        void 
        gradient_edit(
            state_t& imgui,
            color::gradient_t* gradient,
            arena_t* arena,
            math::rect2d_t rect,
            u32 resolution = 64
        ) {
            const u64 grd_id = imgui.verify_id((u64)gradient);

            if (gradient->is_sorted() == false) {
                auto text_color = imgui.theme.text_color;
                imgui.theme.text_color = gfx::color::rgba::yellow;
                text(imgui, "Warning! Gradient is not sorted!");
                imgui.theme.text_color = text_color;
            }

            v2f tmp_cursor = imgui.panel->draw_cursor;
            tmp_cursor.x += imgui.theme.padding;

            rect.add(tmp_cursor);
            imgui.panel->update_cursor_max(rect);

            auto [gradient_box, controls] = math::cut_top(rect, rect.size().y * 0.3f);

            for (u32 i = 1; i < resolution; i++) {
                f32 prc = f32(i)/f32(resolution);
                f32 next_prc = f32(i-1)/f32(resolution);
                v4f color = gradient->sample(prc);
                v4f next_color = gradient->sample(next_prc);

                math::rect2d_t grad_seg {
                    .min = gradient_box.sample(v2f{prc, 0.0f}),
                    .max = gradient_box.sample(v2f{next_prc, 1.0f})
                };

                color32 rect_colors[4]= {
                    color::to_color32(color),
                    color::to_color32(color),
                    color::to_color32(next_color),
                    color::to_color32(next_color),
                };
                
                draw_rect(&imgui.ctx, grad_seg, std::span{rect_colors});
            }

            range_u32(i, 0, gradient->count) {
                v2f top_position = gradient_box.sample(v2f{gradient->positions[i], 0.0f});
                v2f bottom_position = gradient_box.sample(v2f{gradient->positions[i], 1.0f});

                draw_circle(&imgui.ctx, top_position, 7.0f, color::rgba::black, 3, 1.0f, 0.25f);
                draw_circle(&imgui.ctx, top_position, 5.0f, color::rgba::white, 3, 1.0f, 0.25f);
                draw_circle(&imgui.ctx, bottom_position, 7.0f, color::rgba::black, 3, 1.0f, 0.75f);
                draw_circle(&imgui.ctx, bottom_position, 5.0f, color::rgba::white, 3, 1.0f, 0.75f);
            }

            const auto [x,y] = imgui.ctx.input->mouse.pos;
            const b32 shift_held = imgui.ctx.input->keys[key_id::LEFT_SHIFT] || imgui.ctx.input->keys[key_id::RIGHT_SHIFT];
            const b32 delete_held = imgui.ctx.input->keys[key_id::X];
            const v2f mouse{x,y};

            f32 t = (mouse - gradient_box.min).x / gradient_box.size().x;
            auto nearest_sample_distance = gradient->nearest_distance(t);
            const f32 sample_click_threshold = 0.02f;
            const b32 clicked_sample = nearest_sample_distance < sample_click_threshold;

            if (imgui.active.id == grd_id) {
                
                if (gradient_box.contains(mouse)) {
                    if (clicked_sample) {
                        imgui.selected_index = gradient->closest(t);
                        if (delete_held) {
                            gradient->remove(imgui.selected_index);
                            imgui.selected_index = gradient->count + 1;
                        }
                    }
                    // } else {
                        // imgui.selected_index = gradient->count;
                    if (imgui.selected_index > gradient->count) {
                    } else {
                        imgui.selected_index = gradient->closest(t);
                        gradient->positions[imgui.selected_index] = t;
                        gradient->sort();
                    }
                    
                    if (!clicked_sample) {
                        if (shift_held) {
                            gradient->add(arena, gradient->sample(t), t);
                            gradient->sort();
                            imgui.active.id = 0;
                            imgui.selected_index = gradient->closest(t);
                        }
                    }
                }
                if (!imgui.ctx.input->mouse.buttons[0]) {
                    // imgui.selected_index = gradient->count + 1;
                    imgui.hot = imgui.active = 0;                        
                }
            } else if (imgui.hot.id == grd_id) {
                if (imgui.ctx.input->mouse.buttons[0]) {
                    imgui.active = grd_id;
                } else {
                    imgui.active = 0;
                }
            }
            
            if (rect.contains( v2f{x,y} )) {
                if (imgui.hot.id == 0) {
                    imgui.hot = grd_id;
                }
            } else if (imgui.hot.id == grd_id) {
                imgui.hot = 0;
            }

            auto [controls_left, controls_right] = math::cut_left(controls, controls.size().x * 0.5f);
            imgui.panel->draw_cursor = controls_left.min + imgui.theme.padding;

            if (imgui.selected_index < gradient->count) {
                im::same_line(imgui);
            }

            if (im::text(imgui, "Add Color")) {
                gradient->add(arena, v4f{utl::rng::random_s::randv(), 1.0f}, utl::rng::random_s::randf());
                gradient->sort();
            }

            if (imgui.selected_index < gradient->count) {
                if (im::text(imgui, "Randomize")) {
                    gradient->colors[imgui.selected_index] = v4f{utl::rng::random_s::randv(), 1.0f};
                }
            }

            if (imgui.selected_index < gradient->count) {
                im::same_line(imgui);
                    im::text(imgui, "R");
                    im::float_input(imgui, &gradient->colors[imgui.selected_index].r);
                im::same_line(imgui);
                    im::text(imgui, "G");
                    im::float_input(imgui, &gradient->colors[imgui.selected_index].g);
                im::same_line(imgui);
                    im::text(imgui, "B");
                    im::float_input(imgui, &gradient->colors[imgui.selected_index].b);
                im::same_line(imgui);
                    im::text(imgui, "A");
                    im::float_input(imgui, &gradient->colors[imgui.selected_index].a);
            }

            imgui.panel->draw_cursor = controls_right.min + imgui.theme.padding;

            if (imgui.selected_index < gradient->count) {
                color_picker(imgui, gradient->colors + imgui.selected_index, controls_right);
            }


            if (imgui.next_same_line) {
                imgui.next_same_line = false;
                imgui.panel->draw_cursor.x += rect.size().x + imgui.theme.padding;
            } else {
                imgui.panel->draw_cursor.y = rect.max.y + imgui.theme.padding;
                imgui.panel->draw_cursor.x = imgui.panel->min.x + imgui.theme.padding;
            }
        }

    };
}; // namespace gui


#include "stb/stb_truetype.h"

struct font_t {
    struct glyph_t {
        char c{};
        math::rect2d_t screen{};
        math::rect2d_t texture{};
        int   kern{0};
        int   advance{0};
    };
    f32 size{12.0f};
    f32 draw_scale{};
    u32 pixel_count{512};

    u8* ttf_buffer{0};
    u8* bitmap{0};
    stbtt_bakedchar cdata[96];
    stbtt_fontinfo font_info = {};
    u32 id{0};

    void* user_data = 0;
    
    template<typename T>
    T* get() {
        return (T*)user_data;
    }
};

// todo use stbtt_ScaleForPixelHeight

inline void
font_load(
    arena_t* arena,
    font_t* font,
    std::string_view path,
    float size
) 
#ifdef STB_TRUETYPE_IMPLEMENTATION
{
    font->font_info = {};
    font->size = size;

    // font->bitmap = (u8*)push_bytes(arena, font->pixel_count*font->pixel_count);
    tag_array(font->bitmap, u8, arena, font->pixel_count*font->pixel_count);
    // auto memory = begin_temporary_memory(arena);
    // size_t stack_mark = arena_get_mark(arena);

        tag_array(font->ttf_buffer, u8, arena, 1<<20);
        // font->ttf_buffer = (u8*)push_bytes(arena, 1<<20);
        // font->ttf_buffer = (u8*)push_bytes(memory.arena, 1<<20);
        
        FILE* file = 0;
        fopen_s(&file, path.data(), "rb");

        fread(font->ttf_buffer, 1, 1<<20, file);

        auto succeed = stbtt_InitFont(&font->font_info, font->ttf_buffer, stbtt_GetFontOffsetForIndex(font->ttf_buffer,0));
        assert(succeed);

        stbtt_BakeFontBitmap(font->ttf_buffer, 0, size, font->bitmap, font->pixel_count, font->pixel_count, 32, 96, font->cdata);

        font->draw_scale = stbtt_ScaleForPixelHeight(&font->font_info, size);

        fclose(file);

    // arena_set_mark(arena, stack_mark);
    // end_temporary_memory(memory);
}
#else 
{}
#endif

inline font_t::glyph_t
font_get_glyph(font_t* font, f32 x, f32 y, char c, char nc = 0) 
#ifdef STB_TRUETYPE_IMPLEMENTATION
{
    math::rect2d_t screen{};
    math::rect2d_t texture{};
    
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(font->cdata, font->pixel_count, font->pixel_count, c - 32, &x, &y, &q, 1);

    texture.expand(v2f{q.s0, q.t0});
    texture.expand(v2f{q.s1, q.t1});

    screen.expand(v2f{q.x0, q.y0});
    screen.expand(v2f{q.x1, q.y1});

    font_t::glyph_t glyph{
        c, 
        screen,
        texture,
        nc? stbtt_GetCodepointKernAdvance(&font->font_info, c, nc) : 0
    };
    int lsb;
    stbtt_GetCodepointHMetrics(&font->font_info, c, &glyph.advance, &lsb);
    
    return glyph;
}
#else 
{
    return {};
}
#endif


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

    // for (const char c : text) {
    range_u64(i, 0, text.size()) {
        char c = text[i];
        char nc = ((i + 1) < text.size()) ? text[i+1] : 0;
        if (c == 0) break;
        if (c >= 32 && c < 128) {
            const auto glyph = font_get_glyph(font, cursor.x, cursor.y, c, nc);

            cursor.x += (f32)(glyph.kern + glyph.advance) * font->draw_scale;

            size = glm::max(size, cursor);
        } else if (c == '\n') {
            cursor.y += font_get_glyph(font, cursor.x, cursor.y, '_').screen.size().y + 1;
            cursor.x = start_x;
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
    f32 depth,
    v2f screen_size,
    utl::pool_t<gui::vertex_t>* vertices,
    utl::pool_t<u32>* indices,
    color32 text_color = color::rgba::cream
) {
    assert(screen_size.x && screen_size.y);
    f32 start_x = cursor.x;
    cursor.y += font_get_glyph(font, cursor.x, cursor.y, ';').screen.size().y + 1;

    // for (const char c : text) {
    range_u64(i, 0, text.size()) {
        char c = text[i];
        char nc = ((i + 1) < text.size()) ? text[i+1] : 0;
        if (c == 0) break;
        if (c >= 32 && c < 128) { // this matches stb but is broken for me
            const auto glyph = font_get_glyph(font, cursor.x, cursor.y, c, nc);

            const u32 v_start = safe_truncate_u64(vertices->count());
            gui::vertex_t* v = vertices->allocate(4);            
            u32* tris = indices->allocate(6);            

            const v2f p0 = v2f{glyph.screen.min.x, glyph.screen.min.y};
            const v2f p1 = v2f{glyph.screen.min.x, glyph.screen.max.y};
            const v2f p2 = v2f{glyph.screen.max.x, glyph.screen.min.y};
            const v2f p3 = v2f{glyph.screen.max.x, glyph.screen.max.y};
            
            const v2f c0 = v2f{glyph.texture.min.x, glyph.texture.min.y};
            const v2f c1 = v2f{glyph.texture.min.x, glyph.texture.max.y};
            const v2f c2 = v2f{glyph.texture.max.x, glyph.texture.min.y};
            const v2f c3 = v2f{glyph.texture.max.x, glyph.texture.max.y};

            const u32 font_id = font->id;
            const u32 texture_bit = (u32)BIT(30);

            v[0] = gui::vertex_t{ .pos = v3f(p0, depth), .tex = c0, .img = font_id|texture_bit, .col = text_color};
            v[1] = gui::vertex_t{ .pos = v3f(p1, depth), .tex = c1, .img = font_id|texture_bit, .col = text_color};
            v[2] = gui::vertex_t{ .pos = v3f(p2, depth), .tex = c2, .img = font_id|texture_bit, .col = text_color};
            v[3] = gui::vertex_t{ .pos = v3f(p3, depth), .tex = c3, .img = font_id|texture_bit, .col = text_color};

            tris[0] = v_start + 0;
            tris[1] = v_start + 2;
            tris[2] = v_start + 1;

            tris[3] = v_start + 1;
            tris[4] = v_start + 2;
            tris[5] = v_start + 3;

            cursor.x += (f32)(glyph.kern + glyph.advance) * font->draw_scale;
        } else if (c == '\n') {
            cursor.y += font_get_glyph(font, cursor.x, cursor.y, 'G').screen.size().y + 1;
            cursor.x = start_x;
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

#endif // ZTD_GRAPHICS

namespace utl {

// do not give user input
std::string unsafe_evaluate_command(std::string_view command) {
   char buffer[128];
   std::string result = "";

   // Open pipe to file
   FILE* pipe = _popen(command.data(), "r");
   if (!pipe) {
      return "popen failed!";
   }

   // read till end of process:
   while (!feof(pipe)) {

      // use buffer to read and add to result
      if (fgets(buffer, 128, pipe) != NULL)
         result += buffer;
   }

   _pclose(pipe);
   return result;
}

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
        ztd_profile(name, " {}{}", 
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
static constexpr u64 hash_size = 0x0fff;
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
            T* node = (T*)push_bytes(arena, sizeof(T));
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

struct mod_loader_t {
    arena_t* arena{0};
    void* library{0};

    explicit mod_loader_t() {
        load_module();
    }

    void load_module() {
        library = Platform.load_module();
    }

    void load_library(const char* name) {
        library = Platform.load_library(name);
    }

    void unload_library() {
        Platform.unload_library(library);
    }

    template <typename FunctionType>
    FunctionType load_function(std::string_view name) {
        return Platform.load_function<FunctionType>(library, name.data());
    }

    template <typename FunctionType>
    hash_trie_t<std::string_view, FunctionType>* function_cache() {
        return 0;
    }

    template<typename FunctionType, typename ... Args>
    auto call(hash_trie_t<std::string_view, FunctionType>** cache, std::string_view name, Args ... args) {
        auto* func = hash_get(cache, name, arena);
        if (*func == 0) {
            *func = load_function(name);
            assert(*func);
        } 
        return func(std::forward<Args>(args)...);
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
        : data{push_struct<T>(arena, p_count)}, count{p_count}
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

// todo(zack) fix buffer overflow
struct memory_blob_t {
    std::byte* data{0};

    std::byte* read_data() {
        return data + read_offset;
    }

    std::span<u8> data_view() {
        return std::span{(u8*)data, serialize_offset};
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
    : data{arena->start + arena->top}
    {
    }

    void advance(const size_t bytes) noexcept {
        read_offset += bytes;
    }

    template <typename T>
    void allocate(arena_t* arena) {
        push_struct<T>(arena);
        allocation_offset += sizeof(T);
    }

    void allocate(arena_t* arena, size_t bytes) {
        push_bytes(arena, bytes);
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

        // ztd_info("blob", "data_offset: {}", data_offset);
        
        serialize(arena, data_offset);

        u64 cached_serialize_offset = serialize_offset;

        // serialize_offset += 8;//allocation_offset;

        if (ptr.offset) {
            serialize(arena, *ptr.get());
        }

        // serialize_offset = cached_serialize_offset;
    }

    // If no serialize is provided, fallback to memcopy
    template <typename T>
    void serialize(arena_t* arena, const T& obj) {
        allocate<T>(arena);
        utl::copy(&data[serialize_offset], (std::byte*)&obj, sizeof(T));

        serialize_offset += sizeof(T);
    }

    // template <typename T>
    // void serialize(arena_t* arena, const std::optional<T>& opt) {
    //     serialize(arena, u8{opt?1ui8:0ui8});
    //     if (opt) {
    //         serialize(arena, *opt);
    //     }
    // }

    template <typename T>
    void serialize(arena_t* arena, const std::optional<T>& opt) {
        serialize<u8>(arena, u8(opt?1:0));
        if (opt) {
            serialize(arena, *opt);
        }
    }

    template <typename T>
    void serialize(arena_t* arena, const buffer<T>& str) {
        serialize(arena, str.count);

        allocate(arena, str.count);
        utl::copy(&data[serialize_offset], (std::byte*)str.data, str.count);

        serialize_offset += str.count;
    }

    template <>
    void serialize(arena_t* arena, const string_buffer& str) {
        serialize(arena, str.count);

        allocate(arena, str.count);
        utl::copy(&data[serialize_offset], (std::byte*)str.data, str.count);

        serialize_offset += str.count;
    }

    template <>
    void serialize(arena_t* arena, const std::string_view& obj) {
        serialize(arena, obj.size());

        allocate(arena, obj.size());
        utl::copy(&data[serialize_offset], (std::byte*)obj.data(), obj.size());

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

        // ztd_info("blob", "data_offset: {}", data_offset);
        
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

    template <typename T, umm Size>
    T deserialize() {
        const auto t_offset = read_offset;
        
        advance(sizeof(T));

        T t;
        utl::copy(&t, (T*)(data + t_offset), sizeof(T));

        return t;
    }

    template <typename T>
    T deserialize(arena_t* arena) {
        const auto t_offset = read_offset;
        
        advance(sizeof(T));

        T t{};
        utl::copy(&t, (T*)(data + t_offset), sizeof(T));

        return t;
    }   

    template <typename T>
    T deserialize() {
        const auto t_offset = read_offset;
        
        advance(sizeof(T));

        T t;
        utl::copy(&t, (T*)(data + t_offset), sizeof(T));

        return t;
    }   
    
    template<>
    std::string deserialize() {
        const size_t length = deserialize<u64>();
        std::string value((char*)&data[read_offset], length);
        advance(length);
        return value;
    }

    // template<typename T>
    // buffer<T> deserialize() {
    //     const size_t length = deserialize<u64>();
    //     buffer<T> result = {};
    //     if (length > 0) {
    //         result.data = (char*)&data[read_offset];
    //         result.count = length;
    //     }
    //     advance(length);
    //     return result;
    // }

    // template<typename T>
    // buffer<T> deserialize(arena_t* arena) {
    //     const size_t length = deserialize<u64>();
    //     buffer<T> result = {};
    //     if (length > 0) {
    //         tag_array(result.data, T, arena, length);
    //         utl::copy(result.data, (char*)&data[read_offset], length);
    //         result.count = length;
    //     }
    //     advance(length);
    //     return result;
    // }

    template<>
    string_buffer deserialize() {
        const size_t length = deserialize<u64>();
        string_buffer result = {};
        if (length > 0) {
            result.data = (char*)&data[read_offset];
            result.count = length;
        }
        advance(length);
        return result;
    }

    template<>
    string_buffer deserialize(arena_t* arena) {
        const size_t length = deserialize<u64>();
        string_buffer result = {};
        if (length > 0) {
            tag_array(result.data, char, arena, length);
            result.count = length;
            utl::copy(result.data, (char*)&data[read_offset], length);
        }
        advance(length);
        return result;
    }

    template<>
    string_t deserialize() {
        const size_t length = deserialize<u64>();
        string_t value(std::string_view{(char*)&data[read_offset], length});
        advance(length);
        return value;
    }

    template<typename T>
    std::optional<T> try_deserialize() {
        const u8 has_value = deserialize<u8>();
        if (has_value) {
            std::optional<T> value;
            value.emplace(deserialize<T>());
            return value;
        }
        return std::nullopt;
    }

    template<typename T>
    std::optional<T> try_deserialize(arena_t* arena) {
        const u8 has_value = deserialize<u8>();
        if (has_value) {
            std::optional<T> value;
            value.emplace(deserialize<T>(arena));
            return value;
        }
        return std::nullopt;
    }
};

#define ZYY_SERIALIZE_TYPE_1(type, a) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a);}

#define ZYY_SERIALIZE_TYPE_2(type, a, b) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b);}

#define ZYY_SERIALIZE_TYPE_3(type, a, b, c) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c);}

#define ZYY_SERIALIZE_TYPE_4(type, a, b, c, d) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d);}

#define ZYY_SERIALIZE_TYPE_5(type, a, b, c, d, e) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e);}

#define ZYY_SERIALIZE_TYPE_6(type, a, b, c, d, e, f) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f);}

#define ZYY_SERIALIZE_TYPE_7(type, a, b, c, d, e, f, g) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g);}

#define ZYY_SERIALIZE_TYPE_8(type, a, b, c, d, e, f, g, h) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h);}

#define ZYY_SERIALIZE_TYPE_9(type, a, b, c, d, e, f, g, h, i) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i);}

#define ZYY_SERIALIZE_TYPE_10(type, a, b, c, d, e, f, g, h, i, j) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i); \
    serialize(arena, save_data.##j);}

#define ZYY_SERIALIZE_TYPE_11(type, a, b, c, d, e, f, g, h, i, j, k) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i); \
    serialize(arena, save_data.##j); \
    serialize(arena, save_data.##k);}

#define ZYY_SERIALIZE_TYPE_12(type, a, b, c, d, e, f, g, h, i, j, k, l) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i); \
    serialize(arena, save_data.##j); \
    serialize(arena, save_data.##k); \
    serialize(arena, save_data.##l);}

#define ZYY_SERIALIZE_TYPE_13(type, a, b, c, d, e, f, g, h, i, j, k, l, m) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i); \
    serialize(arena, save_data.##j); \
    serialize(arena, save_data.##k); \
    serialize(arena, save_data.##l); \
    serialize(arena, save_data.##m);}

#define ZYY_SERIALIZE_TYPE_14(type, a, b, c, d, e, f, g, h, i, j, k, l, m, n) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i); \
    serialize(arena, save_data.##j); \
    serialize(arena, save_data.##k); \
    serialize(arena, save_data.##l); \
    serialize(arena, save_data.##m); \
    serialize(arena, save_data.##n);}

#define ZYY_SERIALIZE_TYPE_15(type, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i); \
    serialize(arena, save_data.##j); \
    serialize(arena, save_data.##k); \
    serialize(arena, save_data.##l); \
    serialize(arena, save_data.##m); \
    serialize(arena, save_data.##n); \
    serialize(arena, save_data.##o);}

#define ZYY_SERIALIZE_TYPE_16(type, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i); \
    serialize(arena, save_data.##j); \
    serialize(arena, save_data.##k); \
    serialize(arena, save_data.##l); \
    serialize(arena, save_data.##m); \
    serialize(arena, save_data.##n); \
    serialize(arena, save_data.##o); \
    serialize(arena, save_data.##p);}

#define ZYY_SERIALIZE_TYPE_17(type, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q) \
    template<> void ::utl::memory_blob_t::serialize<type>( \
    arena_t* arena, \
    const type& save_data \
) { \
    serialize(arena, type{}.VERSION); \
    serialize(arena, save_data.##a); \
    serialize(arena, save_data.##b); \
    serialize(arena, save_data.##c); \
    serialize(arena, save_data.##d); \
    serialize(arena, save_data.##e); \
    serialize(arena, save_data.##f); \
    serialize(arena, save_data.##g); \
    serialize(arena, save_data.##h); \
    serialize(arena, save_data.##i); \
    serialize(arena, save_data.##j); \
    serialize(arena, save_data.##k); \
    serialize(arena, save_data.##l); \
    serialize(arena, save_data.##m); \
    serialize(arena, save_data.##n); \
    serialize(arena, save_data.##o); \
    serialize(arena, save_data.##p); \
    serialize(arena, save_data.##q);}

namespace res {

namespace magic {

constexpr auto make_magic(const char key[8]) {
    u64 res=0;
    for(size_t i=0;i<8;i++) { res = (res<<8) | key[i];}
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
    constexpr inline static u64 invalid = ~0ui64;

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
    std::string_view path
) {
    std::ifstream file{path.data(), std::ios::binary};

    if(!file.is_open()) {
        ztd_error("res", "Failed to open file");
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

    tag_struct(pack_file_t* packed_file, pack_file_t, arena);

    packed_file->file_count = loader.deserialize<u64>();
    packed_file->resource_size = loader.deserialize<u64>();

    const auto table_start = loader.deserialize<u64>();
    assert(table_start == magic::table_start);

    // need to load strings into string_t
    tag_array(packed_file->table, resource_table_entry_t, arena, packed_file->file_count);
    size_t table_size = sizeof(resource_table_entry_t) * packed_file->file_count;
    loader.deserialize<u64>();
    for (size_t i = 0; i < packed_file->file_count; i++) {
        resource_table_entry_t* entry = packed_file->table + i;
        std::string file_name = loader.deserialize<std::string>();
        entry->name.own(arena, file_name.c_str());
        entry->file_type = loader.deserialize<u64>();
        entry->size = loader.deserialize<u64>();
    }
    
    using resource_data_t = std::byte;
    tag_array(packed_file->resources, resource_t, arena, packed_file->file_count);
    loader.deserialize<u64>();
    for (size_t i = 0; i < packed_file->file_count; i++) {
        size_t resource_size = packed_file->resources[i].size = loader.deserialize<u64>();
        tag_array(packed_file->resources[i].data, resource_data_t, arena, packed_file->resources[i].size);
        utl::copy(packed_file->resources[i].data, loader.read_data(), resource_size);
        loader.advance(resource_size);
    }

    // ztd_info("res", "Loaded Resource File: {}", path);

    return packed_file;
}

void pack_file_print(
    pack_file_t* packed_file
) {
    ztd_info("res", "Asset File contains: {} files", packed_file->file_count);
    for (size_t i = 0; i < packed_file->file_count; i++) {
        ztd_info("res", "\tfile: {} - {}", packed_file->table[i].name.c_str(), packed_file->table[i].size);
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
    ztd_warn("pack_file", "Failed to find file: {}", file_name);
    return pack_file_t::invalid;
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
    ztd_warn("pack_file", "Failed to find file: {}", file_name);
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
    if (pack_file_t::invalid == file_id) {
        return nullptr;
    }
    return pack_file->resources[file_id].data;
}

}; // namespace res 

std::string open_file(std::string_view name) {
    return (std::stringstream{}<<std::ifstream{name.data()}.rdbuf()).str();
}

struct config_t {
    char name[64];
    char value[256];

    void set_name(auto x) {
        auto v=fmt::format("{}", x);
        assert(v.size() < array_count(name));
        utl::copy(name, v.data(), v.size());
    }

    void set(auto x) {
        auto v=fmt::format("{}", x);
        assert(v.size() < array_count(value));
        utl::copy(value, v.data(), v.size());
    }

    f32 as_float() const {
        return (f32)std::atof(value);
    }

    i32 as_int() const {
        return (i32)std::atoi(value);
    }

    std::string_view as_sv() const {
        return std::string_view{value};
    }

    void print() const {
        fmt::print("{}={}\n", name, value);
    }

    std::string str() const {
        return fmt::format("{}={}\n", name, value);
    }
};

struct config_list_t {
    config_t* head{0};
    size_t count{0};
};

bool
read_config(
    arena_t* arena, 
    std::string_view text, 
    config_t** config, 
    size_t* count
) {
    bool success = true;
    u64 start=0;
    u64 end=text.find('\n');

    config_t* c;
    *count = 0;
    *config = nullptr;

    while (end != std::string_view::npos) {
        if (start == std::string_view::npos) break;
        // ztd_info(__FUNCTION__, "Start: {}, End: {}", start, end);
        if (end == start) {
        // if (end == start || text[start] == '#') {
            // ztd_info(__FUNCTION__, "Empty Line");
            start = end + 1;
            end = text.find('\n', start);
            if (end == std::string_view::npos) {
                end = text.size();
            }
            continue;
        }

        std::string_view line{text.substr(start, end-start)};

        u64 equal_pos = line.find('=');

        if (equal_pos != std::string_view::npos) {
            std::string_view name{line.substr(0, equal_pos)};
            std::string_view value{line.substr(equal_pos+1, line.size()-equal_pos+1)};
            assert(name.size() < array_count(c->name));
            assert(value.size() < array_count(c->value));

            c = push_struct<config_t>(arena);
            *config = *config?*config:c;
            
            utl::memzero(c, sizeof(config_t));

            ztd_info(__FUNCTION__, "{}: {}", name, value);            
            utl::copy(c->name, name.data(), name.size());
            utl::copy(c->value, value.data(), value.size());
            *count += 1;
        } else {
            success = false;
            ztd_error(__FUNCTION__, "Parse Error: {}", line);            
            break;
        }

        start = end + 1;
        end = text.find('\n', start);
    }

    return success;
}

config_t* config_find(config_list_t* list, std::string_view name) {
    for (config_t* c=list->head; c < list->head+list->count; c++) {
        if (name == c->name) {
            return c;
        }
    }
    return nullptr;
}

i32 config_get_int(config_list_t* list, std::string_view name, i32 default_value = 0) {
    if (config_t* c = config_find(list, name); c) {
        return c->as_int();
    }
    return default_value;
}

config_t* load_config(arena_t* arena, const char* file_name, size_t* count) {
    std::ifstream file{file_name};
    
    if(!file.is_open()) {
        ztd_error(__FUNCTION__, "Failed to config file: {}", file_name);
        return 0;
    }
    auto text = (std::stringstream{} << file.rdbuf()).str();

    config_t* config{0};
    read_config(arena, text, &config, count);
    return config;
}


string_buffer read_text_file(arena_t* arena, std::string_view filename) {
    string_buffer result = {};
    
    std::ifstream file{filename.data()};

    file.seekg(0, std::ios::end);
    const size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    using text_file_data_t = char;
    tag_array(auto* bytes, text_file_data_t, arena, file_size);
    file.read(bytes, file_size);

    result.data = bytes;
    result.count = file_size;

    return result;
}

buffer<u8> read_bin_file(arena_t* arena, std::string_view filename) {
    buffer<u8> result = {};
    
    std::ifstream file{filename.data(), std::ios::binary};

    if (file.is_open() == false) {
        return result;
    }

    file.seekg(0, std::ios::end);
    const size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    using bin_file_data_t = u8;
    tag_array(auto* bytes, bin_file_data_t, arena, file_size);
    file.read((char*)bytes, file_size);

    result.data = bytes;
    result.count = file_size;

    return result;
}

void write_binary_file(std::string_view filename, std::span<u8> bytes) {
    std::ofstream file{filename.data(), std::ios::binary};

    file.write((char*)bytes.data(), bytes.size());
}

}; // namespace utl


namespace tween {

template <typename T>
inline T lerp(T a, T b, f32 t) {
    return b * t + a * (1.0f - t);
}
template <typename T>
inline T lerp_dt(T a, T b, f32 s, f32 dt) {
    return tween::lerp(a, b, 1.0f - std::pow(s, dt));
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
constexpr f32 
in_out_expo(f32 x) noexcept {
    return x == 0.0f
        ? 0.0f
        : x == 1.0f
        ? 1.0f
        : x < 0.5f ? glm::pow(2.0f, 20.0f * x - 10.0f) * 0.5f
        : (2.0f - glm::pow(2.0f, -20.0f * x + 10.0f)) * 0.5f;
}

constexpr f32 
out_bounce(f32 x) noexcept {
    const f32 n1 = 7.5625f;
    const f32 d1 = 2.75f;

    if (x < 1.0f / d1) {
        return n1 * x * x;
    } else if (x < 2.0f / d1) {
        return n1 * (x -= 1.5f / d1) * x + 0.75f;
    } else if (x < 2.5f / d1) {
        return n1 * (x -= 2.25f / d1) * x + 0.9375f;
    } else {
        return n1 * (x -= 2.625f / d1) * x + 0.984375f;
    }
}



template <typename T>
auto generic(auto&& fn) {
    return [=](T a, T b, f32 x) {
        return a + (b-a) * fn(x);
    };
}

auto over_time(auto&& fn, f32 dt) {
    return [=](auto a, auto b, f32 x) {
        return fn(a, b, 1.0f - std::pow(x, dt));
    };
}

};


// template<>
// buffer<auto>
// utl::memory_blob_t::deserialize<buffer<auto>>(arena_t* arena) {
//     buffer<auto> buffer{};

//     buffer.count = deserialize<u64>();
        
//     if (buffer.count > 0) {
//         buffer.push(arena, std::span{(char*)&data[read_offset], buffer.count});
//     }
//     advance(buffer.count);

//     return buffer;
// }

#if defined(ZTD_GRAPHICS)
template<>
void utl::memory_blob_t::serialize<gfx::color::gradient_t>(arena_t* arena, const gfx::color::gradient_t& gradient) {
    serialize<u64>(arena, 0);
    serialize(arena, gradient.count);
    for(u32 i = 0; i < gradient.count; i++) {
        serialize(arena, gradient.colors[i]);
        serialize(arena, gradient.positions[i]);
    }
}

template<>
gfx::color::gradient_t
utl::memory_blob_t::deserialize<gfx::color::gradient_t>(arena_t* arena) {
    gfx::color::gradient_t gradient = {};
    u64 VERSION = deserialize<u64>();

    u32 count = deserialize<u32>();
    for(u32 i = 0; i < count; i++) {
        v4f color = deserialize<v4f>();
        f32 time = deserialize<f32>();
        gradient.add(arena, color, time);
    }

    return gradient;
}

#endif