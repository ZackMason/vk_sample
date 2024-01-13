#pragma once

#include "ztd_core.hpp"

namespace ztd {

    struct stealth_t {
        f32 cover{0.0f};
        b32 sneaking{0};
    };

    struct lock_picking_t {
        f32 strength{50.0f};
    };

    struct hacking_t {
        u32 level{0};
    };

    struct perception_t {
        f32 strength{50.0f}; // 0 blind, 100 godlike 
    };

    struct health_t {
        f32 max{0.0f};
        f32 current{0.0f};

        bool damage(f32 x) {
            if (max > 0.0f) {
                current -= x;
                return current <= 0.0f;
            } else {
                return false;
            }
        }

        constexpr health_t(f32 v = 0.0f) noexcept
            : max{v}, current{v} {}
    };

    struct movement_t {
        f32 move_speed{1.0f};
    };
    
    struct character_stats_t {
        health_t health{};
        movement_t movement{};
        perception_t perception{};
    };
}