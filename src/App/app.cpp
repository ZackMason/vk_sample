#include <core.hpp>

#include "App/vk_state.hpp"
#include "App/gfx_pipelines.hpp"

#include "App/Game/Util/camera.hpp"

#include "App/Game/World/world.hpp"
#include "App/Game/World/Level/level.hpp"
#include "App/Game/World/Level/Room/room.hpp"
#include "App/Game/Items/base_item.hpp"
#include "App/Game/Weapons/base_weapon.hpp"

#include "App/Game/Physics/physics_world.hpp"


struct mesh_cache_t {
    struct link_t : node_t<link_t> {
        gfx::mesh_list_t mesh;
    };

    utl::deque<link_t> meshes;

    u64 add(arena_t* arena, const gfx::mesh_list_t& r) {
        link_t* link = arena_alloc_ctor<link_t>(arena, 1);
        link->mesh = r;
        meshes.push_back(link);
        return meshes.size() - 1;
    }

    const gfx::mesh_list_t& get(u64 id) const {
        return meshes[id]->mesh;
    }
};


static constexpr size_t max_scene_vertex_count{10'000'000};
static constexpr size_t max_scene_index_count{30'000'000};
struct app_t {
    app_memory_t*       app_mem{nullptr};

    // arenas
    arena_t             main_arena;
    arena_t             temp_arena;
    arena_t             string_arena;
    arena_t             mesh_arena;
    arena_t             texture_arena;

    utl::res::pack_file_t *     resource_file{0};

    mesh_cache_t                mesh_cache;
    utl::str_hash_t                 mesh_hash;
    utl::str_hash_t                 mesh_hash_meta;

    
    gfx::vul::state_t   gfx;

    // todo(zack) clean this up
    gfx::font_t default_font;
    gfx::vul::texture_2d_t* default_font_texture{nullptr};
    VkDescriptorSet default_font_descriptor;

    gfx::vul::texture_2d_t* brick_texture{nullptr};
    VkDescriptorSet brick_descriptor;

    gfx::vul::vertex_buffer_t<gfx::vertex_t, max_scene_vertex_count> vertices;
    gfx::vul::index_buffer_t<max_scene_index_count> indices;

    struct scene_t {
        gfx::vul::scene_buffer_t scene_buffer;
        gfx::vul::object_buffer_t object_buffer;
        gfx::vul::sporadic_buffer_t sporadic_buffer;

        gfx::vul::uniform_buffer_t<gfx::vul::scene_buffer_t>    debug_scene_uniform_buffer;
    } scene;
    
    gfx::vul::pipeline_state_t* gui_pipeline{0};
    gfx::vul::pipeline_state_t* debug_pipeline{0};
    gfx::vul::pipeline_state_t* mesh_pipeline{0};

    struct gui_state_t {
        gfx::gui::ctx_t ctx;
        gfx::vul::vertex_buffer_t<gfx::gui::vertex_t, 40'000> vertices;
        gfx::vul::index_buffer_t<60'000> indices;

        arena_t  arena;
        gfx::gui::panel_t* main_panel{nullptr};
    } gui;

    struct debugging_t {
        u64 mesh_sub_index{0};
        u64 mesh_index{0};
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
        
    game::world_t* game_world;

