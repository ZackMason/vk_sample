#pragma once

#include "App/Game/World/world.hpp"

#define WORLD_GEN_FUNCTION(name) struct name

enum world_generation_step_type {
    invalid,
    player, 
    bads, 
    boss, 
    items, 
    misc, 
    environment,
    count
};

struct world_generation_step_t {
    world_generation_step_t* next{0};
    world_generation_step_t* prev{0};

    std::string_view name;
    world_generation_step_type type{invalid};

    using world_generation_function = void(*)(zyy::world_t*);
    world_generation_function function{0};
};

struct world_generator_t {
    world_generator_t* next{0};
    world_generator_t* prev{0};

    arena_t* arena{0};

    u64 completed_count{0};
    u64 step_count{0};
    world_generation_step_t* first_step{0};

    void execute(zyy::world_t* world, std::function<void(void)> draw_gui = [](){}) {
        TIMED_FUNCTION;
        auto* step = first_step;
        while(step) {
            {
                // std::lock_guard lock{world->render_system()->ticket};
                // rendering::begin_frame(world->render_system());
            }
            // draw_gui();
            step->function(world);
            completed_count++;

            node_next(step);
        }
        if (next) {
            next->execute(world);
        }
    }

    b32 is_done() const {
        return completed_count == step_count && (next?next->is_done():true);
    }

    void add_step_to_last(world_generation_step_t* step) {
        if (first_step == nullptr) {
            first_step = step;
            step_count++;   
            return;
        }
        auto* node = first_step;
        while(node->next) {
            node_next(node);
        }
        step->prev = node;
        node->next = step;
        step_count++;   
    }

    world_generator_t& add_step(
        std::string_view step_name, 
        world_generation_step_type step_type,
        void(*step_fn)(zyy::world_t*)
    );
};

inline static void
world_generator_add_step(
    world_generator_t* generator,
    std::string_view step_name,
    world_generation_step_type step_type,
    void(*step_fn)(zyy::world_t*)
) {
    tag_struct(auto* step, world_generation_step_t, generator->arena);
    step->name = step_name;
    step->type = step_type;
    step->function = step_fn;

    generator->add_step_to_last(step);
}

world_generator_t& world_generator_t::add_step(
    std::string_view step_name, 
    world_generation_step_type step_type,
    void(*step_fn)(zyy::world_t*)
) {
    world_generator_add_step(this, step_name, step_type, step_fn);

    return *this;
}

#define WORLD_GEN_INIT inline static world_generation_step_t steps[] = 
#define WORLD_STEP_LAMBDA [](zyy::world_t* world)
#define WORLD_STEP_TYPE_LAMBDA(type) world_generation_step_type::type, WORLD_STEP_LAMBDA
#define WORLD_GENERATION_STEP(step_name, fn) world_generation_step_t{.name = step_name, .function = WORLD_STEP_LAMBDA fn }



