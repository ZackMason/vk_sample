#include <zyy_core.hpp>

global_variable b32 gs_debug_camera_active;

#include "App/game_state.hpp"

#include "App/Game/Util/loading.hpp"
#include "skeleton.hpp"

#include "ProcGen/terrain.hpp"
#include "ProcGen/mesh_builder.hpp"
#include "App/Game/World/world.hpp"
#include "App/Game/WorldGen/worlds.hpp"

#include "App/Game/World/Level/level.hpp"
#include "App/Game/World/Level/Room/room.hpp"
#include "App/Game/Items/base_item.hpp"

#include "App/Game/Weapons/base_weapon.hpp"
#include "App/Game/Weapons/weapon_common.hpp"

#include "App/Game/Rendering/assets.hpp"
#include "App/Game/Physics/player_movement.hpp"

// Globals
// Remove these!
global_variable f32 gs_reload_time = 0.0f;
global_variable f32 gs_jump_save_time = 0.0f;
global_variable f32 gs_jump_load_time = 0.0f;

global_variable b32 gs_show_console=0;
global_variable b32 gs_show_watcher=false;

global_variable zyy::cam::first_person_controller_t gs_debug_camera;

global_variable VkCullModeFlagBits gs_cull_modes[] = {
    VK_CULL_MODE_BACK_BIT,
    VK_CULL_MODE_FRONT_BIT,
    VK_CULL_MODE_NONE,
};
global_variable u32 gs_cull_mode;

global_variable VkPolygonMode gs_poly_modes[] = {
    VK_POLYGON_MODE_FILL,
    VK_POLYGON_MODE_LINE,
    VK_POLYGON_MODE_POINT,
};
global_variable u32 gs_poly_mode;

global_variable u32 gs_rtx_on = 0;


world_generator_t*
generate_world_from_file(arena_t* arena, std::string_view file) {
    tag_struct(auto* generator, world_generator_t, arena);
    generator->arena = arena;
    generator->data = (void*)file.data();

    generator->add_step("Loading", WORLD_STEP_TYPE_LAMBDA(environment) {
        auto* file_name = (const char*)data;

        load_world_file(world, file_name, [&](auto world, auto prefab, v3f o, const m33& b) {
            generator->add_step(prefab.type_name.data(), WORLD_STEP_TYPE_LAMBDA(environment) {

            });
            zyy::tag_spawn(world, prefab, o, b);
        });
    });
    return generator;
}
#include <filesystem>

#include "App/Game/GUI/debug_gui.hpp"

#include "App/Game/GUI/entity_editor.hpp"

#include "App/Game/Entity/zyy_entity_serialize.hpp"

inline game_state_t*
get_game_state(game_memory_t* mem) {
    return (game_state_t*)mem->game_state;
}

struct game_theme_t {
    f32 padding = 4.0f;

    f32 action_bar_size = 32.0f * 2.0f;

    gfx::color32 action_bar_color = gfx::color::rgba::black;
    gfx::color32 action_bar_fg_color = gfx::color::rgba::ue5_bg;
    gfx::color32 action_button_color = gfx::color::rgba::gray;
};

namespace available_options { enum : u32 {
    name        = 1,
    open        = 2,
    activate    = 4,
    pickup      = 8,
};}

namespace ui_styles { enum : u32 {
    first, second,
};}


struct game_ui_t {
    game_theme_t game_theme{};

    zyy::entity_t* entity = 0;

    u32 options_available = available_options::name;

    zyy::health_t health;
    zyy::wep::ammo_mag_t mag;

    v3f eye;
    v3f look;

    v3f aim_location;

    math::rect2d_t screen{};

    inline static b32 inventory_open = 0;

    inline static u32 highlight_option = 0;
    inline static u32 ui_style = ui_styles::second;
};

void draw_game_ui_first(gfx::gui::im::state_t& imgui, game_ui_t* game_ui) {
    auto screen = game_ui->screen;
    auto game_theme = game_ui->game_theme;

    const auto width_three_split = screen.size().x / 3.0f;

    auto [left_panel, mid_plus_right] = math::cut_left(screen, width_three_split);
    auto [right_panel, mid_panel] = math::cut_right(mid_plus_right, width_three_split);

    game_theme.action_bar_size = (mid_panel.size().x + game_theme.padding * 2.0f) / 11.0f;
    auto [action_bar, middle] = math::cut_bottom(mid_panel, game_theme.action_bar_size + game_theme.padding*2.0f);

    gfx::gui::draw_rect(&imgui.ctx, action_bar, game_theme.action_bar_color);

    for (i32 i = 0; i < 10; i++) {
        math::rect2d_t box{
            .min = action_bar.center() - game_theme.action_bar_size * 0.5f,
            .max = action_bar.center() + game_theme.action_bar_size * 0.5f,
        };
        auto width_plus_pad = (box.size().x + game_theme.padding);
        box.add(v2f{(f32(i)-4.5f) * width_plus_pad, 0.0f}); 
        
        gfx::gui::draw_rect(&imgui.ctx, box, game_theme.action_button_color);
        gfx::gui::string_render(&imgui.ctx, fmt_sv("{}", i), box.min + game_theme.padding, gfx::color::rgba::white);
    }
}

void draw_game_ui_second(gfx::gui::im::state_t& imgui, game_ui_t* game_ui) {
    auto screen = game_ui->screen;
    auto game_theme = game_ui->game_theme;

    const auto split_width = screen.size().x / 4.0f;

    auto [left_panel, mid_plus_right] = math::cut_left(screen, split_width);
    auto [right_panel, mid_panel] = math::cut_right(mid_plus_right, split_width);

    game_theme.action_bar_size = (mid_panel.size().x + game_theme.padding * 2.0f) / 11.0f;
    auto [action_bar, middle] = math::cut_bottom(right_panel, game_theme.action_bar_size + game_theme.padding*2.0f);
    
    auto action_tab = action_bar;
    action_tab.min.x += action_bar.size().x * 0.38f;
    action_tab.min.y += action_bar.size().y * 0.38f;
    action_tab.add(v2f{0.0f, -action_bar.size().y});

//jhere
    gfx::gui::draw_rect(&imgui.ctx, action_bar, game_theme.action_bar_color);
    gfx::gui::draw_rect(&imgui.ctx, action_tab, game_theme.action_bar_color);

    const f32 border_size = 4.0f;
    action_bar.pad(border_size);
    action_tab.pad(border_size);
    action_tab.max.y += border_size * 2.0f;

    gfx::gui::draw_rect(&imgui.ctx, action_bar, game_theme.action_bar_fg_color);
    gfx::gui::draw_rect(&imgui.ctx, action_tab, game_theme.action_bar_fg_color);

    v2f cursor = action_tab.min + 2.0f;
    gfx::gui::string_render(&imgui.ctx, fmt_sv("Health: {} / {}", game_ui->health.current, game_ui->health.max), &cursor, gfx::color::rgba::light_green);

    // for (i32 i = 0; i < 10; i++) {
    //     math::rect2d_t box{
    //         .min = action_bar.center() - game_theme.action_bar_size * 0.5f,
    //         .max = action_bar.center() + game_theme.action_bar_size * 0.5f,
    //     };
    //     auto width_plus_pad = (box.size().x + game_theme.padding);
    //     box.add(v2f{(f32(i)-4.5f) * width_plus_pad, 0.0f}); 
        
    //     gfx::gui::draw_rect(&imgui.ctx, box, game_theme.action_button_color);
    //     gfx::gui::string_render(&imgui.ctx, fmt_sv("{}", i), box.min + game_theme.padding, gfx::color::rgba::white);
    // }
}

void draw_game_inventory(gfx::gui::im::state_t& imgui, game_ui_t* game_ui) {
    const auto screen = game_ui->screen;
    const auto game_theme = game_ui->game_theme;

    constexpr f32 side_border_prc = 0.3f;
    constexpr f32 top_border_prc = 0.15f;
    constexpr f32 border_size = 4.0f;
    constexpr f32 title_prc = 0.1f;
    constexpr f32 title_pad = 0.0f;
    constexpr gfx::color32 title_color = gfx::color::rgba::white;

    const f32 side_border_pxl = screen.size().x * side_border_prc;
    const f32 top_border_pxl  = screen.size().y * top_border_prc;
    
    auto [left_panel, r0]            = math::cut_left (screen, side_border_pxl);
    auto [right_panel, middle_panel] = math::cut_right(r0,     side_border_pxl);

    auto [r2, panel] = math::cut_bottom(middle_panel, top_border_pxl);
    std::tie(r2, panel) = math::cut_top(panel,        top_border_pxl);

    auto [panel_title, panel_body] = math::cut_top(panel, panel.size().y * title_prc);
    panel_title.pad(title_pad);

    // draw bg panel
    gfx::gui::draw_rect(&imgui.ctx, panel, game_theme.action_bar_color);
    panel.pad(border_size);
    // gfx::gui::draw_rect(&imgui.ctx, panel, game_theme.action_bar_fg_color);

    // draw title
    constexpr std::string_view title = "Inventory"sv;
    const auto title_size = gfx::font_get_size(imgui.ctx.font, title);
    
    v2f title_cursor = panel_title.sample(math::half2) - title_size * math::half2;
    gfx::gui::draw_rect(&imgui.ctx, panel_title, game_theme.action_bar_fg_color);

    gfx::gui::string_render(&imgui.ctx, title, &title_cursor, title_color);

}

void draw_game_ui(gfx::gui::im::state_t& imgui, game_ui_t* game_ui) {
    switch(game_ui->ui_style) {
        case ui_styles::first:
            draw_game_ui_first(imgui, game_ui);
            break;
        case ui_styles::second:
            draw_game_ui_second(imgui, game_ui);
            break;
        case_invalid_default;
    }

    if (game_ui->inventory_open) {
        draw_game_inventory(imgui, game_ui);
    }
}

