#ifndef APP_HPP
#define APP_HPP

#include "App/vk_state.hpp"
#include "App/gfx_pipelines.hpp"

#include "App/Game/Util/camera.hpp"

#include "App/Game/Rendering/render_system.hpp"

#include "App/Game/GUI/viewport.hpp"
#include "App/Game/GUI/debug_state.hpp"

namespace fullscreen_mode { enum : u8 {
    window, fullscreen, borderless
};}

namespace vsync_mode { enum : u8 {
    off = 0, on = 1,
};}

namespace ddgi_mode { enum : u8 {
    off = 0, realtime = 1, baked = 2,
};}

struct game_graphics_config_t {
    u64 VERSION{0};

    u16 width;
    u16 height;

    u32 fps_max;
    f32 gamma;

    f32 fov;
    f32 scale;

    u8 fullscreen;
    u8 vsync;
    u8 aa_mode;
    
    // advanced
    u8 ddgi = ddgi_mode::realtime;
    u8 lighting;
};

namespace zyy {
    struct world_t;
};

struct player_controller_t {
    v3f move_input;
    v2f look_input;

    b32 fire1;
    f32 iron_sight;

    b32 jump;
    b32 sprint;
    b32 swap;
    b32 emote;
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
    pc.swap =  input->gamepads->buttons[button_id::action_left].is_held;
    pc.emote = input->gamepads->buttons[button_id::dpad_down].is_pressed;
    return pc;
}

struct input_mapping_t {
    i32 look_button{0};
    i32 fire_button{0};
    i32 aim_button{0};

    u16 move_forward{'W'};
    u16 move_backward{'S'};
    u16 move_left{'A'};
    u16 move_right{'D'};

    u16 jump{key_id::SPACE};
    u16 sprint{key_id::LEFT_SHIFT};
    u16 swap{'X'};
    u16 swap2{key_id::TAB};
    u16 emote{'G'};
};

inline static player_controller_t
keyboard_controller(app_input_t* input, input_mapping_t mapping = input_mapping_t{.look_button=0}) {
    player_controller_t pc{};
    pc.move_input = v3f{
        f32(input->keys['D']) - f32(input->keys['A']),
        // 0.0f,
        f32(input->keys['Q']) - f32(input->keys['E']),
        f32(input->keys['W']) - f32(input->keys['S'])
    };

    const f32 CAMERA_TURN_SPEED = 0.1f;
    f32 look = mapping.look_button > -1 ? f32(input->mouse.buttons[mapping.look_button]) : 1.0f;

    pc.look_input = (CAMERA_TURN_SPEED * (look * v2f{
        input->mouse.delta[0], 
        input->mouse.delta[1] 
    }) + v2f {
        f32(input->keys['L']) - f32(input->keys['J']),
        f32(input->keys['I']) - f32(input->keys['K'])
    } * 1.50f);

    pc.fire1 = input->keys['F'] || input->mouse.buttons[mapping.fire_button];
    pc.iron_sight = input->mouse.buttons[mapping.aim_button];
    pc.jump = input->keys[mapping.jump];
    pc.sprint = input->keys[mapping.sprint];
    pc.swap = input->pressed.keys[mapping.swap] || input->pressed.keys[mapping.swap2];
    pc.emote = input->pressed.keys[mapping.emote];
    return pc;
}

struct debug_console_t;

struct player_game_save_data_t {
    u64 VERSION{0};
    string_buffer name;

    u64 money{0};
};

struct player_save_file_header_t {
    u64 VERSION{0};
    game_graphics_config_t graphics_config;
    player_game_save_data_t player_data;
};


template<>
void
utl::memory_blob_t::serialize<player_game_save_data_t>(
    arena_t* arena, 
    const player_game_save_data_t& save_data
) {
    serialize(arena, save_data.VERSION);
    serialize(arena, save_data.name);
    serialize(arena, save_data.money);
}

template<>
player_game_save_data_t
utl::memory_blob_t::deserialize<player_game_save_data_t>(arena_t* arena) {
    player_game_save_data_t save_data{};

    save_data.VERSION = deserialize<u64>();

    save_data.name = deserialize<string_buffer>();

    save_data.money = deserialize<u64>();

    return save_data;
}

struct game_state_t {
    game_memory_t*       game_memory{nullptr};

    // arenas
    arena_t&            main_arena;
    arena_t             temp_arena;
    arena_t             string_arena;
    arena_t             mesh_arena;
    arena_t             texture_arena;

    debug_state_t* debug_state{0};

    game_graphics_config_t graphics_config{};

    struct modding_t {
        utl::mod_loader_t  loader{};

        utl::hash_trie_t<std::string_view, physics::rigidbody_on_collision_function>* collisions = 0;
    } modding;

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

    } scene;
    
    struct gui_state_t {
        gfx::gui::ctx_t ctx;
        gfx::vul::vertex_buffer_t<gfx::gui::vertex_t, 4'000'000> vertices[2];
        gfx::vul::index_buffer_t<6'000'000> indices[2];
        std::atomic<u64> frame{0};

        arena_t  arena;

        gfx::gui::im::state_t imgui;
        explicit gui_state_t() : imgui {
            .ctx = ctx,
            .theme = gfx::gui::theme_t {
                .fg_color = gfx::color::rgba::dark_red,
                .bg_color = gfx::color::rgba::black,
                .text_color = gfx::color::rgba::cream,
                .disabled_color = gfx::color::rgba::dark_gray,
                .border_color = gfx::color::rgba::white,
                .shadow_color = gfx::color::rgba::dark_red,

                .padding = 4.0f,
                .margin = 8.0f
            },
        }{}
    } gui{};

    struct debugging_t {
        bool show = false;
    } debug;

    zyy::world_t* game_world{0};

    f32 time_scale = 1.0f;
    f32 time_text_anim = 0.0f;
    f32 time = 0.0f;

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
