#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "core.hpp"

#include "App/app.cpp"
#include "App/vk_state.cpp"

debug_table_t gs_debug_table;
size_t gs_main_debug_record_size = __COUNTER__;
