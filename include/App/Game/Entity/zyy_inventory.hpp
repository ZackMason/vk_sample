#pragma once

#include "zyy_core.hpp"

namespace zyy {

    struct item_ref_t {     
        void* entity{0};
    };

    inline static constexpr u32 max_dimension = 8;
    struct inventory_t {
        u32 max_count{16};
        u32 top{0};
        std::array<item_ref_t, max_dimension*max_dimension> items{};

        void add_item(void* item) {
            assert(top < max_count);
            items[top++] = {item};
        }
    };

};
