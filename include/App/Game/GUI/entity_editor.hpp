#ifndef GUI_ENTITY_EDITOR_HPP
#define GUI_ENTITY_EDITOR_HPP

#include "zyy_core.hpp"

#include "App/Game/Entity/entity.hpp"
#include "App/Game/Entity/entity_db.hpp"

#include "App/game_state.hpp"

enum struct edit_type_t {
    TEXT, VEC3, 
};

enum edit_change_type {
    Edit_From = 0,
    Edit_To = 1,
    Edit_Count
};

struct text_edit_t {
    char* where{0};
    char change[Edit_Count];
};

struct vec3_edit_t {
    v3f* where{0};
    v3f change[Edit_Count];
};

void execute_edit(auto& edit, u32 which) {
    *edit.where = edit.change[which];
}

struct edit_event_t {
    edit_event_t* next{0};
    edit_event_t* prev{0};

    edit_type_t type{};
    union {
        text_edit_t text_edit{};   
        vec3_edit_t vec3_edit;   
    };
};

struct entity_editor_t {
    game_state_t* game_state{0};
    gfx::gui::im::state_t imgui;

    arena_t undo_arena{};

    edit_event_t redo_sentinel{};
    edit_event_t undo_sentinel{};

    void redo() {
        auto* cmd = redo_sentinel.next;
        if (cmd != &redo_sentinel) {
            dlist_remove(cmd);
            // todo: Execute the cmd
            switch (cmd->type)
            {
            case edit_type_t::TEXT:
                execute_edit(cmd->text_edit, Edit_To);
                break;
            case edit_type_t::VEC3:
                execute_edit(cmd->vec3_edit, Edit_To);
                break;
            case_invalid_default;
            }
            dlist_insert(&undo_sentinel, cmd);
        }
    }

    void undo() {
        auto* cmd = undo_sentinel.next;
        if (cmd != &undo_sentinel) {
            dlist_remove(cmd);
            // todo: Execute the cmd
            switch (cmd->type)
            {
            case edit_type_t::TEXT:
                execute_edit(cmd->text_edit, Edit_From);
                break;
            case edit_type_t::VEC3:
                execute_edit(cmd->vec3_edit, Edit_From);
                break;
            case_invalid_default;
            }
            dlist_insert(&redo_sentinel, cmd);
        }
    }

    void edit_char(char* where, char what) {
        auto* edit = push_struct<edit_event_t>(&undo_arena);
        edit->type = edit_type_t::TEXT;
        edit->text_edit.where = where;
        edit->text_edit.change[Edit_From] = *where;
        edit->text_edit.change[Edit_To] = what;

        execute_edit(edit->text_edit, Edit_To);
        dlist_insert(&undo_sentinel, edit);
    }

    m44 projection{1.0f};
    zyy::cam::orbit_camera_t camera{};

    v3f* selection{0};

    zyy::db::prefab_t entity;

    zyy::world_t* editor_world{0};

    entity_editor_t(game_state_t* app_)
    : game_state{app_}, imgui{
        .ctx = game_state->gui.ctx,
        .theme = gfx::gui::theme_t {
            .fg_color = gfx::color::rgba::gray,
            .bg_color = gfx::color::rgba::black,
            .text_color = gfx::color::rgba::cream,
            .disabled_color = gfx::color::rgba::dark_gray,
            .border_color = gfx::color::rgba::white,

            .padding = 4.0f,
        },
    }
    {
        undo_arena = arena_sub_arena(&game_state->main_arena, megabytes(1));
        dlist_init(&redo_sentinel);
        dlist_init(&undo_sentinel);
        editor_world = zyy::world_init(&game_state->main_arena, game_state, game_state->game_memory->physics);
    }
};


REFLECT_TYPE(std::string_view) {
    REFLECT_TYPE_INFO(std::string_view)
    };
};

