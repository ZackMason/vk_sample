#pragma once

#include "base_item.hpp"

using lightning_cache_t = utl::hash_trie_t<zyy::entity_t*, b32>;

void lightning_on_hit_effect_impl(
    zyy::world_t* w, 
    zyy::item::effect_t* effect,
    zyy::entity_t* entity,
    v3f p, v3f n,
    lightning_cache_t* cache = 0
);

void lightning_on_hit_effect(
    zyy::world_t* w, 
    zyy::item::effect_t* effect,
    zyy::entity_t* entity,
    v3f p, v3f n
) {
    TIMED_FUNCTION;
    lightning_on_hit_effect_impl(
        w, 
        effect,
        entity,
        p, n,
        0
    );
}