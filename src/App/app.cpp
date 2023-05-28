#include <core.hpp>

#include "App/app.hpp"

#include "ProcGen/map_gen.hpp"
#include "ProcGen/mesh_builder.hpp"
#include "App/Game/World/world.hpp"

#include "App/Game/World/Level/level.hpp"
#include "App/Game/World/Level/Room/room.hpp"
#include "App/Game/Items/base_item.hpp"
#include "App/Game/Weapons/base_weapon.hpp"

global_variable gfx::gui::im::state_t* gs_imgui_state = 0;
global_variable game::world_t gs_saved_world;

void teapot_on_collision(
    physics::rigidbody_t* teapot,
    physics::rigidbody_t* other
) {
    auto* teapot_entity = (game::entity_t*) teapot->user_data;
    // gen_info(__FUNCTION__, "teapot hit: {} - id", entity->id);
    auto* saved_entity = game::find_entity_by_id(&gs_saved_world, teapot_entity->id);
    teapot_entity->transform.origin = saved_entity->transform.origin;
    teapot->position = saved_entity->transform.origin;
    
    teapot->flags = physics::rigidbody_flags::SKIP_SYNC;
}

void player_on_collision(
    physics::rigidbody_t* player,
    physics::rigidbody_t* other
) {
    auto* other_entity = (game::entity_t*) other->user_data;
    gen_info(__FUNCTION__, "hit: {} - id", other_entity->id);
}

namespace tween {

template <typename T, f32 D>
T in_expo(T b, T c, f32 t) {
    return (t==0) ? b : c * glm::pow(2, 10 * (t/D - 1.0f)) + b;
}

template <typename T, f32 D>
T out_expo(T b, T c, f32 t) {
    return (t==D) ? b+c : c * (-glm::pow(2, -10 * t/D) + 1) + b;
}

template <typename T, f32 D>
T in_out_expo(T b, T c, f32 t) {
    if (t==0) return b;
    if (t==D) return b+c;
    t /= D/2;
    if (t < 1) return c/2 * glm::pow(2, 10 * (t - 1)) + b;
    t--;
    return c/2 * (-glm::pow(2, -10 * t) + 2) + b;
}

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

        const size_t index_count = blob.deserialize<u64>();
        const size_t index_bytes = sizeof(u32) * index_count;
        results.meshes[i].index_count = safe_truncate_u64(index_count);

        u32* tris = indices->allocate(index_count);
        std::memcpy(tris, blob.read_data(), index_bytes);
        blob.advance(index_bytes);

        results.meshes[i].aabb = {};
        range_u64(j, 0, vertex_count) {
            results.meshes[i].aabb.expand(v[j].pos);
        }
    }

    results.aabb = {};
    range_u64(m, 0, results.count) {
        results.aabb.expand(results.meshes[m].aabb);
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

    
    if (file_data) {
        gfx::mesh_list_t results = load_bin_mesh_data(arena, file_data, vertices, indices);
    } else {
        gen_warn("app::load_bin_mesh", "Failed to open mesh file: {}", path);
    }

    return {};
}

inline static gfx::vul::texture_2d_t*
make_noise_texture(arena_t* arena, u32 size) {
    auto* tex = arena_alloc<gfx::vul::texture_2d_t>(arena);

    tex->size[0] = tex->size[1] = size;
    tex->channels = 4;
    tex->pixels = arena_alloc_ctor<u8>(arena, size*size*4);
    u64 i = 0;
    range_u64(x, 0, size) {
        range_u64(y, 0, size) {
            const auto n = glm::clamp(utl::noise::fbm(v2f{x,y} * 0.15f) * 0.5f + 0.5f, 0.0f, 1.0f);
            tex->pixels[i++] = u8(n*255.0f);
            tex->pixels[i++] = u8(n*255.0f);
            tex->pixels[i++] = u8(n*255.0f);
            tex->pixels[i++] = 255ui8;
        }
    }

    return tex;
}

inline app_t*
get_app(app_memory_t* mem) {
    // note(zack): App should always be the first thing initialized in perm_memory
    return (app_t*)mem->perm_memory;
}

inline static
game::rendering::lighting::ray_scene_t*
raytrace_test(app_memory_t* app_mem) {
    using namespace game::rendering;
    auto* app = get_app(app_mem);

    auto* scene = lighting::init_ray_scene(&app->main_arena, 64, 64);

    scene->camera.origin = v3f{0.0f, 0.0f, 10.0f};
    scene->camera.direction = v3f{0.0, 0.0, -1.0f};

    auto& teapot = get_mesh(app->render_system, "assets/models/utah-teapot.obj");
    auto* teapot_vertices = &app->render_system->vertices.pool[teapot.meshes->vertex_start];
    auto* teapot_indices = &app->render_system->indices.pool[teapot.meshes->index_start];
    auto teapot_n_vertices = teapot.meshes->vertex_count;
    auto teapot_n_indices = teapot.meshes->index_count;

    auto* teapot_model = lighting::add_model(scene, teapot_vertices, teapot_n_vertices, teapot_indices, teapot_n_indices);

    lighting::add_node(scene, &app->main_arena, teapot_model, math::transform_t{});

    lighting::raytrace(scene);

    return scene;
}

