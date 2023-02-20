#pragma once

#include "core.hpp"

namespace game::item {
    struct effect_t;
};

namespace game::wep {

    enum struct weapon_type {
        PISTOL, RIFLE,
        SHOTGUN, ROCKET,
        SIZE
    };

    struct ammo_clip_t {
        u32 current{};
        u32 max{};
    };

    struct weapon_stats_t {
        f32 damage{};
        f32 pen{};
    };

    enum struct bullet_type {
        PELLET, LASER, ROCKET, GRENADE, SIZE
    };

    struct bullet_t {
        // math::ray_t ray{};
        f32 damage{};

        item::effect_t* effects{};

        bullet_type type{bullet_type::SIZE};
    };

    struct weapon_action_t {
        f32 fire_time{};
        f32 chamber_time{};
        f32 reload_time{};
    };

    struct base_weapon_t {
        f32 fire_rate{};
        f32 load_speed{};
        f32 chamber_speed{};

        u32 chamber_count{};
        u32 chamber_max{};
        u32 chamber_mult{};

        weapon_stats_t stats{};
        ammo_clip_t clip{};
        weapon_action_t action{};

        item::effect_t* effects{};

        utl::closure_t* on_hit{};
        u64             on_hit_count{};

        bool chamber_round(f32 dt) {
            action.chamber_time += dt;
            if (action.chamber_time > chamber_speed) {
                action.chamber_time = 0.0f;
                clip.current -= chamber_count = chamber_max;
                return true;
            }
            return false;
        }

        bool reload(f32 dt) {
            action.reload_time += dt;
            if (action.reload_time > load_speed) {
                clip.current = clip.max;
                return true;
            }
            return false;
        }

        bullet_t* fire(arena_t* arena, f32 dt, u64* count) {
            if (chamber_count == 0) {
                if (clip.current > 0) {
                    if (!chamber_round(dt)) { return nullptr; }
                } else {
                    if (!reload(dt)) { return nullptr; }
                }
                // return nullptr;
            }

            action.fire_time += dt;
            bullet_t* bullets{0};

            while(action.fire_time > fire_rate) {
                action.fire_time -= fire_rate;
                chamber_count -= 1;

                for (size_t bullet = 0; bullet < chamber_mult; bullet++) {
                    (*count)++;
                    bullet_t* this_bullet = arena_alloc_ctor<bullet_t>(arena);
                    if (bullets == nullptr) {
                        bullets = this_bullet;
                    }
                    this_bullet->damage = stats.damage;
                    this_bullet->effects = effects;
                }
            }

            return bullets;
        }
    };

    constexpr inline base_weapon_t 
    create_pistol() noexcept {
        base_weapon_t pistol{
            .fire_rate = 0.7f,
            .load_speed = 0.05f,
            .chamber_speed = 0.05f,
            .chamber_count = 1,
            .chamber_max = 1,
            .chamber_mult = 1,
            .stats = wep::weapon_stats_t {
                .damage = 3.0f,
                .pen = 2.0f
            },
            .clip = wep::ammo_clip_t {
                .current = 18,
                .max = 18
            },
        };
        return pistol;
    }

    constexpr inline base_weapon_t 
    create_rifle() noexcept {
        base_weapon_t rifle{
            .fire_rate = 1.0f,
            .load_speed = 0.1f,
            .chamber_speed = 0.1f,
            .chamber_count = 1,
            .chamber_max = 1,
            .chamber_mult = 1,
            .stats = wep::weapon_stats_t {
                .damage = 1.0f,
                .pen = 1.0f
            },
            .clip = wep::ammo_clip_t {
                .current = 30,
                .max = 30
            }
        };
        return rifle;
    }

    constexpr inline base_weapon_t 
    create_shotgun() noexcept {
        base_weapon_t shotgun{
            .fire_rate = 1.0f,
            .load_speed = 0.1f,
            .chamber_speed = 0.1f,
            .chamber_count = 1,
            .chamber_max = 1,
            .chamber_mult = 8,
            .stats = wep::weapon_stats_t {
                .damage = 1.0f,
                .pen = 1.0f
            },
            .clip = wep::ammo_clip_t {
                .current = 16,
                .max = 16
            }
        };
        return shotgun;
    }

};