game_ui_t create_game_ui(zyy::world_t* world, zyy::entity_t* player) {
    game_ui_t ui{};
    if (player == nullptr) {
        return ui;
    }
    auto capture = gfx::gui::im::want_mouse_capture(world->game_state->gui.imgui);

    ui.screen = math::rect2d_t{v2f{0.0f}, world->render_system()->screen_size()};

    ui.eye = player->camera_controller.transform.origin;

    auto yaw = player->camera_controller.yaw;
    auto pitch = player->camera_controller.pitch;
    ui.look = zyy::cam::get_direction(yaw, pitch);
    ui.eye += ui.look * 1.50f;

    ui.look *= 9.0f;

    ui.health = player->stats.character.health;

    if (!capture && world->game_state->input().pressed.keys['T']) {
        ui.inventory_open ^= true;
    }
    
    math::ray_t game_ui_ray{ui.eye, ui.look};
    DEBUG_DIAGRAM(game_ui_ray);
    auto raycast = world->physics->raycast_world(ui.eye, ui.look, ~zyy::physics_layers::player);
    if (raycast.hit) {
        auto* rb = (physics::rigidbody_t*)raycast.user_data;
        auto* entity = (zyy::entity_t*)rb->user_data;
        if (entity == player) {
            zyy_warn(__FUNCTION__, "Ray Hit Player");
        }
        ui.entity = entity;
    } else {
        ui.entity = nullptr;
    }

    if (player->primary_weapon.entity) {
        auto* weapon = player->primary_weapon.entity;
        const auto weapon_forward = -weapon->global_transform().basis[2];
        math::ray_t aim_ray{weapon->global_transform().origin, weapon_forward * 100.0f};

        ui.mag = weapon->stats.weapon.mag;

        auto aim_raycast = world->physics->raycast_world(aim_ray.origin, aim_ray.direction, ~zyy::physics_layers::player);
        if (aim_raycast.hit) {
            ui.aim_location = aim_raycast.point;
        } else {
            ui.aim_location = aim_ray.at(100.0f);
        }
    }

    if (ui.entity) {
        ui.options_available = available_options::name;

        if (ui.entity->flags & zyy::EntityFlags_Interactable) {
            ui.options_available |= available_options::activate;
        }
        if (ui.entity->flags & zyy::EntityFlags_Pickupable) {
            ui.options_available |= available_options::pickup;
        }
    }

    return ui;
}

void 
draw_game_gui(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    game_state_t* game_state = get_game_state(game_memory);
    auto* world = game_state->game_world;
    auto* player = world->player;
    const auto& vp = game_state->render_system->vp;
    auto& imgui = game_state->gui.imgui;
    const auto center = imgui.ctx.screen_size * 0.5f;

    if(!player || !world) return;

    auto game_ui = create_game_ui(world, player);

    imgui.begin_free_drawing();
    defer {
        imgui.end_free_drawing();
    };

    if (game_memory->input.mouse.scroll.y != 0.0f) {
        game_ui.highlight_option += u32(game_memory->input.mouse.scroll.y);
    }

    draw_game_ui(imgui, &game_ui);

    if (game_ui.entity) {
        v2f look_at_cursor = imgui.ctx.screen_size * v2f{0.5f} + v2f{128.0f, 0.0f};

        constexpr auto selection_color = gfx::color::rgba::yellow;
        constexpr auto normal_color = gfx::color::rgba::white;
        auto color = normal_color;

        gfx::gui::im::draw_circle(imgui, center, 4.0f, gfx::color::rgba::light_gray);
        gfx::gui::im::draw_circle(imgui, center, 2.0f, gfx::color::rgba::white);

        u32 option_count = 0;

        if (option_count == game_ui.highlight_option) color = selection_color;
        if (game_ui.options_available & available_options::name) {
            if (game_ui.entity->name.data) {
                option_count += 1;
                gfx::gui::string_render(&imgui.ctx, fmt_sv("{}", game_ui.entity->name.c_data), &look_at_cursor, color);
            }
        }

        color = normal_color;
        if (option_count == game_ui.highlight_option) color = selection_color;

        constexpr f32 indent = 0.0f;
        look_at_cursor.x += indent;


        if (game_ui.options_available & available_options::pickup) {
            if (option_count == game_ui.highlight_option) {
                if (imgui.ctx.input->pressed.keys[key_id::E]) {
                    // this is really bad
                    if (!pickup_item(player, game_ui.entity)) { 
                        assert(!"Cant Pickup");
                    }
                }
            }
            gfx::gui::string_render(&imgui.ctx, "Pickup", &look_at_cursor, color);
            option_count += 1;
        }

        color = normal_color;
        if (option_count == game_ui.highlight_option) color = selection_color;

        if (game_ui.options_available & available_options::activate) {
            gfx::gui::string_render(&imgui.ctx, "Activate", &look_at_cursor, color);
            option_count += 1;
        }

        if (option_count) {
            game_ui.highlight_option = game_ui.highlight_option % option_count;
        }
    }

    if (player->primary_weapon.entity) {
        const auto& weapon = player->primary_weapon.entity->stats.weapon;
        const auto& stats = player->stats.character;

        {
            const v3f spos = math::world_to_screen(vp, game_ui.aim_location);
            const math::rect3d_t viewable{v3f{0.0f}, v3f{1.0f}};
            if (viewable.contains(spos)) {
                const v2f screen = v2f{spos} * imgui.ctx.screen_size;
                gfx::gui::draw_circle(&imgui.ctx, screen, 3.0f, gfx::color::rgba::white);
            }
        }

        v2f weapon_display_start = imgui.ctx.screen_size * v2f{0.05f, 0.9f};
        
        gfx::gui::string_render(&imgui.ctx, fmt_sv("{} / {}", game_ui.mag.current, game_ui.mag.max), &weapon_display_start, gfx::color::rgba::white);
        gfx::gui::string_render(&imgui.ctx, fmt_sv("Chambered: {}", weapon.chamber_count), &weapon_display_start, gfx::color::rgba::white);
    }
}

inline static gfx::vul::texture_2d_t*
make_grid_texture(arena_t* arena, gfx::vul::texture_2d_t* tex, u32 size, v3f c1, v3f c2, f32 grid_count = 10.0f) {
    *tex = {};
    tex->size[0] = tex->size[1] = size;
    tex->channels = 4;
    tex->pixels = push_struct<u8>(arena, size*size*4);
    u64 i = 0;
    range_u64(x, 0, size) {
        range_u64(y, 0, size) {
            auto n = std::sinf((f32)x/(f32)size*math::constants::pi32*grid_count) * std::sinf((f32)y/(f32)size*math::constants::pi32*grid_count);
            n = glm::step(n, 0.0f); 
            auto c = glm::mix(c1,c2,n);
            auto uc = gfx::color::to_color32(c);
            tex->pixels[i++] = u8(uc & 0xff); uc >>= 8;
            tex->pixels[i++] = u8(uc & 0xff); uc >>= 8;
            tex->pixels[i++] = u8(uc & 0xff); uc >>= 8;
            tex->pixels[i++] = u8(uc & 0xff); uc >>= 8;
        }
    }
    tex->calculate_mip_level();
    return tex;
}

inline static gfx::vul::texture_2d_t*
texture_add_border(gfx::vul::texture_2d_t* texture, v3f c1, u32 size) {
    assert(texture->size.x > 0 && texture->size.y > 0);
    const auto w = (u32)texture->size.x;
    const auto i = [=](u32 x, u32 y) {
        return (y*w+x);
    };
    u32* pixels = (u32*)texture->pixels;
    auto uc = gfx::color::to_color32(c1);
    range_u32(x, 0, (u32)texture->size.x) { 
        range_u32(j, 0, size) {
            pixels[i(x,j)] = uc;
            pixels[i(x,texture->size.y-1-j)] = uc;
        }
    }
    range_u32(y, 0, (u32)texture->size.y) {
        range_u32(j, 0, size) {
            pixels[i(j,y)] = uc;
            pixels[i(texture->size.x-1-j,y)] = uc;
        }
    }
    return texture;
}


inline static gfx::vul::texture_2d_t*
make_error_texture(arena_t* arena, gfx::vul::texture_2d_t* tex, u32 size) {
    return make_grid_texture(arena,
							 tex,
							 size,
							 gfx::color::v3::black,
							 gfx::color::v3::purple);
}

inline static gfx::vul::texture_2d_t*
make_error_texture(arena_t* arena, u32 size) {
    tag_struct(auto* tex, gfx::vul::texture_2d_t, arena);
    return make_grid_texture(arena,
							 tex,
							 size,
							 gfx::color::v3::black,
							 gfx::color::v3::purple);
}

inline static gfx::vul::texture_2d_t*
make_noise_texture(arena_t* arena, u32 size) {
    tag_struct(auto* tex, gfx::vul::texture_2d_t, arena);
    *tex = {};

    tex->size[0] = tex->size[1] = size;
    tex->channels = 4;
    tex->pixels = (u8*)push_bytes(arena, size*size*4);
    // tex->format = VK_FORMAT_R8G8B8A8_SRGB;
    u64 i = 0;
    range_u64(x, 0, size) {
        range_u64(y, 0, size) {
            const auto n = glm::clamp(utl::noise::fbm(v2f{x,y} * 0.05f) * 0.5f + 0.5f, 0.0f, 1.0f);
            tex->pixels[i++] = u8(n*255.0f);
            tex->pixels[i++] = u8(n*255.0f);
            tex->pixels[i++] = u8(n*255.0f);
            tex->pixels[i++] = 255ui8;
        }
    }

    return tex;
}

