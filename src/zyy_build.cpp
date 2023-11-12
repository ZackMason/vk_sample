#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION


#include "zyy_core.hpp"
#include "khr/spirv_reflect.h"
#include "khr/spirv_reflect.c"

#include "App/Game/GUI/debug_state.hpp"

global_variable f32 bloom_filter_radius = 0.0025f;


// #undef zyy_warn
// #undef zyy_info
// #undef zyy_error

// template <typename ... Args>
// auto zyy_info(std::string_view cat, std::string_view str, Args&& ... args) {
//     console_log(DEBUG_STATE.console, fmt_sv(fmt_sv("[info][{}]: {}\n", cat, str), args...), gfx::color::rgba::white);
// }

// template <typename ... Args>
// auto zyy_warn(std::string_view cat, std::string_view str, Args&& ... args) {
//     console_log(DEBUG_STATE.console, fmt_sv(fmt_sv("[warn][{}]: {}\n", cat, str), args...), gfx::color::rgba::yellow);
// }

// template <typename ... Args>
// auto zyy_error(std::string_view cat, std::string_view str, Args&& ... args) {
//     console_log(DEBUG_STATE.console, fmt_sv(fmt_sv("[error][{}]: {}\n", cat, str), args...), gfx::color::rgba::red);
// }


#include "App/game_state.hpp"


platform_api_t Platform;




#include "App/game_state.cpp"
#include "App/vk_state.cpp"

#include "App/bullet.cpp"
#include "App/zyy_lightning_effect.cpp"
#include "App/Game/Items/explosion_effect.hpp"
#include "App/zyy_weapon_common.cpp"

#if ZYY_INTERNAL
debug_table_t gs_debug_table;
size_t gs_main_debug_record_size = __COUNTER__;
#endif