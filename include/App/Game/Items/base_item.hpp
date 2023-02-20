#pragma once


#include "core.hpp"

#include "App/Game/Weapons/base_weapon.hpp"

namespace game::item {

    namespace effect_type {
        enum {
            FIRE    = BIT(0),
            ICE     = BIT(1),
            SHOCK   = BIT(2),
            POISON  = BIT(3),
            BLEED   = BIT(4),
            EXPLOSION= BIT(5),
        };
    };

    struct effect_t {
        u64 type{};
        f32 rate{};
        f32 strength{};

        effect_t* next{};
    };

    namespace augment_character {
        using function_type = game::entity::character_stats_t(*)(const game::entity::character_stats_t&);

        #define AUGMENT_STATS_DEF(x) inline game::entity::character_stats_t \
        x(\
            const game::entity::character_stats_t& _stats\
        ) noexcept 
        
        AUGMENT_STATS_DEF(vitality) {
            auto stats = _stats;
            stats.health.max *= 2.0f;
            return stats;
        }
        
        AUGMENT_STATS_DEF(super_speed) {
            auto stats = _stats;
            stats.movement.move_speed *= 2.0f;
            return stats;
        }

    };

    namespace augment_weapon {
        using function_type = wep::base_weapon_t(*)(const wep::base_weapon_t&);

        #define AUGMENT_WEP_DEF(x) inline wep::base_weapon_t \
        x(\
            const wep::base_weapon_t& _weapon\
        ) noexcept 
        
        AUGMENT_WEP_DEF(double_damage) {
            auto weapon = _weapon;
            weapon.stats.damage *= 2.0f;
            return weapon;
        }

        AUGMENT_WEP_DEF(hair_trigger) {
            auto weapon = _weapon;
            weapon.fire_rate *= 0.5f;
            return weapon;
        }
    };
};