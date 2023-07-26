#ifndef APP_HPP
#define APP_HPP

#include "App/vk_state.hpp"
#include "App/gfx_pipelines.hpp"

#include "App/Game/Util/camera.hpp"

#include "App/Game/Rendering/render_system.hpp"

#include "App/Game/GUI/viewport.hpp"
#include "App/Game/GUI/debug_state.hpp"


namespace game {
    struct world_t;
};

struct player_controller_t {
    v3f move_input;
    v2f look_input;

    bool fire1;
    f32 iron_sight;

    bool jump;
    bool sprint;
};

inline static player_controller_t 
gamepad_controller(app_input_t* input) {
    player_controller_t pc{};

    pc.move_input = v3f{
        input->gamepads[0].left_stick.x,
        f32(input->gamepads[0].buttons[button_id::shoulder_left].is_held) - 
        f32(input->gamepads[0].buttons[button_id::shoulder_right].is_held),
        -input->gamepads[0].left_stick.y,
    };
    
    pc.look_input = v2f{
        input->gamepads[0].right_stick.x,
        input->gamepads[0].right_stick.y,
    } * 2.0f;

    pc.fire1 = input->gamepads->right_trigger > 0.0f;
    pc.iron_sight = input->gamepads->left_trigger > 0.0f;
    pc.jump = input->gamepads->buttons[button_id::action_down].is_held;
    pc.sprint = input->gamepads->buttons[button_id::action_right].is_held;
    return pc;
}

inline static player_controller_t 
keyboard_controller(app_input_t* input) {
    player_controller_t pc{};
    pc.move_input = v3f{
        f32(input->keys['D']) - f32(input->keys['A']),
        f32(input->keys['Q']) - f32(input->keys['E']),
        f32(input->keys['W']) - f32(input->keys['S'])
    };

    const f32 CAMERA_TURN_SPEED = 0.1f;

    pc.look_input = (CAMERA_TURN_SPEED * (f32(input->mouse.buttons[0]) * v2f{
        input->mouse.delta[0], 
        input->mouse.delta[1] 
    }) + v2f {
        f32(input->keys['L']) - f32(input->keys['J']),
        f32(input->keys['I']) - f32(input->keys['K'])
    });

    pc.fire1 = input->keys['F'] || input->mouse.buttons[0];
    pc.iron_sight = input->mouse.buttons[1];
    pc.jump = input->pressed.keys[key_id::SPACE];
    pc.sprint = input->keys[key_id::LEFT_SHIFT];
    return pc;
}

struct debug_console_t;

struct game_state_t {
    game_memory_t*       game_memory{nullptr};

    // arenas
    arena_t&            main_arena;
    arena_t             temp_arena;
    arena_t             string_arena;
    arena_t             mesh_arena;
    arena_t             texture_arena;
    arena_t             game_arena;

    debug_state_t* debug_state{0};

    utl::res::pack_file_t *     resource_file{0};

    gfx::vul::state_t   gfx;
    rendering::system_t* render_system{0};

    // todo(zack) clean this up
    gfx::font_t default_font;
    gfx::font_t large_font;
    gfx::vul::texture_2d_t* default_font_texture{nullptr};
    VkDescriptorSet default_font_descriptor;

    struct scene_t {
        gfx::vul::sporadic_buffer_t sporadic_buffer;

        struct lighting_t {
            v4f directional_light{1.0f, 2.0f, 3.0f, 1.0f};
        } lighting;
    } scene;
    
    gfx::vul::pipeline_state_t* gui_pipeline{0};
    gfx::vul::pipeline_state_t* debug_pipeline{0};
    // gfx::vul::pipeline_state_t* mesh_pipeline{0};
    gfx::vul::pipeline_state_t* sky_pipeline{0};

    struct gui_state_t {
        gfx::gui::ctx_t ctx;
        gfx::vul::vertex_buffer_t<gfx::gui::vertex_t, 4'000'000> vertices[2];
        gfx::vul::index_buffer_t<6'000'000> indices[2];
        std::atomic<u64> frame{0};

        arena_t  arena;
    } gui;

    struct debugging_t {
        debug_console_t* console;

        bool show = false;

        gfx::vul::vertex_buffer_t<gfx::vul::debug_line_vertex_t, 100'000> debug_vertices;

        void draw_aabb(const math::aabb_t<v3f>& aabb, gfx::color3 color) {
            const v3f size = aabb.size();
            const v3f sxxx = v3f{size.x,0,0};
            const v3f syyy = v3f{0,size.y,0};
            const v3f szzz = v3f{0,0,size.z};
            
            draw_line(aabb.min, aabb.min + sxxx, color);
            draw_line(aabb.min, aabb.min + syyy, color);
            draw_line(aabb.min, aabb.min + szzz, color);

            draw_line(aabb.min + sxxx + syyy, aabb.min + sxxx, color);
            draw_line(aabb.min + sxxx + syyy, aabb.min + syyy, color);
            draw_line(aabb.max - sxxx - syyy, aabb.max - sxxx, color);
            draw_line(aabb.max - sxxx - syyy, aabb.max - syyy, color);
            
            draw_line(aabb.min + sxxx, aabb.min + szzz + sxxx, color);
            draw_line(aabb.min + syyy, aabb.min + szzz + syyy, color);
            
            draw_line(aabb.max, aabb.max - sxxx, color);
            draw_line(aabb.max, aabb.max - syyy, color);
            draw_line(aabb.max, aabb.max - szzz, color);
        }

        void draw_line(v3f a, v3f b, gfx::color3 color) {
            auto* points = debug_vertices.pool.allocate(2);
            points[0].pos = a;
            points[1].pos = b;
            points[0].col = color;
            points[1].col = color;
        }
    } debug;

    game::world_t* game_world{0};

    f32 time_scale = 1.0f;
    f32 time_text_anim = 0.0f;

    i32 width() {
        return game_memory->config.window_size[0];
    }
    i32 height() {
        return game_memory->config.window_size[1];
    }

    app_input_t& input() {
        return game_memory->input;
    }
};

#endif