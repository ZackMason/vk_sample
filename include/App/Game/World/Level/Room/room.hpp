#pragma once

#include "zyy_core.hpp"

#include "App/Game/Entity/entity_db.hpp"

static constexpr u32 ROOM_MAX_SPAWNABLE_ENEMIES = 32;
struct room_asset_pack_t {
    const zyy::db::prefab_t* spawnable_enemy[ROOM_MAX_SPAWNABLE_ENEMIES];
    const zyy::db::prefab_t* find_enemy(std::string_view name) const {
        return find_prefab(std::span<const zyy::db::prefab_t* const>(spawnable_enemy), name);
    }
    const zyy::db::prefab_t* find_prefab(std::span<const zyy::db::prefab_t* const> list, std::string_view name) const {
        for (const auto* prefab: list) {
            if (prefab->type_name == name) {
                return prefab;
            }
        }
        return nullptr;
    }

    


};
