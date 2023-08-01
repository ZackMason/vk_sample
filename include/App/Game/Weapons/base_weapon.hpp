#pragma once

#include "core.hpp"

namespace game::item {
    struct effect_t;
};

namespace game::wep {

    enum struct weapon_part_type {
        HANDLE, BODY, SIGHT,
        TRIGGER, CHAMBER, MAGAZINE,
        SIZE
    };

    constexpr const char* to_string(weapon_part_type type) noexcept {
        switch(type){ 
            case weapon_part_type::BODY: return "weapon_part_type::BODY";
            case weapon_part_type::HANDLE: return "weapon_part_type::HANDLE";
            case weapon_part_type::SIGHT: return "weapon_part_type::SIGHT";
            case weapon_part_type::TRIGGER: return "weapon_part_type::TRIGGER";
            case weapon_part_type::CHAMBER: return "weapon_part_type::CHAMBER";
            case weapon_part_type::MAGAZINE: return "weapon_part_type::MAGAZINE";
        }
        return "<Unknown>";
    }

    struct weapon_part_t;

    #define MAX_WEAPON_SLOTS 5
    struct weapon_slot_t {
        weapon_part_t*      part;
        weapon_part_type    type;
    };

    struct weapon_part_t {
        string_t            name;
        string_t            desc;
        weapon_part_type    type;
        item::effect_t*     effects{};

        weapon_slot_t   slots[MAX_WEAPON_SLOTS];
        size_t          num_slots;
    };

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
        f32 spread{};
    };

    enum struct bullet_type {
        PELLET, LASER, ROCKET, GRENADE, SIZE
    };

    struct bullet_t {
        // math::ray_t ray{};
        f32 damage{};
        f32 spread{};

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

        struct sound_effects_t {
            u64 reload{0x2};
            u64 fire{0x3};
        } sound_effects;

        bool chamber_round(f32 dt) {
            if (action.chamber_time == 0.0f) {
            }
            action.chamber_time += dt;
            if (action.chamber_time > chamber_speed) {
                Platform.audio.play_sound(sound_effects.reload);
                action.chamber_time = 0.0f;
                clip.current -= chamber_max;
                chamber_count += chamber_max;
                return true;
            }
            return false;
        }

        bool reload(f32 dt) {
            action.reload_time += dt;
            if (action.reload_time > load_speed) {
                action.reload_time = 0.0f;
                clip.current = clip.max;
                Platform.audio.play_sound(sound_effects.reload);
                return true;
            }
            return false;
        }

        void update(f32 dt) {
            if (chamber_count == 0) {
                if (clip.current > 0) {
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
                chamber_count -= 1;
                if (Platform.audio.play_sound) {
                    Platform.audio.play_sound(sound_effects.fire);
                }

                for (size_t bullet = 0; bullet < chamber_mult; bullet++) {
                    (*count)++;
                    bullet_t* this_bullet = arena_alloc_ctor<bullet_t>(arena);
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
                .pen = 1.0f,
                .spread = 1.0f
            },
            .clip = wep::ammo_clip_t {
                .current = 16,
                .max = 16
            }
        };
        return shotgun;
    }

};
