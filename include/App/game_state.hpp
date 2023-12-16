#ifndef APP_HPP
#define APP_HPP

#include <zyy_core.hpp>
#include "App/vk_state.hpp"
#include "App/gfx_pipelines.hpp"

#include "App/Game/Util/camera.hpp"

#include "App/Game/Rendering/render_system.hpp"

#include "App/Game/GUI/viewport.hpp"
#include "App/Game/GUI/debug_state.hpp"
#include "skeleton.hpp"

struct loaded_skeletal_mesh_t {
    string_buffer name;
    gfx::anim::skeleton_t skeleton;
    buffer<gfx::anim::animation_t> animations;
    gfx::mesh_list_t mesh;
};

// todo remove this, this will crash if we reload and its not reset
global_variable utl::pool_t<gfx::skinned_vertex_t>* gs_skinned_vertices;

template<>
loaded_skeletal_mesh_t
utl::memory_blob_t::deserialize<loaded_skeletal_mesh_t>(arena_t* arena) {
    loaded_skeletal_mesh_t result = {};

    // read file meta data
    const auto meta = deserialize<u64>();
    const auto vers = deserialize<u64>();
    const auto mesh = deserialize<u64>();

    assert(meta == utl::res::magic::meta);
    assert(vers == utl::res::magic::vers);
    assert(mesh == utl::res::magic::skel);

    result.mesh.count = deserialize<u64>();
    zyy_warn(__FUNCTION__, "Loading {} meshes", result.mesh.count);
    tag_array(result.mesh.meshes, gfx::mesh_view_t, arena, result.mesh.count);

    u64 total_vertex_count = 0;
    utl::pool_t<gfx::skinned_vertex_t>& vertices{*gs_skinned_vertices};

    for (size_t j = 0; j < result.mesh.count; j++) {
        std::string name = deserialize<std::string>();
        zyy_info(__FUNCTION__, "Mesh name: {}", name);
        const size_t vertex_count = deserialize<u64>();
        const size_t vertex_bytes = sizeof(gfx::skinned_vertex_t) * vertex_count;

        total_vertex_count += vertex_count;

        const u32 vertex_start = safe_truncate_u64(vertices.count());
        const u32 index_start = 0;

        result.mesh.meshes[j].vertex_count = safe_truncate_u64(vertex_count);
        result.mesh.meshes[j].vertex_start = vertex_start;
        result.mesh.meshes[j].index_start = index_start;

        gfx::skinned_vertex_t* v = vertices.allocate(vertex_count);

        utl::copy(v, read_data(), vertex_bytes);
        advance(vertex_bytes);

        // no indices for skeletal meshes atm
        // const size_t index_count = blob.deserialize<u64>();
        // const size_t index_bytes = sizeof(u32) * index_count;
        // results.meshes[j].index_count = safe_truncate_u64(index_count);

        // u32* tris = indices->allocate(index_count);
        // utl::copy(tris, blob.read_data(), index_bytes);
        // blob.advance(index_bytes);

        result.mesh.meshes[j].aabb = {};
        range_u64(k, 0, vertex_count) {
            result.mesh.meshes[j].aabb.expand(v[k].pos);
        }
    }

    result.mesh.aabb = {};
    range_u64(m, 0, result.mesh.count) {
        result.mesh.aabb.expand(result.mesh.meshes[m].aabb);
    }

    const auto anim = deserialize<u64>();
    assert(anim == utl::res::magic::anim);

    const auto animation_count = deserialize<u64>();
    const auto total_anim_size = deserialize<u64>();

    auto* animations = (gfx::anim::animation_t*)read_data();
    advance(total_anim_size);

    const auto skeleton_size = deserialize<u64>();
    assert(skeleton_size == sizeof(gfx::anim::skeleton_t));
    auto* skeleton = (gfx::anim::skeleton_t*)read_data();
    advance(skeleton_size);

    range_u64(a, 0, animation_count) {
        auto& animation = animations[a];
        animation.optimize();
        // zyy_info(__FUNCTION__, "Animation: {}, size: {}", animation.name, sizeof(gfx::anim::animation_t));
        // range_u64(n, 0, animation.node_count) {
        //     auto& node = animation.nodes[n];
        //     auto& timeline = node.bone;
        //     // if (timeline) {
        //     //     zyy_info(__FUNCTION__, "Bone: {}, id: {}", timeline->name, timeline->id);
        //     // }
        // }
    }
    // range_u64(b, 0, skeleton->bone_count) {
    //     auto& bone = skeleton->bones[b];
    //     zyy_info(__FUNCTION__, "Bone: {}, parent: {}", bone.name_hash, bone.parent);
    // }

    result.animations.data = animations;
    result.animations.count = animation_count;

    result.skeleton = *skeleton;
    
    return result;
}

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
    b32 interact;

    b32 inv1;
    b32 inv2;
    b32 inv3;
    b32 inv4;
};

