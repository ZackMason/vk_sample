#pragma once

#include "core.hpp"

namespace game {
    using game::db::prefab_t;

    enum struct cardinal {
        North, South,
        East,  West,
        NorthEast, SouthEast,
        NorthWest, SouthWest,
        Size
    };

    struct room_prefab_t {
        std::string_view name;

        const prefab_t* room_prefab;
        const prefab_t* prefabs[128];

        math::aabb_t<v3f> aabb{v3f{0.0f}};

        virtual void init(world_t* world) = 0;
        virtual void update(world_t* world, f32 dt) {}
    };
};

namespace game::room_prefabs {
    struct room_01_p : room_prefab_t {
        void init(world_t* world) override {};
    } room_01;
};