void
app_init_graphics(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    game_state_t* game_state = get_game_state(game_memory);

    gfx::vul::state_t& vk_gfx = game_state->gfx;
    {
        auto memory = begin_temporary_memory(&game_state->main_arena);
        game_state->gfx.init(&game_memory->config, memory.arena);
        end_temporary_memory(memory);
    }

    // make_grid_texture(&game_state->texture_arena, &vk_gfx.null_texture, 256, v3f{0.3f}, v3f{0.6f}, 2.0f);
    // texture_add_border(&vk_gfx.null_texture, v3f{0.9f}, 4);
    // texture_add_border(&vk_gfx.null_texture, gfx::color::v3::yellow, 4);
    // texture_add_border(&vk_gfx.null_texture, v3f{0.2f}, 4);

    // make_grid_texture(&game_state->texture_arena, &vk_gfx.null_texture, 256, v3f{0.7f, 0.7f, 0.7f}, v3f{0.66f, 0.66f, 0.66f}, 2.0f);
    make_grid_texture(&game_state->texture_arena, &vk_gfx.null_texture, 256, v3f{0.98f}, v3f{0.66f, 0.66f, 0.66f}, 2.0f);

    // make_grid_texture(&game_state->texture_arena, &vk_gfx.null_texture, 256, v3f{0.7f, 0.13f, 0.13f}, v3f{0.79f, 0.16f, 0.16f}, 2.0f);
    texture_add_border(&vk_gfx.null_texture, v3f{0.2f}, 4);

    // make_error_texture(&game_state->texture_arena, &vk_gfx.null_texture, 256);
    vk_gfx.load_texture_sampler(&vk_gfx.null_texture);

    game_state->render_system = rendering::init<megabytes(32)>(vk_gfx, &game_state->main_arena);
    game_state->render_system->resource_file = game_state->resource_file;
    vk_gfx.create_vertex_buffer(&game_state->gui.vertices[0]);
    vk_gfx.create_index_buffer(&game_state->gui.indices[0]);
    vk_gfx.create_vertex_buffer(&game_state->gui.vertices[1]);
    vk_gfx.create_index_buffer(&game_state->gui.indices[1]);
    auto* rs = game_state->render_system;

    gfx::gui::ctx_clear(&game_state->gui.ctx, &game_state->gui.vertices[0].pool, &game_state->gui.indices[0].pool);
    // load all meshes from the resource file
    // range_u64(i, 0, game_state->resource_file->file_count) {
    //     if (game_state->resource_file->table[i].file_type != utl::res::magic::mesh) { 
    //         continue; 
    //     }

    //     std::byte* file_data = utl::res::pack_file_get_file(game_state->resource_file, i);

    //     auto loaded_mesh = load_bin_mesh_data(
    //         &game_state->mesh_arena,
    //         file_data, 
    //         &game_state->render_system->vertices.pool,
    //         &game_state->render_system->indices.pool
    //     );
    //     const char* file_name = game_state->resource_file->table[i].name.c_data;

    //     range_u64(m, 0, loaded_mesh.count) {
    //         u64 ids[2];
    //         u32 mask = game_state->render_system->texture_cache.load_material(game_state->render_system->arena, game_state->gfx, loaded_mesh.meshes[m].material, ids);
    //         loaded_mesh.meshes[m].material.albedo_id = (mask&0x1) ? ids[0] : std::numeric_limits<u64>::max();
    //         loaded_mesh.meshes[m].material.normal_id = (mask&0x2) ? ids[1] : std::numeric_limits<u64>::max();
    //     }
    //     rendering::add_mesh(game_state->render_system, file_name, loaded_mesh);
    // }
    
    gs_skinned_vertices = &rs->skinned_vertices.pool;

    range_u64(i, 0, game_state->resource_file->file_count) {
        arena_t* arena = &game_state->mesh_arena;
        if (game_state->resource_file->table[i].file_type != utl::res::magic::skel) { 
            continue; 
        }

        const char* file_name = game_state->resource_file->table[i].name.c_data;
        std::byte*  file_data = utl::res::pack_file_get_file(game_state->resource_file, i);

        gfx::mesh_list_t results;
        utl::memory_blob_t blob{file_data};

        auto& animation_file = 
            *utl::hash_get(&game_state->animations, std::string_view{file_name}, arena) =
            blob.deserialize<loaded_skeletal_mesh_t>(arena);

        animation_file.name.push(arena, std::string_view{file_name});

        gfx::anim::animator_t animator;

        animator.play_animation(animation_file.animations.data + 0);
        range_u64(_, 0, 1000) {
            animator.update(1.0f/60.0f);
        }
    }


    {
        auto* arena = &game_state->texture_arena;

        gfx::font_load(arena, &game_state->default_font, "./res/fonts/Go-Mono-Bold.ttf", 18.0f);
        // gfx::font_load(arena, &game_state->default_font, "./res/fonts/.ttf", 18.0f);

        tag_struct(auto* font_backend, font_backend_t, &game_state->texture_arena);
        tag_struct(font_backend->texture, gfx::vul::texture_2d_t, &game_state->texture_arena);
        
        game_state->default_font.user_data = font_backend;
        
        vk_gfx.load_font_sampler(
            arena,
            font_backend->texture,
            &game_state->default_font);
    }
    

    if (1)
    {
        auto* blood_texture = make_noise_texture(&game_state->texture_arena, 256);
        u64 i = 0;
        range_u64(x, 0, 256) {
            range_u64(y, 0, 256) {
                v2f uv{f32(x), f32(y)};
                auto dd = glm::distance(uv, v2f{128.0f}) / 128.0f;
                dd = 1.0f - glm::sqrt(dd);
                auto n = f32(blood_texture->pixels[i])/255.0f;
                n *= n;
                n = tween::smoothstep_cubic(0.45f,0.65f,n) * dd;
                n = glm::clamp(n, 0.0f, 1.0f);
                blood_texture->pixels[i++] = u8(n*255.0f);
                blood_texture->pixels[i++] = u8(n*255.0f * 0.3f);
                blood_texture->pixels[i++] = u8(n*255.0f * 0.3f);
                blood_texture->pixels[i++] = u8(n*255.0f);
            }
        }
        vk_gfx.load_texture_sampler(blood_texture);

        rs->texture_cache.insert("blood", *blood_texture);
    }

    for (size_t i = 0; i < array_count(game_state->gui.ctx.dyn_font); i++) {
        auto* arena = &game_state->texture_arena;
        auto& font = game_state->gui.ctx.dyn_font[i];
        font = load_font(arena, vk_gfx, "./res/fonts/Go-Mono-Bold.ttf", (f32)i+8);

        // game_state->default_font = *font;
    }
    
    set_ui_textures(game_state);
    
    { //@test loading shader objects
        auto& mesh_pass = rs->frames[0].mesh_pass;
        auto& anim_pass = rs->frames[0].anim_pass;
        auto& rt_compute_pass = rs->frames[0].rt_compute_pass;

        rs->shader_cache.load(
            game_state->main_arena, 
            vk_gfx,
            assets::shaders::probe_integrate_comp,
            &rt_compute_pass.descriptor_set_layouts[1],
            1
        );
        rs->shader_cache.load(
            game_state->main_arena, 
            vk_gfx,
            assets::shaders::probe_integrate_depth_comp,
            &rt_compute_pass.descriptor_set_layouts[2],
            1
        );

        rs->shader_cache.load(
            game_state->main_arena, 
            vk_gfx,
            assets::shaders::simple_vert,
            mesh_pass.descriptor_layouts,
            mesh_pass.descriptor_count
        );
        // rs->shader_cache.load(
        //     game_state->main_arena, 
        //     vk_gfx,
        //     assets::shaders::skinned_vert,
        //     anim_pass.descriptor_layouts,
        //     anim_pass.descriptor_count
        // );
        rs->shader_cache.load(
            game_state->main_arena, 
            vk_gfx,
            assets::shaders::simple_frag,
            mesh_pass.descriptor_layouts,
            mesh_pass.descriptor_count
        );
        using const_shader_ptr_t = const VkShaderEXT*;
        const_shader_ptr_t shaders[2]{
            rs->shader_cache.get(assets::shaders::simple_vert.filename),
            rs->shader_cache.get(assets::shaders::simple_frag.filename)
        };

        assert(shaders[0]!=VK_NULL_HANDLE);
        assert(shaders[1]!=VK_NULL_HANDLE);

        const auto make_material = [&](std::string_view n, auto&& mat) {
            return rendering::create_material(
                game_state->render_system, 
                n, std::move(mat), 
                VK_NULL_HANDLE, 
                mesh_pass.pipeline_layout,
                shaders, array_count(shaders)
            );
        };

        make_material("default", gfx::material_t::plastic(gfx::color::v4::ray_white));
        // make_material("default", gfx::material_t::plastic(gfx::color::v4::red));
        { // 1
            auto triplanar_mat = gfx::material_t::plastic(gfx::color::v4::white);
            triplanar_mat.flags = gfx::material_lit | gfx::material_triplanar;
            make_material("triplanar", triplanar_mat);
        }
        { // 2
            auto unlit_mat = gfx::material_t::plastic(gfx::color::v4::reddish);
            unlit_mat.flags = 0;//gfx::material_lit;
            unlit_mat.emission = 10.0f;

            make_material("unlit", unlit_mat);
        }
        { // 3
            auto unlit_mat = gfx::material_t::plastic(gfx::color::v4::white);
            // unlit_mat.flags = 0;
            unlit_mat.roughness = 0.1f;
            //unlit_mat.emission = 10.0f;

            make_material("blood", unlit_mat);
        }
        { // 4
            auto unlit_mat = gfx::material_t::plastic(gfx::color::v4::yellowish);
            unlit_mat.flags = gfx::material_billboard;
            unlit_mat.roughness = 1.0f;
            unlit_mat.emission = 10.0f;

            make_material("particle", unlit_mat);
        }
        { // 5
            auto unlit_mat = gfx::material_t::plastic(gfx::color::v4::reddish);
            unlit_mat.flags = gfx::material_billboard;
            unlit_mat.roughness = 1.0f;
            unlit_mat.emission = 10.0f;

            make_material("particle-red", unlit_mat);
        }
        { // 6
            auto unlit_mat = gfx::material_t::plastic(gfx::color::v4::orange);
            unlit_mat.flags = gfx::material_billboard;
            unlit_mat.roughness = 1.0f;
            unlit_mat.emission = 10.0f;

            make_material("particle-orange", unlit_mat);
        }
        { // 7
            auto unlit_mat = gfx::material_t::plastic(v4f{0.4f, 0.3f, 0.9f, 1.0f});
            unlit_mat.flags = gfx::material_billboard;
            unlit_mat.roughness = 1.0f;
            unlit_mat.emission = 10.0f;

            make_material("particle-blue", unlit_mat);
        }
        { // 8  smoke
            auto unlit_mat = gfx::material_t::plastic(v4f{0.6f, 0.6f, 0.6f, 1.0f});
            unlit_mat.flags = gfx::material_lit;
            unlit_mat.roughness = 0.70f;
            unlit_mat.emission = 0.30f;
            

            make_material("smoke", unlit_mat);
        }
        { // 9
            auto unlit_mat = gfx::material_t::plastic(v4f{0.18f, 0.83f, 0.13f, 1.0f});
            unlit_mat.flags = 0;
            unlit_mat.roughness = 1.0f;
            unlit_mat.emission = 10.0f;

            make_material("goo", unlit_mat);
        }
        { // 10 water
            auto unlit_mat = gfx::material_t::plastic(v4f{0.098f, 0.3216f, 0.8078f, 1.0f});
            unlit_mat.flags = gfx::material_water | gfx::material_lit;
            unlit_mat.roughness = 0.5f;
            unlit_mat.emission = 0.0f;

            make_material("water", unlit_mat);
        }
        { // 11 water triplanar
            auto unlit_mat = gfx::material_t::plastic(v4f{0.098f, 0.3216f, 0.8078f, 1.0f});
            unlit_mat.flags = gfx::material_water | gfx::material_lit | gfx::material_triplanar;
            unlit_mat.roughness = 0.5f;
            unlit_mat.emission = 0.0f;

            make_material("water_triplanar", unlit_mat);
        }
        { // 12 emission
            auto unlit_mat = gfx::material_t::plastic(gfx::color::v4::white);
            unlit_mat.flags = 0;//gfx::material_lit;
            unlit_mat.emission = 10.0f;

            make_material("emission white", unlit_mat);
        }
        make_material("blue-plastic", gfx::material_t::plastic(gfx::color::v4::blue));
        make_material("gold-metal", gfx::material_t::plastic(gfx::color::v4::yellow));
        make_material("silver-metal", gfx::material_t::plastic(gfx::color::v4::white));
        make_material("sand", gfx::material_t::plastic(gfx::color::v4::sand));
        make_material("dirt", gfx::material_t::plastic(gfx::color::v4::dirt));

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::screen_vert,
            rs->get_frame_data().postprocess_pass.descriptor_layouts,
            rs->get_frame_data().postprocess_pass.descriptor_count
        );
        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::invert_frag,
            rs->get_frame_data().postprocess_pass.descriptor_layouts,
            rs->get_frame_data().postprocess_pass.descriptor_count
        );
        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::tonemap_frag,
            rs->get_frame_data().postprocess_pass.descriptor_layouts,
            rs->get_frame_data().postprocess_pass.descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::upscale_frag,
            rs->get_frame_data().bloom_pass.descriptor_layouts,
            rs->get_frame_data().bloom_pass.descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::downsample_frag,
            rs->get_frame_data().bloom_pass.descriptor_layouts,
            rs->get_frame_data().bloom_pass.descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::gui_vert,
            game_state->render_system->pipelines.gui.set_layouts,
            game_state->render_system->pipelines.gui.set_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::gui_frag,
            game_state->render_system->pipelines.gui.set_layouts,
            game_state->render_system->pipelines.gui.set_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::skybox_vert,
            game_state->render_system->pipelines.sky.set_layouts,
            game_state->render_system->pipelines.sky.set_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::skybox_frag,
            game_state->render_system->pipelines.sky.set_layouts,
            game_state->render_system->pipelines.sky.set_count
        );
        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::voidsky_frag,
            game_state->render_system->pipelines.sky.set_layouts,
            game_state->render_system->pipelines.sky.set_count
        );
    }

    game_state->gui.ctx.font = &game_state->default_font;
    game_state->gui.ctx.input = &game_memory->input;
    game_state->gui.ctx.screen_size = v2f{
        (f32)game_memory->config.window_size[0], 
        (f32)game_memory->config.window_size[1]
    };

    {
        auto memory = begin_temporary_memory(&game_state->main_arena);

        utl::config_list_t config{};
        config.head = utl::load_config(memory.arena, "config.ini", &config.count);
        game_state->render_system->light_probe_ray_count = 
            utl::config_get_int(&config, "ddgi", 64);

        FLOG("Light Probe Ray Count: {}", game_state->render_system->light_probe_ray_count);

        end_temporary_memory(memory);
    }
}