inline static player_controller_t 
gamepad_controller(app_input_t* input) {
    player_controller_t pc = {};

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
    i32 aim_button{1};

    u16 move_forward{'W'};
    u16 move_backward{'S'};
    u16 move_left{'A'};
    u16 move_right{'D'};

    u16 jump{key_id::SPACE};
    u16 sprint{key_id::LEFT_SHIFT};
    u16 interact{'E'};
    u16 swap{'X'};
    u16 swap2{key_id::TAB};
    u16 emote{'G'};

    u16 inv1{'1'};
    u16 inv2{'2'};
    u16 inv3{'3'};
    u16 inv4{'4'};
};

inline static player_controller_t
keyboard_controller(app_input_t* input, input_mapping_t mapping = input_mapping_t{.look_button=0}) {
    player_controller_t pc = {};
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
    pc.interact = input->pressed.keys[mapping.interact];

    pc.inv1 = input->pressed.keys[mapping.inv1];
    pc.inv2 = input->pressed.keys[mapping.inv2];
    pc.inv3 = input->pressed.keys[mapping.inv3];
    pc.inv4 = input->pressed.keys[mapping.inv4];

    return pc;
}

struct debug_console_t;

struct minion_profile_t {
    u64 VERSION{0};

    string_buffer name = {};

    f32 max_health;
};

struct player_profile_t {
    u64 VERSION{0};
    string_buffer name;

    u64 money{0};
    buffer<minion_profile_t> minions = {};
};

struct settings_save_file_header_t {
    u64 VERSION{0};
    game_graphics_config_t graphics_config;
    player_profile_t player_data;
};

template<>
void
utl::memory_blob_t::serialize<player_profile_t>(
    arena_t* arena, 
    const player_profile_t& save_data
) {
    serialize(arena, save_data.VERSION);
    serialize(arena, save_data.name);
    serialize(arena, save_data.money);
}

template<>
player_profile_t
utl::memory_blob_t::deserialize<player_profile_t>(arena_t* arena) {
    player_profile_t save_data{};

    save_data.VERSION = deserialize<u64>();

    save_data.name = deserialize<string_buffer>();

    save_data.money = deserialize<u64>();

    return save_data;
}

template<>
void
utl::memory_blob_t::serialize<settings_save_file_header_t>(
    arena_t* arena, 
    const settings_save_file_header_t& header
) {
    serialize(arena, header.VERSION);
    serialize(arena, header.graphics_config);
    serialize(arena, header.player_data);
}

template<>
settings_save_file_header_t
utl::memory_blob_t::deserialize<settings_save_file_header_t>(arena_t* arena) {
    settings_save_file_header_t header{};

    header.VERSION = deserialize<u64>();

    header.graphics_config = deserialize<game_graphics_config_t>();
    header.player_data = deserialize<player_profile_t>();

    return header;
}



struct font_backend_t {
    gfx::vul::texture_2d_t* texture{nullptr};
    // VkDescriptorSet         descriptor{0};
};

gfx::font_t* load_font(arena_t* arena, auto& gfx, std::string_view file, f32 size) {
    gfx::font_t* font = 0;
    tag_struct(font, gfx::font_t, arena);
    gfx::font_load(arena, font, "./res/fonts/Go-Mono-Bold.ttf", size);
    
    tag_struct(auto* font_backend, font_backend_t, arena);
    tag_struct(font_backend->texture, gfx::vul::texture_2d_t, arena);
        
    font->user_data = font_backend;
        
    gfx.load_font_sampler(
        arena,
        font_backend->texture,
        font);

    return font;
}

struct dynamic_font_t {
    arena_t* arena = 0;
    buffer<gfx::font_t> fonts = {};
    
    gfx::font_t* get_font(u32 size) {
        for (auto& font: fonts.view()) {
            if (font.size == (f32)size) {
                return &font;
            }
        }
        return 0;
    }
};

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

    utl::hash_trie_t<std::string_view, loaded_skeletal_mesh_t>* animations = 0;

    gfx::vul::state_t   gfx;
    rendering::system_t* render_system{0};

    // todo(zack) clean this up
    gfx::font_t default_font;
    gfx::font_t large_font;
    // gfx::vul::texture_2d_t* default_font_texture{nullptr};
    VkDescriptorSet ui_descriptor;

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
