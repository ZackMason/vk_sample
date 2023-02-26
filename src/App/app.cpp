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

#include "App/Game/Rendering/render_system.hpp"

struct app_t {
    app_memory_t*       app_mem{nullptr};

    // arenas
    arena_t             main_arena;
    arena_t             temp_arena;
    arena_t             string_arena;
    arena_t             mesh_arena;
    arena_t             texture_arena;

    utl::res::pack_file_t *     resource_file{0};

    gfx::vul::state_t   gfx;
    game::rendering::system_t* render_system{0};

    // todo(zack) clean this up
    gfx::font_t default_font;
    gfx::vul::texture_2d_t* default_font_texture{nullptr};
    VkDescriptorSet default_font_descriptor;

    gfx::vul::texture_2d_t* brick_texture{nullptr};
    VkDescriptorSet brick_descriptor;

    struct scene_t {
        gfx::vul::sporadic_buffer_t sporadic_buffer;

        gfx::vul::uniform_buffer_t<gfx::vul::scene_buffer_t>    debug_scene_uniform_buffer;

        struct lighting_t {
            v4f directional_light{1.0f, 2.0f, 3.0f, 1.0f};
        } lighting;
    } scene;
    
    gfx::vul::pipeline_state_t* gui_pipeline{0};
    gfx::vul::pipeline_state_t* debug_pipeline{0};
    gfx::vul::pipeline_state_t* mesh_pipeline{0};
    gfx::vul::pipeline_state_t* sky_pipeline{0};

    struct gui_state_t {
        gfx::gui::ctx_t ctx;
        gfx::vul::vertex_buffer_t<gfx::gui::vertex_t, 4'000'000> vertices;
        gfx::vul::index_buffer_t<6'000'000> indices;

        arena_t  arena;
    } gui;

    struct debugging_t {
        bool show = false;
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
        
    game::world_t* game_world{0};
    game::phys::physx_state_t* physics{0};

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

