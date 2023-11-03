#pragma once

#include "base_item.hpp"

using lightning_cache_t = utl::hash_trie_t<zyy::entity_t*, b32>;

export_fn(void) lightning_on_hit_effect(
    zyy::world_t* w, 
    zyy::item::effect_t* effect,
    zyy::entity_t* self_entity,
    zyy::entity_t* hit_entity,
    v3f p, v3f n
);