export_fn(void) 
app_on_init(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    Platform = game_memory->platform;
    game_memory->game_state = push_bytes(&game_memory->arena, sizeof(game_state_t));
    game_state_t* game_state = get_game_state(game_memory);
    new (game_state) game_state_t{game_memory, game_memory->arena};
    
    game_state->game_memory = game_memory;

    utl::rng::random_s::randomize();
    
    arena_t* main_arena = &game_state->main_arena;
    // arena_t* string_arena = &game_state->string_arena;


    // game_state->string_arena = arena_create(megabytes(2));
    game_state->mesh_arena = arena_create(megabytes(2));
    game_state->texture_arena = arena_create(megabytes(2));
    // game_state->gui.arena = arena_create(megabytes(1));

    // assets::sounds::load();
#ifdef DEBUG_STATE
    tag_struct(game_state->debug_state, debug_state_t, main_arena, game_memory->input.time);
    game_state->debug_state->arena = arena_create();
    game_state->debug_state->watch_arena = arena_create(megabytes(4));

    gs_debug_state = game_state->debug_state;    

// #if 0
// #ifdef ZYY_INTERNAL 
    tag_struct(gs_debug_state->console, debug_console_t, main_arena);
    auto* console = gs_debug_state->console;
    console->user_data = game_state;

    console_add_command(console, "help", [](void* data) {
        auto* console = (debug_console_t*)data;
        range_u64(i, 0, console->command_count) {
            console_log(
                console, 
                console->console_commands[i].name, 
                gfx::color::rgba::yellow, 
                console->console_commands[i].command.command, 
                console->console_commands[i].command.data 
            );
        }
    }, console);

    console_add_command(console, "noclip", [](void* data) {
        // auto* console = (debug_console_t*)data;
        gs_debug_camera_active = !gs_debug_camera_active;
        
    }, console);

    // time is broken during loops, this is a reminder
    console_add_command(console, "rtime", [](void* data) {
        auto* game_state = (game_state_t*)data;
        auto* console = DEBUG_STATE.console;
        console_log(console, fmt_sv("Run Time: {} seconds", game_state->input().time));
    }, console);

    console_add_command(console, "build", [](void* data) {
        // auto* console = (debug_console_t*)data;
        std::system("start /B ninja");
        
    }, console);

    console_add_command(console, "code", [](void* data) {
        auto* console = (debug_console_t*)data;
        const auto& message = console->last_message();
        auto i = std::string_view{message.text}.find_first_of("$");
        if (i != std::string_view::npos) {
            std::stringstream ss{std::string_view{message.text}.substr(i+1).data()};
            std::string path;
            ss >> path;
            console_log(console, fmt_sv("Opening {}", path));
            std::system(fmt_sv("start /B code ./{}", path).data());
        } else {
            console_log(console, "Parse error", gfx::color::rgba::red);
        }
        
    }, console);

    console_add_command(console, "run", [](void* data) {
        auto* console = (debug_console_t*)data;
        const auto& message = console->last_message();
        auto i = std::string_view{message.text}.find_first_of("$");
        if (i != std::string_view::npos) {
            std::stringstream ss{std::string_view{message.text}.substr(i+1).data()};
            std::string path;
            ss >> path;
            console_log(console, fmt_sv("running shell command {}", path));
            std::system(fmt_sv("start /B {}", path).data());
        } else {
            console_log(console, "Parse error", gfx::color::rgba::red);
        }
        
    }, console);

    console_add_command(console, "dtimeout", [](void* data) {
        auto* console = (debug_console_t*)data;
        const auto time = console->last_float();
        if (time) {
            console_log(console, fmt_sv("Setting Debug Timeout to {}", *time));
            DEBUG_SET_TIMEOUT(*time);
        } else {
            console_log(console, "Parse error", gfx::color::rgba::red);
        }
    }, console);

    console_add_command(console, "fdist", [](void* data) {
        auto* console = (debug_console_t*)data;
        const auto time = console->last_float();
        if (time) {
            DEBUG_SET_FOCUS_DISTANCE(*time);
            console_log(console, fmt_sv("Setting Debug Focus Distance to {}", *time));
        } else {
            console_log(console, "Parse error", gfx::color::rgba::red);
        }
    }, console);

    // console_add_command(console, "build", [](void* data) {
    //     // auto* console = (debug_console_t*)data;
    //     std::system("start /B build n game");
        
    // }, console);

    console_add_command(console, "select", [](void* data) {
        auto* console = (debug_console_t*)data;
        auto* game_state = (game_state_t*)console->user_data;
        auto args = console->get_args();
        if (args) { // args = "select <>"
            auto [name, rest] = utl::cut_left(*args, " "sv);
            auto* entity = zyy::find_entity_by_name(game_state->game_world, rest);
            if (entity) {
                DEBUG_STATE.selection = &entity->transform.origin;
            } else {
                assert(!"Failed to find entity");
                FLOG("name: {}, rest: {}", name, rest);
                DEBUG_STATE.selection = nullptr;
            }
        } else {
            assert(!"No Args");
        }
    }, console);

    console_add_command(console, "view", [](void* data) {
        auto* console = (debug_console_t*)data;
        auto* game_state = (game_state_t*)console->user_data;
        auto args = console->get_args();
        if (args) { // args = "select <>"
            auto [view, name] = utl::cut_left(*args, " "sv);
            auto* entity = zyy::find_entity_by_name(game_state->game_world, name);
            if (entity) {
                gs_debug_camera.transform.origin = entity->transform.origin;
            } else {
                assert(!"Failed to find entity");
                FLOG("name: {}", name);
            }
        } else {
            assert(!"No Args");
        }
    }, console);

    console_add_command(console, "overflow", [](void* data) {
        auto* console = (debug_console_t*)data;
        auto* game_state = (game_state_t*)console->user_data;
        auto* world = game_state->game_world;

        tag_array(auto* arr, char, &world->arena, 1);
        arr[1] = 'a';
    }, console);

    CLOG("Enter a command");

#endif
    auto& mod_loader = game_state->modding.loader;
    mod_loader.load_library(".\\build\\code.dll");
    // mod_loader.load_library(".\\build\\code.dll");
    

    game_state->resource_file = utl::res::load_pack_file(&game_state->mesh_arena, "./res/res.pack");

    
    physics::api_t* physics = game_memory->physics;
    physics->entity_transform_offset = offsetof(zyy::entity_t, transform);
    if (physics) {
        zyy_info("app_init", "Creating physics scene");
        assert(physics && physics->create_scene);
        physics->create_scene(physics, 0);
        zyy_info("app_init", "Created physics scene");
    }
    app_init_graphics(game_memory);

    game_state->game_world = zyy::world_init(game_state, physics);


    zyy::world_init_effects(game_state->game_world);
    
    zyy::world_init_effects(game_state->game_world);

    {
        // auto& rs = game_state->render_system;
        // auto& vertices = rs->scene_context->vertices.pool;
        // auto& indices = rs->scene_context->indices.pool;
        // prcgen::mesh_builder_t builder{vertices, indices};

        // builder.add_box({v3f{-1.0f}, v3f{1.0f}});
        // rendering::add_mesh(rs, "unit_cube", builder.build(&game_state->mesh_arena));
    }

    gs_debug_camera.camera = &game_state->game_world->camera;
    gs_debug_camera.prio = 100;

    using namespace gfx::color;

    game_state->scene.sporadic_buffer.time = game_state->input().time;
    game_state->scene.sporadic_buffer.mode = 1;
    // game_state->scene.sporadic_buffer.use_lighting = 1;

    zyy_info("game_state", "world size: {}mb", GEN_TYPE_INFO(zyy::world_t).size/megabytes(1));

    {
        auto memory = begin_temporary_memory(&game_state->main_arena);

        utl::config_list_t config{};
        config.head = utl::load_config(memory.arena, "config.ini", &config.count);
        
        auto* level = utl::config_find(&config, "start_level");
        if (level) {
            auto* world = game_state->game_world;
            std::string_view level_name{level->as_sv()};
            tag_array(auto* str, char, &world->arena, level_name.size()+1);
            utl::copy(str, level_name.data(), level_name.size()+1);
            world->world_generator = generate_world_from_file(&world->arena, str);
        }

        // CLOG(fmt_sv("Light Probe Ray Count: {}", game_state->render_system->light_probe_ray_count));

        end_temporary_memory(memory);
    }

    game_state->sfx = fmod_sound::init(main_arena);

    { // handle arguments
        #define command(name, shortname) name##_sid: case shortname##_sid
        #define description(desc) if (helping) { CLOG(desc); break; }
        #define usage(how) "\n\n\tUsage: " ## how
        b32 helping = 0;
        range_u64(i, 1, Platform.argument_count) {
            auto s = std::string_view{Platform.arguments[i]};
            switch (sid(s)) {
                case command("--help", "-h"): { description("Show command info" usage("--help COMMAND"))
                    helping = 1;
                }   break;
                case command("--sound", "-s"): { description("Play a sound" usage("--sound FILENAME"))
                    if (++i < Platform.argument_count) {
                        game_state->sfx->play_sound(Platform.arguments[i]);
                    } else {
                        assert(!"Invalid Argument Count: Sound");
                    }
                }   break;
                case command("--profile", "-p"): { description("Create a profile" usage("--profile NAME"))
                    if (++i < Platform.argument_count) {
                        create_new_profile(main_arena, Platform.arguments[i]);
                    } else {
                        assert(!"Invalid Argument Count: Profile");
                    }
                }   break;
                default: {
                    assert(!"Unknown Argument (See console)");
                    FLOG("Unknown Argument[{}]: {}", i, s);
                }   break;
            }
        }
        #undef command
        #undef description
        #undef usage
    }


}