inline static void
object_data_gui(gfx::gui::im::state_t& imgui, reflect::type_t type, std::byte* data, const char* prop_name = 0, size_t depth = 0) {
    using namespace gfx::gui;
    char depth_str[32];
    utl::memzero(depth_str, 32);
    std::memset(depth_str, '-', depth?depth-1:0);

    auto show_property = type.get_property("show");
    bool show = true;

    if (show_property.valid()) {
        show_property.get_value_raw(data, (std::byte*)&show);
        if (im::text(imgui, fmt_sv("{} {}: {}", depth_str, type.name, prop_name?prop_name:""))) {
            show = !show;
            show_property.set_value_raw(data, show);
        }
    } else {
        im::text(imgui, fmt_sv("{} {}: {}", depth_str, type.name, prop_name?prop_name:""));
    }

    if (!show) return;
    if (type == reflect::type<f32>::info) {
        im::indent(imgui, v2f{depth*28.0f,0.0f});
        im::float_slider(imgui, (f32*)data);
    } else if (type == reflect::type<u64>::info) {
        im::text(imgui, fmt_sv("{}", *(u64*)data));
    } else if (type == reflect::type<v3f>::info) {
        const auto theme = imgui.theme;

        imgui.theme.fg_color = gfx::color::rgba::red;
        im::same_line(imgui);
        im::indent(imgui, v2f{depth*28.0f,0.0f});
        im::float_slider(imgui, (f32*)data);

        imgui.theme.fg_color = gfx::color::rgba::green;
        im::same_line(imgui);
        im::indent(imgui, v2f{depth*28.0f,0.0f});
        im::indent(imgui, v2f{depth*28.0f,0.0f});
        im::float_slider(imgui, (f32*)data+1);

        imgui.theme.fg_color = gfx::color::rgba::blue;
        im::indent(imgui, v2f{depth*28.0f,0.0f});
        im::indent(imgui, v2f{depth*28.0f,0.0f});
        im::indent(imgui, v2f{depth*28.0f,0.0f});
        im::float_slider(imgui, (f32*)data+2);

        imgui.theme = theme;
    } else 
    for(auto& p: std::span{type.properties, type.property_count}) {
        if (p.type) {
            object_data_gui(imgui, *p.type, data + p.offset, p.name.data(), depth + 2);
        } else {
            im::text(imgui, "Unregistered type");
        }
    }
}

inline static void
object_gui(gfx::gui::im::state_t& imgui, auto& value, size_t depth = 0) {
    const auto type = reflect::type<std::remove_reference<decltype(value)>::type>::info;
            
    object_data_gui(imgui, type, (std::byte*)&value, 0, depth+2);
}


