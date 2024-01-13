#ifndef GUI_ENTITY_EDITOR_HPP
#define GUI_ENTITY_EDITOR_HPP

#include "ztd_core.hpp"

#include "dialog_window.hpp"
#include "App/Game/Entity/entity.hpp"
#include "App/Game/Entity/ztd_entity_prefab.hpp"
#include "App/Game/Entity/ztd_coroutine_callbacks.hpp"
#include "App/Game/Entity/ztd_physics_callbacks.hpp"


#include "App/game_state.hpp"

#include <filesystem>

namespace csg {
    enum struct brush_type {
        box, sphere,
    };

    struct csg_box_t {
        v3f size{1.0f};
    };
    struct csg_sphere_t {
        f32 radius{1.0f};
    };

    struct brush_t {
        brush_t* next{0};
        brush_type type{brush_type::box};
        u32 gfx_id{0};

        math::transform_t transform{};

        union {
            csg_box_t box{};
            csg_sphere_t sphere;
        };

        void apply_scale() {
            if (type == brush_type::box) {
                box.size.x = transform.get_basis_magnitude(0);
                box.size.y = transform.get_basis_magnitude(1);
                box.size.z = transform.get_basis_magnitude(2);
                transform.basis = m33{1.0f};
            }
        }
    };

    struct world_t {
        arena_t arena={};
        brush_t* brushes{0};
        b32 rebuild = 1;

        u64 box_mesh_id = 0;
        gfx::mesh_list_t box_mesh;

        // gfx::mesh_list_t& build_unit_cube(utl::allocator_t& vertices, utl::allocator_t& indices) {
        //     gfx::mesh_builder_t builder{vertices, indices};

        //     math::rect3d_t box{};
        //     box.expand(v3f{0.5f});
        //     box.expand(v3f{-0.5f});

        //     builder.add_box(box);
            
        //     return box_mesh = builder.build(&arena);
        // }

        // gfx::mesh_list_t build(utl::pool_t<gfx::vertex_t>& vertices, utl::pool_t<u32>& indices) {
        //     // if (rebuild == 0) return *this;

        //     gfx::mesh_builder_t builder{vertices, indices};

        //     node_for(auto, brushes, brush) {
        //         if (brush->type == brush_type::box) {
        //             math::rect3d_t box{};
        //             // box.expand(brush->box.size*0.5f);
        //             // box.expand(-brush->box.size*0.5f);
        //             box.expand(v3f{0.5f});
        //             box.expand(v3f{-0.5f});

        //             builder.add_box(box, brush->transform);
        //         }
        //     }

        //     rebuild = 0;
            
        //     return builder.build(&arena);
        // }

        brush_t* new_box_brush(u32 gfx_id) {
            tag_struct(auto* brush, brush_t, &arena);
            brush->type = brush_type::box;
            brush->box = {};
            brush->gfx_id = gfx_id;

            node_push(brush, brushes);

            rebuild = 1;

            return brush;
        }
    };
};

struct instanced_prefab_t {
    instanced_prefab_t* next = 0;
    instanced_prefab_t* prev = 0;
    ztd::prefab_t prefab{};
    math::transform_t transform{};
    particle_system_t* particle_system = 0;
    rendering::gfx_entity_id gfx_id = 0;
    u32 gfx_entity_count = 0;

    u32 instance_count = 1;
    u32 instance_offset = 0;
    m44* instances = 0;
    rendering::instance_extra_data_t* instance_colors = 0;
    u32 dynamic = 0; // 1bit is_dynamic 2bit double buffered

    std::pair<m44*, rendering::instance_extra_data_t*> dynamic_instances() {
        if (dynamic & 2) {
            dynamic = dynamic&1;
            return {instances, instance_colors};
        } else {
            dynamic = (dynamic&1)|2;
            return {instances + instance_count, instance_colors + instance_count};
        }
    }
};

struct saved_prefab_t {
    ztd::prefab_t prefab;
    m44 transform;
};

enum struct edit_type_t {
    GROUP, TEXT, VEC3, BASIS, INSERT, REMOVE, PREFAB, BYTES
};

enum edit_change_type {
    Edit_From = 0,
    Edit_To = 1,
    Edit_Count
};

struct prefab_edit_t {
    ztd::prefab_t* where{0};
    ztd::prefab_t change[Edit_Count];
};

struct text_edit_t {
    char* where{0};
    char change[Edit_Count];
};

struct vec3_edit_t {
    v3f* where{0};
    v3f change[Edit_Count];
};

struct basis_edit_t {
    m33* where{0};
    m33 change[Edit_Count];
};

struct prefab_insert_t {
    instanced_prefab_t* prefab;
    instanced_prefab_t* head;
};

struct byte_edit_t {
    u64* where{0};
    u64 change[Edit_Count];
};

struct group_event_t {
    struct edit_event_t* head{0};
};

void execute_edit(auto& edit, u32 which) {
    *edit.where = edit.change[which];
}

void execute_insert(auto& edit, u32 which) {
    if (which == Edit_From) {
        dlist_remove(edit.prefab);
    } else if (which == Edit_To) {
        dlist_insert_as_last(edit.head, edit.prefab);
    }
}

struct edit_event_t {
    edit_event_t* next{0};
    edit_event_t* prev{0};

    edit_type_t type{};
    union {
        text_edit_t     text_edit{};
        vec3_edit_t     vec3_edit;
        basis_edit_t    basis_edit;
        byte_edit_t     byte_edit;
        prefab_edit_t*  prefab_edit;
        prefab_insert_t prefab_insert;
        group_event_t   group;
    };
};

void execute(edit_event_t* cmd, b32 undo = 0);

void execute_group(auto& edit, u32 which) {
    if (!edit.head) {
        return;
    }
    if (which == Edit_To) {
        for (
            auto* event = edit.head->prev;
            event != edit.head;
            event = event->prev
        ) {
            execute(event, false);
        }
    } else {
        for (
            auto* event = edit.head->next;
            event != edit.head;
            node_next(event)
        ) {
            execute(event, true);
        }
    }
}

void execute(edit_event_t* cmd, b32 undo) {
    switch (cmd->type) {
        case edit_type_t::GROUP:
            execute_group(cmd->group, !undo ? Edit_To : Edit_From);
            break;
        case edit_type_t::REMOVE:
            execute_insert(cmd->prefab_insert, !undo ? Edit_From : Edit_To);
            break;
        case edit_type_t::INSERT:
            execute_insert(cmd->prefab_insert, !undo ? Edit_To : Edit_From);
            break;
        case edit_type_t::TEXT:
            execute_edit(cmd->text_edit, !undo ? Edit_To : Edit_From);
            break;
        case edit_type_t::VEC3:
            execute_edit(cmd->vec3_edit, !undo ? Edit_To : Edit_From);
            break;
        case edit_type_t::BASIS:
            execute_edit(cmd->basis_edit, !undo ? Edit_To : Edit_From);
            break;
        case edit_type_t::BYTES:
            execute_edit(cmd->byte_edit, !undo ? Edit_To : Edit_From);
            break;
        case edit_type_t::PREFAB:
            execute_edit(*cmd->prefab_edit, !undo ? Edit_To : Edit_From);
            break;
        case_invalid_default;
    }
}

enum struct selection_mode {
    pos, scale, rotation,
};

struct entity_editor_t {
    game_state_t* game_state{0};
    // gfx::gui::im::state_t imgui;

    arena_t arena{};
    arena_t undo_arena{};

    f32 time_scale{1.0f};

    edit_event_t redo_sentinel{};
    edit_event_t undo_sentinel{};
    edit_event_t* event_group{0};

    ztd::world_path_t paths{};
    gfx::color::gradient_t* gradient{0};

    void redo() {
        auto* cmd = redo_sentinel.next;
        if (cmd != &redo_sentinel) {
            dlist_remove(cmd);
            
            execute(cmd);
            
            dlist_insert(&undo_sentinel, cmd);
        }
    }

    void undo() {
        auto* cmd = undo_sentinel.next;
        if (cmd != &undo_sentinel) {
            dlist_remove(cmd);
            execute(cmd, 0b11);
            dlist_insert(&redo_sentinel, cmd);
        }
    }

    void on_add_to_undo() {
        dlist_init(&redo_sentinel);
    }

    void insert_edit_event(auto* edit) {
        if (event_group) {
            dlist_insert(event_group->group.head, edit);
        } else {
            dlist_insert(&undo_sentinel, edit);
        }
    }

    void begin_event_group() {
        tag_struct(auto* edit, edit_event_t, &undo_arena);
        edit->type = edit_type_t::GROUP;
        tag_struct(edit->group.head, edit_event_t, &undo_arena);

        dlist_init(edit->group.head);
        event_group = edit;
    }

    void end_event_group() {
        if (event_group) {
            auto* edit = event_group;
            event_group = 0;
            insert_edit_event(edit);
        }
    }

    void edit_char(char* where, char what) {
        tag_struct(auto* edit, edit_event_t, &undo_arena);
        edit->type = edit_type_t::TEXT;
        edit->text_edit.where = where;
        edit->text_edit.change[Edit_From] = *where;
        edit->text_edit.change[Edit_To] = what;

        execute_edit(edit->text_edit, Edit_To);
        insert_edit_event(edit);
        on_add_to_undo();
    }

    void edit_vec3(v3f* where, v3f what, v3f from) {
        tag_struct(auto* edit, edit_event_t, &undo_arena);
        edit->type = edit_type_t::VEC3;
        edit->vec3_edit.where = where;
        edit->vec3_edit.change[Edit_From] = from;
        edit->vec3_edit.change[Edit_To] = what;

        execute_edit(edit->vec3_edit, Edit_To);
        insert_edit_event(edit);
        on_add_to_undo();
    }