void
app_init_graphics(app_memory_t* app_mem) {
    TIMED_FUNCTION;
    app_t* app = get_app(app_mem);

    gfx::vul::state_t& vk_gfx = app->gfx;
    {
        temp_arena_t t = app->main_arena;
        app->gfx.init(&app_mem->config, &t);
    }
    
    app->render_system = game::rendering::init<megabytes(256)>(vk_gfx, &app->main_arena);
    vk_gfx.create_vertex_buffer(&app->render_system->vertices);
    vk_gfx.create_index_buffer(&app->render_system->indices);
    vk_gfx.create_vertex_buffer(&app->debug.debug_vertices);
    vk_gfx.create_vertex_buffer(&app->gui.vertices[0]);
    vk_gfx.create_index_buffer(&app->gui.indices[0]);
    vk_gfx.create_vertex_buffer(&app->gui.vertices[1]);
    vk_gfx.create_index_buffer(&app->gui.indices[1]);
    auto* rs = app->render_system;

    // load all meshes from the resource file
    range_u64(i, 0, app->resource_file->file_count) {
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

    app->gui_pipeline = gfx::vul::create_gui_pipeline(&app->mesh_arena, &vk_gfx, rs->render_passes[0]);
    app->sky_pipeline = gfx::vul::create_skybox_pipeline(&app->mesh_arena, &vk_gfx, rs->render_targets[0].render_pass);
    app->mesh_pipeline = gfx::vul::create_mesh_pipeline(&app->mesh_arena, &vk_gfx, rs->render_targets[0].render_pass,
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
        rs->render_targets[0].render_pass
    );

    gfx::font_load(&app->texture_arena, &app->default_font, "./assets/fonts/Go-Mono-Bold.ttf", 18.0f);
    app->default_font_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&app->texture_arena, 1);
    {
        temp_arena_t ta = app->texture_arena;
        vk_gfx.load_font_sampler(
            &ta,
            app->default_font_texture,
            &app->default_font);
    }

    gfx::vul::texture_2d_t* ui_textures[4096];
    for(size_t i = 0; i < array_count(ui_textures); i++) { ui_textures[i] = app->default_font_texture; }
    gfx::vul::texture_2d_t scene_c_fb;
    gfx::vul::texture_2d_t scene_d_fb;
    scene_c_fb.sampler = rs->render_targets->sampler;
    scene_c_fb.image_view = rs->render_targets->attachments->next->view;
    scene_c_fb.image_layout = rs->render_targets->attachments->next->description.finalLayout;
    ui_textures[1] = &scene_c_fb;
    
    scene_d_fb.sampler = rs->render_targets->sampler;
    scene_d_fb.image_view = rs->render_targets->attachments->view;
    scene_d_fb.image_layout = rs->render_targets->attachments->description.finalLayout;
    ui_textures[2] = &scene_d_fb;

    ui_textures[3] = make_noise_texture(&app->texture_arena, 128);
    vk_gfx.load_texture_sampler(ui_textures[3], &app->texture_arena);

    // gfx::vul::texture_2d_t rt_tex;
    
    // auto* scene = raytrace_test(app_mem);

    // rt_tex.size[0] = (i32)scene->width;
    // rt_tex.size[1] = (i32)scene->height;
    // rt_tex.pixels = (u8*)scene->pixels;
    // rt_tex.channels = 4;
    // vk_gfx.load_texture_sampler(&rt_tex, &app->texture_arena);
    // ui_textures[4] = &rt_tex;

    app->default_font_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        app->gui_pipeline->descriptor_set_layouts[0],
        ui_textures, array_count(ui_textures));

    app->brick_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&app->texture_arena, 1);
    {
        temp_arena_t ta = app->texture_arena;
        vk_gfx.load_texture_sampler(app->brick_texture, "./assets/textures/rock_01_a.png", &ta);
    }
    app->brick_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        app->mesh_pipeline->descriptor_set_layouts[4],
        &app->brick_texture, 1);

  

    app->gui.ctx.font = &app->default_font;
    app->gui.ctx.input = &app_mem->input;
    app->gui.ctx.screen_size = v2f{
        (f32)app_mem->config.window_size[0], 
        (f32)app_mem->config.window_size[1]
    };
}

