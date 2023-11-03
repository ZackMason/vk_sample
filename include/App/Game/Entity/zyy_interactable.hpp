#pragma once

#include "zyy_core.hpp"

namespace zyy {
    struct entity_t;

    struct interactable_t;

    struct interactable_state_t {
        b32 available{0};
    };

    using interact_function = interactable_state_t(*)(interactable_t*, entity_t*);

    struct interactable_t {
        interactable_t* next;
        entity_t* owner;
        const char* menu_text;
        interact_function function;
    };

    interactable_state_t
    open_interaction(interactable_t* interactable, entity_t* entity) {
        interactable_state_t result{};
        result.available = 0;
        return result;
    }

    interactable_state_t
    exit_interaction(interactable_t* interactable, entity_t* entity) {
        interactable_state_t result{};
        result.available = 1;
        return result;
    }

    export_fn(interactable_t*) 
    npc_interactions(arena_t* arena, entity_t* entity) {
        tag_struct(auto* open, interactable_t, arena);
            open->menu_text = "open";
            open->owner = entity;
            open->function = open_interaction;

        tag_struct(auto* exit, interactable_t, arena);
            exit->menu_text = "exit";
            exit->owner = entity;
            exit->function = exit_interaction;

        open->next = exit;
        exit->next = nullptr;

        return open;
    }
}