export_fn(void) 
app_on_deinit(game_memory_t* game_memory) {
    game_state_t* game_state = get_game_state(game_memory);

    gfx::vul::state_t& vk_gfx = game_state->gfx;
    vkDeviceWaitIdle(vk_gfx.device);

    rendering::cleanup(game_state->render_system);
    zyy_info(__FUNCTION__, "Render System cleaned up");

    vk_gfx.cleanup();
    zyy_info(__FUNCTION__, "Graphics API cleaned up");

    game_state->~game_state_t();
}

export_fn(void) 
app_on_unload(game_memory_t* game_memory) {
    zyy_warn(__FUNCTION__, "Reloading Game...");
    game_state_t* game_state = get_game_state(game_memory);
    
    game_state->modding.loader.unload_library();
    // game_state->render_system->ticket.lock();
}

export_fn(void)
app_on_reload(game_memory_t* game_memory) {
    Platform = game_memory->platform;
    game_state_t* game_state = get_game_state(game_memory);
    gs_debug_state = game_state->debug_state;
    gs_reload_time = 1.0f;
    gs_debug_camera.camera = &game_state->game_world->camera;

    game_state->modding.loader.load_library(".\\build\\code.dll");
    // game_state->modding.loader.load_module();
    // game_state->render_system->ticket.unlock();
    zyy_warn(__FUNCTION__, "Reload Complete");
}

b32 
app_on_input(game_state_t* game_state, app_input_t* input) {
    if (input->pressed.keys[key_id::F10]) {
        gs_jump_load_time = 1.0f;
    }
    if (input->pressed.keys[key_id::F9]) {
        gs_jump_save_time = 1.0f;
    }
    if (input->pressed.keys[key_id::F12]) {
        CLOG("noclip");
    }

    if (input->pressed.keys[key_id::F1]) {
        if (auto b = gs_show_watcher = !gs_show_watcher) {
            game_state->gui.imgui.active = "watcher_text_box"_sid;
        } else {
            game_state->gui.imgui.hot = 
            game_state->gui.imgui.active = 0;
        }
    }
    if (input->pressed.keys[key_id::F2]) {
        if (auto b = gs_show_console = ((gs_show_console + 1) % 3)) {
            game_state->gui.imgui.active = "console_text_box"_sid;
        } else {
            game_state->gui.imgui.hot = 
            game_state->gui.imgui.active = 0;
        }
    }
    if (input->pressed.keys[key_id::F4]) {
        if (game_state->time_scale == 0.0f) {
            game_state->time_scale = 1.0f;
        } else {
            game_state->time_scale = 0.0f;
        }
    }
    if (input->pressed.keys[key_id::F5]) {
        zyy::world_destroy_all(game_state->game_world);
        zyy::world_free(game_state->game_world);
        game_state->game_world = zyy::world_init(game_state, game_state->game_memory->physics);

        gs_debug_camera.camera = &game_state->game_world->camera;
        return 1;
    }

    if (input->gamepads[0].is_connected) {
        if (input->gamepads[0].buttons[button_id::dpad_left].is_released) {
            game_state->time_scale *= 0.5f;
            game_state->time_text_anim = 1.0f;
        }
        if (input->gamepads[0].buttons[button_id::dpad_right].is_released) {
            game_state->time_scale *= 2.0f;
            game_state->time_text_anim = 1.0f;
        }
    } 
    if (input->pressed.keys[key_id::F6]) {
        if (game_state->time_scale==0.0f) {
            game_state->time_scale = 1.0f;
        }
        game_state->time_scale *= 0.5f;
        game_state->time_text_anim = 1.0f;
    }
    if (input->pressed.keys[key_id::F7]) {
        if (game_state->time_scale==0.0f) {
            game_state->time_scale = 1.0f;
        }
        game_state->time_scale *= 2.0f;
        game_state->time_text_anim = 1.0f;
    }

    return 0;
}