export_fn(void) 
app_on_init(app_memory_t* app_mem) {
    // utl::profile_t p{"app init"};
    app_t* app = get_app(app_mem);
    new (app) app_t;

    app->app_mem = app_mem;
    app->main_arena.start = (std::byte*)app_mem->perm_memory + sizeof(app_t);
    app->main_arena.size = app_mem->perm_memory_size - sizeof(app_t);

    arena_t* main_arena = &app->main_arena;
    arena_t* string_arena = &app->string_arena;

    app->temp_arena = arena_sub_arena(&app->main_arena, megabytes(4));
    app->string_arena = arena_sub_arena(&app->main_arena, megabytes(16));
    app->mesh_arena = arena_sub_arena(&app->main_arena, megabytes(32));
    app->texture_arena = arena_sub_arena(&app->main_arena, megabytes(32));
    app->game_arena = arena_sub_arena(&app->main_arena, megabytes(512));
    app->gui.arena = arena_sub_arena(main_arena, megabytes(8));

    defer {
        arena_clear(&app->temp_arena);
    };

    using namespace std::string_view_literals;
#ifdef GEN_INTERNAL
    app->debug.console = arena_alloc<debug_console_t>(main_arena);

    console_log(app->debug.console, "Hello world", string_arena);
    console_log(app->debug.console, "Hello world", string_arena);
    console_log(app->debug.console, "Hello world", string_arena);
    console_log(app->debug.console, "Hello world", string_arena);
    console_log_ex(app->debug.console, "does this work?", string_arena, [](void* app_){
        static int i;
        app_t* app = (app_t*)app_;
        console_log(app->debug.console, string_t{}.own(&app->string_arena, fmt_sv("You tell me {}", i++)), &app->string_arena);
    }, app);

#endif
    TIMED_FUNCTION;
    {
        temp_arena_t temp = *main_arena;
        app->resource_file = utl::res::load_pack_file(&app->mesh_arena, &temp, &app->string_arena, "./assets/res.pack");
    }
    
    physics::api_t* physics = app_mem->physics;
    if (physics) {
        gen_info("app_init", "Creating physics scene");
        assert(physics && physics->create_scene);
        physics->create_scene(physics, 0);
        gen_info("app_init", "Created physics scene");
    }
    app->game_world = game::world_init(&app->game_arena, app, physics);

    // setup graphics

    {
        auto entity_info = GEN_TYPE_INFO(game::entity_t);
        gen_info("app", "typeof {}: {} - {} bytes", entity_info.name, entity_info.type_id, entity_info.size);
    }

    app_init_graphics(app_mem);

    const auto make_material = [&](std::string_view n, auto&& mat) {
        return game::rendering::create_material(
            app->render_system, n, std::move(mat), app->mesh_pipeline->pipeline, app->mesh_pipeline->pipeline_layout
        );
    };

    make_material("red-plastic", gfx::material_t::plastic(gfx::color::v4::red));
    make_material("blue-plastic", gfx::material_t::plastic(gfx::color::v4::blue));
    make_material("gold-metal", gfx::material_t::metal(gfx::color::v4::yellow));
    make_material("silver-metal", gfx::material_t::metal(gfx::color::v4::white));
    make_material("sand", gfx::material_t::plastic(gfx::color::v4::sand));
    make_material("dirt", gfx::material_t::plastic(gfx::color::v4::dirt));
    
    using namespace gfx::color;

    app->scene.sporadic_buffer.mode = 1;
    app->scene.sporadic_buffer.use_lighting = 1;

    if (0)
    {
        prcgen::mesh_builder_t mesh_builder{
            app->render_system->vertices.pool,
            app->render_system->indices.pool
        };

        prcgen::map::room_volume_t vol;
        prcgen::map::room_volume_init(&vol);

        range_u64(x, 0, 63) {
            range_u64(y, 0, 63) {
                range_u64(z, 0, 63) {
                    prcgen::marching_cubes::grid_cell_t g;
                    prcgen::marching_cubes::make_unit_cell(&g, (v3f{x,y,z}-v3f{32.0f})*10.0f, 10.0f);
                    
                    range_u64(i, 0, 8) {
                        const auto d0 = prcgen::map::box_sdf_t{.origin = v3f{0.0}, .size = v3f{64.0f, 2.0f, 64.0f}}.distance(g.p[i]);
                        const auto d1 = prcgen::map::box_sdf_t{.origin = v3f{0.0}, .size = v3f{8.0f, 64.0f, 8.0f}}.distance(g.p[i]);
                        const auto d2 = prcgen::map::sphere_sdf_t{.origin = v3f{20.0f, 0.0f, 0.0f}, .radius = 14.0f}.distance(g.p[i]);
                        const auto w0 = prcgen::map::box_sdf_t{.origin = v3f{32.0, 10.f, 0.0f}, .size = v3f{2.0f, 40.0f, 64.0f}}.distance(g.p[i]);
                        const auto w1 = prcgen::map::box_sdf_t{.origin = v3f{-32.0, 10.f, 0.0f}, .size = v3f{2.0f, 40.0f, 64.0f}}.distance(g.p[i]);
                        const auto w2 = prcgen::map::box_sdf_t{.origin = v3f{0.0, 10.f, -32.0f}, .size = v3f{64.0f, 40.0f, 2.0f}}.distance(g.p[i]);
                        const auto w3 = prcgen::map::box_sdf_t{.origin = v3f{0.0, 10.f, 32.0f}, .size = v3f{64.0f, 40.0f, 2.0f}}.distance(g.p[i]);
                        
                        g.v[i] = std::min({d0, d1, d2, w0, w1, w2, w3});
                        
                    }

                    math::triangle_t triangles[32];
                    u64 triangle_count = prcgen::marching_cubes::to_polygon(g, 0.0f, triangles);

                    if (triangle_count) {
                        mesh_builder.add_triangles(triangles, triangle_count);
                    }
                }
            }
        }

        auto mesh_id = game::rendering::add_mesh(app->render_system, "ground", mesh_builder.build(&app->mesh_arena));

        // auto* e0 = game::world_create_entity(app->game_world, 0);
        // game::entity_init(e0, mesh_id);
    }
    #if 0
    
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

    auto* player = game::spawn(app->game_world, app->render_system, game::db::characters::assassin, v3f{0.0f, 4.0f, 0.0f});
    player->physics.rigidbody->on_collision = player_on_collision;

    game::spawn(app->game_world, app->render_system,
        game::db::weapons::shotgun,
        v3f{10,3,10});


    // game::spawn(app->game_world, app->render_system, game::db::rooms::room_01);

    
    // range_u64(i, 0, 1) {
    //     v3f pos = v3f{0.0f, 35.0f * f32(i), 0.0f};
    //     auto* r = game::spawn(app->game_world, app->render_system, game::db::rooms::tower_01, pos);
    //     r->transform.set_rotation(axis::up * math::constants::pi32 * f32(i));
    // }
    game::spawn(app->game_world, app->render_system, game::db::rooms::room_0);

    utl::rng::random_t<utl::rng::xor64_random_t> rng;
    loop_iota_u64(i, 200) {
        auto* e = game::spawn(
            app->game_world, 
            app->render_system,
            game::db::misc::teapot,
            rng.randnv<v3f>() * 100.0f * planes::xz + axis::up * 8.0f
        );
        e->transform.set_scale(v3f{2.f});
        e->transform.set_rotation(rng.randnv<v3f>());
        e->gfx.material_id = rng.rand() % 6;
        e->physics.rigidbody->on_collision = teapot_on_collision;
    }

    gen_info("app", "player id: {} - {}", 
        app->game_world->player->id,
        (void*)game::find_entity_by_id(app->game_world, app->game_world->player->id)
    );

    
    app->game_world->player->transform.origin.y = 5.0f;
    app->game_world->player->transform.origin.z = -5.0f;

    game::script_manager_t<game::player_script_t> scripts{app->game_world};

    game::player_script_t plr{};
    static_assert(game::CScript<game::player_script_t>);
    assert(scripts.get_type_index<decltype(plr)>() == 0);
    

    scripts.add_script(&plr);
    assert(scripts.get_script<decltype(plr)>(0, 1) != nullptr);

    scripts.begin_play();
    scripts.update(0);
    assert(plr.score == 1);

    assert(plr.id != uid::invalid_id);
    scripts.remove_script(&plr);
    assert(plr.id == uid::invalid_id);

    std::memcpy(&gs_saved_world, app->game_world, sizeof(game::world_t));
    gen_info("app", "world size: {}mb", GEN_TYPE_INFO(game::world_t).size/megabytes(1));
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
    app_t* app = get_app(app_mem);
}

