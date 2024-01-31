#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#define ZTD_GRAPHICS

#include "ztd_core.hpp"
#include "khr/spirv_reflect.h"


namespace ztd {

struct program_context_t {
    utl::allocator_t allocator{};
};

global_variable program_context_t global_context{};

}

#include "App/Game/GUI/debug_state.hpp"

global_variable f32 bloom_filter_radius = 0.0025f;


// #undef ztd_info
// #undef ztd_warn
// #undef ztd_error

// template <typename ... Args>
// auto ztd_info(std::string_view cat, std::string_view str, Args&& ... args) {
//     console_log(DEBUG_STATE.console, fmt_sv(fmt_sv("[info][{}]: {}\n", cat, str), std::forward<Args>(args)...), gfx::color::rgba::white);
// }

// template <typename ... Args>
// auto ztd_warn(std::string_view cat, std::string_view str, Args&& ... args) {
//     console_log(DEBUG_STATE.console, fmt_sv(fmt_sv("[warn][{}]: {}\n", cat, str), std::forward<Args>(args)...), gfx::color::rgba::yellow);
// }

// template <typename ... Args>
// auto ztd_error(std::string_view cat, std::string_view str, Args&& ... args) {
//     console_log(DEBUG_STATE.console, fmt_sv(fmt_sv("[error][{}]: {}\n", cat, str), std::forward<Args>(args)...), gfx::color::rgba::red);
// }
#include "App/lua_modding.cpp"


#include "App/game_state.hpp"


platform_api_t Platform;


#include "App/game_state.cpp"
#include "App/vk_state.cpp"

#include "App/bullet.cpp"
#include "App/ztd_lightning_effect.cpp"
#include "App/Game/Items/explosion_effect.hpp"
#include "App/ztd_weapon_common.cpp"

#include "App/Game/Entity/ztd_physics_callbacks.hpp"

#if ZTD_INTERNAL
debug_table_t gs_debug_table;
size_t gs_main_debug_record_size = __COUNTER__;
#endif