void game_on_gameplay(game_state_t* game_state, app_input_t* input, f32 dt) {
    TIMED_FUNCTION;
 
    // TODO(ZACK): GAME CODE HERE

    if (game_state->render_system == nullptr) return;

    auto* rs = game_state->render_system;
    auto* world = game_state->game_world;
    
    zyy::world_update(world, dt);

    // DEBUG_DIAGRAM_(v3f{axis::right * 50.0f}, 0.01f);

    if (world->player && world->player->global_transform().origin.y < -100.0f) {
        if (world->player->parent) {
            world->player->transform.origin.y = world->player->parent->global_transform().inv_xform(v3f{0.0f}).y;
        } else {
            world->player->transform.origin.y = 0.0f;
        }
        // zyy_warn("killz", "Reset players vertical position");
        // game_state->game_memory->input.pressed.keys[key_id::F10] = 1;
    }

    {
        TIMED_BLOCK(PhysicsStep);
        
        if (world->physics) {
            world->physics->simulate(world->physics, dt);
        } else {
            zyy_warn("game", "No physics in world");
        }

    }
    zyy::world_update_kinematic_physics(world);

        
    {
        TIMED_BLOCK(GameplayUpdatePostSimulate);

        for (size_t i{0}; i < world->entity_capacity; i++) {
            auto* e = world->entities + i;
            auto brain_id = e->brain_id;
            if (e->flags & zyy::EntityFlags_Breakpoint) {
                __debugbreak();
            }
            if (e->is_alive() == false) {
                continue;
            }


            DEBUG_WATCH(e);

            const bool is_physics_object = e->physics.flags != zyy::PhysicsEntityFlags_None && e->physics.rigidbody;
            const bool is_pickupable = (e->flags & zyy::EntityFlags_Pickupable);
            const bool is_not_renderable = !e->is_renderable();

            e->coroutine->run(world->frame_arena);

            if (is_physics_object && e->physics.impulse != v3f{0.0f}) {
                e->physics.rigidbody->add_impulse(e->physics.impulse);
                e->physics.impulse = {};
            }

            if (brain_id != uid::invalid_id) {
                world_update_brain(world, e, dt);
            }

            if (is_pickupable) {
                e->transform.rotate(axis::up, std::min(0.5f, dt));
            }

            // @debug

            const auto entity_aabb = e->global_transform().xform_aabb(e->aabb);
            if (e->type != zyy::entity_type::player) {
                DEBUG_DIAGRAM_(entity_aabb, 0.0000f);
            }
            if (e->gfx.particle_system) {
                const auto particle_aabb = e->gfx.particle_system->aabb;
                DEBUG_DIAGRAM_(particle_aabb, 0.0000f);
            // } else if (e->gfx.mesh_id != -1) {
            //     auto& mesh = world->render_system()->mesh_cache.get(e->gfx.mesh_id);
            //     range_u64(j, 0, mesh.count) {
            //         const auto mesh_aabb = e->global_transform().xform_aabb(mesh.meshes[i].aabb);
            //         DEBUG_DIAGRAM_(mesh_aabb, 0.0000f);
            //     }
            }
        }
    }
    if (app_on_input(game_state, input)) {
        return;
    }

    zyy::world_kill_free_queue(world);

    if (gs_debug_camera_active) {
        auto& imgui = game_state->gui.imgui;
        auto real_dt = input->dt;
        auto pc = keyboard_controller(input);
        const bool ignore_mouse = gfx::gui::im::want_mouse_capture(imgui);
        const bool ignore_keyboard = gfx::gui::im::want_mouse_capture(imgui);

        const v2f head_move = pc.look_input;

        auto& yaw = gs_debug_camera.yaw;
        auto& pitch = gs_debug_camera.pitch;

        const v3f forward = zyy::cam::get_direction(yaw, pitch);
        const v3f right   = glm::cross(forward, axis::up);

        if (!ignore_mouse) {
            yaw += head_move.x * real_dt;
            pitch -= head_move.y * real_dt;
        }

        local_persist f32 debug_camera_move_speed = 9.0f;
        DEBUG_WATCH(&debug_camera_move_speed);

        if (ignore_mouse==0 && ignore_keyboard==0) {
            gs_debug_camera.translate(real_dt * debug_camera_move_speed * (pc.move_input.z * forward + pc.move_input.x * right));
        }
        gs_debug_camera.translate(v3f{0.0f});
    }

   
    // local_persist f32 accum = 0.0f;
    // const u32 sub_steps = 1;
    // const f32 step = 1.0f/(60.0f*sub_steps);

    // accum += f32(dt > 0.0f) * std::min(dt, step*15);
    // if (dt > 0.0f)

    // for (size_t i{0}; i < world->entity_capacity; i++) {
    //     auto* e = world->entities + i;

    //     if (e->is_alive() == false) {
    //         continue;
    //     }

        // if (e->physics.rigidbody && accum > 0.0f) {
        //     // if (e->physics.rigidbody->type == physics::rigidbody_type::CHARACTER) {
        //         // continue;
        //     // }
            // auto* rb = e->physics.rigidbody;
        //     e->transform.origin = rb->position + rb->velocity * accum;
            // auto orientation = rb->orientation;
        //     orientation += (orientation * glm::quat(0.0f, rb->angular_velocity)) * (0.5f * accum);
        //     orientation = glm::normalize(orientation);
        //     // e->transform.set_rotation(orientation);
        // }

        // if(e->primary_weapon.entity) {
        //     auto forward = -e->primary_weapon.entity->transform.basis[2];
        // //     e->primary_weapon.entity->transform.set_rotation(glm::quatLookAt(forward, axis::up));
        // //     e->primary_weapon.entity->transform.set_scale(v3f{3.0f});
        //     e->primary_weapon.entity->transform.origin =
        //         e->global_transform().origin +
        //         forward + axis::up * 0.3f;
        // //         //  0.5f * (right + axis::up);
        // }
    // }

    
    {
        const auto r = game_state->sfx->fmod.system->update();
        assert(r == 0);
    }
    

    std::lock_guard lock{game_state->render_system->ticket};
    {
        // TIMED_BLOCK(GameplayLock);
        // lock.lock();
    }

    { // @debug
        auto* env = &game_state->render_system->environment_storage_buffer.pool[0];
        DEBUG_WATCH((v3f*)&env->fog_color);
        DEBUG_WATCH((v3f*)&env->ambient_color);
        DEBUG_WATCH(&env->fog_density);
        DEBUG_WATCH(&env->ambient_strength);
        DEBUG_WATCH(&env->contrast);
        DEBUG_WATCH((v3f*)&env->sun.color);
        DEBUG_WATCH((v3f*)&env->sun.direction);
    }

    TIMED_BLOCK(GameSubmitRenderJobs);
    // rendering::begin_frame(game_state->render_system);
    // game_state->debug.debug_vertices.pool.clear();

    
    world->camera.reset_priority();

        
    if (world->player) {
        // world->player->camera_controller.transform.origin = world->player->physics.rigidbody->position + 
        //     axis::up * world->player->camera_controller.head_height + axis::up * world->player->camera_controller.head_offset;
        // world->player->camera_controller.translate(v3f{0.0f});
    
        DEBUG_SET_FOCUS(world->player->global_transform().origin);
    }

    auto camera_position = world->camera.origin;
    auto camera_forward = world->camera.basis[2];
    
    rs->camera_pos = camera_position;
    rs->set_view(world->camera.inverse().to_matrix(), game_state->width(), game_state->height());

    // make sure not to allocate from buffer during sweep

    auto& forward_blend    = world->render_groups[0] = rendering::begin_render_group(rs, world->entity_capacity + 10'000);
    auto& forward_additive = world->render_groups[1] = rendering::begin_render_group(rs, world->entity_capacity + 128);
    forward_blend.push_command().set_blend(gfx::blend_mode::alpha_blend);
    forward_additive.push_command().set_blend(gfx::blend_mode::additive);

    // arena_begin_sweep(&world->render_system()->instance_storage_buffer.pool);
    // arena_begin_sweep(&world->particle_arena);

    for (size_t i{0}; i < world->entity_capacity; i++) {
        auto* e = world->entities + i;
        const bool is_not_renderable = !e->is_renderable();

        if (e->is_alive() == false) {
            continue;
        }

        if (is_not_renderable) {
            // zyy_warn("render", "Skipping: {} - {}", (void*)e, e->flags);
            continue;
        }

        for (u32 m = 0; m < e->gfx.gfx_entity_count; m++) {
            rendering::set_entity_material(rs, e->gfx.gfx_id + m, e->gfx.material_id);
        }

        if (e->gfx.buffer) {
            // arena_sweep_keep(&world->render_system()->instance_storage_buffer.pool, e->gfx.instance_end());
        }

        b32 is_particle = 0;
        b32 is_additive = 0;
        auto instance_count = e->gfx.instance_count();
        auto instance_offset = e->gfx.instance_offset();

        if (e->gfx.particle_system) {
            is_particle = 1;
            if (e->gfx.particle_system->flags & ParticleSettingsFlags_AdditiveBlend) {
                is_additive = 1;
            }
            auto* ps = e->gfx.particle_system;
            auto transform = e->global_transform();
            // arena_sweep_keep(&world->particle_arena, (std::byte*)(ps->particles + ps->max_count));
            if (dt != 0.0f) {
                particle_system_update(e->gfx.particle_system, transform, dt);
            }
            // particle_system_sort_view(
            //     e->gfx.particle_system, 
            //     transform,
            //     camera_position, camera_forward
            // );
            particle_system_build_colors(
                e->gfx.particle_system, 
                e->gfx.dynamic_color_instance_buffer, 
                e->gfx._instance_count
            );
            particle_system_build_matrices(
                e->gfx.particle_system, 
                transform,
                e->gfx.dynamic_instance_buffer, 
                e->gfx._instance_count
            );
            range_u32(gi, 0, e->gfx.gfx_entity_count) {
                rendering::set_entity_instance_data(rs, e->gfx.gfx_id + gi, e->gfx.instance_offset(0), ps->live_count);
            }
        }

        v3f size = e->aabb.size()*0.5f;
        // v4f bounds{e->aabb.center(), glm::max(glm::max(size.x, size.y), size.z) };

        // if (e->type == zyy::entity_type::player) continue;

        if (instance_count == 0) continue;

        // auto albedo_id = std::numeric_limits<u32>::max();

        // Todo(Zack): Add gfx setting for blending
        
        auto& draw_command = is_additive ? forward_additive.push_command() : forward_blend.push_command();
        draw_command.type = gfx::render_command_type::draw_mesh;
        auto& draw_mesh = draw_command.draw_mesh;
            draw_mesh.mesh_id = e->gfx.mesh_id;
            draw_mesh.material_id = e->gfx.material_id;
            draw_mesh.transform = e->global_transform().to_matrix();
            draw_mesh.gfx_id = e->gfx.gfx_id;
            draw_mesh.gfx_count = e->gfx.gfx_entity_count;
            draw_mesh.instance_count = instance_count;
            draw_mesh.instance_offset = instance_offset;
            draw_mesh.albedo_id = e->gfx.albedo_id;

        // rendering::submit_job(
        //     game_state->render_system, 
        //     e->gfx.mesh_id, 
        //     e->gfx.material_id, // todo make material per mesh
        //     e->global_transform().to_matrix(),
        //     e->gfx.gfx_id,
        //     e->gfx.gfx_entity_count,
        //     instance_count,
        //     e->gfx.instance_offset(),
        //     e->gfx.albedo_id
        // );
    }

    local_persist u32 show_light_probes;
    DEBUG_WATCH(&show_light_probes);

    if (show_light_probes) {
        auto* probes = &rs->probe_storage_buffer.pool[0];
        auto* probe_box = &rs->light_probes;
        auto& probe_settings = rs->light_probe_settings_buffer.pool[0];
        
        probe_box->aabb.min = probe_settings.aabb_min;
        probe_box->aabb.max = probe_settings.aabb_max;
        
        u32 probe_count = probe_settings.dim.x * probe_settings.dim.y * probe_settings.dim.z;
        u32 albedo_id = safe_truncate_u64(rs->texture_cache.get_id("white.png"));
        // auto& mesh_list = rendering::get_mesh(rs, "res/models/sphere.obj");
        auto mesh_id = rendering::get_mesh_id(rs, "res/models/sphere.obj");

        range_u32(i, 0, probe_count) {
            auto& p = probes[i];
            
            auto pos = rendering::lighting::probe_position(probe_box, &p, i);
            
            // local_persist u32 sphere_gfx_id = ~0ui32;
            // if (sphere_gfx_id == ~0ui32) {
            //     sphere_gfx_id = rendering::register_entity(rs);
            //     rendering::set_entity_material(rs, sphere_gfx_id, 0);
            //     rendering::initialize_entity(rs, sphere_gfx_id, mesh_list.meshes[0].vertex_start, mesh_list.meshes[0].index_start);
            //     rendering::set_entity_albedo(rs, sphere_gfx_id, albedo_id);
            // } 

            math::transform_t t{};
            t.origin = pos;
            t.set_scale(v3f{0.2f});
                
            auto& draw_command = forward_blend.push_command();
            draw_command.type = gfx::render_command_type::draw_mesh;
            auto& draw_mesh = draw_command.draw_mesh;
                draw_mesh.mesh_id = mesh_id;
                draw_mesh.material_id = 0;
                draw_mesh.transform = t.to_matrix();
                draw_mesh.gfx_id = 0;
                draw_mesh.gfx_count = 1;
                draw_mesh.instance_count = 1;
                draw_mesh.instance_offset = 0;
                draw_mesh.albedo_id = albedo_id;
                draw_mesh.rtx_on = 0;
        }
    }

    // arena_sweep_keep(&world->render_system()->instance_storage_buffer.pool, (std::byte*)(world->effects.blood_splats + world->effects.blood_splat_max));
    zyy::world_render_bloodsplat(world);

    rendering::end_render_group(rs, world->render_groups[0]);
    rendering::end_render_group(rs, world->render_groups[1]);

    if (!draw_entity_gui(game_state, game_state->gui.imgui)) {
        draw_gui(game_state->game_memory);
        draw_game_gui(game_state->game_memory);
        draw_worlds(game_state, game_state->gui.imgui);
                
#ifdef DEBUG_STATE
        auto& imgui = game_state->gui.imgui;
        DEBUG_STATE_DRAW(imgui, rs->projection, rs->view, rs->viewport());
        if (gs_show_watcher) {
            DEBUG_STATE_DRAW_WATCH_WINDOW(imgui);
        }

        if (gs_show_console == 1) {
            if (DEBUG_STATE.console->open_percent < 0.33f) {
                DEBUG_STATE.console->open_percent += imgui.ctx.input->dt;
            } else {
                DEBUG_STATE.console->open_percent = .33f;
            }
        } else if (gs_show_console == 2) {
            if (DEBUG_STATE.console->open_percent < 1.0f) {
                DEBUG_STATE.console->open_percent += imgui.ctx.input->dt;
            } else {
                DEBUG_STATE.console->open_percent = 1.0f;
            }
        } else {
            if (DEBUG_STATE.console->open_percent > 0.0f) {
                DEBUG_STATE.console->open_percent -= imgui.ctx.input->dt;
            } else {
                DEBUG_STATE.console->open_percent = 0.0f;
            }
        }
        if (DEBUG_STATE.console->open_percent > 0.0f) {
            local_persist v2f cpos = v2f{400.0, 0.0f};
            draw_console(imgui, DEBUG_STATE.console, &cpos);
        }
#endif


    }


    // game_state->render_system->ticket.unlock();
}

void
game_on_update(game_memory_t* game_memory) {
    game_state_t* game_state = get_game_state(game_memory);
    game_state->time += game_memory->input.dt * game_state->time_scale;

    auto* world_generator = game_state->game_world->world_generator;
    if (world_generator && world_generator->is_done() == false) {
        
        // Todo(Zack): add config setting to turn this on and off
        // try {
            world_generator->execute(game_state->game_world, [&](){draw_gui(game_memory);});
        // } catch ( std::exception& e) {
            // DEBUG_STATE.alert(e.what());
            // zyy_error("world_generator->execute", "Exception loading world: {}", e.what());
            // game_state->game_world->world_generator->force_completion();
        // }
        std::lock_guard lock{game_state->render_system->ticket};
        set_ui_textures(game_state);
        game_memory->input.keys[key_id::F9] = 1;
    // } else {
        // local_persist f32 accum = 0.0f;
        // const u32 sub_steps = 1;
        // const f32 step = 1.0f/(60.0f*sub_steps);

        // game_memory->input.dt *= game_state->time_scale;
        // accum += f32(game_memory->input.dt > 0.0f) * std::min(game_memory->input.dt * game_state->time_scale, step*15);
        // if (accum >= step) {
        //     accum -= step;
            // game_on_gameplay(game_state, &game_memory->input, step);
        // }
        
    }
    game_on_gameplay(game_state, &game_memory->input, game_memory->input.dt * game_state->time_scale);
}

inline static u32
wait_for_frame(game_state_t* game_state) {
    TIMED_FUNCTION;
    gfx::vul::state_t& vk_gfx = game_state->gfx;

    // vkDeviceWaitIdle(vk_gfx.device);

    return rendering::wait_for_frame(game_state->render_system);
}

inline static void
present_frame(game_state_t* game_state, VkCommandBuffer command_buffer, u32 image_index, u64 frame_count) {
    rendering::present_frame(game_state->render_system, image_index);
}

void
game_on_render(game_memory_t* game_memory, u32 image_index) { 
    TIMED_FUNCTION;
    
    game_state_t* game_state = get_game_state(game_memory);
    u64 frame_count = game_state->render_system->frame_count%2;

    gfx::vul::state_t& vk_gfx = game_state->gfx;

    game_state->scene.sporadic_buffer.time = game_state->input().time;
    *vk_gfx.sporadic_uniform_buffer.data = game_state->scene.sporadic_buffer;
    vk_gfx.sporadic_uniform_buffer.data->lighting_info = game_state->graphics_config.ddgi != 0 ? gfx::vul::sporadic_light_info::light_probe : 0;

#if 0
    {
        auto* rs = game_state->render_system;
        auto& command_buffer = vk_gfx.compute_command_buffer[frame_count%2];

        vkResetCommandBuffer(command_buffer, 0);
        auto command_buffer_begin_info = gfx::vul::utl::command_buffer_begin_info();

        VK_OK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

        if (vk_gfx.compute_index != vk_gfx.graphics_index) {
            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                rs->light_probes.irradiance_texture.image,
                0,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
                vk_gfx.graphics_index,
                vk_gfx.compute_index
            );
            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                rs->light_probes.visibility_texture.image,
                0,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
                vk_gfx.graphics_index,
                vk_gfx.compute_index
            );
        }

        rendering::begin_rt_pass(game_state->render_system, command_buffer, frame_count);

        if (vk_gfx.compute_index != vk_gfx.graphics_index) {
            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                rs->light_probes.irradiance_texture.image,
                VK_ACCESS_SHADER_WRITE_BIT,
                0,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
                vk_gfx.compute_index,
                vk_gfx.graphics_index
            );
            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                rs->light_probes.visibility_texture.image,
                VK_ACCESS_SHADER_WRITE_BIT,
                0,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
                vk_gfx.compute_index,
                vk_gfx.graphics_index
            );
        }

        VK_OK(vkEndCommandBuffer(command_buffer));

        auto& queue = vk_gfx.compute_queue;
        auto& device = vk_gfx.device;
        auto& compute_fence = vk_gfx.compute_fence[frame_count%2];

        vkWaitForFences(device, 1, &compute_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &compute_fence);

        VkSubmitInfo csi = gfx::vul::utl::submit_info();
        csi.commandBufferCount = 1;
        csi.pCommandBuffers = &command_buffer;

        VK_OK(vkQueueSubmit(queue, 1, &csi, compute_fence));
    }