        loop_iota_u64(j, results.meshes[i].vertex_count) {
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

void
app_init_graphics(app_memory_t* app_mem) {
    app_t* app = get_app(app_mem);

    gfx::vul::state_t& vk_gfx = app->gfx;
    app->gfx.init(&app_mem->config);
    app->render_system = game::rendering::init<megabytes(256)>(vk_gfx, &app->main_arena);

    auto* rs = app->render_system;

    app->sky_pipeline = gfx::vul::create_skybox_pipeline(&app->mesh_arena, &vk_gfx, rs->render_passes[0]);
    app->gui_pipeline = gfx::vul::create_gui_pipeline(&app->mesh_arena, &vk_gfx, rs->render_passes[0]);
    app->mesh_pipeline = gfx::vul::create_mesh_pipeline(&app->mesh_arena, &vk_gfx, rs->render_passes[0],
        gfx::vul::mesh_pipeline_info_t{
            vk_gfx.sporadic_uniform_buffer.buffer,
            app->render_system->job_storage_buffer.buffer,
            app->render_system->material_storage_buffer.buffer,
            app->render_system->environment_storage_buffer.buffer
        }
    );
    assert(app->mesh_pipeline);

    vk_gfx.create_uniform_buffer(&app->scene.debug_scene_uniform_buffer);

    app->debug_pipeline = gfx::vul::create_debug_pipeline(
        &app->mesh_arena, 
        &vk_gfx, 
        rs->render_passes[0],
        app->scene.debug_scene_uniform_buffer.buffer, 
        sizeof(gfx::vul::scene_buffer_t)
    );

    gfx::font_load(&app->texture_arena, &app->default_font, "./assets/fonts/Go-Mono-Bold.ttf", 18.0f);
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
        app->mesh_pipeline->descriptor_set_layouts[4],
        &app->brick_texture, 1);

    vk_gfx.create_vertex_buffer(
        &app->render_system->vertices
    );
    vk_gfx.create_index_buffer(
        &app->render_system->indices
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
    app->gui.ctx.input = &app_mem->input;
    app->gui.ctx.screen_size = v2f{
        (f32)app_mem->config.window_size[0], 
        (f32)app_mem->config.window_size[1]
    };
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
    app->physics = arena_alloc_ctor<game::phys::physx_state_t>(
        main_arena, 1,
        *main_arena, megabytes(256)
    );
    
    game::phys::init_physx_state(*app->physics);

    app->resource_file = utl::res::load_pack_file(&app->mesh_arena, &app->string_arena, "./assets/res.pack");
    
    app->game_world = game::world_init(main_arena);
    
    // setup graphics

    app_init_graphics(app_mem);

    const auto make_material = [&](std::string_view n, auto&& mat) {
        return game::rendering::create_material(
            app->render_system, n, std::move(mat), app->mesh_pipeline->pipeline, app->mesh_pipeline->pipeline_layout
        );
    };

    u32 red_plastic_mat = make_material("red-plastic", gfx::material_t::plastic(gfx::color::v4::red));
    u32 blue_plastic_mat = make_material("blue-plastic", gfx::material_t::plastic(gfx::color::v4::blue));
    u32 gold_mat = make_material("gold-metal", gfx::material_t::metal(gfx::color::v4::yellow));

    make_material("silver-metal", gfx::material_t::metal(gfx::color::v4::white));
    make_material("sand", gfx::material_t::plastic(gfx::color::v4::sand));
    make_material("dirt", gfx::material_t::plastic(gfx::color::v4::dirt));
    
    using namespace gfx::color;

    app->scene.sporadic_buffer.mode = 2;
    app->scene.sporadic_buffer.use_lighting = 1;

    #if 0
    // gfx::mesh_builder_t mesh_builder{
    //     app->vertices.pool,
    //     app->indices.pool
    // };
    
    // static constexpr f32 terrain_dim = 100.0f;
    // static constexpr f32 terrain_grid_size = 1.0f;

    // for (f32 x = -terrain_dim; x < terrain_dim; x += terrain_grid_size) {
    //     for (f32 y = -terrain_dim; y < terrain_dim; y += terrain_grid_size) {
    //         gfx::vertex_t v[4];
    //         v[0].pos = v3f{x                    , 0.0f, y};
    //         v[1].pos = v3f{x                    , 0.0f, y + terrain_grid_size};
    //         v[2].pos = v3f{x + terrain_grid_size, 0.0f, y};
    //         v[3].pos = v3f{x + terrain_grid_size, 0.0f, y + terrain_grid_size};

    //         constexpr f32 noise_scale = 0.2f;
    //         for (size_t i = 0; i < 4; i++) {
    //             v[i].pos.y -= utl::noise::noise21(v2f{v[i].pos.x, v[i].pos.z} * noise_scale) * 4.0f;
    //             v[i].col = gfx::color::v3::dirt * 
    //                 (utl::noise::noise21(v2f{v[i].pos.x, v[i].pos.z} * noise_scale) 
    //                 * 0.5f + 0.5f);
    //         }

    //         const v3f n = glm::normalize(glm::cross(v[1].pos - v[0].pos, v[2].pos - v[0].pos));
    //         for (size_t i = 0; i < 4; i++) {
    //             v[i].nrm = n;
    //         }
            
    //         mesh_builder.add_quad(v);
    //     }
    // }

    // app->mesh_cache.add(
    //     &app->mesh_arena,
    //     mesh_builder.build(&app->mesh_arena)
    // );
    // utl::str_hash_create(app->mesh_hash);
    // utl::str_hash_create(app->mesh_hash_meta);

    // utl::str_hash_add(app->mesh_hash, "ground", 0);    
    // game::rendering::add_mesh(app->render_system, "ground", app->mesh_cache.get(0));
    #endif

    // load all meshes from the resource file
    for (size_t i = 0; i < app->resource_file->file_count; i++) {
        if (app->resource_file->table[i].file_type != utl::res::magic::mesh) { 
            continue; 
        }

        std::byte* file_data = utl::res::pack_file_get_file(app->resource_file, i);

        auto loaded_mesh = load_bin_mesh_data(
            &app->mesh_arena,
            file_data, 
            &app->render_system->vertices.pool,
            &app->render_system->indices.pool
        );
        const char* file_name = app->resource_file->table[i].name.c_data;

        game::rendering::add_mesh(app->render_system, file_name, loaded_mesh);
    }

    game::spawn(app->game_world, app->render_system->mesh_hash, game::entity::db::characters::soldier);
    auto* e2 = game::spawn(app->game_world, app->render_system->mesh_hash, game::entity::db::rooms::room_0);


    utl::rng::random_t<utl::rng::xor64_random_t> rng;
    loop_iota_u64(i, 100) {
        game::entity::physics_entity_t* e1 =
            (game::entity::physics_entity_t*)
            game::world_create_entity(app->game_world, 1);
        
        e1->transform.origin = rng.randnv<v3f>() * 100.0f;
        e1->transform.set_scale(v3f{0.2f});
        // e1->transform.set_scale(v3f{rng.randf()*10.0f});
        e1->transform.set_rotation(rng.randnv<v3f>());
        e1->name.view("Teapot");

        game::entity::physics_entity_init_convex(
            e1, 
            game::rendering::get_mesh_id(app->render_system, "assets/models/utah-teapot.obj"),
            utl::res::pack_file_get_file_by_name(app->resource_file, "assets/models/utah-teapot.obj.convex.physx"),
            safe_truncate_u64(utl::res::pack_file_get_file_size(app->resource_file, "assets/models/utah-teapot.obj.convex.physx")),
            app->physics
        );
        // e1->gfx.material_id = rng.rand() & BIT(23) ? gold_mat : red_plastic_mat;
        e1->gfx.material_id = rng.rand() % 6;
    }

    // game::entity::physics_entity_t* e2 =
    //     (game::entity::physics_entity_t*)
    //     game::world_create_entity(app->game_world, 1);

    // game::entity::physics_entity_init_trimesh(
    //     e2, 
    //     game::rendering::get_mesh_id(app->render_system, "assets/models/rooms/room_0.obj"),
    //     utl::res::pack_file_get_file_by_name(app->resource_file, "assets/models/rooms/room_0.obj.trimesh.physx"),
    //     safe_truncate_u64(utl::res::pack_file_get_file_size(app->resource_file, "assets/models/rooms/room_0.obj.trimesh.physx")),
    //     app->physics
    // );
    // e2->name.view("room");

    e2->gfx.material_id = blue_plastic_mat;
    
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
    
    const v3f move = v3f{
        input->gamepads[0].left_stick.x,
        f32(input->gamepads[0].buttons[button_id::shoulder_left].is_held) - 
        f32(input->gamepads[0].buttons[button_id::shoulder_right].is_held),
        -input->gamepads[0].left_stick.y,
    };
    const v2f head_move = v2f{
        input->gamepads[0].right_stick.x,
        input->gamepads[0].right_stick.y,
    } * 2.0f;

    auto& yaw = app->game_world->player.camera_controller.yaw;
    auto& pitch = app->game_world->player.camera_controller.pitch;

    const v3f forward   = game::cam::get_direction(yaw, pitch);
    const v3f up        = v3f{0.0f, 1.0f, 0.0f};
    const v3f right     = glm::cross(forward, up);

    // if (input->mouse.buttons[1] || input->gamepads[0].is_connected) {
    yaw += head_move.x * input->dt;
    pitch -= head_move.y * input->dt;
    // }

    app->game_world->player.camera_controller.translate((forward * move.z + right * move.x + up * move.y) * 10.0f * input->dt);
    app->game_world->player.transform.origin = 
        app->game_world->player.camera_controller.transform.origin;
}

void fake_controller_input(app_input_t* input, v2f screen_size) {
    // if (input->gamepads[0].is_connected == false || 1) {
        const v3f move = v3f{
            f32(input->keys['D']) - f32(input->keys['A']),
            f32(input->keys['Q']) - f32(input->keys['E']),
            f32(input->keys['S']) - f32(input->keys['W'])
        };

        // todo clamp
        const v2f head_move = (f32(input->mouse.buttons[0]) * 100.0f * v2f{
            input->mouse.delta[0], 
            input->mouse.delta[1] 
        } / screen_size) + 
        v2f {
            f32(input->keys['L']) - f32(input->keys['J']),
            f32(input->keys['I']) - f32(input->keys['K'])
        };

        input->gamepads->left_stick.x += move.x;
        input->gamepads->left_stick.y += move.z;

        input->gamepads->right_stick.x += head_move.x;
        input->gamepads->right_stick.y += head_move.y;

        input->gamepads->buttons[button_id::shoulder_left].is_held |= input->keys['Q'];
        input->gamepads->buttons[button_id::shoulder_right].is_held |= input->keys['E'];
    // }
}

void 
app_on_input(app_t* app, app_input_t* input) {
    fake_controller_input(input, app->gui.ctx.screen_size);
    camera_input(app, input);

    if (input->gamepads[0].buttons[button_id::dpad_left].is_released) {
        app->time_scale *= 0.5f;
        app->time_text_anim = 1.0f;
    }
    if (input->gamepads[0].buttons[button_id::dpad_right].is_released) {
        app->time_scale *= 2.0f;
        app->time_text_anim = 1.0f;
    }
}

void game_on_gameplay(app_t* app, app_input_t* input) {
    input->dt *= app->time_scale;
    app_on_input(app, input);

    // TODO(ZACK): GAME CODE HERE

    // Submit renderables
    

    // local_persist bool do_once = true;
    // loop through type erased pools
    // if (do_once)
    for (size_t pool = 0; pool < game::pool_count(); pool++)  {
        // do_once = false;
        for (game::entity::entity_t* e = game::pool_start(app->game_world, pool); e; e = e->next) {
            if ((e->flags & game::entity::EntityFlags_Renderable) == 0) { 
                gen_warn("render", "Skipping: {} - {}", (void*)e, e->flags);
                return;
            }
            game::rendering::submit_job(
                app->render_system, 
                e->gfx.mesh_id, 
                e->gfx.material_id, 
                e->global_transform().to_matrix()
            );
        }
    }
}

void game_on_render(app_t* app) {

}

void
game_on_update(app_memory_t* app_mem) {
    // utl::profile_t p{"on_update"};
    app_t* app = get_app(app_mem);

    game_on_gameplay(app, &app_mem->input);


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

    {

        app->render_system->camera_pos = app->game_world->camera.affine_invert().origin;
        m44 proj = (app_mem->input.keys['U'] ?
            glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f,  -1000.0f, 1000.f) :
            glm::perspective(45.0f, aspect, 0.3f, 1000.0f));
        proj[1][1] *= -1.0f;
        app->render_system->vp = proj * app->game_world->camera.to_matrix();
    
    }
    // draw gui 

    auto string_mark = arena_get_mark(&app->string_arena);

    arena_t* display_arenas[] = {
        &app->main_arena,
        &app->temp_arena,
        &app->string_arena,
        &app->mesh_arena,
        &app->texture_arena,
        app->game_world->arena,
        &app->physics->default_allocator.arena,
        &app->render_system->arena,
        &app->render_system->frame_arena,
        &app->render_system->vertices.pool,
        &app->render_system->indices.pool,
        &app->gui.vertices.pool,
        &app->gui.indices.pool,
    };

    const char* display_arena_names[] = {
        "- Main Arena",
        "- Temp Arena",
        "- String Arena",
        "- Mesh Arena",
        "- Texture Arena",
        "- Entity Arena",
        "- Physics Arena",
        "- Rendering Arena",
        "- Rendering Frame Arena",
        "- 3D Vertex",
        "- 3D Index",
        "- 2D Vertex",
        "- 2D Index",
    };

    gfx::gui::ctx_clear(&app->gui.ctx);
    
    app->time_text_anim -= app_mem->input.dt;
    if (app->time_text_anim > 0.0f) {
        gfx::gui::string_render(
            &app->gui.ctx, 
            string_t{}.view(fmt_str("Time Scale: {}", app->time_scale).c_str()),
            app->gui.ctx.screen_size/2.0f, gfx::color::to_color32(v4f{0.80f, .9f, 1.0f, app->time_text_anim})
        );
    }

    local_persist gfx::gui::im::state_t state{
        .ctx = app->gui.ctx,
        .theme = gfx::gui::theme_t {
            .fg_color = gfx::color::rgba::gray,
            // .bg_color = gfx::color::rgba::purple,
            .bg_color = gfx::color::rgba::dark_gray,
            .text_color = gfx::color::rgba::cream,
            .disabled_color = gfx::color::rgba::dark_gray,
            .border_color = gfx::color::rgba::white,

            .padding = 4.0f,
        },
    };

    {
        using namespace std::string_view_literals;
        using namespace gfx::gui;

        local_persist bool show_entities = false;
        if (im::begin_panel(state, "Main UI"sv)) {

            local_persist bool show_stats = false;
            local_persist bool show_resources = false;
            local_persist bool show_window = false;
            local_persist bool show_gui = !false;
            local_persist bool show_theme = !false;
            local_persist bool show_colors= !false;
            local_persist bool show_files[0xff];

            if (im::text(state, "Stats"sv, &show_stats)) {
                im::text(state, fmt_sv("- FPS: {:.2f} - {:.2f} ms", 1.0f / app_mem->input.dt, app_mem->input.dt * 1000.0f));
                
                if (im::text(state, "- Window"sv, &show_window)) {
                    im::text(state, fmt_sv("--- Width: {}", app_mem->config.window_size[0]));
                    im::text(state, fmt_sv("--- Height: {}", app_mem->config.window_size[1]));
                }

                if (im::text(state, "- GUI"sv, &show_gui)) {
                    im::text(state, fmt_sv("--- Active: {:X}", state.active.id));
                    im::text(state, fmt_sv("--- Hot: {:X}", state.hot.id));
                    if (im::text(state, "--- Theme"sv, &show_theme)) {
                        im::text(state, fmt_sv("----- Margin: {}", state.theme.margin));
                        im::text(state, fmt_sv("----- Padding: {}", state.theme.padding));
                        im::text(state, fmt_sv("----- Shadow Offset: {}", state.theme.shadow_distance));
                        if (im::text(state, "----- Color"sv, &show_colors)) {
                            local_persist gfx::color32* edit_color = &state.theme.fg_color;

                            im::color_edit(state, edit_color);

                            if (im::text(state, fmt_sv("------- Foreground: {:X}", state.theme.fg_color))) {
                                edit_color = &state.theme.fg_color;
                            }
                            if (im::text(state, fmt_sv("------- Background: {:X}", state.theme.bg_color))) {
                                edit_color = &state.theme.bg_color;
                            }
                            if (im::text(state, fmt_sv("------- Text: {:X}", state.theme.text_color))) {
                                edit_color = &state.theme.text_color;
                            }
                            if (im::text(state, fmt_sv("------- Border: {:X}", state.theme.border_color))) {
                                edit_color = &state.theme.border_color;
                            }
                            if (im::text(state, fmt_sv("------- Active: {:X}", state.theme.active_color))) {
                                edit_color = &state.theme.active_color;
                            }
                            if (im::text(state, fmt_sv("------- Disabled: {:X}", state.theme.disabled_color))) {
                                edit_color = &state.theme.disabled_color;
                            }
                            if (im::text(state, fmt_sv("------- Shadow: {:X}", state.theme.shadow_color))) {
                                edit_color = &state.theme.shadow_color;
                            }
                        }
                    }
                }

                if (im::text(state, fmt_sv("- Light Mode: {}", app->scene.sporadic_buffer.use_lighting))) {
                    app->scene.sporadic_buffer.use_lighting = !app->scene.sporadic_buffer.use_lighting;
                }
                if (im::text(state, fmt_sv("- Mode: {}", app->scene.sporadic_buffer.mode))) {
                    app->scene.sporadic_buffer.mode = ++app->scene.sporadic_buffer.mode % 4;
                }
                if (im::text(state, fmt_sv("- Resource File Count: {}", app->resource_file->file_count), &show_resources)) {
                    loop_iota_u64(rf, app->resource_file->file_count) {
                        assert(rf < array_count(show_files));
                        if (im::text(state, fmt_sv("--- File Name: {}", app->resource_file->table[rf].name.c_data), show_files + rf)) {
                            im::text(state, fmt_sv("----- Size: {} bytes", app->resource_file->table[rf].size));
                            im::text(state, fmt_sv("----- Type: {:X}", app->resource_file->table[rf].file_type));
                        }
                    }
                }
            }

            if (im::text(state, "Debug"sv, &app->debug.show)) {
                if(im::text(state, "- Breakpoint"sv)) {
                    volatile u32 _tx=0;
                    _tx += 1;
                }
            }

            local_persist bool show_gfx = !false;
            local_persist bool show_mats = false;
            local_persist bool show_env = !false;
            local_persist bool show_mat_id[8] = {};
            if (im::text(state, "Graphics"sv, &show_gfx)) { 
                local_persist bool show_sky = false;
                if (im::text(state, "- Environment"sv, &show_env)) { 
                    auto& env = app->render_system->environment_storage_buffer.pool[0];
                    local_persist bool show_amb = false;
                    if (im::text(state, "--- Ambient"sv, &show_amb)) { 
                        gfx::color32 tc = gfx::color::to_color32(env.ambient_color);
                        im::color_edit(state, &tc);
                        env.ambient_color = gfx::color::to_color4(tc);
                    }
                    local_persist bool show_fog = !false;
                    if (im::text(state, "--- Fog"sv, &show_fog)) { 
                        gfx::color32 tc = gfx::color::to_color32(env.fog_color);
                        im::color_edit(state, &tc);
                        env.fog_color = gfx::color::to_color4(tc);
                        im::float_slider(state, &env.fog_density);
                    }
                }
                if (im::text(state, "- Materials"sv, &show_mats)) { 
                    loop_iota_u64(i, app->render_system->materials.size()) {
                        auto* mat = app->render_system->materials[i];
                        if (im::text(state, fmt_sv("--- Name: {}", std::string_view{mat->name}), show_mat_id + i)) {
                            gfx::color32 color = gfx::color::to_color32(mat->albedo);
                            im::color_edit(state, &color);
                            mat->albedo = gfx::color::to_color4(color);

                            app->render_system->material_storage_buffer.pool[i] = *mat;
                        }
                    }
                }
                if (im::text(state, "- SkyBox"sv, &show_sky)) { 
                    local_persist bool show_dir = false;
                    local_persist bool show_color = false;
                    if (im::text(state, "--- Sun Color"sv, &show_color)) {
                        local_persist gfx::color32 sun_color = gfx::color::rgba::light_blue;
                        im::color_edit(state, &sun_color);
                    }
                    if (im::text(state, "--- Sun Direction"sv, &show_dir)) { 
                        im::float_slider(state, &app->scene.lighting.directional_light.x, -1.0f, 1.0f);
                        im::float_slider(state, &app->scene.lighting.directional_light.y, -1.0f, 1.0f);
                        im::float_slider(state, &app->scene.lighting.directional_light.z, -1.0f, 1.0f);
                    }
                }
            }

            local_persist bool show_arenas = false;
            if (im::text(state, "Memory"sv, &show_arenas)) {
                for (size_t i = 0; i < array_count(display_arenas); i++) {
                    im::text(state, arena_display_info(display_arenas[i], display_arena_names[i]));
                }
            }

            if (im::text(state, "Entities"sv, &show_entities)) {
                u64 entity_count = 1; // always has player
                for (size_t pool = 0; pool < game::pool_count(); pool++)  {
                    entity_count += game::get_pool(app->game_world, pool)->count;
                }

                im::text(state, fmt_sv("- Count: {}", entity_count));

                if (im::text(state, "- Kill All"sv)) {
                    for (size_t pool = 0; pool < game::pool_count(); pool++)  {
                        node_for(game::entity::entity_t, game::pool_start(app->game_world, pool), e) {
                            game::get_pool(app->game_world, pool)->clear();
                        }
                    }
                }
            }

            im::end_panel(state);
        }

        const m44 vp = 
            app->render_system->vp;

        for (size_t pool = 0; show_entities && pool < game::pool_count(); pool++)  {
            for (game::entity::entity_t* e = game::pool_start(app->game_world, pool); e; e = e->next) {
                const v3f ndc = math::world_to_screen(vp, e->global_transform().origin);
                
                if (im::begin_panel_3d(state, 
                    e->name.c_data ? 
                    fmt_sv("Entity: {}\0{}"sv, std::string_view{e->name}, (void*)e) :
                    fmt_sv("Entity: {}", (void*)e),
                    vp, e->global_transform().origin
                )) {
                    im::text(state, 
                        e->name.c_data ? 
                        fmt_sv("Entity: {}", std::string_view{e->name}) :
                        fmt_sv("Entity: {}", (void*)e)
                    );

                    im::text(state, 
                        fmt_sv("Screen Pos: {:.2f} {:.2f}", ndc.x, ndc.y)
                    );

                    im::text(state,
                        fmt_sv("Origin: {:.2f} {:.2f} {:.2f}", 
                            e->global_transform().origin.x,
                            e->global_transform().origin.y,
                            e->global_transform().origin.z
                        )
                    );

                    if (im::text(state, fmt_sv("kill\0: {}"sv, (void*)e))) {
                        auto* te = e->next;
                        gen_info("gui", "Killing entity: {}", (void*)e);
                        game::world_destroy_entity(app->game_world, e);
                        if (!te) {
                            break;
                        }
                        e = te;
                    }

                    im::end_panel(state);
                }
            }
        }

    }

    arena_set_mark(&app->string_arena, string_mark);


    *vk_gfx.sporadic_uniform_buffer.data = app->scene.sporadic_buffer;
    

    // render

    app->debug.debug_vertices.pool.clear();
        
    v3f cam_dir = 
        game::cam::get_direction(
            app->game_world->player.camera_controller.yaw, 
            app->game_world->player.camera_controller.pitch
        );
    v3f cam_pos = app->game_world->player.camera_controller.transform.origin + cam_dir * 3.0f;
    
    constexpr f32 axis_line_size = 0.1f;
    app->debug.draw_line(cam_pos, cam_pos + v3f{axis_line_size, 0.0f, 0.0f}, v3f{1.0f, 0.0f, 0.0f});
    app->debug.draw_line(cam_pos, cam_pos + v3f{0.0f, axis_line_size, 0.0f}, v3f{0.0f, 1.0f, 0.0f});
    app->debug.draw_line(cam_pos, cam_pos + v3f{0.0f, 0.0f, axis_line_size}, v3f{0.0f, 0.0f, 1.0f});
    
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

    gfx::vul::begin_render_pass(vk_gfx,
        app->render_system->render_passes[0],
        app->render_system->framebuffers[imageIndex],
        vk_gfx.command_buffer, imageIndex
    );
    
    
    vkCmdBindPipeline(vk_gfx.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->sky_pipeline->pipeline);

    // vk_gfx.record_pipeline_commands(
    //     app->sky_pipeline->pipeline, 
    //     app->sky_pipeline->render_passes[0],
    //     app->sky_pipeline->framebuffers[imageIndex],
    //     vk_gfx.command_buffer, imageIndex, [&]{
    {
        VkBuffer buffers[1] = { app->render_system->vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(vk_gfx.command_buffer,
            0, 1, buffers, offsets);

        vkCmdBindIndexBuffer(vk_gfx.command_buffer,
            app->render_system->indices.buffer, 0, VK_INDEX_TYPE_UINT32
        );

        struct sky_push_constant_t {
            m44 vp;
            v4f directional_light;
        } sky_constants;

        // const f32 aspect = (f32)app_mem->config.window_size[0] / (f32)app_mem->config.window_size[1];
        
        m44 proj = glm::perspective(45.0f, aspect, 0.3f, 1000.0f);
        proj[1][1] *= -1.0f;

        sky_constants.vp = proj * glm::lookAt(v3f{0.0f}, 
            game::cam::get_direction(
                app->game_world->player.camera_controller.yaw,
                app->game_world->player.camera_controller.pitch
            ), 
            v3f{0.0f, 1.0f, 0.0f}
        );
        sky_constants.directional_light = app->scene.lighting.directional_light;

        vkCmdPushConstants(vk_gfx.command_buffer, app->sky_pipeline->pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(sky_push_constant_t), &sky_constants
        );

        const auto& meshes = app->render_system->mesh_cache.get(
            utl::str_hash_find(app->render_system->mesh_hash, "assets/models/sphere.obj")
        );
    
        for (size_t j = 0; j < meshes.count; j++) {
            vkCmdDrawIndexed(vk_gfx.command_buffer,
                meshes.meshes[j].index_count,
                1, 
                meshes.meshes[j].index_start,
                meshes.meshes[j].vertex_start,
                0           
            );
        }

        VkClearDepthStencilValue clearValue = {};
        clearValue.depth = 1.0f;
        clearValue.stencil = 0;

        // Specify the range of the image to be cleared
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        // Clear the depth buffer
        // vkCmdClearDepthStencilImage(vk_gfx.command_buffer, vk_gfx.depth_stencil_texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &subresourceRange);
        
    // });
    }


    // gfx::vul::utl::buffer_host_memory_barrier(vk_gfx.command_buffer, app->render_system->job_storage_buffer.buffer);
    // gfx::vul::utl::buffer_host_memory_barrier(vk_gfx.command_buffer, app->gui.vertices.buffer);
    // gfx::vul::utl::buffer_host_memory_barrier(vk_gfx.command_buffer, app->gui.indices.buffer);
    {
        vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline->pipeline_layout,
            0, 4, app->mesh_pipeline->descriptor_sets, 0, nullptr);

        vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline->pipeline_layout,
            4, 1, &app->brick_descriptor, 0, nullptr);

        VkBuffer buffers[1] = { app->render_system->vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(vk_gfx.command_buffer,
            0, 1, buffers, offsets);

        vkCmdBindIndexBuffer(vk_gfx.command_buffer,
            app->render_system->indices.buffer, 0, VK_INDEX_TYPE_UINT32
        );
    }

    game::rendering::build_commands(app->render_system, vk_gfx.command_buffer);

    

    if (app->debug.show) {
    //     vk_gfx.record_pipeline_commands(
    //         app->debug_pipeline->pipeline, 
    //         app->debug_pipeline->render_passes[0],
    //         app->debug_pipeline->framebuffers[imageIndex],
    //         vk_gfx.command_buffer, imageIndex, 
    //         [&]{
    // {
        vkCmdBindPipeline(vk_gfx.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->debug_pipeline->pipeline);
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
    }

    vkCmdBindPipeline(vk_gfx.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_pipeline->pipeline);
    {

    // vk_gfx.record_pipeline_commands(
    //     app->gui_pipeline->pipeline, app->gui_pipeline->render_passes[0],
    //     app->gui_pipeline->framebuffers[imageIndex],
    //     vk_gfx.command_buffer, imageIndex, 
    //     [&]{
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
    }

    gfx::vul::end_render_pass(vk_gfx.command_buffer);

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
}

export_fn(void) 
app_on_update(app_memory_t* app_mem) {
    local_persist u64 scene_state = 1;
    if (app_mem->input.keys[257 /*enter*/] || 
        app_mem->input.gamepads->buttons[button_id::action_down].is_pressed) {
        scene_state = 1;
    }
    switch (scene_state) {
        // case 0: // Main Menu
        //     main_menu_on_update(app_mem);
        //     break;
        case 1: // Game
            game_on_update(app_mem);
            break;
        default:
            gen_warn("scene", "Unknown scene: {}", scene_state);
            scene_state = 1;
    }
}
