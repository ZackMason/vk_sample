#pragma once

#include "zyy_core.hpp"

namespace zyy {
    struct entity_t;

    struct interactable_t;

    using interact_function = b32(*)(interactable_t*, entity_t*);

    struct interactable_t {
        const char* menu_text;
        interact_function function;
    };

    b32
    exit_interaction(interactable_t* interactable, entity_t* entity) {
        return false;
    }

}