#endif

    {
        auto* rs = game_state->render_system;
        auto command_buffer = rendering::begin_commands(rs);
        
        auto& khr = vk_gfx.khr;
        auto& ext = vk_gfx.ext;
        rs->get_frame_data().mesh_pass.object_descriptors = VK_NULL_HANDLE;
        
        // rendering::memory_barriers(rs, command_buffer);
        const u64 gui_frame = (game_state->gui.ctx.frame&1);
        auto& gui_vertices = game_state->gui.vertices[gui_frame];
        auto& gui_indices = game_state->gui.indices[gui_frame];
        auto& imgui = game_state->gui.imgui;
        // gui_vertices.insert_memory_barrier(command_buffer);
        // gui_indices.insert_memory_barrier(command_buffer);
        
        {
            if (game_state->graphics_config.ddgi == ddgi_mode::realtime) {
                rendering::begin_rt_pass(game_state->render_system, command_buffer);
            }
        
            VkBuffer buffers[1] = { game_state->render_system->scene_context->vertices.buffer };
            VkDeviceSize offsets[1] = { 0 };

            gfx::vul::begin_debug_marker(command_buffer, "Forward Sky");

            vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);

            vkCmdBindIndexBuffer(command_buffer,
                game_state->render_system->scene_context->indices.buffer, 0, VK_INDEX_TYPE_UINT32
            );

            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                rs->frame_images[frame_count%2].texture.image,
                0,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
            
            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                rs->frame_images[4 + (frame_count%2)].texture.image,
                0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });

            {
                // New structures are used to define the attachments used in dynamic rendering
                VkRenderingAttachmentInfoKHR colorAttachment{};
                colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
                // colorAttachment.imageView = vk_gfx.swap_chain_image_views[image_index];
                colorAttachment.imageView = rs->frame_images[frame_count%2].texture.image_view;
                colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.clearValue.color = { 0.0f,0.0f,0.0f,0.0f };

                // A single depth stencil attachment info can be used, but they can also be specified separately.
                // When both are specified separately, the only requirement is that the image view is identical.			
                VkRenderingAttachmentInfoKHR depthStencilAttachment{};
                depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
                depthStencilAttachment.imageView = rs->frame_images[4+(frame_count%2)].texture.image_view;
                // depthStencilAttachment.imageView = vk_gfx.depth_stencil_texture.image_view;
                depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };

                VkRenderingInfoKHR renderingInfo{};
                renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
                renderingInfo.renderArea = { 0, 0, rs->width, rs->height};
                renderingInfo.layerCount = 1;
                renderingInfo.colorAttachmentCount = 1;
                renderingInfo.pColorAttachments = &colorAttachment;
                renderingInfo.pDepthAttachment = &depthStencilAttachment;
                renderingInfo.pStencilAttachment = &depthStencilAttachment;
            
                khr.vkCmdBeginRenderingKHR(command_buffer, &renderingInfo);
            }
            VkColorBlendEquationEXT blend_fn[1];
            blend_fn[0] = gfx::vul::utl::alpha_blending();
            // blend_fn[0] = gfx::vul::utl::additive_blending();
            ext.vkCmdSetColorBlendEquationEXT(command_buffer, 0, 1, blend_fn);
            // VkBool32 fb_blend[1] { false };
            VkBool32 fb_blend[1] { true };
            ext.vkCmdSetColorBlendEnableEXT(command_buffer,0, 1, fb_blend);

            auto view_dir = glm::inverse(rs->view) * v4f{axis::forward, 0.0f};
            rendering::draw_skybox(
                rs, command_buffer, 
                // assets::shaders::voidsky_frag.filename,
                assets::shaders::skybox_frag.filename,
                view_dir, rs->environment_storage_buffer.pool[0].sun.direction
            );

            gfx::vul::end_debug_marker(command_buffer);
            gfx::vul::begin_debug_marker(command_buffer, "Forward ");

            auto viewport = gfx::vul::utl::viewport((f32)rs->width, (f32)rs->height, 0.0f, 1.0f);
            auto scissor = gfx::vul::utl::rect2D(rs->width, rs->height, 0, 0);

            ext.vkCmdSetViewportWithCountEXT(command_buffer, 1, &viewport);
            ext.vkCmdSetScissorWithCountEXT(command_buffer, 1, &scissor);
            ext.vkCmdSetCullModeEXT(command_buffer, gs_cull_modes[gs_cull_mode]);
            ext.vkCmdSetPolygonModeEXT(command_buffer, gs_poly_modes[gs_poly_mode]);
            ext.vkCmdSetFrontFaceEXT(command_buffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            ext.vkCmdSetDepthTestEnableEXT(command_buffer, VK_TRUE);
            ext.vkCmdSetDepthWriteEnableEXT(command_buffer, VK_TRUE);
            ext.vkCmdSetDepthCompareOpEXT(command_buffer, VK_COMPARE_OP_LESS_OR_EQUAL);
            ext.vkCmdSetPrimitiveTopologyEXT(command_buffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            // ext.vkCmdSetLogicOpEnableEXT(command_buffer, VK_FALSE);

            {
                VkVertexInputBindingDescription2EXT vertexInputBinding{};
                vertexInputBinding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
                vertexInputBinding.binding = 0;
                vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                vertexInputBinding.stride = sizeof(gfx::vertex_t);
                vertexInputBinding.divisor = 1;

                VkVertexInputAttributeDescription2EXT vertexAttributes[] = {
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::vertex_t, pos) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::vertex_t, nrm) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::vertex_t, col) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(gfx::vertex_t, tex) }
                };

                ext.vkCmdSetVertexInputEXT(command_buffer, 1, &vertexInputBinding, array_count(vertexAttributes), vertexAttributes);
            }

            build_shader_commands(rs, command_buffer);
            khr.vkCmdEndRenderingKHR(command_buffer);

            gfx::vul::end_debug_marker(command_buffer);

        }
    
        { // UI START
           
            rendering::draw_bloom(rs, command_buffer, &rs->frame_images[frame_count%2].texture);

            gfx::vul::begin_debug_marker(command_buffer, "Post Process Pass");

            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                vk_gfx.swap_chain_images[image_index],
                0,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
            
            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                vk_gfx.depth_stencil_texture.image,
                0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                // VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });

            // New structures are used to define the attachments used in dynamic rendering
            VkRenderingAttachmentInfoKHR colorAttachment{};
            colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            colorAttachment.imageView = vk_gfx.swap_chain_image_views[image_index];
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.clearValue.color = { 0.0f,0.0f,0.0f,0.0f };

            // A single depth stencil attachment info can be used, but they can also be specified separately.
            // When both are specified separately, the only requirement is that the image view is identical.			
            VkRenderingAttachmentInfoKHR depthStencilAttachment{};
            depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            depthStencilAttachment.imageView = vk_gfx.depth_stencil_texture.image_view;
            depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };

            VkRenderingInfoKHR renderingInfo{};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = { 0, 0, rs->width, rs->height };
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;
            renderingInfo.pDepthAttachment = &depthStencilAttachment;
            renderingInfo.pStencilAttachment = &depthStencilAttachment;

            khr.vkCmdBeginRenderingKHR(command_buffer, &renderingInfo);
        }
        {
            ext.vkCmdSetCullModeEXT(command_buffer, VK_CULL_MODE_NONE);
            ext.vkCmdSetFrontFaceEXT(command_buffer, VK_FRONT_FACE_CLOCKWISE);
            ext.vkCmdSetPolygonModeEXT(command_buffer, gs_poly_modes[0]);

            ext.vkCmdSetDepthTestEnableEXT(command_buffer, VK_FALSE);
            ext.vkCmdSetDepthWriteEnableEXT(command_buffer, VK_FALSE);

            {   // tonemap
                rendering::pp_material_t parameters = rs->postprocess_params;
                
                rendering::draw_postprocess(
                    rs, command_buffer,
                    assets::shaders::tonemap_frag.filename,
                    // gs_rtx_on && 0 ?
                    // &rs->frame_images[6].texture :
                    &rs->frame_images[frame_count%2].texture,
                    parameters
                );
            }
            gfx::vul::end_debug_marker(command_buffer);
            gfx::vul::begin_debug_marker(command_buffer, "UI Pass");

            VkColorBlendEquationEXT blend_fn[1];
            blend_fn[0] = gfx::vul::utl::alpha_blending();
            VkBool32 fb_blend[1] { true };
            ext.vkCmdSetColorBlendEquationEXT(command_buffer, 0, 1, blend_fn);
            ext.vkCmdSetColorBlendEnableEXT(command_buffer,0, 1, fb_blend);
            
            ext.vkCmdSetDepthTestEnableEXT(command_buffer, VK_FALSE);
            ext.vkCmdSetDepthWriteEnableEXT(command_buffer, VK_FALSE);

            vkCmdBindDescriptorSets(command_buffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, game_state->render_system->pipelines.gui.layout,
                0, 1, &game_state->ui_descriptor, 0, nullptr);

            {
                struct gui_pc_t {
                    m44 v;
                    m44 p;
                } constants;
                constants.v = rs->view;
                constants.p = rs->projection;
                vkCmdPushConstants(command_buffer, game_state->render_system->pipelines.gui.layout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                    0, sizeof(constants), &constants
                );
            }

            {
                VkVertexInputBindingDescription2EXT vertexInputBinding{};
                vertexInputBinding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
                vertexInputBinding.binding = 0;
                vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                vertexInputBinding.stride = sizeof(gfx::gui::vertex_t);
                vertexInputBinding.divisor = 1;

                VkVertexInputAttributeDescription2EXT vertexAttributes[] = {
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::gui::vertex_t, pos) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(gfx::gui::vertex_t, tex) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 2, 0, VK_FORMAT_R32_UINT, offsetof(gfx::gui::vertex_t, nrm) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 3, 0, VK_FORMAT_R32_UINT, offsetof(gfx::gui::vertex_t, img) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 4, 0, VK_FORMAT_R32_UINT, offsetof(gfx::gui::vertex_t, col) },
                };

                ext.vkCmdSetVertexInputEXT(command_buffer, 1, &vertexInputBinding, array_count(vertexAttributes), vertexAttributes);
            }
            
            VkDeviceSize offsets[1] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, &gui_vertices.buffer, offsets);
            vkCmdBindIndexBuffer(command_buffer, gui_indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            VkShaderEXT gui_shaders[2] {
                *rs->shader_cache.get("./res/shaders/bin/gui.vert.spv"),
                *rs->shader_cache.get("./res/shaders/bin/gui.frag.spv"),
            };
            assert(gui_shaders[0] != VK_NULL_HANDLE);
            assert(gui_shaders[1] != VK_NULL_HANDLE);

            VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
            ext.vkCmdBindShadersEXT(command_buffer, 2, stages, gui_shaders);

            for (auto* draw_command = imgui.commands.next;
                draw_command != &imgui.commands;
                node_next(draw_command)
            ) {
                if (draw_command->type == gfx::gui::draw_command_t::draw_type::draw) {
                    vkCmdDrawIndexed(command_buffer,
                        draw_command->draw.index_count,
                        1, // instance count
                        draw_command->draw.index_start, // first index
                        0, //draw_command->draw.vertex_start, // first vertex
                        0  // first instance
                    );
                } else {
                    auto scissor = gfx::vul::utl::rect2D(draw_command->clip);
                    ext.vkCmdSetScissorWithCountEXT(command_buffer, 1, &scissor);
                }
            }

            for (auto* panel = imgui.panels.next;
                panel != &imgui.panels;
                node_next(panel)
            ) {
                if (panel->name == imgui.active.id) {
                    dlist_remove(panel);
                    dlist_insert_as_last(&imgui.panels, panel);
                    break;
                }
            }
            for (auto* panel = imgui.panels.next;
                panel != &imgui.panels;
                node_next(panel)
            ) {
                for (auto* draw_command = panel->draw_commands.next;
                    draw_command != &panel->draw_commands;
                    node_next(draw_command)
                ) {
                    if (draw_command->type == gfx::gui::draw_command_t::draw_type::draw) {
                        vkCmdDrawIndexed(command_buffer,
                            draw_command->draw.index_count,
                            1, // instance count
                            draw_command->draw.index_start, // first index
                            0, //draw_command->draw.vertex_start, // first vertex
                            0  // first instance
                        );
                    } else {
                        auto scissor = gfx::vul::utl::rect2D(draw_command->clip);
                        ext.vkCmdSetScissorWithCountEXT(command_buffer, 1, &scissor);
                    }
                }
            }

            // vkCmdDrawIndexed(command_buffer,
            //     (u32)gui_indices.pool.count(),
            //     1, // instance count
            //     0, // first index
            //     0, // first vertex
            //     0  // first instance
            // );
        }

        khr.vkCmdEndRenderingKHR(command_buffer);
        gfx::vul::end_debug_marker(command_buffer);

        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            vk_gfx.swap_chain_images[image_index],
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
        
        VK_OK(vkEndCommandBuffer(command_buffer));
        present_frame(game_state, command_buffer, image_index, frame_count);
    }
}


