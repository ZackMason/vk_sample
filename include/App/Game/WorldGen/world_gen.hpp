#pragma once

#include "App/Game/World/world.hpp"

#define WORLD_GEN_FUNCTION(name) struct name


struct world_generation_step_t {
    std::string_view name;

    using world_generation_function = void(*)(game::world_t*);
    world_generation_function function{0};
};

#define WORLD_GEN_INIT inline static world_generation_step_t steps[] = 
#define WORLD_GENERATION_STEP(step_name, fn) world_generation_step_t{.name = step_name, .function = [](game::world_t* world) fn }

template <typename World>
void load_world(game_state_t* game_state) {
    game_state->loading_steps = World::steps;
    game_state->steps_finished = 0;
    game_state->loading_step_count = array_count(World::steps);

    for (auto& step: World::steps) {
        {
            std::lock_guard lock{game_state->render_system->ticket};
            rendering::begin_frame(game_state->render_system);
        }
        step.function(game_state->game_world);
        game_state->steps_finished += 1;
    }

    game_state->steps_finished = game_state->loading_step_count = 0;
    game_state->loading_steps = 0;
}

