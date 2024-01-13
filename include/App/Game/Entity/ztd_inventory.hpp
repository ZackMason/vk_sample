#pragma once

#include "ztd_core.hpp"

namespace ztd {
    
struct inventory_entry_t {
    struct entity_t* entity = 0;
    uid::id_type     id{uid::invalid_id};
};

struct inventory_t {
    buffer<inventory_entry_t> items = {};

    void init(arena_t* arena, u64 size) {
        items.reserve(arena, size);
    }

    b32 has() const {
        return items.count > 0;
    }

    b32 add(struct entity_t* item) {
        for (auto& [entity, id]: items.view()) {
            if (entity == nullptr) {
                entity = item;
                // id     = item->id;
                return 1;
            }
        }
        return 0;
    }
};

}