void
main_menu_on_update(game_memory_t* game_memory) {
}

global_variable u64 scene_state = 1;



inline entity_editor_t* get_entity_editor(game_memory_t* game_state) {
    local_persist entity_editor_t ee{get_game_state(game_state)};
    return &ee;
}

export_fn(void) 
app_on_render(game_memory_t* game_memory) {
    return;
    auto* game_state = get_game_state(game_memory);
    // local_persist u32 frame_count = 0;
    // if (frame_count < 3) {
    //     zyy_info("frame", "Frame: {}", frame_count);
    // }
    // std::lock_guard lock{game_state->render_system->ticket};
    switch (scene_state) {
        case 0:{
    
            // std::lock_guard lock{game_state->render_system->ticket};
            // entity_editor_render(get_entity_editor(game_memory));
            // u32 image_index = wait_for_frame(game_state);
            // game_on_render(game_memory, image_index);
        }   break;
        case 1:{
            // Game           
    
            // u32 image_index = wait_for_frame(game_state);
            // std::lock_guard lock{game_state->render_system->ticket};

            // game_state->render_system->camera_pos = game_state->game_world->camera.origin;
            // game_state->render_system->set_view(game_state->game_world->camera.inverse().to_matrix(), game_state->width(), game_state->height());

            // game_on_render(game_memory, image_index);
        
        }   break;
        default:
            zyy_warn("scene::render", "Unknown scene: {}", scene_state);
            break;
        //     scene_state = 1;
    }
}

export_fn(void) 
app_on_update(game_memory_t* game_memory) {
    auto* game_state = get_game_state(game_memory);

    if (game_memory->input.keys[key_id::LEFT_CONTROL] && 
        game_memory->input.pressed.keys[key_id::TAB]) { 
    // if (game_memory->input.pressed.keys[key_id::F3] || 
    //     game_memory->input.gamepads->buttons[button_id::dpad_down].is_pressed) {
        scene_state = !scene_state;
        return;
    }

    u32 image_index = wait_for_frame(game_state);
    rendering::begin_frame(game_state->render_system);

    begin_gui(game_state);

    switch (scene_state) {
        case 0: // Editor
            entity_editor_update(get_entity_editor(game_memory));
            break;
        case 1: { // Game
            game_on_update(game_memory);
        }         
            break;
        default:
            zyy_warn("scene::update", "Unknown scene: {}", scene_state);
            scene_state = 1;
            break;
    }

    game_on_render(game_memory, image_index);
    game_state->render_system->frame_count++;
    game_state->render_system->frame_count = game_state->render_system->frame_count % 2;

#ifdef DEBUG_STATE
    if (gs_debug_state) {
        DEBUG_STATE.begin_frame();
    }
#endif
}