    f32 time_scale = 1.0f;
    f32 time_text_anim = 0.0f;
};

inline gfx::mesh_list_t
load_bin_mesh_data(
    arena_t* arena,
    std::byte* data,
    utl::pool_t<gfx::vertex_t>* vertices,
    utl::pool_t<u32>* indices
) {
    gfx::mesh_list_t results;

    utl::memory_blob_t blob{data};

    // read file meta data
    const auto meta = blob.deserialize<u64>();
    const auto vers = blob.deserialize<u64>();
    const auto mesh = blob.deserialize<u64>();

    assert(meta == utl::res::magic::meta);
    assert(vers == utl::res::magic::vers);
    assert(mesh == utl::res::magic::mesh);

    results.count = blob.deserialize<u64>();
    gen_warn("app::load_bin_mesh_data", "Loading {} meshes", results.count);
    results.meshes  = arena_alloc_ctor<gfx::mesh_view_t>(arena, results.count);

    for (size_t i = 0; i < results.count; i++) {
        std::string name = blob.deserialize<std::string>();
        gen_info("app::load_bin_mesh_data", "Mesh name: {}", name);
        const size_t vertex_count = blob.deserialize<u64>();
        const size_t vertex_bytes = sizeof(gfx::vertex_t) * vertex_count;

        const u32 vertex_start = safe_truncate_u64(vertices->count);
        const u32 index_start = safe_truncate_u64(indices->count);

        results.meshes[i].vertex_count = safe_truncate_u64(vertex_count);
        results.meshes[i].vertex_start = vertex_start;
        results.meshes[i].index_start = index_start;

        gfx::vertex_t* v = vertices->allocate(vertex_count);

        std::memcpy(v, blob.read_data(), vertex_bytes);
        blob.advance(vertex_bytes);

        loop_itoa(j, results.meshes[i].vertex_count) {
            results.meshes[i].aabb.expand(v[j].pos);
        }

        const size_t index_count = blob.deserialize<u64>();
        const size_t index_bytes = sizeof(u32) * index_count;
        results.meshes[i].index_count = safe_truncate_u64(index_count);

        u32* tris = indices->allocate(index_count);
        std::memcpy(tris, blob.read_data(), index_bytes);
        blob.advance(index_bytes);
    }

    return results;
}

inline gfx::mesh_list_t
load_bin_mesh(
    arena_t* arena,
    utl::res::pack_file_t* packed_file,
    std::string_view path,
    utl::pool_t<gfx::vertex_t>* vertices,
    utl::pool_t<u32>* indices
) {
    const size_t file_index = utl::res::pack_file_find_file(packed_file, path);
    
    std::byte* file_data = utl::res::pack_file_get_file(packed_file, file_index);

    gfx::mesh_list_t results;
    
    if (file_data) {
        return load_bin_mesh_data(arena, file_data, vertices, indices);
    } else {
        gen_warn("app::load_bin_mesh", "Failed to open mesh file: {}", path);
    }

    return results;
}

inline app_t*
get_app(app_memory_t* mem) {
    // note(zack): App should always be the first thing initialized in perm_memory
    return (app_t*)mem->perm_memory;
}

export_fn(void) 
app_on_init(app_memory_t* app_mem) {
    utl::profile_t p{"app init"};
    app_t* app = get_app(app_mem);
    new (app) app_t;

    app->app_mem = app_mem;
    app->main_arena.start = (std::byte*)app_mem->perm_memory + sizeof(app_t);
    app->main_arena.size = app_mem->perm_memory_size - sizeof(app_t);

    arena_t* main_arena = &app->main_arena;

    app->temp_arena = arena_sub_arena(&app->main_arena, megabytes(4));
    app->string_arena = arena_sub_arena(&app->main_arena, megabytes(16));
    app->mesh_arena = arena_sub_arena(&app->main_arena, megabytes(512));
    app->texture_arena = arena_sub_arena(&app->main_arena, megabytes(512));
    app->gui.arena = arena_sub_arena(main_arena, megabytes(8));

    gen_info("init", "init physx");
    static game::phys::physx_state_t physx_state{
        .default_allocator = {app->main_arena, megabytes(256)}
    };
    game::phys::init_physx_state(physx_state);

    app->resource_file = utl::res::load_pack_file(&app->mesh_arena, &app->string_arena, "./assets/res.pack");
    
    app->game_world = game::world_init(main_arena);
    
    // setup graphics

    gfx::vul::state_t& vk_gfx = app->gfx;
    app->gfx.init(&app_mem->config);
    app->gui_pipeline = gfx::vul::create_gui_pipeline(&app->mesh_arena, &vk_gfx);
    app->mesh_pipeline = gfx::vul::create_mesh_pipeline(&app->mesh_arena, &vk_gfx,
        gfx::vul::mesh_pipeline_info_t{
            vk_gfx.scene_uniform_buffer.buffer,
            vk_gfx.sporadic_uniform_buffer.buffer,
            vk_gfx.object_uniform_buffer.buffer
        }
    );
    assert(app->mesh_pipeline);

    vk_gfx.create_uniform_buffer(&app->scene.debug_scene_uniform_buffer);

    app->debug_pipeline = gfx::vul::create_debug_pipeline(
        &app->mesh_arena, 
        &vk_gfx, 
        app->scene.debug_scene_uniform_buffer.buffer, 
        sizeof(gfx::vul::scene_buffer_t)
    );

    gfx::font_load(&app->texture_arena, &app->default_font, "./assets/fonts/Go-Mono-Bold.ttf", 15.0f);
    app->default_font_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&app->texture_arena, 1);
    vk_gfx.load_font_sampler(
        &app->texture_arena,
        app->default_font_texture,
        &app->default_font);


    gfx::vul::texture_2d_t* ui_textures[64];
    for(size_t i = 0; i < array_count(ui_textures); i++) { ui_textures[i] = app->default_font_texture; }
    