export_fn(void)
app_on_reload(app_memory_t* app_mem) {
    app_t* app = get_app(app_mem);
}

void 
camera_input(app_t* app, player_controller_t pc, f32 dt) {
    auto* world = app->game_world;
    auto* physics = app->game_world->physics;
    auto* player = world->player;
    auto* rigidbody = player->physics.rigidbody;
    const bool is_on_ground = rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
    const bool is_on_wall = rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;

    app->game_world->player->camera_controller.transform = 
        // app->game_world->player->physics.rigidbody->position;
        app->game_world->player->transform;
    
    const v3f move = pc.move_input;
    const v2f head_move = pc.look_input;

    auto& yaw = player->camera_controller.yaw;
    auto& pitch = player->camera_controller.pitch;

    const v3f forward = game::cam::get_direction(yaw, pitch);
    const v3f right   = glm::cross(forward, axis::up);

    if (gs_imgui_state && gfx::gui::im::want_mouse_capture(*gs_imgui_state) == false) {
        yaw += head_move.x * dt;
        pitch -= head_move.y * dt;
    }

    const f32 move_speed = 12.0f * (pc.sprint ? 1.75f : 1.0f);

    // rigidbody->velocity += (forward * move.z + right * move.x + axis::up * move.y) * dt;
    rigidbody->add_relative_force((forward * move.z + right * move.x + axis::up * move.y) * dt * move_speed);
    // rigidbody->orientation = glm::quat{yaw * axis::up};
    
    if(player->primary_weapon.entity) {
        player->primary_weapon.entity->transform.set_rotation(v3f{yaw * axis::up});
    }
    rigidbody->angular_velocity = v3f{0.0f};


    if (pc.jump && (is_on_ground || is_on_wall)) {
        // rigidbody->add_relative_force(axis::up * 100.0f);
        rigidbody->velocity.y = 1.0f;
    }

    
    if (pc.fire1 && player->primary_weapon.entity) {
        auto ro = player->primary_weapon.entity->global_transform().origin + forward*3.0f;
        auto rd = forward;

        temp_arena_t fire_arena = world->frame_arena[world->frame_count&1];
        u64 fired{0};
        auto* bullets = player->primary_weapon.entity->stats.weapon().fire(&fire_arena, dt, &fired);

        // range_u64(bullet, 0, fired) {
        
            if (auto ray = physics->raycast_world(physics, ro, rd); ray.hit) {
                auto* rb = (physics::rigidbody_t*)ray.user_data;
                if (rb == player->physics.rigidbody) {
                    gen_warn(__FUNCTION__, "player shot them self");
                }
                if (rb->type == physics::rigidbody_type::DYNAMIC) {
                    auto hp = ray.point - rb->position;
                    auto f = rb->transform_direction(rd);
                    rb->add_force_at_point(f*100.0f, hp);
                }
            }
        // }
    }
}

