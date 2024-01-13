#pragma once

#include "base_item.hpp"

using lightning_cache_t = utl::hash_trie_t<ztd::entity_t*, b32>;

export_fn(void) lightning_on_hit_effect(
    ztd::world_t* w, 
    ztd::item::effect_t* effect,
    ztd::entity_t* self_entity,
    ztd::entity_t* hit_entity,
    v3f p, v3f n
);