    app->default_font_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        app->gui_pipeline->descriptor_set_layouts[0],
        ui_textures, 64);

    app->brick_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&app->texture_arena, 1);
    vk_gfx.load_texture_sampler(app->brick_texture, "./assets/textures/brick_01.png", &app->texture_arena);
    app->brick_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        app->mesh_pipeline->descriptor_set_layouts[3],
        &app->brick_texture, 1);

    vk_gfx.create_vertex_buffer(
        &app->vertices
    );
    vk_gfx.create_index_buffer(
        &app->indices
    );

    vk_gfx.create_vertex_buffer(
        &app->debug.debug_vertices
    );

    vk_gfx.create_vertex_buffer(
        &app->gui.vertices
    );
    vk_gfx.create_index_buffer(
        &app->gui.indices
    );

    app->gui.ctx.vertices = &app->gui.vertices.pool;
    app->gui.ctx.indices = &app->gui.indices.pool;
    app->gui.ctx.font = &app->default_font;
    app->gui.ctx.screen_size = v2f{
        (f32)app_mem->config.window_size[0], 
        (f32)app_mem->config.window_size[1]
    };

    using namespace gfx::color;
    gfx::gui::panel_t* panel = arena_alloc_ctor<gfx::gui::panel_t>(&app->gui.arena, 1, gfx::gui::panel_t{});
    app->gui.main_panel = panel;
    panel->theme.bg_color = "#1e1e1eff"_rgba;
    panel->min = v2f{0.0f, 0.0f};
    panel->max = v2f{400.0f, 200.0f};


    size_t text_count = 11;
    gfx::gui::text_t* arena_text = arena_alloc_ctor<gfx::gui::text_t>(&app->gui.arena, text_count, gfx::gui::text_t{});
    for (size_t i = 0; i < text_count; i++) {
        arena_text[i].next = i != text_count - 1 ? arena_text + i + 1 : 0; 
        arena_text[i].font_size = 12;
        arena_text[i].offset = v2f{10.0f, 15.0f * i + 20.f};
        arena_text[i].theme.text_color = gfx::color::rgba::white;
        arena_text[i].theme.shadow_color = gfx::color::rgba::black;
        arena_text[i].theme.shadow_distance = 1;
    }

    panel->text_widgets = arena_text;

    gfx::gui::panel_t* mesh_info_panel = arena_alloc_ctor<gfx::gui::panel_t>(&app->gui.arena, 1, gfx::gui::panel_t{});
    mesh_info_panel->min = v2f{app->gui.ctx.screen_size.x-200.0f, 0.0f};
    mesh_info_panel->max = v2f{app->gui.ctx.screen_size.x, 100.0f};
    mesh_info_panel->theme.bg_color = "#1e1e1eff"_rgba;
    
    size_t mesh_text_count = 5;
    gfx::gui::text_t* mesh_text = arena_alloc_ctor<gfx::gui::text_t>(&app->gui.arena, mesh_text_count, gfx::gui::text_t{});
    for (size_t i = 0; i < mesh_text_count; i++) {
        mesh_text[i].next = i != mesh_text_count - 1 ? mesh_text + i + 1 : 0; 
        mesh_text[i].font_size = 12;
        mesh_text[i].offset = v2f{10.0f, 15.0f * i + 20.f};
        mesh_text[i].theme.text_color = gfx::color::rgba::white;
        mesh_text[i].theme.shadow_color = gfx::color::rgba::black;
        mesh_text[i].theme.shadow_distance = 1;
    }
    mesh_info_panel->text_widgets = mesh_text;
    panel->next = mesh_info_panel;

    app->scene.sporadic_buffer.mode = 2;
    app->scene.sporadic_buffer.use_lighting = 1;

    
    gfx::mesh_builder_t mesh_builder{
        app->vertices.pool,
        app->indices.pool
    };
    
    static constexpr f32 terrain_dim = 100.0f;
    static constexpr f32 terrain_grid_size = 1.0f;

    for (f32 x = -terrain_dim; x < terrain_dim; x += terrain_grid_size) {
        for (f32 y = -terrain_dim; y < terrain_dim; y += terrain_grid_size) {
            gfx::vertex_t v[4];
            v[0].pos = v3f{x                    , 0.0f, y};
            v[1].pos = v3f{x                    , 0.0f, y + terrain_grid_size};
            v[2].pos = v3f{x + terrain_grid_size, 0.0f, y};
            v[3].pos = v3f{x + terrain_grid_size, 0.0f, y + terrain_grid_size};

            constexpr f32 noise_scale = 0.2f;
            for (size_t i = 0; i < 4; i++) {
                v[i].pos.y -= utl::noise::noise21(v2f{v[i].pos.x, v[i].pos.z} * noise_scale) * 4.0f;
                v[i].col = gfx::color::v3::dirt * 
                    (utl::noise::noise21(v2f{v[i].pos.x, v[i].pos.z} * noise_scale) 
                    * 0.5f + 0.5f);
            }

            const v3f n = glm::normalize(glm::cross(v[1].pos - v[0].pos, v[2].pos - v[0].pos));
            for (size_t i = 0; i < 4; i++) {
                v[i].nrm = n;
            }
            
            mesh_builder.add_quad(v);
        }
    }

    app->mesh_cache.add(
        &app->mesh_arena,
        mesh_builder.build(&app->mesh_arena)
    );
    utl::str_hash_create(app->mesh_hash);
    utl::str_hash_create(app->mesh_hash_meta);

    utl::str_hash_add(app->mesh_hash, "ground", 0);    


    // load all meshes from the resource file
    for (size_t i = 0; i < app->resource_file->file_count; i++) {
        if (app->resource_file->table[i].file_type != utl::res::magic::mesh) { 
            continue; 
        }

        std::byte* file_data = utl::res::pack_file_get_file(app->resource_file, i);

        auto loaded_mesh = load_bin_mesh_data(
            &app->mesh_arena,
            file_data, 
            &app->vertices.pool,
            &app->indices.pool
        );
        const auto mesh_id = app->mesh_cache.add(
            &app->mesh_arena,
            loaded_mesh
        );
        const char* file_name = app->resource_file->table[i].name.c_data;
        utl::str_hash_add(app->mesh_hash, file_name, mesh_id);

        assert(utl::str_hash_find(app->mesh_hash, file_name) != utl::invalid_hash);
    }

    game::entity::entity_t* e0 = game::world_create_entity(app->game_world);
    game::entity::entity_init(e0, 0);
    e0->transform.origin = v3f{0.0f, -10.0f, 0.0f};
    e0->name.view("ground");

    game::spawn(app->game_world, app->mesh_hash, game::entity::db::characters::soldier);
    
    const auto teapot_mesh = utl::str_hash_find(app->mesh_hash,"assets/models/utah-teapot.obj");

    game::entity::physics_entity_t* e1 =
        (game::entity::physics_entity_t*)
        game::world_create_entity(app->game_world, 1);
    
    e1->transform.scale(v3f{0.2f});
    e1->transform.origin = v3f{0,10,0};
    e1->name.view("Teapot");

    game::entity::physics_entity_init_convex(
        e1, 
        teapot_mesh,
        utl::res::pack_file_get_file_by_name(app->resource_file, "assets/models/utah-teapot.obj.convex.physx"),
        safe_truncate_u64(utl::res::pack_file_get_file_size(app->resource_file, "assets/models/utah-teapot.obj.convex.physx")),
        &physx_state
    );

    game::entity::physics_entity_t* e2 =
        (game::entity::physics_entity_t*)
        game::world_create_entity(app->game_world, 1);

    game::entity::physics_entity_init_trimesh(
        e2, 
        utl::str_hash_find(app->mesh_hash, "assets/models/rooms/room_0.obj"),
        utl::res::pack_file_get_file_by_name(app->resource_file, "assets/models/rooms/room_0.obj.trimesh.physx"),
        safe_truncate_u64(utl::res::pack_file_get_file_size(app->resource_file, "assets/models/rooms/room_0.obj.trimesh.physx")),
        &physx_state
    );
    e2->name.view("room");
    
    arena_clear(&app->temp_arena);
    app->game_world->player.transform.origin.y = 5.0f;
    app->game_world->player.transform.origin.z = -5.0f;
}