void 
app_on_input(app_t* app, app_input_t* input) {
    camera_input(app, 
        input->gamepads->is_connected ? 
        gamepad_controller(input) : keyboard_controller(input),
        input->dt
    );

    if (input->pressed.keys['P']) {
        const game::entity_t player = *app->game_world->player;
        std::memcpy(app->game_world, &gs_saved_world, sizeof(game::world_t));
        *app->game_world->player = player;
        range_u64(i, 0, app->game_world->entity_count) {
            auto* e = app->game_world->entities + i;
            if (e->physics.flags == game::PhysicsEntityFlags_Dynamic &&
                e->physics.rigidbody
            ) {
                e->physics.rigidbody->velocity = v3f{0.0f};
                e->physics.rigidbody->angular_velocity = v3f{0.0f};
            }
        }
    }

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
    TIMED_FUNCTION;

    // std::lock_guard lock{app->render_system->ticket};

    input->dt *= app->time_scale;
    app_on_input(app, input);

    // TODO(ZACK): GAME CODE HERE

    if (app->render_system == nullptr) return;

    auto* world = app->game_world;

    local_persist f32 accum = 0.0f;
    const u32 sub_steps = 5;
    const f32 step = 1.0f/(60.0f*sub_steps);

    accum += f32(input->dt > 0.0f) * std::min(input->dt, 1.0f);

    for (size_t i{0}; i < app->game_world->entity_count; i++) {
        auto* e = app->game_world->entities + i;
        if (e->physics.rigidbody) {
            e->physics.rigidbody->position = e->global_transform().origin;
            e->physics.rigidbody->orientation = e->global_transform().get_orientation();
            app->game_world->physics->set_rigidbody(0, e->physics.rigidbody);
        }
    }

    while (accum >= step) {
        TIMED_BLOCK(PhysicsStep);
        // range_u32(_i, 0, sub_steps) {
            accum -= step;
            world->physics->simulate(world->physics, step);
        // }
    }

    game::rendering::begin_frame(app->render_system);
    app->debug.debug_vertices.pool.clear();
        
    for (size_t i{0}; i < app->game_world->entity_capacity; i++) {
        auto* e = app->game_world->entities + i;
        if (e->id == uid::invalid_id) {
            continue;
        }
        const bool is_physics_object = e->physics.flags != game::PhysicsEntityFlags_None && e->physics.rigidbody;
        const bool is_pickupable = (e->flags & game::EntityFlags_Pickupable);
        const bool is_not_renderable = (e->flags & game::EntityFlags_Renderable) == 0;

        if (is_physics_object) {
            const auto skip = (e->physics.rigidbody->flags & physics::rigidbody_flags::SKIP_SYNC);
            e->physics.rigidbody->integrate(input->dt, 1 && e->physics.flags & game::PhysicsEntityFlags_Character);
            app->game_world->physics->sync_rigidbody(0, e->physics.rigidbody);
            if (!skip) {
                e->transform.origin = e->physics.rigidbody->position;
                e->transform.set_rotation(e->physics.rigidbody->orientation);
            }
        }

        if (is_pickupable) {
            e->transform.rotate(axis::up, input->dt);
            if (math::intersect(math::sphere_t{e->global_transform().origin, 2.0f}, app->game_world->player->global_transform().origin)) {
                app->game_world->player->primary_weapon.entity = e;
                e->parent = app->game_world->player;
                e->transform.origin = v3f{0.5, 0.0f, -0.5f};
                e->flags &= ~game::EntityFlags_Pickupable;
            }
        }

        if (is_not_renderable) {
            gen_warn("render", "Skipping: {} - {}", (void*)e, e->flags);
            return;
        }

        if (e == app->game_world->player) {
            e->camera_controller.transform = e->global_transform();
            e->camera_controller.translate(axis::up);
        }

        app->debug.draw_aabb(e->global_transform().xform_aabb(e->aabb), gfx::color::v3::yellow);

        game::rendering::submit_job(
            app->render_system, 
            e->gfx.mesh_id, 
            e->gfx.material_id, 
            e->global_transform().to_matrix()
        );
    }
}

void
game_on_update(app_memory_t* app_mem) {
    // utl::profile_t p{"on_update"};
    app_t* app = get_app(app_mem);

    game_on_gameplay(app, &app_mem->input);
}

void 
draw_game_gui(app_memory_t* app_mem) {
    TIMED_FUNCTION;
    app_t* app = get_app(app_mem);
    auto* world = app->game_world;
    auto* player = world->player;

    local_persist u64 frame{0}; frame++;
    auto& imgui = *gs_imgui_state;

    if (player->primary_weapon.entity) {
        const auto center = imgui.ctx.screen_size * 0.5f;
        gfx::gui::im::draw_circle(imgui, center, 2.0f, gfx::color::rgba::white);
        gfx::gui::im::draw_circle(imgui, center, 4.0f, gfx::color::rgba::light_gray);
    }
}

