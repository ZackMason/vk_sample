#ifndef GUI_VIEWPORT_HPP
#define GUI_VIEWPORT_HPP

#include "zyy_core.hpp"

struct viewport_t {
    f32 h_split[2]{1.0f,0.0f};
    f32 v_split[2]{1.f,0.0f};

    u32 images[4];

};

inline static void
draw_viewport(gfx::gui::im::state_t& imgui, viewport_t* viewport) {
    const auto ss = imgui.ctx.screen_size; 
    auto& h_split = viewport->h_split;
    auto& v_split = viewport->v_split;
    math::rect2d_t screens[4]{
        math::rect2d_t{v2f{0.0f}, v2f{h_split[0], v_split[0]} * ss},
        math::rect2d_t{v2f{h_split[0], 0.0f} * ss, v2f{1.0f, v_split[1]} * ss},
        math::rect2d_t{v2f{0.0f, v_split[0]} * ss, v2f{h_split[1], 1.0f} * ss},
        math::rect2d_t{v2f{h_split[1], v_split[1]} * ss, ss},
    };
    range_u64(i, 0, 4) {
        gfx::gui::im::image(imgui, viewport->images[i], screens[i]);
    }
}






#endif