    void edit_bytes(u8* where, u64 what, u64 from) {
        tag_struct(auto* edit, edit_event_t, &undo_arena);
        edit->type = edit_type_t::BYTES;
        edit->byte_edit.where = (u64*)where;
        edit->byte_edit.change[Edit_From] = from;
        edit->byte_edit.change[Edit_To] = what;

        execute_edit(edit->byte_edit, Edit_To);
        insert_edit_event(edit);
        on_add_to_undo();
    }

    void edit_prefab(ztd::prefab_t* where, ztd::prefab_t what, ztd::prefab_t from) {
        tag_struct(auto* edit, edit_event_t, &undo_arena);
        edit->type = edit_type_t::PREFAB;
        tag_struct(edit->prefab_edit, prefab_edit_t, &undo_arena);
        edit->prefab_edit->where = where;
        edit->prefab_edit->change[Edit_From] = from;
        edit->prefab_edit->change[Edit_To] = what;

        execute_edit(*edit->prefab_edit, Edit_To);
        insert_edit_event(edit);
        on_add_to_undo();
    }

    void edit_basis(m33* where, m33 what, m33 from) {
        tag_struct(auto* edit, edit_event_t, &undo_arena);
        edit->type = edit_type_t::BASIS;
        edit->basis_edit = {};
        edit->basis_edit.where = where;
        edit->basis_edit.change[Edit_From] = from;
        edit->basis_edit.change[Edit_To] = what;

        execute_edit(edit->basis_edit, Edit_To);
        insert_edit_event(edit);
        on_add_to_undo();
    }

    void edit_bytes(u8* where, u64 what) {
        edit_bytes(where, what, *where);
    }

    void edit_prefab(ztd::prefab_t* where, ztd::prefab_t what) {
        edit_prefab(where, what, *where);
    }

    void edit_vec3(v3f* where, v3f what) {
        edit_vec3(where, what, *where);
    }

    void edit_basis(m33* where, m33 what) {
        edit_basis(where, what, *where);
    }

    instanced_prefab_t* instance_prefab(const ztd::prefab_t& prefab, const math::transform_t& transform, b32 push_undo = 1) {
        tag_struct(auto* inst, instanced_prefab_t, &arena);
        inst->prefab = prefab;
        inst->transform = transform;
        if (push_undo) {
            add_prefab(inst);
        } else {
            dlist_insert_as_last(&prefabs, inst);
        }

        auto* rs = game_state ? game_state->render_system : 0;
        if (rs && prefab.gfx.mesh_name[0]) {

            auto& mesh_list = rendering::get_mesh(rs, prefab.gfx.mesh_name.view());
            inst->gfx_id  = rendering::register_entity(rs);
            inst->gfx_entity_count = (u32)mesh_list.count;

            rendering::set_entity_material(rs, inst->gfx_id, prefab.gfx.material_id);
            rendering::initialize_entity(rs, inst->gfx_id, mesh_list.meshes[0].vertex_start, mesh_list.meshes[0].index_start);
            rendering::set_entity_albedo(rs, inst->gfx_id, u32(mesh_list.meshes[0].material.albedo_id));
            for(u32 i = 1; i < (u32)mesh_list.count; i++) {
                auto& mesh = mesh_list.meshes[i];
                rendering::register_entity(rs);
                rendering::set_entity_material(rs, inst->gfx_id + i, prefab.gfx.material_id);
                rendering::initialize_entity(rs, inst->gfx_id + i, mesh.vertex_start, mesh.index_start);
                rendering::set_entity_albedo(rs, inst->gfx_id + i, u32(mesh.material.albedo_id));
            }
        }
        if (rs && prefab.emitter) {
            inst->particle_system = particle_system_create(&arena, prefab.emitter->max_count);
            
            u32 count = prefab.emitter->max_count;
            inst->instance_offset = safe_truncate_u64(rs->scene_context->instance_storage_buffer.pool.count());

            inst->instances = rs->scene_context->instance_storage_buffer.pool.allocate(count * 2);
            inst->instance_colors = rs->scene_context->instance_color_storage_buffer.pool.allocate(count * 2);
            inst->dynamic = 1;
            inst->instance_count = count;
            std::fill(inst->instances, inst->instances+count*2+1, m44{1.0f});
            std::fill(inst->instance_colors, inst->instance_colors+count*2+1, rendering::instance_extra_data_t{});
                
            particle_system_settings_t& settings = *inst->particle_system;
            settings = *prefab.emitter;
            inst->particle_system->_stream_count = settings.stream_rate;
        }
        
        return inst;
    }

    void add_prefab(instanced_prefab_t* what) {
        tag_struct(auto* edit, edit_event_t, &undo_arena);
        edit->type = edit_type_t::INSERT;
        edit->prefab_insert.head = &prefabs;
        edit->prefab_insert.prefab = what;

        execute_insert(edit->prefab_insert, Edit_To);
        insert_edit_event(edit);
        on_add_to_undo();
    }

    instanced_prefab_t* copy_prefab(instanced_prefab_t* what) {
        auto* prefab = instance_prefab(what->prefab, what->transform);

        if (prefab->prefab.emitter) {
            if (prefab->prefab.emitter->particle_color._type == gfx::color::color_variant_type::gradient) {
                prefab->prefab.emitter->particle_color.gradient.count = 0;
                prefab->prefab.emitter->particle_color.gradient.add(
                    &arena,
                    what->prefab.emitter->particle_color.gradient.colors,
                    what->prefab.emitter->particle_color.gradient.positions,
                    what->prefab.emitter->particle_color.gradient.count
                );
            }
        }
        
        selection = prefab;
        entity = &prefab->prefab;

        return prefab;
    }

    void remove_prefab(instanced_prefab_t* what) {
        tag_struct(auto* edit, edit_event_t, &undo_arena);
        edit->type = edit_type_t::REMOVE;
        edit->prefab_insert.head = &prefabs;
        edit->prefab_insert.prefab = what;

        if (selection.prefab == what) {
            selection.prefab = 0;
            entity = &creating_entity;
        }

        execute_insert(edit->prefab_insert, Edit_From);
        insert_edit_event(edit);
        on_add_to_undo();
    }

    m44 projection{1.0f};
    ztd::cam::orbit_camera_t camera{};

    struct selection_t {
        selection_mode mode;

        v3f* selection{0};
        m33* basis{0};
        instanced_prefab_t* prefab{0};
        csg::brush_t* brush{0};

        selection_t& operator=(v3f* v) {
            selection = v;
            basis = 0;
            prefab = 0;
            brush = 0;
            mode = selection_mode::pos;
            return *this;
        }
        selection_t& operator=(math::transform_t* transform) {
            if (transform) {
                *this = &transform->origin;
                basis = &transform->basis;
            } else {
                *this = (v3f*)0;
            }
            return *this;
        }
        selection_t& operator=(math::transform_t& transform) {
            *this = &transform;
            return *this;
        }
        selection_t& operator=(instanced_prefab_t* p) {
            if (p) {
                *this = &p->transform;
            } else {
                *this = (math::transform_t*)0;
            }
            prefab = p;
            return *this;
        }
        selection_t& operator=(csg::brush_t* b) {
            *this = (instanced_prefab_t*)0;
            *this = &b->transform;
            brush = b;
            return *this;
        }

        void set_mode(u32 m) {
            mode = m == 0 ? selection_mode::pos : m == 1 ? selection_mode::scale : selection_mode::rotation;
        }

        operator v3f*() {
            return selection;
        }
        operator m33*() {
            return basis;
        }
        operator instanced_prefab_t*() {
            return prefab;
        }
    } selection;

    stack_buffer<selection_t, 32> selection_stack{};

    b32 has_selection(v3f* o) {
        if (o == selection) {
            return 1;
        }
        for (const auto& s : selection_stack.view()) {
            if (o == s.selection) {
                return 1;
            }
        }
        return 0;
    }

    f32 snapping = 0.0f;
    b32 global = 1;
    u32 mode = 0;

    struct light_probe_editor_settings_t {
        math::rect3d_t aabb{math::r3zero};
        f32            grid_size{1.61f};
    } light_probe_settings{};

    instanced_prefab_t prefabs{};
    ztd::prefab_t creating_entity{};
    ztd::prefab_t* entity{&creating_entity};

    csg::world_t  csg_world{};
    ztd::world_t* editor_world{0};

    void save_to_file(const char* name) {
        std::ofstream file{name, std::ios::binary};

        world_save_file_header_t header {
            .prefab_count = dlist_count(prefabs), 
            .light_probe_count = 1,
            .light_probe_min = light_probe_settings.aabb.min,
            .light_probe_max = light_probe_settings.aabb.max,
            .light_probe_grid_size = light_probe_settings.grid_size
        };
        
        file.write((char*)&header, sizeof(header));

        for(auto* prefab = prefabs.next;
            prefab != &prefabs;
            node_next(prefab)
        ) {
            file.write((char*)&prefab->prefab, sizeof(ztd::prefab_t));
            file.write((char*)&prefab->transform, sizeof(math::transform_t));
        }
    }


