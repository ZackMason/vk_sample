#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "zyy_core.hpp"

#include "khr/spirv_reflect.h"
#include "khr/spirv_reflect.c"

platform_api_t Platform;


#include "App/bullet.cpp"
#include "App/game_state.cpp"
#include "App/vk_state.cpp"

#if ZYY_INTERNAL
debug_table_t gs_debug_table;
size_t gs_main_debug_record_size = __COUNTER__;
#endif