void 
draw_gui(app_memory_t* app_mem) {
    TIMED_FUNCTION;
    app_t* app = get_app(app_mem);

    local_persist u64 frame{0}; frame++;
    auto string_mark = arena_get_mark(&app->string_arena);

    arena_t* display_arenas[] = {
        &app->main_arena,
        &app->temp_arena,
        &app->string_arena,
        &app->mesh_arena,
        &app->texture_arena,
        &app->game_arena,
        &app->game_world->arena,
        // &app->physics->default_allocator.arena,
        // &app->physics->default_allocator.heap_arena,
        &app->render_system->arena,
        &app->render_system->frame_arena,
        &app->render_system->vertices.pool,
        &app->render_system->indices.pool,
        &app->gui.vertices[!(frame&1)].pool,
        &app->gui.indices[!(frame&1)].pool,
    };

    const char* display_arena_names[] = {
        "- Main Arena",
        "- Temp Arena",
        "- String Arena",
        "- Mesh Arena",
        "- Texture Arena",
        "- Game Arena",
        "- World Arena",
        // "- Physics Arena",
        // "- Physics Heap",
        "- Rendering Arena",
        "- Rendering Frame Arena",
        "- 3D Vertex",
        "- 3D Index",
        "- 2D Vertex",
        "- 2D Index",
    };
    gfx::gui::ctx_clear(&app->gui.ctx, &app->gui.vertices[frame&1].pool, &app->gui.indices[frame&1].pool);
    
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
            .bg_color = gfx::color::rgba::black,
            // .bg_color = gfx::color::rgba::dark_gray,
            .text_color = gfx::color::rgba::cream,
            .disabled_color = gfx::color::rgba::dark_gray,
            .border_color = gfx::color::rgba::white,

            .padding = 4.0f,
            .margin = 8.0f
        },
    };
    gs_imgui_state = &state;

    // state.theme.bg_color = 
    //    gfx::color::rgba::dark_gray & ( ~(u32(f32(0xff) * gs_panel_opacity) << 24) );

    {
        using namespace std::string_view_literals;
        using namespace gfx::gui;
        const m44 vp = 
            app->render_system->vp;

        // draw_console(state, app->debug.console, v2f{0.0, 800.0f});
        

        local_persist v3f default_widget_pos{10.0f};
        local_persist v3f* widget_pos = &default_widget_pos;

        im::clear(state);

        im::gizmo(state, widget_pos, vp);

        local_persist bool show_entities = false;
        if (im::begin_panel(state, "Main UI"sv)) {
            local_persist bool show_player = !false;
            local_persist bool show_stats = !false;
            local_persist bool show_resources = false;
            local_persist bool show_window = false;
            local_persist bool show_gfx_cfg = false;
            local_persist bool show_perf = true;
            local_persist bool show_gui = false;
            local_persist bool show_theme = false;
            local_persist bool show_colors= false;
            local_persist bool show_files[0xff];

            if (im::text(state, "Player"sv, &show_player)) {
                const bool on_ground = app->game_world->player->physics.rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
                im::text(state, fmt_sv("- On Ground: {}", on_ground?"true":"false"));
                const auto v = app->game_world->player->physics.rigidbody->velocity;
                im::text(state, fmt_sv("- Velocity: {}", v));

            }
            
            if (im::text(state, "Stats"sv, &show_stats)) {
                im::text(state, fmt_sv("- FPS: {:.2f} - {:.2f} ms", 1.0f / app_mem->input.dt, app_mem->input.dt * 1000.0f));

                {
                    const auto [x,y] = app_mem->input.mouse.pos;
                    im::text(state, fmt_sv("- Mouse: [{:.2f}, {:.2f}]", x, y));
                }
#if GEN_INTERNAL
                local_persist bool show_record[128];
                if (im::text(state, "- Perf"sv, &show_perf)) {
                    range_u64(i, 0, gs_main_debug_record_size) {
                        auto* record = gs_debug_table.records + i;
                        const auto cycle = record->history[record->hist_id?record->hist_id-1:array_count(record->history)-1];

                        im::same_line(state);
                        if (im::text(state,
                            fmt_sv("--- {}:", 
                                record->func_name ? record->func_name : "<Unknown>"), 
                            show_record + i
                        )) {
                            im::text(state,
                                fmt_sv(" {:.2f}ms", cycle/1e+6)
                            );

                            f32 hist[4096/32];
                            math::statistic_t debug_stat;
                            math::begin_statistic(debug_stat);
                            range_u64(j, 0, array_count(hist)) {
                                const auto ms = f64(record->history[j*32])/f64(1e+6);
                                math::update_statistic(debug_stat, ms);
                                hist[j] = (f32)ms;
                            }
                            math::end_statistic(debug_stat);

                            im::same_line(state);
                            im::text(state, "--- "sv);

                            im::histogram(state, hist, array_count(hist), (f32)debug_stat.range.max, v2f{4.0f*128.0f, 32.0f});
                        } else {
                            im::text(state,
                                fmt_sv(" {:.2f}ms", cycle/1e+6)
                            );
                        }
                    }
                }
#endif
                if (im::text(state, "- GFX Config"sv, &show_gfx_cfg)) {
                    auto& config = app_mem->config.graphics_config;

                    local_persist u64 _win_size = 0;

                    const v2i _sizes[] = {
                        v2i{1920, 1080}, // 120
                        v2i{1600, 900}, // 100
                        v2i{1280, 720}, // 80
                        v2i{960,  540}, // 60
                        v2i{640,  360}, // 40
                    };
                    f32 closest = 1000000.0f;
                    range_u64(i, 0, array_count(_sizes)) {
                        const auto dist = glm::distance(v2f(_sizes[i]), v2f(config.window_size));
                        if (dist < closest) {
                            closest = dist;
                            _win_size = i;
                        }
                    }

                    if (im::text(state, "--- Next"sv)) {
                        _win_size = (_win_size + 1) % array_count(_sizes);
                        config.window_size = _sizes[_win_size];
                    }

                    im::text(state, fmt_sv("--- Width: {}", config.window_size.x));
                    im::text(state, fmt_sv("--- Height: {}", config.window_size.y));


                    if (im::text(state, fmt_sv("--- V-Sync: {}", config.vsync))) {
                        config.vsync = !config.vsync;
                    }
                    
                    if (im::text(state, fmt_sv("--- Fullscreen: {}", config.fullscreen))) {
                        config.fullscreen = !config.fullscreen;
                    }
                }

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
                if(im::text(state, "- Assert"sv)) {
                    assert(0);
                }
                if(im::text(state, "- Breakpoint"sv)) {
                    volatile u32 _tx=0;
                    _tx += 1;
                }
            }

            local_persist bool show_gfx = false;
            local_persist bool show_mats = false;
            local_persist bool show_probes = false;
            local_persist bool show_env = !false;
            local_persist bool show_mat_id[8] = {};
            if (im::text(state, "Graphics"sv, &show_gfx)) { 
                local_persist bool show_sky = false;
                if (im::text(state, "- Probes"sv, &show_probes)) { 
                    auto& probes = app->render_system->light_probes;
                    range_u64(i, 0, probes.probe_count) {
                        auto& p = probes.probes[i];
                        if (im::draw_circle_3d(state, vp, p.position, 0.1f, gfx::color::rgba::white)) {
                            if (im::begin_panel_3d(state, "probe"sv, vp, p.position)) {
                                im::text(state, fmt_sv("Probe ID: {}"sv, p.id));
                                im::end_panel(state);
                            }
                        }
                    }
                }
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

            local_persist bool show_arenas = !false;
            if (im::text(state, "Memory"sv, &show_arenas)) {
                for (size_t i = 0; i < array_count(display_arenas); i++) {
                    im::text(state, arena_display_info(display_arenas[i], display_arena_names[i]));
                }
            }

            if (im::text(state, "Entities"sv, &show_entities)) {
                const u64 entity_count = app->game_world->entity_count;

                im::text(state, fmt_sv("- Count: {}", entity_count));

                if (im::text(state, "- Kill All"sv)) {
                    // app->game_world->entities.clear();
                    app->game_world->entity_count = 1;
                }
            }
            
            im::end_panel(state);
        }


    
    
        // for (game::entity_t* e = game_itr(app->game_world); e; e = e->next) {
        for (size_t i = 0; i < app->game_world->entity_count; i++) {
            auto* e = app->game_world->entities + i;
            // if (!show_entities && ((e->flags & BIT(23)) == 0)) continue;
            const v3f ndc = math::world_to_screen(vp, e->global_transform().origin);

            bool is_selected = widget_pos == &e->transform.origin;
            bool not_player = e != app->game_world->player;
            bool opened = false;
            if (show_entities || 
                is_selected || 
                (not_player && im::draw_hiding_circle_3d(state, vp, e->global_transform().origin, 0.1f, static_cast<u32>(utl::rng::fnv_hash_u64(e->id)), 2.0f, 4)))

            if (im::begin_panel_3d(state, 
                e->name.c_data ? 
                fmt_sv("Entity: {}\0{}"sv, std::string_view{e->name}, (void*)e) :
                fmt_sv("Entity: {}", (void*)e),
                vp, e->global_transform().origin
            )) {
                opened = true;
                if (state.ctx.input->mouse.buttons[0]) {
                    widget_pos = &e->transform.origin;
                }
                im::text(state, 
                    e->name.c_data ? 
                    fmt_sv("Entity: {}", std::string_view{e->name}) :
                    fmt_sv("Entity: {}", (void*)e)
                );

                im::text(state, 
                    fmt_sv("Screen Pos: {:.2f} {:.2f}", ndc.x, ndc.y)
                );

                if (im::text(state,
                    fmt_sv("Origin: {:.2f} {:.2f} {:.2f}", 
                        e->global_transform().origin.x,
                        e->global_transform().origin.y,
                        e->global_transform().origin.z
                    )
                )) {
                    widget_pos = &e->transform.origin;
                }

                switch(e->physics.flags) {
                    case game::PhysicsEntityFlags_None:
                        im::text(state, "Physics Type: None");
                        break;
                    case game::PhysicsEntityFlags_Static:
                        im::text(state, "Physics Type: Static");
                        break;
                    case game::PhysicsEntityFlags_Dynamic:
                        im::text(state, "Physics Type: Dynamic");
                        break;
                }

                switch(e->type) {
                    case game::entity_type::weapon: {
                        auto stats = e->stats.weapon();
                        im::text(state, "Type: Weapon");
                        im::text(state, "- Stats");
                        im::text(state, fmt_sv("--- Damage: {}", stats.stats.damage));
                        im::text(state, fmt_sv("--- Pen: {}", stats.stats.pen));
                        im::text(state, fmt_sv("- Fire Rate: {}", stats.fire_rate));
                        im::text(state, fmt_sv("- Load Speed: {}", stats.load_speed));
                        im::text(state, "- Ammo");
                        im::text(state, fmt_sv("--- {}/{}", stats.clip.current, stats.clip.max));
                    }   break;
                }

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
            const auto panel = im::get_last_panel(state);
            if (is_selected && opened && im::draw_circle(state, panel.max, 8.0f, gfx::color::rgba::red, 4)) {
                widget_pos = &default_widget_pos;
            }
        }

        draw_game_gui(app_mem);

        local_persist viewport_t viewport{};
        viewport.images[0] = 1;
        viewport.images[1] = 2;
        viewport.images[2] = 1;
        viewport.images[3] = 2;

        draw_viewport(state, &viewport);

        // const math::aabb_t<v2f> screen{v2f{0.0f}, state.ctx.screen_size};
        // im::image(state, 2, math::aabb_t<v2f>{v2f{state.ctx.screen_size.x - 400, 0.0f}, v2f{state.ctx.screen_size.x, 400.0f}});
        // im::image(state, 1, screen);
    }


    arena_set_mark(&app->string_arena, string_mark);
}

inline static u32
wait_for_frame(app_t* app, u64 frame_count) {
    gfx::vul::state_t& vk_gfx = app->gfx;
    vkWaitForFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[frame_count&1], VK_TRUE, UINT64_MAX);
    vkResetFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[frame_count&1]);
    u32 imageIndex;
    vkAcquireNextImageKHR(vk_gfx.device, vk_gfx.swap_chain, UINT64_MAX, 
        vk_gfx.image_available_semaphore[frame_count&1], VK_NULL_HANDLE, &imageIndex);
    return imageIndex;
}