    entity_editor_t(game_state_t* app_)
    : game_state{app_}
    {
        if (app_) {
            // undo_arena = arena_sub_arena(&game_state->main_arena, megabytes(1));
            editor_world = ztd::world_init(game_state, game_state->game_memory->physics);
            // auto& mesh = csg_world.build_unit_cube(app_->render_system->scene_context->vertices.allocator, app_->render_system->scene_context->indices.allocator);
            // csg_world.box_mesh_id = rendering::add_mesh(app_->render_system, "box_brush", mesh);
        }

        dlist_init(&prefabs);
        dlist_init(&redo_sentinel);
        dlist_init(&undo_sentinel);
        dlist_init(&paths);

        camera.move(v3f{-10.0f, 4.0f, 00.0f});

        // gradient.add(&arena, v4f{0.89f, 0.84f, 0.3f, 1.0f}, 0.0f);
        // gradient.add(&arena, v4f{0.09f, 0.14f, 0.93f, 1.0f}, 1.0f);
    }
};


template<>
void utl::memory_blob_t::serialize<entity_editor_t>(arena_t* arena, const entity_editor_t& ee) {
    world_save_file_header_t header {
        .prefab_count = dlist_count(ee.prefabs), 
        .light_probe_count = 1,
        .light_probe_min = ee.light_probe_settings.aabb.min,
        .light_probe_max = ee.light_probe_settings.aabb.max,
        .light_probe_grid_size = ee.light_probe_settings.grid_size
    };
        
    serialize(arena, header);

    for (auto* inst = ee.prefabs.next;
        inst != &ee.prefabs;
        node_next(inst)
    ) {
        assert(inst);
        serialize(arena, inst->prefab);
        serialize(arena, inst->transform);
    }
}

// why is this here?
REFLECT_TYPE(std::string_view) {
    REFLECT_TYPE_INFO(std::string_view)
    };
};

// inline static void
// object_data_gui(gfx::gui::im::state_t& imgui, reflect::type_t type, std::byte* data, const char* prop_name = 0, size_t depth = 0) {
//     using namespace gfx::gui;
//     char depth_str[32];
//     utl::memzero(depth_str, 32);
//     std::memset(depth_str, '-', depth?depth-1:0);

//     auto show_property = type.get_property("show");
//     bool show = true;

//     if (show_property.valid()) {
//         show_property.get_value_raw(data, (std::byte*)&show);
//         if (im::text(imgui, fmt_sv("{} {}: {}", depth_str, type.name, prop_name?prop_name:""))) {
//             show = !show;
//             show_property.set_value_raw(data, show);
//         }
//     } else {
//         im::text(imgui, fmt_sv("{} {}: {}", depth_str, type.name, prop_name?prop_name:""));
//     }

//     if (!show) return;
//     if (type == reflect::type<f32>::info) {
//         im::indent(imgui, v2f{depth*28.0f,0.0f});
//         im::float_slider(imgui, (f32*)data);
//     } else if (type == reflect::type<u64>::info) {
//         im::text(imgui, fmt_sv("{}", *(u64*)data));
//     } else if (type == reflect::type<v3f>::info) {
//         const auto theme = imgui.theme;

//         imgui.theme.fg_color = gfx::color::rgba::red;
//         im::same_line(imgui);
//         im::indent(imgui, v2f{depth*28.0f,0.0f});
//         im::float_slider(imgui, (f32*)data);

//         imgui.theme.fg_color = gfx::color::rgba::green;
//         im::same_line(imgui);
//         im::indent(imgui, v2f{depth*28.0f,0.0f});
//         im::indent(imgui, v2f{depth*28.0f,0.0f});
//         im::float_slider(imgui, (f32*)data+1);

//         imgui.theme.fg_color = gfx::color::rgba::blue;
//         im::indent(imgui, v2f{depth*28.0f,0.0f});
//         im::indent(imgui, v2f{depth*28.0f,0.0f});
//         im::indent(imgui, v2f{depth*28.0f,0.0f});
//         im::float_slider(imgui, (f32*)data+2);

//         imgui.theme = theme;
//     } else 
//     for(auto& p: std::span{type.properties, type.property_count}) {
//         if (p.type) {
//             object_data_gui(imgui, *p.type, data + p.offset, p.name.data(), depth + 2);
//         } else {
//             im::text(imgui, "Unregistered type");
//         }
//     }
// }

// inline static void
// object_gui(gfx::gui::im::state_t& imgui, auto& value, size_t depth = 0) {
//     const auto type = reflect::type<std::remove_reference<decltype(value)>::type>::info;
            
//     object_data_gui(imgui, type, (std::byte*)&value, 0, depth+2);
// }

// void load_world_file_for_edit(entity_editor_t* ee, const char* name);

static bool
save_world_window(
    gfx::gui::im::state_t& imgui,
    entity_editor_t* ee,
    bool load = 0
) {
    using namespace gfx::gui;
    local_persist dialog_window_t dialog_box{.position = v2f{500.0f, 300.0f}};;
    char file[512] = {};

    auto draw_files = [&](){
        auto* game_state = ee->game_state;

        im::space(imgui, 64.0f);
        local_persist v2f rect{0.0f, 256.0f};
        local_persist f32 scroll;

        im::begin_scroll_rect(imgui, &scroll, &rect);
        defer {
            im::end_scroll_rect(imgui, &scroll, &rect);
        };
        
        for (const auto& entry : std::filesystem::recursive_directory_iterator("./res/worlds/")) {
            auto filename = entry.path().string();
            auto extension_match = utl::has_extension(filename, "zmap") || utl::has_extension(filename, "zyy");
            if (entry.is_directory()) {
                continue;

            } else if (entry.is_regular_file() && extension_match) {
                if (dialog_box.text_buffer[0]) {
                    if (std::strstr(filename.c_str(), dialog_box.text_buffer) == nullptr) {
                        continue;
                    }
                }
                if (im::text(imgui, filename)) {
                    utl::copy(file, filename.c_str(), filename.size());
                }
            }
        }
    };

    dialog_box
        .set_title(load?"Load World":"Save World")
        .set_description("Select a file")
        .draw(imgui, draw_files)
        .into(file);

    if (dialog_box.done == 2) {
        return false;
    }

    if (!file[0]) {
        return true;
    }

    if (load) {
        // load_world_file_for_edit(ee, file);
        {
            ee->begin_event_group();
            defer {
                ee->end_event_group();
            };
            for (auto* prefab = ee->prefabs.next;
                prefab != &ee->prefabs;
                node_next(prefab)
            ) {
                ee->remove_prefab(prefab);
            }
        }

        arena_t arena = arena_create(megabytes(16));
        defer {
            arena_clear(&arena);
        };
        auto bytes = utl::read_bin_file(&arena, file);
        
        utl::memory_blob_t blob{(std::byte*)bytes.data};
        auto header = blob.deserialize<world_save_file_header_t>();

        ee->begin_event_group();
        defer {
            ee->end_event_group();
        };

        range_u64(i, 0, header.prefab_count) {
            auto prefab = blob.deserialize<ztd::prefab_t>(&ee->arena);
            auto transform = blob.deserialize<math::transform_t>();
            ee->instance_prefab(prefab, transform);
        }

        if (header.light_probe_count > 0) {
            ee->light_probe_settings.aabb.min = header.light_probe_min;
            ee->light_probe_settings.aabb.max = header.light_probe_max;
            ee->light_probe_settings.grid_size = header.light_probe_grid_size;
        }
    } else {
        // ee->save_to_file(file);   
        arena_t arena = arena_create(megabytes(16));
        push_bytes(&arena, 1);
        utl::memory_blob_t blob{&arena};
        blob.serialize(&arena, *ee);

        utl::write_binary_file(file, blob.data_view());
        arena_clear(&arena);
    }
    
    return false;
}

static bool
load_mesh_window(
    gfx::gui::im::state_t& imgui,
    entity_editor_t* ee
) {
    using namespace gfx::gui;
    local_persist dialog_window_t dialog_box{.position = v2f{500.0f, 300.0f}};;
    char file[512] = {};

    auto draw_files = [&](){
        auto* game_state = ee->game_state;
        auto* pack_file = game_state->resource_file;

        local_persist u64 file_start = 0;
        im::space(imgui, 64.0f);
        
        local_persist v2f rect{0.0f, 256.0f};
        local_persist f32 scroll;

        im::begin_scroll_rect(imgui, &scroll, &rect);
        defer {
            im::end_scroll_rect(imgui, &scroll, &rect);
        };

        for(u64 rf = 0; rf < pack_file->file_count; rf++) {
            if (pack_file->table[rf].file_type != utl::res::magic::mesh) {
                continue;  
            }
            if (dialog_box.text_buffer[0] && std::strstr(pack_file->table[rf].name.c_data, dialog_box.text_buffer) == nullptr) {
                continue;
            }
            if (im::text(imgui, fmt_sv("- {}", pack_file->table[rf].name.c_data))) {
                utl::copy(file, pack_file->table[rf].name.c_data, std::strlen(pack_file->table[rf].name.c_data));
            }
        }
        im::space(imgui, 32.0f);
    };

    dialog_box
        .set_title("Load Mesh")
        .set_description("Select a file")
        .draw(imgui, draw_files)
        .into(file);

        
    if (dialog_box.done == 2) {
        return false;
    }

    if (!file[0]) {
        return true;
    }

    auto slen = std::strlen(file);
    utl::copy(ee->entity->gfx.mesh_name.buffer, file, slen);
    ee->entity->gfx.mesh_name.buffer[slen] = 0;

    return false;
}

