#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "core.hpp"

#include "khr/spirv_reflect.h"
#include "khr/spirv_reflect.c"

#include "App/app.cpp"
#include "App/vk_state.cpp"

#if GEN_INTERNAL
debug_table_t gs_debug_table;
size_t gs_main_debug_record_size = __COUNTER__;
#endif