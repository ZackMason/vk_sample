#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "zyy_core.hpp"

#include "khr/spirv_reflect.h"
#include "khr/spirv_reflect.c"

platform_api_t Platform;


global_variable f32 bloom_filter_radius = 0.0025f;


#include "App/game_state.cpp"
#include "App/vk_state.cpp"

#include "App/bullet.cpp"
#include "App/zyy_lightning_effect.cpp"
#include "App/zyy_weapon_common.cpp"

#if ZYY_INTERNAL
debug_table_t gs_debug_table;
size_t gs_main_debug_record_size = __COUNTER__;
#endif