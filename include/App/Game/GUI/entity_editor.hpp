#ifndef GUI_ENTITY_EDITOR_HPP
#define GUI_ENTITY_EDITOR_HPP

#include "core.hpp"

#include "App/Game/Entity/entity.hpp"
#include "App/Game/Entity/entity_db.hpp"

#include "App/app.hpp"

struct entity_editor_t {
    app_t* app{0};
    gfx::gui::im::state_t imgui;

    m44 projection{1.0f};
    game::cam::orbit_camera_t camera{};

    v3f* selection{0};

    game::db::entity_def_t entity;

    entity_editor_t(app_t* app_)
    : app{app_}, imgui{
        .ctx = app->gui.ctx,
        .theme = gfx::gui::theme_t {
            .fg_color = gfx::color::rgba::gray,
            .bg_color = gfx::color::rgba::black,
            .text_color = gfx::color::rgba::cream,
            .disabled_color = gfx::color::rgba::dark_gray,
            .border_color = gfx::color::rgba::white,

            .padding = 4.0f,
        },
    }
    {}
};

inline static void
entity_editor_update(entity_editor_t* ee) {
    auto* app = ee->app;

    game::rendering::begin_frame(app->render_system);
}

inline static void
entity_editor_render(entity_editor_t* ee) {
    using namespace gfx::gui;
    using namespace std::string_view_literals;

    local_persist u64 frame{0}; frame++;
    local_persist bool show_load = false;
    local_persist bool show_save = false;
    local_persist bool show_graphics = false;
    local_persist bool show_physics = false;

    auto* app = ee->app;
    auto& imgui = ee->imgui;
    gfx::gui::ctx_clear(&app->gui.ctx, &app->gui.vertices[frame&1].pool, &app->gui.indices[frame&1].pool);

    if (im::begin_panel(imgui, "EE")) {
        im::text(imgui, "Entity Editor"sv);
        im::text(imgui, "============================="sv);
        im::same_line(imgui);
        if (im::text(imgui, "Load"sv, &show_load)) {
            show_save = false;
        }
        if (im::text(imgui, "  Save"sv, &show_save)) {
            show_load = false;
        }
        if (show_load || show_save) {
            im::text(imgui, "============================="sv);
            im::text(imgui, show_load ? "Loading"sv : "Saving"sv);

            local_persist size_t pos=0;
            local_persist char file_name[512];
            std::string_view file_view{file_name};

            im::same_line(imgui);
            im::text(imgui, "File Name: "sv);
            im::text_edit(imgui, std::string_view{file_name}, &pos, "file_name::edit"_sid);

            im::same_line(imgui);
            if (im::text(imgui, "Accept"sv)) {
                if (show_load) {
                    ee->entity = game::db::load_from_file(app->main_arena, fmt_sv("./assets/entity/{}", file_view));
                } else { // show_save
                    std::ofstream file{fmt_str("./assets/entity/{}", file_view), std::ios::binary};
                    file.write((const char*)&ee->entity, sizeof(game::db::entity_def_t));
                }
                show_save = show_load = false;
            }
            if (im::text(imgui, "  Cancel"sv)) {
                show_save = show_load = false;
            }
        }

        im::text(imgui, "============================="sv);
        
        size_t type_pos=std::strlen(ee->entity.type_name);
        size_t mesh_pos=std::strlen(ee->entity.gfx.mesh_name);
        
        im::same_line(imgui);
        im::text(imgui, "Type Name: \t"sv);
        im::text_edit(imgui, std::string_view{ee->entity.type_name}, &type_pos, "type_name::edit"_sid);

        std::string_view types[]{
            "environment",
            "player",
            "enemy",
            "weapon",
            "weapon_part",
            "item",
            "trigger",
        };

        if (im::text(imgui, fmt_sv("Type: {}"sv, types[(u32)ee->entity.type]))) {
            ee->entity.type = (game::entity_type)(((u32)ee->entity.type + 1) % (u32)game::entity_type::SIZE);
        }
  
        if (im::text(imgui, "Graphics"sv, &show_graphics)) {
            im::same_line(imgui);
            im::text(imgui, "-- Mesh Name: \t"sv);
            im::text_edit(imgui, std::string_view{ee->entity.gfx.mesh_name}, &mesh_pos, "mesh_name::edit"_sid);
            
            im::text(imgui, fmt_sv("-- Albedo Name: {}"sv, ee->entity.gfx.albedo_tex.sv()));
            im::text(imgui, fmt_sv("-- Normal Name: {}"sv, ee->entity.gfx.normal_tex.sv()));
            im::text(imgui, fmt_sv("-- Anim Name: {}"sv, ee->entity.gfx.animations.sv()));
        }
        
        if (im::text(imgui, "Physics"sv, &show_physics)) {
            if (!ee->entity.physics) {
                ee->entity.physics.emplace(game::db::entity_def_t::physics_t{});
            }
            if (im::text(imgui, "Clear"sv)) {
                ee->entity.physics.reset();
                show_physics = false;
            } else {
                std::string_view physics_flags[]{
                    "PhysicsEntityFlags_Static",
                    "PhysicsEntityFlags_Dynamic",
                    "PhysicsEntityFlags_Trigger",
                    "PhysicsEntityFlags_Character"
                };
                u64 flag = ee->entity.physics->flags;
                im::bitflag(imgui, std::span{physics_flags}, &flag);
                ee->entity.physics->flags = (u32)flag;
            }
        }

        im::end_panel(imgui);
    }

    if (ee->selection) {
        im::gizmo(imgui, ee->selection, ee->projection * ee->camera.view());
    }

    local_persist viewport_t viewport{};
    viewport.images[0] = 4;
    viewport.images[1] = 1;
    viewport.images[2] = 1;
    viewport.images[3] = 2;

    draw_viewport(imgui, &viewport);
}


#endif