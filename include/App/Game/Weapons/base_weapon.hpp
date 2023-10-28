#pragma once

#include "zyy_core.hpp"


#include "bullet.hpp"


namespace zyy::item {
    struct effect_t;
};

namespace zyy::wep {
  

    struct ammo_mag_t {
        u32 current{};
        u32 max{};
    };

    struct weapon_stats_t {
        f32 damage{};
        f32 pen{};
        f32 spread{};
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
        ammo_mag_t mag{};
        weapon_action_t action{};

        item::effect_t* effects{};
        spawn_bullet_function bullet_fn{spawn_bullet};

        struct sound_effects_t {
            u64 reload{0x2};
            u64 fire{0x3};
        } sound_effects;

        bool chamber_round(f32 dt) {
            if (action.chamber_time == 0.0f) {
            }
            action.chamber_time += dt;
            if (action.chamber_time > chamber_speed) {
                // Platform.audio.play_sound(sound_effects.reload);
                action.chamber_time = 0.0f;
                mag.current -= chamber_max;
                chamber_count += chamber_max;
                return true;
            }
            return false;
        }

        bool reload(f32 dt) {
            action.reload_time += dt;
            if (action.reload_time > load_speed) {
                action.reload_time = 0.0f;
                mag.current = mag.max;
                // Platform.audio.play_sound(sound_effects.reload);
                return true;
            }
            return false;
        }

        void update(f32 dt) {
            if (chamber_count == 0) {
                if (mag.current > 0) {
                    chamber_round(dt);
                } else {
                    reload(dt);
                }
            } else if (action.fire_time > 0.0f) {
                action.fire_time -= dt;
            }
        }

        bullet_t* fire(arena_t* arena, f32 dt, u64* count) {
            if (chamber_count == 0) {
                return nullptr;
            }

            bullet_t* bullets{0};

            while(action.fire_time <= 0.0f) {
                action.fire_time += fire_rate;
                if (chamber_count == 0) {
                    break;
                }
                chamber_count -= 1;
                if (Platform.audio.play_sound) {
                    // Platform.audio.play_sound(sound_effects.fire);
                }

                for (size_t bullet = 0; bullet < chamber_mult; bullet++) {
                    (*count)++;
                    // Todo(Zack): tag this
                    bullet_t* this_bullet = push_struct<bullet_t>(arena);
                    if (bullets == nullptr) {
                        bullets = this_bullet;
                    }
                    this_bullet->damage = stats.damage;
                    this_bullet->effects = effects;
                    this_bullet->spread = stats.spread;
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
            .mag = wep::ammo_mag_t {
                .current = 18,
                .max = 18
            },
        };
        return pistol;
    }

    constexpr inline base_weapon_t 
    create_rpg() noexcept {
        base_weapon_t rpg{
            .fire_rate = 1.0f,
            .load_speed = 1.0f,
            .chamber_speed = 0.5f,
            .chamber_count = 1,
            .chamber_max = 1,
            .chamber_mult = 1,
            .stats = wep::weapon_stats_t {
                .damage = 10.0f,
                .pen = 1.0f
            },
            .mag = wep::ammo_mag_t {
                .current = 4,
                .max = 4
            },
            .bullet_fn = spawn_rocket,
        };
        return rpg;
    }

    constexpr inline base_weapon_t 
    create_rifle() noexcept {
        base_weapon_t rifle{
            .fire_rate = 0.1f,
            .load_speed = 1.0f,
            .chamber_speed = 0.05f,
            .chamber_count = 1,
            .chamber_max = 1,
            .chamber_mult = 1,
            .stats = wep::weapon_stats_t {
                .damage = 1.0f,
                .pen = 1.0f
            },
            .mag = wep::ammo_mag_t {
                .current = 30,
                .max = 30
            },
        };
        return rifle;
    }

    constexpr inline base_weapon_t 
    create_knife() noexcept {
        base_weapon_t rifle{
            .fire_rate = 0.3f,
            .load_speed = 1.0f,
            .chamber_speed = 0.05f,
            .chamber_count = 1,
            .chamber_max = 1,
            .chamber_mult = 1,
            .stats = wep::weapon_stats_t {
                .damage = 4.0f,
                .pen = 3.0f
            },
            .mag = wep::ammo_mag_t {
                .current = 8,
                .max = 8
            },
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
                .pen = 1.0f,
                .spread = 1.0f
            },
            .mag = wep::ammo_mag_t {
                .current = 16,
                .max = 16
            }
        };
        return shotgun;
    }

};
