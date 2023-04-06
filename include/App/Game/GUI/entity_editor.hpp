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
    
    auto* app = ee->app;
    auto& imgui = ee->imgui;
    gfx::gui::ctx_clear(&app->gui.ctx, &app->gui.vertices[frame&1].pool, &app->gui.indices[frame&1].pool);

    if (im::begin_panel(imgui, "EE")) {
        im::text(imgui, "Entity Editor"sv);

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