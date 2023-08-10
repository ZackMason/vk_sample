#pragma once

#include "zyy_core.hpp"
#include "uid.hpp"

DEFINE_TYPED_ID(brain_id);

enum struct brain_type {
    player,
    flyer,
    chaser,
    shooter,

    invalid
};

struct blackboard_t {
    brain_id owner{uid::invalid_id};

    v3f interest_points[16];
    f32 last_seen{};
};

struct brain_t {
    brain_t* next;
    brain_t* prev;

    brain_id    id{uid::invalid_id};
    brain_type  type{brain_type::invalid};

    u64 group{};
    u64 ignore_mask{};

    f32 sight{10.0f};
    f32 cos_fov{1.0};
};
