#pragma once

#include "core.hpp"

namespace game::level {

#define GAME_MAX_LEVEL_DIM 16
    
    struct level_tile_t {
        u64 id{};
    };

    struct level_t {
        using tiles_t = level_tile_t[GAME_MAX_LEVEL_DIM][GAME_MAX_LEVEL_DIM];
        tiles_t tiles;

    };

};