static bool
load_texture_window(
    gfx::gui::im::state_t& imgui,
    entity_editor_t* ee
) {
    using namespace gfx::gui;
    local_persist dialog_window_t dialog_box{.position = v2f{500.0f, 300.0f}};
    char file[512] = {};

    auto draw_files = [&](){
        auto* game_state = ee->game_state;
        im::space(imgui, 64.0f);

        local_persist v2f rect{0.0f, 256.0f};
        local_persist f32 scroll;

        im::begin_scroll_rect(imgui, &scroll, &rect);
        defer {
            im::end_scroll_rect(imgui, &scroll, &rect);
        };

        for (const auto& entry : std::filesystem::recursive_directory_iterator("./res/textures/")) {
            auto filename = entry.path().filename().string();
            if (entry.is_directory()) {
                continue;
            } else if (entry.is_regular_file() && utl::has_extension(filename, "png")) {
                if (dialog_box.text_buffer[0]) {
                    if (std::strstr(filename.c_str(), dialog_box.text_buffer) == nullptr) {
                        continue;
                    }
                }
                if (im::text(imgui, filename)) {
                    auto trimmed = std::string_view{filename};
                    // auto trimmed = utl::trim_filename(filename, '/');
                
                    utl::copy(file, trimmed.data(), trimmed.size());
                }
            }
        }
        im::space(imgui, 32.0f);
    };

    dialog_box
        .set_title("Load Texture")
        .set_description("Select a file")
        .draw(imgui, draw_files)
        .into(file);

        
    if (dialog_box.done == 2) {
        return false;
    }

    if (!file[0]) {
        return true;
    }

    auto slen = std::strlen(file);
    utl::copy(ee->entity->gfx.albedo_texture.buffer, file, slen);
    ee->entity->gfx.albedo_texture.buffer[slen] = 0;

    return false;
}

using enumerate_callbacks_function = void(*)(const char**, u32*);

static bool
load_function_window(
    gfx::gui::im::state_t& imgui,
    entity_editor_t* ee,
    char* buffer,
    enumerate_callbacks_function enumerate_callbacks = 0
) {
    using namespace gfx::gui;
    local_persist dialog_window_t dialog_box{.position = v2f{500.0f, 300.0f}};;
    char file[512] = {};
    local_persist std::string function_names = "";
    local_persist b32 loaded_names = 0;

    if (!loaded_names) {
        loaded_names = 1;
        function_names = utl::unsafe_evaluate_command("dumpbin /exports build\\code.dll");
    }

    auto draw_files = [&](){
        local_persist v2f rect{0.0f, 256.0f};
        local_persist f32 scroll;

        im::begin_scroll_rect(imgui, &scroll, &rect);
        defer {
            im::end_scroll_rect(imgui, &scroll, &rect);
        };
     
        // im::space(imgui, 64.0f);    
        auto* game_state = ee->game_state;
        std::string_view splice;
        std::string_view names = function_names;
        names = names.substr(names.find_first_of('='));

        while((splice = utl::cut(names, '=', '\n', 1)) != ""sv) {
            if (names.empty()) break;
            // filter c++ mangled names
            // splice.data()[splice.size()] = 0;
            if (splice[0] != '?' && im::text(imgui, splice)) {
                assert(splice.size() < array_count(file));
                utl::copy(file, splice.data(), splice.size());
            }
            
            // move to the equal sign we just cut
            auto pos = names.find_first_of('=');
            if (pos == std::string_view::npos) {
                break;
            }
            names = names.substr(pos);

            // move to the end
            pos = names.find_first_of('\n');
            if (pos == std::string_view::npos) {
                break;
            }
            names = names.substr(pos);
        }        
        // im::space(imgui, 32.0f);
    };

    auto draw_functions_enumerated = [&]() {
        auto memory = begin_temporary_memory(&ee->arena);

        u32 count;
        enumerate_callbacks(0, &count);

        tag_array(const char** names, const char*, memory.arena, count);

        enumerate_callbacks(names, &count);

        range_u32(i, 0, count) {
            if (im::text(imgui, names[i])) {
                utl::copy(file, names[i], std::strlen(names[i]));
            }
        }
        
        end_temporary_memory(memory);
    };

    dialog_box
        .set_title("Load Callback")
        .set_description("Select a function");

    if (enumerate_callbacks) {
        dialog_box.draw(imgui, draw_functions_enumerated);
    } else {
        dialog_box.draw(imgui, draw_files);
    }

    dialog_box.into(file);

        
    if (dialog_box.done == 2) {
        return false;
    }

    if (!file[0]) {
        return true;
    }

    auto slen = std::strlen(file);
    utl::copy(buffer, file, slen);
    buffer[slen] = 0;
    loaded_names = 0;

    return false;
}

static bool
load_entity_window(
    gfx::gui::im::state_t& imgui,
    entity_editor_t* ee,
    bool load = 0
) {
    using namespace gfx::gui;
    local_persist dialog_window_t dialog_box{.position = v2f{500.0f, 300.0f}};;
    char file[512] = {};

    auto draw_files = [&](){
        im::space(imgui, 64.0f);
        local_persist v2f rect{0.0f, 256.0f};
        local_persist f32 scroll;

        im::begin_scroll_rect(imgui, &scroll, &rect);
        defer {
            im::end_scroll_rect(imgui, &scroll, &rect);
        };

        for (const auto& entry : std::filesystem::recursive_directory_iterator("./res/entity/")) {
            auto filename = entry.path().string();
            if (entry.is_directory()) {
                continue;
            } else if (entry.is_regular_file() && utl::has_extension(filename, "entt")) {
                if (dialog_box.text_buffer[0]) {
                    if (std::strstr(filename.c_str(), dialog_box.text_buffer) == nullptr) {
                        continue;
                    }
                }
                if (im::text(imgui, filename)) {
                    utl::copy(file, filename.c_str(), filename.size());
                }
            }
        }
        im::space(imgui, 32.0f);
    };

    dialog_box
        .set_title(load ? "Load Entity" : "Save Entity")
        .set_description("Select a file")
        .draw(imgui, draw_files)
        .into(file);

        
    if (dialog_box.done == 2) {
        return false;
    }

    if (!file[0]) {
        return true;
    }

    if (load) {
        *ee->entity = load_from_file(&ee->game_state->main_arena, file);
    } else { // show_save
        std::ofstream ffile{file, std::ios::binary};

        if (ffile.is_open() == false) {
            dialog_box.done = 0;
            return true;
        }
        auto memory = begin_temporary_memory(&ee->arena);

        utl::memory_blob_t blob{memory.arena};

        blob.serialize(memory.arena, *ee->entity);

        ffile.write((const char*)blob.data, blob.serialize_offset);

        end_temporary_memory(memory);
    }

    return false;
}