export_fn(void) 
app_on_deinit(app_memory_t* app_mem) {
    app_t* app = get_app(app_mem);

    gfx::vul::state_t& vk_gfx = app->gfx;
    vk_gfx.cleanup();

    app->~app_t();
}

export_fn(void) 
app_on_unload(app_memory_t* app_mem) {
    // app_t* app = get_app(app_mem);

    // app->gfx.cleanup();
}

export_fn(void)
app_on_reload(app_memory_t* app_mem) {
    // s_app_mem = app_mem;
    // app_t* app = get_app(app_mem);

    

    // app->gfx.init(&app_mem->config);
}


void 
camera_input(app_t* app, app_input_t* input) {
    app->game_world->player.camera_controller.transform.origin = 
        app->game_world->player.transform.origin;

    v3f move = v3f{
        f32(input->keys['D']) - f32(input->keys['A']),
        f32(input->keys['Q']) - f32(input->keys['E']),
        f32(input->keys['W']) - f32(input->keys['S'])
    };

    v2f head_move = 200.0f * v2f{
        input->mouse.delta[0],
        input->mouse.delta[1]
    } / app->gui.ctx.screen_size;

    if (input->gamepads[0].is_connected) {
        move = v3f{
            input->gamepads[0].left_stick.x,
            f32(input->gamepads[0].shoulder_left.is_held) - f32(input->gamepads[0].shoulder_right.is_held),
            -input->gamepads[0].left_stick.y,
        };
        head_move = v2f{
            input->gamepads[0].right_stick.x,
            input->gamepads[0].right_stick.y,
        } * 2.0f;
    }

    auto& yaw = app->game_world->player.camera_controller.yaw;
    auto& pitch = app->game_world->player.camera_controller.pitch;

    const v3f forward   = game::cam::get_direction(yaw, pitch);
    const v3f up        = v3f{0.0f, 1.0f, 0.0f};
    const v3f right     = glm::cross(forward, up);

    if (input->mouse.buttons[1] || input->gamepads[0].is_connected) {
        yaw += head_move.x * input->dt;
        pitch -= head_move.y * input->dt;
    }

    app->game_world->player.camera_controller.translate((forward * move.z + right * move.x + up * move.y) * 10.0f * input->dt);
    app->game_world->player.transform.origin = 
        app->game_world->player.camera_controller.transform.origin;
}

