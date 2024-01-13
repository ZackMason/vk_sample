#pragma once

#include "ztd_core.hpp"

enum struct game_screen_type : u32 {
    intro,
    main_menu,
    boss_battle,
    invalid,
};

struct game_screen_t {
    game_screen_type type{game_screen_type::invalid};
};