inline static void
entity_editor_render(entity_editor_t* ee) {
    TIMED_FUNCTION;
    using namespace gfx::gui;
    using namespace std::string_view_literals;

    // local_persist char mesh_name[512];
    local_persist bool show_load = false;
    local_persist bool show_save = false;
    local_persist bool show_graphics = false;
    local_persist bool show_physics = false;
    local_persist bool show_mesh_dialog = false;
    local_persist bool show_texture_dialog = false;
    local_persist bool show_animation_dialog = false;
    local_persist bool show_sound_dialog;
    local_persist char* show_function_dialog;
    
    auto* game_state = ee->game_state;
    auto* rs = game_state->render_system;
    auto& imgui = game_state->gui.imgui;
    auto* input = imgui.ctx.input;
    
    const auto width = game_state->gui.ctx.screen_size.x;
    
    if (input->pressed.keys[key_id::F1]) {
        show_animation_dialog ^= true;
    }
    if (input->pressed.keys[key_id::F2]) {
        show_sound_dialog ^= true;
    }
    if (show_sound_dialog) {
        show_sound_dialog = sound_browser(imgui, game_state);
    }
    if (show_animation_dialog) {
        show_animation_dialog = animation_browser(imgui, game_state, game_state->resource_file);
    }
    if (show_mesh_dialog) {
        show_mesh_dialog = load_mesh_window(imgui, ee);
    }
    if (show_texture_dialog) {
        show_texture_dialog = load_texture_window(imgui, ee);
    }
    if (show_function_dialog) {
        if (show_function_dialog == ee->entity->coroutine.name) {
            show_function_dialog = load_function_window(imgui, ee, show_function_dialog, enumerate_coroutine_callbacks) ? show_function_dialog : 0;
        } else {
            show_function_dialog = load_function_window(imgui, ee, show_function_dialog) ? show_function_dialog : 0;
        }
    }

    bool ctrl_held = game_state->input().keys[key_id::LEFT_CONTROL] || game_state->input().keys[key_id::RIGHT_CONTROL];
    bool shift_held = game_state->input().keys[key_id::LEFT_SHIFT] || game_state->input().keys[key_id::RIGHT_SHIFT];

    if (ctrl_held && game_state->input().pressed.keys[key_id::D]) {
        game_state->input().pressed.keys[key_id::D] = 0;
        if (ee->selection.prefab) {
            ee->copy_prefab(ee->selection.prefab);
        }
    }

    if (ctrl_held && game_state->input().pressed.keys[key_id::Z]) {
        game_state->input().pressed.keys[key_id::Z] = 0;
        if (shift_held) {
            ee->redo();
        } else {
            ee->undo();
        }
    }

    local_persist im::panel_state_t gradient_panel{
        .pos = v2f{500.0f},
        .size = v2f{200.0f},
        .open = 1
    };

    // gradient_panel.size = {};

    // ee->gradient.sort();

    if (ee->gradient) {
        auto [open, want_to_close] = im::begin_panel(imgui, "Gradient Editor", &gradient_panel);

        local_persist f32 grad_scroll;
        local_persist v2f grad_size {400.0f, 128.0f};

        if (open) {
            im::begin_scroll_rect(imgui, &grad_scroll, &grad_size);
            
            im::gradient_edit(imgui, ee->gradient, &ee->arena, math::zero_to(grad_size - v2f{16.0f}), 356);
            
            im::end_scroll_rect(imgui, &grad_scroll, &grad_size);
            // grad_size.x -= imgui.theme.padding;

            im::end_panel(imgui, &gradient_panel);
        }
        if (want_to_close) {
            ee->gradient = 0;
        }
    }

    local_persist v2f pos{0.0f};
    local_persist b32 opened = 1;
    local_persist v2f size = {};

    if (im::begin_panel(imgui, "World Editor", &pos, &size, &opened)) {
        local_persist bool saving = false;
        local_persist bool loading = false;
        im::same_line(imgui);
        if (im::text(imgui, "Save World", &saving)) {
            loading = 0;
        }
        if (im::text(imgui, " - Load World", &loading)) {
            saving = 0;
        }
        if (saving) {
            saving = save_world_window(imgui, ee, 0);
        }
        if (loading) {
            loading = save_world_window(imgui, ee, 1);
        }

        if (im::text(imgui, "Add Brush")) {
            auto gfx_id = rendering::register_entity(rs);
            auto& mesh_list = ee->csg_world.box_mesh;
            rendering::set_entity_material(rs, gfx_id, 1);
            rendering::initialize_entity(rs, gfx_id, mesh_list.meshes[0].vertex_start, mesh_list.meshes[0].index_start);
            rendering::set_entity_albedo(rs, gfx_id, 32);

            ee->selection = ee->csg_world.new_box_brush(gfx_id);
        }

        if (ee->entity == &ee->creating_entity) {
            im::text(imgui, "========= New Prefab ==========");
        } else {
            im::text(imgui, "======== Edit Prefab ==========");
        }

        auto prefab_start = *ee->entity;
        
        im::same_line(imgui);
        if (im::text(imgui, "Load"sv, &show_load)) {
            show_save = false;
        }
        im::same_line(imgui);
        if (im::text(imgui, "  Save"sv, &show_save)) {
            show_load = false;
        }
        im::same_line(imgui);
        if (im::text(imgui, "  Clear"sv)) {
            *ee->entity = {};
            // utl::memzero(mesh_name, array_count(mesh_name));
        }
        if (im::text(imgui, "  New"sv)) {
            ee->entity = &ee->creating_entity;
        }

        if (show_load || show_save) {
            im::text(imgui, "============================="sv);
            im::text(imgui, show_load ? "Loading"sv : "Saving"sv);

            if (show_load) {
                show_load = load_entity_window(imgui, ee, show_load);
            }
            if (show_save) {
                show_save = load_entity_window(imgui, ee, show_load);
            }

            if (im::text(imgui, "  Cancel"sv)) {
                show_save = show_load = false;
            }
        }

        im::space(imgui, 32.0f);
        
        std::string_view mesh_view{ee->entity->gfx.mesh_name.view()};
        size_t type_pos=std::strlen(ee->entity->type_name.data());
        size_t mesh_pos=mesh_view.size();
        
        im::same_line(imgui);
        im::text(imgui, "Type Name: \t"sv);
        im::text_edit(imgui, ee->entity->type_name.view(), &type_pos, "type_name::edit"_sid);

        std::string_view types[]{
            "environment",
            "player",
            "bad",
            "weapon",
            "weapon_part",
            "item",
            "trigger",
            "effect",
            "door",
            "container",
            "SIZE",
        };

        if (im::text(imgui, fmt_sv("Type: {}"sv, types[(u32)ee->entity->type]))) {
            ee->entity->type = (ztd::entity_type)(((u32)ee->entity->type + 1) % (u32)ztd::entity_type::SIZE);
        }

        std::string_view entity_flags[] {
            "<x>",
            "<x>",
            "<x>",
            "EntityFlags_Pickupable",
            "EntityFlags_Interactable",
            "<x>",
            "<x>",
        };
        
        local_persist bool show_flags;

        if (im::text(imgui, "Flags"sv, &show_flags)) {
            im::bitflag(imgui, std::span{entity_flags}, &ee->entity->flags);
        }

  
        if (im::text(imgui, "Graphics"sv, &show_graphics)) {
            im::same_line(imgui);
            if (im::text(imgui, "-- Mesh Name: \t"sv)) {
                show_mesh_dialog = true;
            }
            im::text_edit(imgui, mesh_view, &mesh_pos, "mesh_name::edit"_sid);

            im::same_line(imgui);
            im::text(imgui, "-- Material Id: \t"sv);
            im::uint_drag(imgui, &ee->entity->gfx.material_id, 1, 0, 100);
            
            std::string_view albedo_view = ee->entity->gfx.albedo_texture.view();
            u64 albedo_write = albedo_view.size();
            im::same_line(imgui);
            if (im::text(imgui, "-- Albedo Name: "sv)) {
                show_texture_dialog = true;
            }
            im::text_edit(imgui, albedo_view, &albedo_write, "albedo_texture::edit"_sid);

            im::text(imgui, fmt_sv("-- Normal Name: {}"sv, ee->entity->gfx.normal_texture.view()));
            im::text(imgui, fmt_sv("-- Anim Name: {}"sv, ee->entity->gfx.animations.view()));
        }
        
        if (ee->entity->physics) {
            im::same_line(imgui);
            if (im::button(imgui, "[x]", gfx::color::rgba::ue5_bg, 0)) {
                ee->entity->physics.reset();
            }
        }
        if (im::text(imgui, "Physics"sv, &show_physics)) {
            if (!ee->entity->physics) {
                ee->entity->physics.emplace(ztd::prefab_t::physics_t{});
            }
            auto& physics = *ee->entity->physics;
        
            std::string_view physics_flags[]{
                "PhysicsEntityFlags_Static",
                "PhysicsEntityFlags_Dynamic",
                "PhysicsEntityFlags_Trigger",
                "PhysicsEntityFlags_Character",
                "PhysicsEntityFlags_Kinematic"
            };
            u64 flag = physics.flags;
            im::bitflag(imgui, std::span{physics_flags}, &flag);
            physics.flags = (u32)flag;

            if (!im::text(imgui, "On Collision")) {
                std::string_view view{ee->entity->physics->on_collision.name};
                u64 wp = view.size();
                im::text_edit(imgui, view, &wp, "on_col"_sid);
            }
            if (!im::text(imgui, "On Collision End")) {
                std::string_view view{ee->entity->physics->on_collision_end.name};
                u64 wp = view.size();
                im::text_edit(imgui, view, &wp, "on_col_end"_sid);
            }
            if (!im::text(imgui, "On Trigger")) {
                std::string_view view{ee->entity->physics->on_trigger.name};
                u64 wp = view.size();
                im::text_edit(imgui, view, &wp, "on_trg"_sid);
            }
            if (!im::text(imgui, "On Trigger End")) {
                std::string_view view{ee->entity->physics->on_trigger_end.name};
                u64 wp = view.size();
                im::text_edit(imgui, view, &wp, "on_trg_end"_sid);
            }

            local_persist bool show_shape[8] = {};

            b32 add_shape = im::text(imgui, "Add shape");

            for (u64 i = 0; i < array_count(physics.shapes); i++) {
                if (!physics.shapes[i]) {
                    if (add_shape) {
                        physics.shapes[i].emplace(ztd::prefab_t::physics_t::shape_t{});
                        add_shape = 0;
                    } else {
                        continue;
                    }
                }
                auto& shape = *physics.shapes[i];

                if (im::text(imgui, fmt_sv("Shape [{}]:", i), &show_shape[i])) {
                    if (im::text(imgui, "Delete")) {
                        physics.shapes[i].reset();
                        show_shape[i]=0;
                    }

                    std::string_view shape_types[]{
                        "Convex",
                        "Trimesh",
                        "Sphere",
                        "Capsule",
                        "Box",
                    };

                    local_persist u64 sflag = 0;
                    sflag = im::tabs(imgui, std::span{shape_types}, sflag);
                    
                    switch(sflag) {
                        case "Convex"_sid: shape.shape = physics::collider_shape_type::CONVEX; break;
                        case "Trimesh"_sid: shape.shape = physics::collider_shape_type::TRIMESH; break;
                        case "Sphere"_sid: shape.shape = physics::collider_shape_type::SPHERE; break;
                        case "Capsule"_sid: shape.shape = physics::collider_shape_type::CAPSULE; break;
                        case "Box"_sid: shape.shape = physics::collider_shape_type::BOX; break;
                        case_invalid_default;
                    }

                    if (im::text(imgui, fmt_sv("Trigger [{}]", shape.flags?'x':' '))) {
                        shape.flags = !shape.flags;
                    }
                    
                    switch (shape.shape) {
                    case physics::collider_shape_type::SPHERE: {
                        auto world_pos = shape.sphere.origin;
                        if (ee->selection.prefab) {
                            world_pos = ee->selection.prefab->transform.xform(world_pos);
                        }
                        gfx::gui::draw_wire_sphere(&imgui.ctx, world_pos, shape.sphere.radius, rs->projection * rs->view);
                        // why???
                        // if (ee->selection.prefab) {
                        //     shape.sphere.origin = ee->selection.prefab->transform.inv_xform(world_pos);
                        // }
                        im::same_line(imgui);

                        // add math::transform_t* parent
                        if (im::text(imgui, "Origin: ")) {
                            auto* selected_instance = ee->selection.prefab;
                            if (selected_instance) {
                                ee->selection.selection = &shape.sphere.origin;
                            } else {
                                ee->selection = &shape.sphere.origin;
                            }
                        }
                        im::float3_input(imgui, &shape.sphere.origin);
                        im::same_line(imgui);
                        im::text(imgui, "Radius: ");
                        im::float_drag(imgui, &shape.sphere.radius, 0.1f, 0.0f);
                    } break;
                    case physics::collider_shape_type::BOX: {
                        im::same_line(imgui);
                        im::text(imgui, "Size: ");
                        im::float3_drag(imgui, &shape.box.size);
                    } break;
                    default:
                        break;
                    }
                }
            }
        }
        local_persist bool show_weapon;
        local_persist bool show_stats;

        if (!ee->entity->stats) {
            if (im::text(imgui, "Create Character")) {
                ee->entity->stats.emplace(ztd::character_stats_t{});
                show_stats = true;
            }
        } else if (im::text(imgui, "Show Character", &show_stats)) {

            auto& stats = *ee->entity->stats;

            #define float_prop(text_, ...) \
                im::same_line(imgui);\
                im::text(imgui, #text_);\
                im::float_drag(imgui, &stats.text_, __VA_ARGS__);
            
            float_prop(health.max, 1.0f);
            float_prop(movement.move_speed);

            #undef float_prop

            if (im::text(imgui, "Clear Stats")) {
                ee->entity->stats.reset();
                show_stats = false;
            }
        }

        local_persist bool show_particle;

        if (!ee->entity->emitter) {
            if (im::text(imgui, "Create Particle System")) {
                ee->entity->emitter.emplace(particle_system_settings_t{});
                show_particle = true;
            }
        } else if (im::text(imgui, "Show Particle System", &show_particle)) {
            auto& particle = *ee->entity->emitter;

            #define float_prop(text_, ...) \
                im::same_line(imgui);\
                im::text(imgui, #text_);\
                im::float_input(imgui, &particle.text_, __VA_ARGS__)
            
            #define selectable_float3_prop(text_, ...) \
                im::same_line(imgui);\
                if (im::text(imgui, #text_)) { ee->selection = &particle.text_; ee->selection_stack.clear(); }\
                im::float3_input(imgui, &particle.text_, __VA_ARGS__)

            #define float3_prop(text_, ...) \
                im::same_line(imgui);\
                im::text(imgui, #text_); \
                im::float3_input(imgui, &particle.text_, __VA_ARGS__)

            #define color_prop(text_, ...) \
                im::same_line(imgui);\
                im::text(imgui, #text_); \
                im::float4_input(imgui, &particle.text_, __VA_ARGS__)
            
            #define uint_prop(text_, ...) \
                im::same_line(imgui);\
                im::text(imgui, #text_);\
                im::uint_input(imgui, &particle.text_, __VA_ARGS__)


            uint_prop(max_count);
            uint_prop(flags);

            float_prop(template_particle.life_time);
            color_prop(template_particle.color);
            float_prop(template_particle.scale);
            float3_prop(template_particle.velocity);

            float3_prop(acceleration);
            float3_prop(velocity_random.min);
            float3_prop(velocity_random.max);

            float3_prop(angular_velocity_random.min);
            float3_prop(angular_velocity_random.max);

            uint_prop(stream_rate);
            float_prop(spawn_rate);
            float_prop(life_random.min);
            float_prop(life_random.max);

            float_prop(scale_over_life_time.min);
            float_prop(scale_over_life_time.max);

            std::string_view emitter_types[] = {
                "sphere",
                "box",
            };

            particle.emitter_type = im::enumeration<particle_emitter_type>(imgui, emitter_types, particle.emitter_type);

            switch (particle.emitter_type) {
                case particle_emitter_type::sphere:
                    selectable_float3_prop(sphere.origin);
                    float_prop(sphere.radius);
                break;
                case particle_emitter_type::box:
                    selectable_float3_prop(box.min);
                    selectable_float3_prop(box.max);
                break;
                case_invalid_default;
            };

            std::string_view color_types[] = {
                "hex",
                "uniform",
                "range",
                "gradient",
                "curve",
            };
            switch(particle.particle_color.set_type(im::enumeration<gfx::color::color_variant_type>(imgui, color_types, particle.particle_color._type))) {
                case gfx::color::color_variant_type::hex:
                    im::color_edit(imgui, &particle.particle_color.hex);
                    ee->gradient = nullptr;
                    break;
                case gfx::color::color_variant_type::uniform:
                    im::float4_drag(imgui, &particle.particle_color.uniform);
                    ee->gradient = nullptr;
                    break;
                case gfx::color::color_variant_type::gradient:
                    ee->gradient = &particle.particle_color.gradient;
                    break;
            }
            
            #undef float_prop
            #undef float3_prop
            #undef color_prop
            #undef selectable_float3_prop
            #undef uint_prop

            if (im::text(imgui, "Clear Particle System")) {
                ee->entity->emitter.reset();
                show_particle = false;
            }
        }

        if (!ee->entity->weapon) {
            if (im::text(imgui, "Create Weapon")) {
                ee->entity->weapon.emplace(ztd::wep::base_weapon_t{});
                show_weapon = true;
            }
        } else if (im::text(imgui, "Show Weapon", &show_weapon)) {

            auto& weapon = *ee->entity->weapon;

            #define float_prop(text_) \
                im::same_line(imgui);\
                im::text(imgui, #text_);\
                im::float_drag(imgui, &weapon.text_);
            
            #define uint_prop(text_) \
                im::same_line(imgui);\
                im::text(imgui, #text_);\
                im::uint_drag(imgui, &weapon.text_);
            
            float_prop(fire_rate);
            float_prop(load_speed);
            float_prop(chamber_speed);

            float_prop(stats.damage);
            float_prop(stats.spread);
            uint_prop(chamber_max);
            uint_prop(chamber_mult);
            uint_prop(mag.max);

            #undef uint_prop
            #undef float_prop


            if (im::text(imgui, "Clear Weapon")) {
                ee->entity->weapon.reset();
                show_weapon = false;
            }
        }

        local_persist u64 open_tab = 0;
        std::string_view tabs[] = {
            "Coroutine"sv,
            "Hit Effect"sv,
            "Spawn Bullet"sv,
        };

        #define open_function_window(buffer) if (im::text(imgui, "Show Exported")) { show_function_dialog = buffer; }

        switch (open_tab = im::tabs(imgui, std::span{tabs}, open_tab)) {
            case "Coroutine"_sid: {
                open_function_window(ee->entity->coroutine.name);
                std::string_view view{ee->entity->coroutine.name};
                u64 wp = view.size();
                im::text_edit(imgui, view, &wp, "coroutine::edit"_sid);
            } break;
            case "Hit Effect"_sid: {
                open_function_window(ee->entity->on_hit_effect.name);
                std::string_view view{ee->entity->on_hit_effect.name};
                u64 wp = view.size();
                im::text_edit(imgui, view, &wp, "on_hit::edit"_sid);
            } break;
            case "Spawn Bullet"_sid: {
                open_function_window(ee->entity->spawn_bullet.name);
                std::string_view view{ee->entity->spawn_bullet.name};
                u64 wp = view.size();
                im::text_edit(imgui, view, &wp, "spawn_bullet::edit"_sid);
            } break;
        }

        #undef open_function_window
        

        local_persist bool show_brain;
        if (ee->entity->brain_type == brain_type::invalid) {
            if (im::text(imgui, "Create Brain")) {
                ee->entity->brain_type = brain_type::player;
            }
        } else if (im::text(imgui, "Show Brain", &show_brain)) {
            std::string_view brain_types[] = {
                "player"sv,
                "flyer"sv,
                "chaser"sv,
                "shooter"sv,
                "person"sv,
                "guard"sv,
                "invalid"sv,
            };
            ee->entity->brain_type = im::enumeration<brain_type>(imgui, brain_types, ee->entity->brain_type);
        }
        
        auto diff = utl::memdif((const u8*)&prefab_start, (const u8*)ee->entity, sizeof(prefab_start));
        if (diff != 0) {
            std::swap(prefab_start, *ee->entity);

            auto smart_edit = [&](auto* e) {
                auto d = utl::memdif((const u8*)&prefab_start, (const u8*)e, sizeof(prefab_start));
                if (d <= 8) {
                    // CLOG("Optimized");
                    u8* r = (u8*)&prefab_start;
                    u8* w = (u8*)e;
                    for (u64 i = 0; i < sizeof(ztd::prefab_t); i++) {
                        if (*w != *r) {
                            ee->edit_bytes(w, *(u64*)r);
                            break;
                        }
                        w++; r++;
                    }
                } else {
                    ee->edit_prefab(e, prefab_start);
                }
            };

            puts("Edits");
            ee->begin_event_group();
            defer {
                ee->end_event_group();
            };

            smart_edit(ee->entity);
            for (const auto& selection: ee->selection_stack.view()) {
                if (selection.prefab) {
                    smart_edit(&selection.prefab->prefab);
                }
            }
        }

        im::text(imgui, "============================");

        if (im::text(imgui, "Add to World")) {
            auto* prefab = ee->instance_prefab(*ee->entity, math::transform_t{});

            ee->selection = prefab;

            ee->entity = &ee->creating_entity;
            *ee->entity = {};
            ee->selection_stack.clear();
        }

        im::text(imgui, "============================");
        if (im::text(imgui, "Add Path")) {
            tag_struct(auto* path, ztd::world_path_t, &ee->arena);

            dlist_insert_as_last(&ee->paths, path);
        }
        local_persist im::panel_state_t path_panel{
            .pos = v2f{0.0f, 600.0f},
            .open = 0,
        };
        if (!path_panel.open && im::text(imgui, "Show Paths")) {
            path_panel.open = 1;
        }
        if (path_panel.open && im::begin_panel(imgui, "Paths", &path_panel)) {

            local_persist bool open_paths[32] = {};
            u32 path_i = 0;
            for (auto* p = ee->paths.next;
                p != &ee->paths;
                node_next(p)
            ) {
                if (im::text(imgui, fmt_sv("Path[{}]", path_i), open_paths + path_i)) {
                    if (im::text(imgui, "Add Point")) {
                        v3f point{0.0f};
                        if (p->count > 0) {
                            point = p->points[p->count-1] + axis::forward;
                        }
                        p->add_point(&ee->arena, point);
                    }
                    im::text(imgui, fmt_sv("Count: {}", p->count));
                    range_u64(pi, 0, p->count) {
                        if (im::text(imgui, fmt_sv("Point: {}", p->points[pi]))) {
                            ee->selection = p->points + pi;
                            ee->entity = &ee->creating_entity; // clear entity selection
                        }
                    }
                }

                path_i += 1;
            }
            im::end_panel(imgui, &path_panel);
        }

        im::text(imgui, "====== Editor Settings =====");
        {
            im::text(imgui, fmt_sv("Editor Total Memory Size: {}{}", 
                math::pretty_bytes(ee->arena.top + ee->undo_arena.top), 
                math::pretty_bytes_postfix(ee->arena.top + ee->undo_arena.top)));

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

            im::text(imgui, fmt_sv("Undo Memory Size: {}{}", 
                math::pretty_bytes(ee->undo_arena.top), 
                math::pretty_bytes_postfix(ee->undo_arena.top)));
            im::text(imgui, fmt_sv("Undo Count: {}", undo_count));
            im::text(imgui, fmt_sv("Redo Count: {}", redo_count));
        }

        im::text(imgui, "======== Light Probe =======");
        if (im::text(imgui, "Min")) {
            ee->selection = &ee->light_probe_settings.aabb.min;
            ee->selection_stack.clear();
        }
        if (im::text(imgui, "Max")) {
            ee->selection = &ee->light_probe_settings.aabb.max;
            ee->selection_stack.clear();
        }

        im::same_line(imgui);
        im::text(imgui, "Grid Size");
        im::float_input(imgui, &ee->light_probe_settings.grid_size);

        im::text(imgui, fmt_sv("Camera: {}", ee->camera.position));

        im::end_panel(imgui, &pos, &size);
    }


    local_persist im::panel_state_t transform_panel{
        .pos = v2f{680.0f, 0.0f},
        .open = 1,
    };

    // transform_panel.size = {};


    if (im::begin_panel(imgui, "Transform", &transform_panel)) {
        im::same_line(imgui);
        if (im::text(imgui, "Local ")) {
            ee->mode = 0;
        }
        im::same_line(imgui);
        if (im::text(imgui, "Global ")) {
            ee->mode = 1;
        };

        im::indent(imgui, 12.0f);

        im::same_line(imgui);
        if (im::text(imgui, "Translate -")) {
            ee->selection.set_mode(0);
        }
        im::same_line(imgui);
        if (im::text(imgui, " Scale -")) {
            ee->selection.set_mode(1);
        }
        im::same_line(imgui);
        if (im::text(imgui, " Rotate")) {
            ee->selection.set_mode(2);
        }


        im::same_line(imgui);
        im::indent(imgui, 12.0f);
        if (im::text(imgui, fmt_sv("Snapping [{}]", ee->snapping != 0.0f ? 'x' : ' '))) {
            ee->snapping = ee->snapping == 0.0f ? 1.0f : 0.0f;
        }
        if (ee->snapping != 0.0f) {
            im::same_line(imgui);
            im::float_drag(imgui, &ee->snapping);

            im::same_line(imgui);
            im::indent(imgui, 8.0f);            
            if (im::text(imgui, "x0.1")) {
                ee->snapping = 0.10f;
            }
            im::same_line(imgui);
            im::indent(imgui, 8.0f);            
            if (im::text(imgui, "x0.5")) {
                ee->snapping = 0.50f;
            }
            im::same_line(imgui);
            im::indent(imgui, 8.0f);            
            if (im::text(imgui, "x1")) {
                ee->snapping = 1.0f;
            }
            im::same_line(imgui);
            im::indent(imgui, 8.0f);            
            if (im::text(imgui, "x5")) {
                ee->snapping = 5.0f;
            }
            im::same_line(imgui);
            im::indent(imgui, 8.0f);            
            if (im::text(imgui, "x10")) {
                ee->snapping = 10.0f;
            }
            if (ee->selection.mode == selection_mode::rotation) {
                im::same_line(imgui);
                im::indent(imgui, 8.0f);            
                if (im::text(imgui, "x15")) {
                    ee->snapping = 15.0f;
                }
                im::same_line(imgui);
                im::indent(imgui, 8.0f);            
                if (im::text(imgui, "x30")) {
                    ee->snapping = 30.0f;
                }
                im::same_line(imgui);
                im::indent(imgui, 8.0f);            
                if (im::text(imgui, "x45")) {
                    ee->snapping = 45.0f;
                }
                im::same_line(imgui);
                im::indent(imgui, 8.0f);            
                if (im::text(imgui, "x90")) {
                    ee->snapping = 90.0f;
                }
            }
            im::indent(imgui, 8.0f);            
            if (im::text(imgui, "x100")) {
                ee->snapping = 100.0f;
            }
        }

        im::end_panel(imgui, &transform_panel);
    }


    local_persist b32 show_entity_list = 1;
    local_persist v2f entity_window_pos{1600.0f, 0.0f};
    local_persist v2f entity_window_size;

    // entity_window_size = {};

    if (im::begin_panel(imgui, "Entities", &entity_window_pos, &entity_window_size, &show_entity_list)) {
        entity_window_pos = imgui.ctx.screen_rect().max - entity_window_size;
        auto theme = imgui.theme;

        im::begin_drag(imgui);

        if (im::is_dragging(imgui)) {
            u64 dummy = 0;
            im::drag_user_data(imgui, dummy, "delete");
            im::text(imgui, "Delete");
        }

        for(auto* prefab = ee->prefabs.next;
            prefab != &ee->prefabs;
            node_next(prefab)
        ) {
            auto has_selected = ee->has_selection(&prefab->transform.origin);

            if (has_selected) {
                imgui.theme.text_color = gfx::color::flatten_color(gfx::color::rgba::yellow);
            }

            im::drag_user_data(imgui, *prefab, "prefab_t");

            im::same_line(imgui);
            
            if (im::button(imgui, "[x]", gfx::color::rgba::ue5_bg, imgui.theme.text_color, (u64)prefab)) {
                ee->remove_prefab(prefab);
            }

            // click on entity in list
            if (im::text(imgui, 
                prefab->prefab.type_name.empty() ? 
                    fmt_sv("Entity: {} ", (void*)prefab):
                    fmt_sv("{}\0{}"sv, prefab->prefab.type_name.view(), (void*)prefab)
            )) {
                if (ee->selection == prefab) {
                    ee->camera.position = prefab->transform.origin;
                }
                if (shift_held && ee->selection.prefab) {
                    ee->selection_stack.push(ee->selection);
                } else {
                    ee->selection_stack.clear();
                }
                ee->selection = prefab;
                ee->entity = &prefab->prefab;
                ee->gradient = 0;
            }
            imgui.theme = theme;
        }

        if (im::released_drag(imgui)) {
            auto drag_event = *imgui.drag_event;
            if ("prefab_t"sv == drag_event.type_name) {
                auto* prefab = (instanced_prefab_t*)drag_event.user_data;

                if (drag_event.dropped_id) {
                    if ("delete"sv == drag_event.dropped_type_name) {
                        ee->remove_prefab(prefab);
                    }
                    if ("prefab_t"sv == drag_event.dropped_type_name) {
                        auto* dropped_prefab = (instanced_prefab_t*)drag_event.dropped_user_data;
                        ee->remove_prefab(prefab);
                        // Todo(Zack): hook up insert with undo system
                        dlist_insert(dropped_prefab, prefab);
                    }
                }
            }
        }

        im::end_drag(imgui);

        im::end_panel(imgui, &entity_window_pos, &entity_window_size);
    }

    auto want_to_copy = shift_held;

    if (ee->selection.selection != nullptr) {
        if (ee->selection.mode == selection_mode::scale) {
            auto [released, _start] = im::gizmo_scale(imgui, ee->selection, ee->selection, rs->projection * rs->view, ee->snapping);
            if (released) {
                if (want_to_copy && ee->selection.prefab) {
                    auto* dragged = ee->selection.prefab;
                    auto* copied = ee->copy_prefab(ee->selection.prefab); // selects copied, spawns at drag spot
                    dragged->transform.basis = imgui.gizmo_basis_start;
                } else {
                    ee->edit_basis(ee->selection, *ee->selection.basis, imgui.gizmo_basis_start);
                }
            }
        } else if (ee->selection.mode == selection_mode::rotation) {
            auto [released, _start] = im::gizmo_rotate(imgui, ee->selection, ee->selection, rs->projection * rs->view, ee->snapping);
            if (released) {
                if (want_to_copy && ee->selection.prefab) {
                    auto* dragged = ee->selection.prefab;
                    auto* copied = ee->copy_prefab(ee->selection.prefab); // selects copied, spawns at drag spot
                    dragged->transform.basis = imgui.gizmo_basis_start;
                } else {
                    ee->edit_basis(ee->selection, *ee->selection.basis, imgui.gizmo_basis_start);
                }
            }
        } else if (ee->selection.mode == selection_mode::pos) {
            auto space = ee->mode ? im::gizmo_mode::global : im::gizmo_mode::local;
            auto [released, start] = im::gizmo(imgui, ee->selection, rs->projection * rs->view, ee->snapping, ee->selection.basis?*ee->selection.basis:m33{1.0f}, space);
            if (released) {
                if (want_to_copy && ee->selection.prefab) {
                    auto* dragged = ee->selection.prefab;
                    auto* copied = ee->copy_prefab(ee->selection.prefab); // selects copied, spawns at drag spot
                    dragged->transform.origin = start;
                } else {
                    auto delta = *ee->selection.selection - start;
                    ee->begin_event_group();
                    defer {
                        ee->end_event_group();
                    };
                    ee->edit_vec3(ee->selection, *ee->selection.selection, start);
                    for (auto& other_selected: ee->selection_stack.view()) {
                        ee->edit_vec3(other_selected.selection, *other_selected.selection + delta);
                    }
                }
            }
        }
    }
}

inline static void
entity_editor_update(entity_editor_t* ee) {
    auto* game_state = ee->game_state;
    auto* pack_file = game_state->resource_file;
    auto* input = &game_state->input();
    auto dt = input->dt * ee->time_scale;

    if (input->pressed.keys[key_id::F6]) {
        ee->time_scale *= 0.5f;
    } else if (input->pressed.keys[key_id::F7]) {
        ee->time_scale *= 2.0f;
    }

    auto& imgui = game_state->gui.imgui;
    const auto& screen_size = imgui.ctx.screen_size;
    auto& camera = ee->camera;
    auto [mx, my] = input->mouse.pos;
   
    game_state->render_system->set_view(camera.view(), (u32)game_state->width(), (u32)game_state->height());

    auto* rs = game_state->render_system;
    auto vp = rs->projection * rs->view;

    auto draw = [&](const auto& entity, auto transform, auto id, u32 count, instanced_prefab_t* prefab = 0) {
        if (entity.gfx.mesh_name[0]) {
            if (utl::res::pack_file_get_file_by_name(pack_file, entity.gfx.mesh_name.view())) {

                auto mesh_id = rendering::get_mesh_id(rs, entity.gfx.mesh_name.view());
                auto material_id = entity.gfx.material_id;
                auto gfx_id = id;
                auto gfx_count = count;
                auto instance_count = prefab ? prefab->instance_count : 1;
                auto instance_offset = prefab ? prefab->instance_offset : 0;
                if (prefab && (prefab->dynamic & 2)) {
                    instance_offset += instance_count;
                }

                auto albedo_id = std::numeric_limits<u32>::max();
                if (entity.gfx.albedo_texture.empty() == false) {
                    auto aid = (u32)rs->texture_cache.get_id(entity.gfx.albedo_texture.view());
                    if (aid == 0) {
                        ztd_warn(__FUNCTION__, "Missing Texture: {}", entity.gfx.albedo_texture.view());
                        tag_array(auto* texture, char, &ee->game_state->texture_arena, entity.gfx.albedo_texture.size()+1);
                        utl::copy(texture, entity.gfx.albedo_texture.buffer, entity.gfx.albedo_texture.size());
                        texture[entity.gfx.albedo_texture.size()] = '\0';
                        aid = (u32)rs->texture_cache.load(&rs->arena, *rs->vk_gfx, texture);
                    }
                    albedo_id = aid;
                }

                auto aabb = rendering::get_mesh_aabb(rs, entity.gfx.mesh_name.view());

                auto world_aabb = math::transform_t{transform}.xform_aabb(aabb);
                DEBUG_DIAGRAM(world_aabb);

                rendering::submit_job(
                    rs, 
                    mesh_id, 
                    material_id, // todo make material per mesh
                    transform,
                    gfx_id,
                    gfx_count,
                    instance_count,
                    instance_offset,
                    albedo_id
                );
            }
        }
    };

    entity_editor_render(ee);
    // draw_gui(ee->game_state->game_memory);

    draw(ee->creating_entity, m44{1.0f}, 0, 1);

    for(auto* p = ee->paths.next;
        p != &ee->paths;
        node_next(p)
    ) {
        if (p->count > 0) {
            range_u64(i, 0, p->count) {
                math::rect3d_t point_cube{
                    .min = p->points[i] - v3f{0.1f},
                    .max = p->points[i] + v3f{0.1f}
                };
                gfx::gui::draw_cube(&imgui.ctx, point_cube, gfx::color::rgba::red);
                if (i > 0) {
                    auto sp0 = screen_size * v2f{math::world_to_screen(vp, p->points[i])};
                    auto sp1 = screen_size * v2f{math::world_to_screen(vp, p->points[i-1])};
                    gfx::gui::draw_line(&imgui.ctx, sp0, sp1, 2.0f, gfx::color::rgba::yellow);
                }
            }
        }
    }

    for(auto* p = ee->prefabs.next;
        p != &ee->prefabs;
        node_next(p)
    ) {
        if (p->prefab.emitter) {
            if (p->particle_system) {
                p->prefab.emitter->max_count = std::min(p->prefab.emitter->max_count, p->instance_count);
                *(particle_system_settings_t*)p->particle_system = *p->prefab.emitter;

                // auto* instances = p->instances;
                // auto* color_instances = p->instance_colors;
                auto [instance, color_instances] = p->dynamic_instances();
                // if (p->dynamic & 2) {
                //     instances = instances + p->instance_count;
                //     color_instances = color_instances + p->instance_count;
                // }
                auto transform = p->transform.to_matrix();
                particle_system_update(p->particle_system, transform, dt);
                // particle_system_sort_view(p->particle_system, transform, ee->camera.position, -ee->camera.forward());
                particle_system_build_matrices(p->particle_system, p->transform.to_matrix(), instance, p->instance_count);
                particle_system_build_colors(p->particle_system, color_instances, p->instance_count);
            }
        }
        draw(p->prefab, p->transform.to_matrix(), p->gfx_id, p->gfx_entity_count, p);
    }

    node_for(auto, ee->csg_world.brushes, brush) {
        auto mesh_id = ee->csg_world.box_mesh_id;
        auto material_id = 1;
        auto gfx_id = brush->gfx_id;
        auto gfx_count = 1;

        rendering::submit_job(
            rs, 
            mesh_id, 
            material_id, // todo make material per mesh
            brush->transform.to_matrix(),
            gfx_id,
            gfx_count,
            1,
            0
        );
    }


    auto capture_input = gfx::gui::im::want_mouse_capture(imgui);

    const b32 alt_pressed = input->keys[key_id::LEFT_ALT] || input->keys[key_id::RIGHT_ALT];
    
    const keyboard_input_mapping_t mapping{
        .look_button = alt_pressed ? 0 : 1
    };
    auto pc = input->gamepads[0].is_connected ? gamepad_controller(input) : keyboard_controller(input, mapping);
    local_persist f32 camera_move_speed = 10.0f;

    if (!capture_input) {
        if (!alt_pressed && input->mouse.buttons[0]) {
            pc.move_input.x -= input->mouse.delta[0] * 0.1f;
            pc.move_input.y += input->mouse.delta[1] * 0.1f;
        }

        camera.move(camera_move_speed * input->dt * camera.forward() * pc.move_input.z);
        camera.move(camera_move_speed * input->dt * camera.right() * pc.move_input.x);
        camera.move(camera_move_speed * input->dt * axis::up * pc.move_input.y);
        camera.rotate(dt * pc.look_input);

        if (input->mouse.scroll[1]) {
            ee->camera.distance += -input->mouse.scroll[1];

            ee->camera.distance = glm::max(1.0f, ee->camera.distance);
            
        }

        if (input->pressed.keys[key_id::B] && ee->selection.basis) {
            ee->selection.set_mode(1);
        }
        if (input->pressed.keys[key_id::R] && ee->selection.basis) {
            ee->selection.set_mode(2);
        }
        if (input->pressed.keys[key_id::G]) {
            ee->selection.set_mode(0);
        }

        if (input->pressed.keys[key_id::F] && ee->selection.selection) {
            ee->camera.position = *ee->selection.selection;
        }
    }
}

// void load_world_file_for_edit(entity_editor_t* ee, const char* name) {
//     std::ifstream file{name, std::ios::binary};

//     for (auto* prefab = ee->prefabs.next;
//         prefab != &ee->prefabs;
//         node_next(prefab)
//     ) {
//         ee->remove_prefab(prefab);
//     }

//     world_save_file_header_t header = {};

//     file.read((char*)&header, sizeof(header));
    
//     assert(header.prefab_size == sizeof(ztd::prefab_t));
    
//     range_u64(i, 0, header.prefab_count) {
//         math::transform_t transform;
//         ztd::prefab_t prefab = {};

//         file.read((char*)&prefab, header.prefab_size);
//         file.read((char*)&transform, sizeof(transform));

//         if (prefab.VERSION != ztd::prefab_t{}.VERSION) {
//             ztd_error(__FUNCTION__, "Prefab version ({}) not supported", prefab.VERSION);
//         } else {
//             auto* inst = ee->instance_prefab(prefab, transform);
//         }

//     }
// }




#endif