void 
app_on_input(app_t* app, app_input_t* input) {
    camera_input(app, input);

    if (input->gamepads[0].dpad_left.is_released) {
        app->time_scale *= 0.5f;
        app->time_text_anim = 1.0f;
    }
    if (input->gamepads[0].dpad_right.is_released) {
        app->time_scale *= 2.0f;
        app->time_text_anim = 1.0f;
    }

    if (input->pressed.keys['P']) {
        app->debug.mesh_sub_index++;
    }

    if (input->pressed.keys['O']) {
        app->debug.mesh_index++;
    }

    if (input->keys['M']) {
        app->scene.sporadic_buffer.mode = 1;
    } else if (input->keys['N']) {
        app->scene.sporadic_buffer.mode = 0;
    } else if (input->keys['B']) {
        app->scene.sporadic_buffer.mode = 2;
    }
    if (input->keys['L']) {
        app->scene.sporadic_buffer.use_lighting = 1;
    } else if (input->keys['K']) {
        app->scene.sporadic_buffer.use_lighting = 0;
    }
}

void
game_on_update(app_memory_t* app_mem) {
    // utl::profile_t p{"on_update"};
    app_t* app = get_app(app_mem);

    app_mem->input.dt *= app->time_scale;

    app_on_input(app, &app_mem->input);

    gfx::vul::state_t& vk_gfx = app->gfx;

    // note(zack): print the first 3 frames, if 
    // a bug shows up its helpful to know if 
    // maybe it only happens on the second frame
    local_persist u32 frame_count = 0;
    if (frame_count++ < 3) {
        gen_info("app::on_update", "frame: {}", frame_count);
    }

    const f32 anim_time = glm::mod(
        app_mem->input.time, 10.0f
    );

    // update uniforms

    app->scene.sporadic_buffer.num_instances = 1;

    const f32 w = (f32)app_mem->config.window_size[0];
    const f32 h = (f32)app_mem->config.window_size[1];
    const f32 aspect = (f32)app_mem->config.window_size[0] / (f32)app_mem->config.window_size[1];

    app->scene.scene_buffer.time = app_mem->input.time;
    app->scene.scene_buffer.view = app->game_world->camera.to_matrix();
    app->scene.scene_buffer.proj =
        app_mem->input.keys['U'] ?
        glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f,  -1000.0f, 1000.f) :
        glm::perspective(45.0f, aspect, 0.3f, 1000.0f);
    app->scene.scene_buffer.proj[1][1] *= -1.0f;

    app->scene.scene_buffer.scene = glm::mat4{0.5f};
    app->scene.scene_buffer.light_pos = v4f{3,13,3,0};
    app->scene.scene_buffer.light_col = v4f{0.2f, 0.1f, 0.8f, 1.0f};
    app->scene.scene_buffer.light_KaKdKs = v4f{0.5f};

    app->scene.object_buffer.color = v4f{1.0f};

    // draw gui 

    auto string_mark = arena_get_mark(&app->string_arena);

    arena_t* display_arenas[] = {
        0,
        &app->main_arena,
        &app->temp_arena,
        &app->string_arena,
        &app->mesh_arena,
        &app->texture_arena,
        app->game_world->arena,
        &app->vertices.pool,
        &app->indices.pool,
        &app->gui.vertices.pool,
        &app->gui.indices.pool,
    };

    const char* display_arena_names[] = {
        "Fps:",
        "Main Arena",
        "Temp Arena",
        "String Arena",
        "Mesh Arena",
        "Texture Arena",
        "Entity Arena",
        "3D Vertex",
        "3D Index",
        "2D Vertex",
        "2D Index",
    };

    app->gui.main_panel->text_widgets[0].text.own(&app->string_arena, fmt_str(" {:.2f} - {:.2f} ms", 1.0f / app_mem->input.dt, app_mem->input.dt * 1000.0f).c_str());
    for (size_t i = 1; i < array_count(display_arenas); i++) {
        app->gui.main_panel->text_widgets[i].text.own(&app->string_arena, arena_display_info(display_arenas[i], display_arena_names[i]).c_str());
    }

    const u64 mesh_index = app->debug.mesh_index%app->mesh_cache.meshes.size();
    const u64 sub_index = app->debug.mesh_sub_index%app->mesh_cache.get(mesh_index).count;

    std::string display_mesh_info[] = {
        fmt_str("Mesh Info: {} - {}", mesh_index, sub_index),
        fmt_str("Vertex Start: {}", app->mesh_cache.get(mesh_index).meshes[sub_index].vertex_start),
        fmt_str("Vertex Count: {}", app->mesh_cache.get(mesh_index).meshes[sub_index].vertex_count),
        fmt_str("Index Start: {}", app->mesh_cache.get(mesh_index).meshes[sub_index].index_start),
        fmt_str("Index Count: {}", app->mesh_cache.get(mesh_index).meshes[sub_index].index_count),
    };
    
    for (size_t i = 0; i < array_count(display_mesh_info); i++) {
        app->gui.main_panel->next->text_widgets[i].text.view(display_mesh_info[i].c_str());
    }

    gfx::gui::ctx_clear(&app->gui.ctx);

    gfx::gui::panel_render(&app->gui.ctx, app->gui.main_panel);

    gfx::gui::string_render(
        &app->gui.ctx, 
        string_t{}.view(fmt_str("Lighting Mode: {}", app->scene.sporadic_buffer.use_lighting).c_str()),
        v2f{10.0f, 300.0f}
    );
    
    app->time_text_anim -= app_mem->input.dt;
    if (app->time_text_anim > 0.0f) {
        gfx::gui::string_render(
            &app->gui.ctx, 
            string_t{}.view(fmt_str("Time Scale: {}", app->time_scale).c_str()),
            v2f{10.0f, 320.0f}, gfx::color::to_color32(v4f{0.80f, .9f, 1.0f, app->time_text_anim})
        );
    }

    const m44 vp = 
        app->scene.scene_buffer.proj *
        app->scene.scene_buffer.view;

    {
        app->scene.scene_buffer.light_pos.w = 1.0f;
        v4f sp = vp * app->scene.scene_buffer.light_pos;

        sp /= sp.w;
        sp.x = sp.x * 0.5f + 0.5f;
        sp.y = sp.y * 0.5f + 0.5f;

        gfx::gui::string_render(
            &app->gui.ctx, 
            string_t{}.view("Light"),
            v2f{sp.x, sp.y} * app->gui.ctx.screen_size, 
            gfx::color::to_color32(v4f{0.90f, .9f, 0.60f, 1.0f})
        );
    }

    for (size_t pool = 0; pool < game::pool_count(); pool++)  {
        for (game::entity::entity_t* e = game::pool_start(app->game_world, pool); e; e = e->next) {

            v4f sp = vp * v4f{e->global_transform().origin, 1.0f};

            sp /= sp.w;
            sp.x = sp.x * 0.5f + 0.5f;
            sp.y = sp.y * 0.5f + 0.5f;
            

            gfx::gui::string_render(
                &app->gui.ctx, 
                e->name.c_data ? 
                string_t{}.view(fmt_str("Entity: {}", std::string_view{e->name}).c_str()) :
                string_t{}.view(fmt_str("Entity: {}", (void*)e).c_str()),
                v2f{sp.x, sp.y} * app->gui.ctx.screen_size, 
                gfx::color::to_color32(v4f{0.80f, .9f, 1.0f, 1.0f})
            );
        }
    }

    arena_set_mark(&app->string_arena, string_mark);


    *vk_gfx.scene_uniform_buffer.data = app->scene.scene_buffer;
    *vk_gfx.sporadic_uniform_buffer.data = app->scene.sporadic_buffer;
    *vk_gfx.object_uniform_buffer.data = app->scene.object_buffer;
    *app->scene.debug_scene_uniform_buffer.data = app->scene.scene_buffer;
    // render

    app->debug.debug_vertices.pool.clear();
        
    vkWaitForFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[0], VK_TRUE, UINT64_MAX);
    vkResetFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[0]);

    u32 imageIndex;
    vkAcquireNextImageKHR(vk_gfx.device, vk_gfx.swap_chain, UINT64_MAX, 
        vk_gfx.image_available_semaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(vk_gfx.command_buffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(vk_gfx.command_buffer, &beginInfo) != VK_SUCCESS) {
        gen_error("vulkan:record_command", "failed to begin recording command buffer!");
        std::terminate();
    }
    
    vk_gfx.record_pipeline_commands(
        app->mesh_pipeline->pipeline, 
        app->mesh_pipeline->render_passes[0],
        app->mesh_pipeline->framebuffers[imageIndex],
        vk_gfx.command_buffer, imageIndex, [&]{

        vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline->pipeline_layout,
            0, 3, app->mesh_pipeline->descriptor_sets, 0, nullptr);

        vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline->pipeline_layout,
            3, 1, &app->brick_descriptor, 0, nullptr);

        VkBuffer buffers[1] = { app->vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(vk_gfx.command_buffer,
            0, 1, buffers, offsets);

        vkCmdBindIndexBuffer(vk_gfx.command_buffer,
            app->indices.buffer, 0, VK_INDEX_TYPE_UINT32
        );

        const u32 instance_count = 1;
        const u32 first_instance = 0;

        const auto draw = [&](game::entity::entity_t* e) {
            const auto& meshes = app->mesh_cache.get(e->gfx.mesh_id%app->mesh_cache.meshes.size());
            gfx::vul::object_push_constants_t push_constants;

            if ((e->flags & game::entity::EntityFlags_Renderable) == 0) { 
                gen_warn("render", "Skipping: {} - {}", (void*)e, e->flags);
                return;
            }

            push_constants.model = e->global_transform().to_matrix();
            push_constants.material.albedo = gfx::color::v4::green;

            // push constants cannot be bigger
            static_assert(sizeof(push_constants) <= 256);
            vkCmdPushConstants(vk_gfx.command_buffer, app->mesh_pipeline->pipeline_layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                0, sizeof(gfx::vul::object_push_constants_t), &push_constants
            );
        
            for (size_t j = 0; j < meshes.count; j++) {
                app->debug.draw_aabb(
                    e->global_transform().xform_aabb(meshes.meshes[j].aabb),
                    gfx::color::v3::red
                );
                if (meshes.meshes[j].index_count) {
                    vkCmdDrawIndexed(vk_gfx.command_buffer,
                        meshes.meshes[j].index_count,
                        instance_count, 
                        meshes.meshes[j].index_start,
                        meshes.meshes[j].vertex_start,
                        first_instance           
                    );
                } else {
                    vkCmdDraw(vk_gfx.command_buffer,
                        meshes.meshes[j].vertex_count,
                        meshes.meshes[j].instance_count,
                        meshes.meshes[j].vertex_start,
                        meshes.meshes[j].instance_start
                    );
                }
            }
        };

        // loop through type erased pools
        draw(&app->game_world->player);                
        for (size_t pool = 0; pool < game::pool_count(); pool++)  {
            for (game::entity::entity_t* e = game::pool_start(app->game_world, pool); e; e = e->next) {
                draw(e);                
            }
        }
    });

    
    vk_gfx.record_pipeline_commands(
        app->debug_pipeline->pipeline, app->debug_pipeline->render_passes[0],
        app->debug_pipeline->framebuffers[imageIndex],
        vk_gfx.command_buffer, imageIndex, 
        [&]{

        vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->debug_pipeline->pipeline_layout,
            0, 1, app->debug_pipeline->descriptor_sets, 0, nullptr);

        VkBuffer buffers[1] = { app->debug.debug_vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(vk_gfx.command_buffer,
            0, 1, buffers, offsets);

        const u32 instance_count = 1;
        const u32 first_instance = 0;

        vkCmdDraw(vk_gfx.command_buffer,
            (u32)app->debug.debug_vertices.pool.count,
            1, // instance count
            0, // vertex start
            0 // instance start
        );
    });

    vk_gfx.record_pipeline_commands(
        app->gui_pipeline->pipeline, app->gui_pipeline->render_passes[0],
        app->gui_pipeline->framebuffers[imageIndex],
        vk_gfx.command_buffer, imageIndex, 
        [&]{
            vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_pipeline->pipeline_layout,
                0, 1, &app->default_font_descriptor, 0, nullptr);

            VkDeviceSize offsets[1] = { 0 };
            vkCmdBindVertexBuffers(vk_gfx.command_buffer,
                0, 1, &app->gui.vertices.buffer, offsets);

            vkCmdBindIndexBuffer(vk_gfx.command_buffer,
                app->gui.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(vk_gfx.command_buffer,
                (u32)app->gui.ctx.indices->count,
                1,
                0,
                0,
                0
            );
    });
    
    if (vkEndCommandBuffer(vk_gfx.command_buffer) != VK_SUCCESS) {
        gen_error("vulkan:record_command:end", "failed to record command buffer!");
        std::terminate();
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {vk_gfx.image_available_semaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vk_gfx.command_buffer;

    VkSemaphore signalSemaphores[] = {vk_gfx.render_finished_semaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vk_gfx.gfx_queue, 1, &submitInfo, vk_gfx.in_flight_fence[0]) != VK_SUCCESS) {
        gen_error("vk:submit", "failed to submit draw command buffer!");
        std::terminate();
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {vk_gfx.swap_chain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(vk_gfx.present_queue, &presentInfo);
}

void
main_menu_on_update(app_memory_t* app_mem) {
    // utl::profile_t p{"on_update"};
    app_t* app = get_app(app_mem);

    app_mem->input.dt *= app->time_scale;

    gfx::vul::state_t& vk_gfx = app->gfx;

    // note(zack): print the first 3 frames, if 
    // a bug shows up its helpful to know if 
    // maybe it only happens on the second frame
    local_persist u32 frame_count = 0;
    if (frame_count++ < 3) {
        gen_info("app::on_update", "frame: {}", frame_count);
    }

    app->scene.sporadic_buffer.num_instances = 1;

    const f32 w = (f32)app_mem->config.window_size[0];
    const f32 h = (f32)app_mem->config.window_size[1];
    const f32 aspect = (f32)app_mem->config.window_size[0] / (f32)app_mem->config.window_size[1];

    app->scene.scene_buffer.time = app_mem->input.time;
    app->scene.scene_buffer.view = app->game_world->camera.to_matrix();
    app->scene.scene_buffer.proj =
        app_mem->input.keys['U'] ?
        glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f,  -1000.0f, 1000.f) :
        glm::perspective(45.0f, aspect, 0.3f, 1000.0f);
    app->scene.scene_buffer.proj[1][1] *= -1.0f;

    app->scene.scene_buffer.scene = glm::mat4{0.5f};
    app->scene.scene_buffer.light_pos = v4f{3,13,3,0};
    app->scene.scene_buffer.light_col = v4f{0.2f, 0.1f, 0.8f, 1.0f};
    app->scene.scene_buffer.light_KaKdKs = v4f{0.5f};

    app->scene.object_buffer.color = v4f{1.0f};

    // draw gui 

    auto string_mark = arena_get_mark(&app->string_arena);

    gfx::gui::ctx_clear(
        &app->gui.ctx
    );

    gfx::gui::string_render(
        &app->gui.ctx, 
        string_t{}.view("Play"),
        app->gui.ctx.screen_size/2.0f
    );

    arena_set_mark(&app->string_arena, string_mark);

    *vk_gfx.scene_uniform_buffer.data = app->scene.scene_buffer;
    *vk_gfx.sporadic_uniform_buffer.data = app->scene.sporadic_buffer;
    *vk_gfx.object_uniform_buffer.data = app->scene.object_buffer;
    *app->scene.debug_scene_uniform_buffer.data = app->scene.scene_buffer;
    // render

    app->debug.debug_vertices.pool.clear();
        
    vkWaitForFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[0], VK_TRUE, UINT64_MAX);
    vkResetFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[0]);

    u32 imageIndex;
    vkAcquireNextImageKHR(vk_gfx.device, vk_gfx.swap_chain, UINT64_MAX, 
        vk_gfx.image_available_semaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(vk_gfx.command_buffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(vk_gfx.command_buffer, &beginInfo) != VK_SUCCESS) {
        gen_error("vulkan:record_command", "failed to begin recording command buffer!");
        std::terminate();
    }

    vk_gfx.record_pipeline_commands(
        app->mesh_pipeline->pipeline, 
        app->mesh_pipeline->render_passes[0],
        app->mesh_pipeline->framebuffers[imageIndex],
        vk_gfx.command_buffer, imageIndex, [&]{

    });
    
    vk_gfx.record_pipeline_commands(
        app->gui_pipeline->pipeline, app->gui_pipeline->render_passes[0],
        app->gui_pipeline->framebuffers[imageIndex],
        vk_gfx.command_buffer, imageIndex, 
        [&]{
            vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_pipeline->pipeline_layout,
                0, 1, &app->default_font_descriptor, 0, nullptr);

            VkDeviceSize offsets[1] = { 0 };
            vkCmdBindVertexBuffers(vk_gfx.command_buffer,
                0, 1, &app->gui.vertices.buffer, offsets);

            vkCmdBindIndexBuffer(vk_gfx.command_buffer,
                app->gui.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(vk_gfx.command_buffer,
                (u32)app->gui.ctx.indices->count,
                1,
                0,
                0,
                0
            );
    });
    
    if (vkEndCommandBuffer(vk_gfx.command_buffer) != VK_SUCCESS) {
        gen_error("vulkan:record_command:end", "failed to record command buffer!");
        std::terminate();
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {vk_gfx.image_available_semaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vk_gfx.command_buffer;

    VkSemaphore signalSemaphores[] = {vk_gfx.render_finished_semaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vk_gfx.gfx_queue, 1, &submitInfo, vk_gfx.in_flight_fence[0]) != VK_SUCCESS) {
        gen_error("vk:submit", "failed to submit draw command buffer!");
        std::terminate();
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {vk_gfx.swap_chain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(vk_gfx.present_queue, &presentInfo);
}

export_fn(void) 
app_on_update(app_memory_t* app_mem) {
    local_persist u64 scene_state = 0;
    if (app_mem->input.keys[257 /*enter*/]) {
        scene_state = 1;
    }
    switch (scene_state) {
        case 0: // Main Menu
            main_menu_on_update(app_mem);
            break;
        case 1: // Game
            game_on_update(app_mem);
            break;
        default:
            gen_warn("scene", "Unknown scene: {}", scene_state);
    }
}