inline static void
present_frame(app_t* app, VkCommandBuffer command_buffer, u32 imageIndex, u64 frame_count) {
    gfx::vul::state_t& vk_gfx = app->gfx;
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {vk_gfx.image_available_semaphore[frame_count&1]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    VkSemaphore signalSemaphores[] = {vk_gfx.render_finished_semaphore[frame_count&1]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vk_gfx.gfx_queue, 1, &submitInfo, vk_gfx.in_flight_fence[frame_count&1]) != VK_SUCCESS) {
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
game_on_render(app_memory_t* app_mem) { 
    TIMED_FUNCTION;
    
    app_t* app = get_app(app_mem);

    gfx::vul::state_t& vk_gfx = app->gfx;

    // note(zack): print the first 3 frames, if 
    // a bug shows up its helpful to know if 
    // maybe it only happens on the second frame
    local_persist u32 frame_count = 0;
    if (frame_count++ < 3) {
        gen_info("game::on_render", "frame: {}", frame_count);
    }

    const f32 anim_time = glm::mod(
        app_mem->input.time, 10.0f
    );

    // update uniforms

    app->scene.sporadic_buffer.num_instances = 1;

    u32 imageIndex = wait_for_frame(app, frame_count);
    //std::lock_guard lock{app->render_system->ticket};

    {
        app->render_system->camera_pos = app->game_world->camera.affine_invert().origin;
        app->render_system->set_view(app->game_world->camera.to_matrix());
    }

    *vk_gfx.sporadic_uniform_buffer.data = app->scene.sporadic_buffer;
    auto& command_buffer = vk_gfx.command_buffer[frame_count&1];
    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
        gen_error("vulkan:record_command", "failed to begin recording command buffer!");
        std::terminate();
    }

    gfx::vul::begin_render_pass(vk_gfx,
        app->render_system->render_targets->render_pass,
        app->render_system->render_targets->framebuffer,
        command_buffer
    );
    
    {
        TIMED_BLOCK(SkyPass);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->sky_pipeline->pipeline);
        VkBuffer buffers[1] = { app->render_system->vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(command_buffer,
            0, 1, buffers, offsets);

        vkCmdBindIndexBuffer(command_buffer,
            app->render_system->indices.buffer, 0, VK_INDEX_TYPE_UINT32
        );

        struct sky_push_constant_t {
            m44 vp;
            v4f directional_light;
        } sky_constants;

        
        sky_constants.vp = app->render_system->projection * glm::lookAt(v3f{0.0f}, 
            game::cam::get_direction(
                app->game_world->player->camera_controller.yaw,
                app->game_world->player->camera_controller.pitch
            ), 
            axis::up
        );
        sky_constants.directional_light = app->scene.lighting.directional_light;

        vkCmdPushConstants(command_buffer, app->sky_pipeline->pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(sky_push_constant_t), &sky_constants
        );

        const auto& meshes = app->render_system->mesh_cache.get(
            utl::str_hash_find(app->render_system->mesh_hash, "assets/models/sphere.obj")
        );
    
        for (size_t j = 0; j < meshes.count; j++) {
            vkCmdDrawIndexed(command_buffer,
                meshes.meshes[j].index_count,
                1, 
                meshes.meshes[j].index_start,
                meshes.meshes[j].vertex_start,
                0           
            );
        }
    }


    {
        vkCmdBindDescriptorSets(command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline->pipeline_layout,
            0, 4, app->mesh_pipeline->descriptor_sets, 0, nullptr);

        vkCmdBindDescriptorSets(command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline->pipeline_layout,
            4, 1, &app->brick_descriptor, 0, nullptr);

        VkBuffer buffers[1] = { app->render_system->vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(command_buffer,
            0, 1, buffers, offsets);

        vkCmdBindIndexBuffer(command_buffer,
            app->render_system->indices.buffer, 0, VK_INDEX_TYPE_UINT32
        );
        game::rendering::build_commands(app->render_system, command_buffer);
    }


    // if (app->debug.show) 
    {
        TIMED_BLOCK(DebugPass);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->debug_pipeline->pipeline);

        vkCmdPushConstants(command_buffer, app->sky_pipeline->pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(m44), &app->render_system->vp
        );

        VkBuffer buffers[1] = { app->debug.debug_vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(command_buffer,
            0, 1, buffers, offsets);

        const u32 instance_count = 1;
        const u32 first_instance = 0;

        vkCmdDraw(command_buffer,
            (u32)app->debug.debug_vertices.pool.count,
            1, // instance count
            0, // vertex start
            0 // instance start
        );
    }
    gfx::vul::end_render_pass(command_buffer);

    gfx::vul::begin_render_pass(vk_gfx,
        app->render_system->render_passes[0],
        app->render_system->framebuffers[imageIndex],
        command_buffer
    );

    {
        TIMED_BLOCK(GUIPass);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_pipeline->pipeline);
        vkCmdBindDescriptorSets(command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_pipeline->pipeline_layout,
            0, 1, &app->default_font_descriptor, 0, nullptr);

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(command_buffer,
            0, 1, &app->gui.vertices[frame_count&1].buffer, offsets);

        vkCmdBindIndexBuffer(command_buffer,
            app->gui.indices[frame_count&1].buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(command_buffer,
            (u32)app->gui.ctx.indices->count,
            1,
            0,
            0,
            0
        );
    }

    gfx::vul::end_render_pass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        gen_error("vulkan:record_command:end", "failed to record command buffer!");
        std::terminate();
    }

    present_frame(app, command_buffer, imageIndex, frame_count);
}

void
main_menu_on_update(app_memory_t* app_mem) {
}

global_variable u64 scene_state = 1;

#include "App/Game/GUI/entity_editor.hpp"

inline entity_editor_t* get_entity_editor(app_memory_t* app) {
    local_persist entity_editor_t ee{get_app(app)};
    return &ee;
}

export_fn(void) 
app_on_render(app_memory_t* app_mem) {
    switch (scene_state) {
        case 0:{
            entity_editor_render(get_entity_editor(app_mem));
            game_on_render(app_mem);
        }   break;
        case 1:{
            // Game
            draw_gui(app_mem);
            game_on_render(app_mem);
        }   break;
        default:
            gen_warn("scene::render", "Unknown scene: {}", scene_state);
            break;
        //     scene_state = 1;
    }
}

export_fn(void) 
app_on_update(app_memory_t* app_mem) {
    if (app_mem->input.pressed.keys[key_id::F3] || 
        app_mem->input.gamepads->buttons[button_id::action_down].is_pressed) {
        scene_state = !scene_state;
    }
    switch (scene_state) {
        case 0: // Editor
            entity_editor_update(get_entity_editor(app_mem));
            break;
        case 1: // Game
            game_on_update(app_mem);
            break;
        default:
            gen_warn("scene::update", "Unknown scene: {}", scene_state);
            break;
        //     scene_state = 1;
    }
}
