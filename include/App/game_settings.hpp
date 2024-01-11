#pragma once

#include "zyy_core.hpp"

namespace fullscreen_mode { enum : u8 {
    window, fullscreen, borderless
};}

namespace vsync_mode { enum : u8 {
    off = 0, on = 1,
};}

namespace ddgi_mode { enum : u8 {
    off = 0, realtime = 1, baked = 2,
};}

struct game_graphics_config_t {
    u64 VERSION{0};

    u16 width;
    u16 height;

    u32 fps_max;
    f32 gamma;

    f32 fov;
    f32 scale;

    u8 fullscreen;
    u8 vsync;
    u8 aa_mode;
    
    // advanced
    u8 ddgi = ddgi_mode::realtime;
    u8 lighting;
};

