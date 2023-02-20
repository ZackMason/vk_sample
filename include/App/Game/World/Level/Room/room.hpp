#pragma once

#include "core.hpp"

namespace game::level::room {

    enum cardinal_t {
        North, South,
        East,  West,
        NorthEast, SouthEast,
        NorthWest, SouthWest,
        Size
    };

    struct room_t : public node_t<room_t> {
        string_t name;

        u64 mesh_id{};

        math::aabb_t<v3f> aabb{v3f{0.0f}};

        struct doorway_t {
            v3f offset{};
            cardinal_t direction = cardinal_t::Size;
            room_t* room{0};
        };
        
        doorway_t   doorways[4];
        size_t      doorway_count{0};
    };

    inline void
    connect_rooms(room_t* a, room_t* b) {
        assert(a && "Room A is Null");
        assert(b && "Room B is Null");

        
    }

};