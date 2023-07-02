#pragma once

#include "core.hpp"
#include "Room/room.hpp"

namespace game {

    struct room_t {
        const room_prefab_t* prefab{0};
        entity_t* entities{0};
    };

    static constexpr size_t GAME_MAX_LEVEL_DIM = 16;
    struct level_t {
        using rooms_t = room_t[GAME_MAX_LEVEL_DIM][GAME_MAX_LEVEL_DIM];
        rooms_t tiles;
    };

};