inline static void
entity_editor_render(entity_editor_t* ee) {
    TIMED_FUNCTION;
    using namespace gfx::gui;
    using namespace std::string_view_literals;

    const u64 frame{++ee->game_state->gui.ctx.frame};
    
    local_persist char mesh_name[512];
    local_persist bool show_load = false;
    local_persist bool show_save = false;
    local_persist bool show_graphics = false;
    local_persist bool show_physics = false;


    auto* game_state = ee->game_state;
    auto& imgui = ee->imgui;
    const auto width = game_state->gui.ctx.screen_size.x;
    gfx::gui::ctx_clear(&game_state->gui.ctx, &game_state->gui.vertices[frame&1].pool, &game_state->gui.indices[frame&1].pool);


    bool ctrl_held = game_state->input().keys[key_id::LEFT_CONTROL] || game_state->input().keys[key_id::RIGHT_CONTROL];
    bool shift_held = game_state->input().keys[key_id::LEFT_SHIFT] || game_state->input().keys[key_id::RIGHT_SHIFT];
    if (ctrl_held && game_state->input().pressed.keys[key_id::Z]) {
        game_state->input().pressed.keys[key_id::Z] = 0;
        if (shift_held) {
            ee->redo();
        } else {
            ee->undo();
        }
    }

    if (im::begin_panel(imgui, "EE", v2f{width/3.0f,0.0f} )) {
        im::text(imgui, "Entity Editor"sv);
        im::text(imgui, "============================="sv);
        im::same_line(imgui);
        if (im::text(imgui, "Load"sv, &show_load)) {
            show_save = false;
        }
        im::same_line(imgui);
        if (im::text(imgui, "  Save"sv, &show_save)) {
            show_load = false;
        }
        if (im::text(imgui, "  Clear"sv)) {
            ee->entity = {};
            utl::memzero(mesh_name, array_count(mesh_name));
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
                    ee->entity = zyy::db::load_from_file(game_state->main_arena, fmt_sv("./res/entity/{}", file_view));
                } else { // show_save
                    std::ofstream file{fmt_str("./res/entity/{}", file_view), std::ios::binary};
                    file.write((const char*)&ee->entity, sizeof(zyy::db::prefab_t));
                }
                show_save = show_load = false;
            }
            if (im::text(imgui, "  Cancel"sv)) {
                show_save = show_load = false;
            }
        }

        im::text(imgui, "============================="sv);
        
        
        std::string_view mesh_view{mesh_name};
        size_t type_pos=std::strlen(ee->entity.type_name);
        size_t mesh_pos=std::strlen(mesh_name);
        
        im::same_line(imgui);
        im::text(imgui, "Type Name: \t"sv);
        im::text_edit(imgui, std::string_view{ee->entity.type_name}, &type_pos, "type_name::edit"_sid);

        std::string_view types[]{
            "environment",
            "player",
            "bad",
            "weapon",
            "weapon_part",
            "item",
            "trigger",
            "effect",
            "SIZE",
        };

        if (im::text(imgui, fmt_sv("Type: {}"sv, types[(u32)ee->entity.type]))) {
            ee->entity.type = (zyy::entity_type)(((u32)ee->entity.type + 1) % (u32)zyy::entity_type::SIZE);
        }
  
        if (im::text(imgui, "Graphics"sv, &show_graphics)) {
            im::same_line(imgui);
            im::text(imgui, "-- Mesh Name: \t"sv);
            if (im::text_edit(imgui, mesh_view, &mesh_pos, "mesh_name::edit"_sid)) {
                utl::memzero(ee->entity.gfx.mesh_name, array_count(ee->entity.gfx.mesh_name));
                for (u64 i = 0; i < mesh_pos; i++) {
                    ee->edit_char(ee->entity.gfx.mesh_name + i, mesh_name[i]);
                    ee->edit_char(mesh_name + i, 0);
                }
                // utl::copy(ee->entity.gfx.mesh_name, mesh_name, mesh_pos);
            }
            
            im::text(imgui, fmt_sv("-- Albedo Name: {}"sv, ee->entity.gfx.albedo_texture));
            im::text(imgui, fmt_sv("-- Normal Name: {}"sv, ee->entity.gfx.normal_texture));
            im::text(imgui, fmt_sv("-- Anim Name: {}"sv, ee->entity.gfx.animations));
        }
        
        if (im::text(imgui, "Physics"sv, &show_physics)) {
            if (!ee->entity.physics) {
                ee->entity.physics.emplace(zyy::db::prefab_t::physics_t{});
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

        im::text(imgui, "============================");
        {
            u64 redo_count = 0;
            u64 undo_count = 0;
            for(auto* u = ee->undo_sentinel.next;
                u != &ee->undo_sentinel;
                u = u->next
            ) {
                undo_count++;
            }
            for(auto* r = ee->redo_sentinel.next;
                r != &ee->redo_sentinel;
                r = r->next
            ) {
                redo_count++;
            }

            im::text(imgui, fmt_sv("Undo Count: {}", undo_count));
            im::text(imgui, fmt_sv("Redo Count: {}", redo_count));
        }

        im::text(imgui, fmt_sv("Camera: {}", ee->camera.position));

        // local_persist shader_1_t s1;
        // local_persist shader_2_t s2{.name="test"};
        // object_gui(imgui, s1);
        // object_gui(imgui, s2);


        im::end_panel(imgui);
    }

    if (ee->selection) {
        im::gizmo(imgui, ee->selection, ee->projection * ee->camera.view());
    }

}

inline static void
entity_editor_update(entity_editor_t* ee) {
    auto* game_state = ee->game_state;
    auto* pack_file = game_state->resource_file;
    auto dt = game_state->input().dt;

    auto& camera = ee->camera;
    auto* input = &game_state->input();

    auto pc = input->gamepads[0].is_connected ? gamepad_controller(input) : keyboard_controller(input);

    camera.move(dt * camera.forward() * pc.move_input.z);
    camera.move(dt * camera.right() * pc.move_input.x);
    camera.move(dt * axis::up * pc.move_input.y);

    game_state->render_system->set_view(camera.view(), (u32)game_state->width(), (u32)game_state->height());


    if (utl::res::pack_file_get_file_by_name(pack_file, ee->entity.gfx.mesh_name))
    {
        auto* e = zyy::spawn(ee->editor_world, game_state->render_system, ee->entity);
        v3f size = e->aabb.size()*0.5f;
        v4f bounds{e->aabb.center(), glm::max(glm::max(size.x, size.y), size.z) };

        rendering::submit_job(
            game_state->render_system, 
            e->gfx.mesh_id, 
            e->gfx.material_id, // todo make material per mesh
            e->global_transform().to_matrix(),
            bounds,
            e->gfx.gfx_id,
            e->gfx.gfx_entity_count,
            e->gfx.instance_count(),
            e->gfx.instance_offset()
        );

        zyy::world_destroy_entity(ee->editor_world, e);

    }

    entity_editor_render(ee);
    
}


#endif