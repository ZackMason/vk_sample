#include <core.hpp>

#include "App/app.hpp"

#include "skeleton.hpp"

#include "ProcGen/terrain.hpp"
#include "ProcGen/mesh_builder.hpp"
#include "App/Game/World/world.hpp"

#include "App/Game/World/Level/level.hpp"
#include "App/Game/World/Level/Room/room.hpp"
#include "App/Game/Items/base_item.hpp"
#include "App/Game/Weapons/base_weapon.hpp"

#include "App/Game/Rendering/assets.hpp"

#include "App/Game/GUI/entity_editor.hpp"

global_variable gfx::gui::im::state_t* gs_imgui_state = 0;
global_variable f32 gs_dt;

global_variable VkCullModeFlagBits gs_cull_modes[] = {
    VK_CULL_MODE_BACK_BIT,
    VK_CULL_MODE_FRONT_BIT,
};
global_variable u32 gs_cull_mode;

global_variable VkPolygonMode gs_poly_modes[] = {
    VK_POLYGON_MODE_FILL,
    VK_POLYGON_MODE_LINE,
    VK_POLYGON_MODE_POINT,
};
global_variable u32 gs_poly_mode;

void teapot_on_collision(
    physics::rigidbody_t* teapot,
    physics::rigidbody_t* other
) {
    // gen_info(__FUNCTION__, "teapot hit: {} - id", teapot->id);
    // auto* teapot_entity = (game::entity_t*) teapot->user_data;
    // teapot->add_relative_impulse(teapot->inverse_transform_direction(axis::up) * 10.0f, gs_dt);
    
    // teapot->flags = physics::rigidbody_flags::SKIP_SYNC;
}

void player_on_collision(
    physics::rigidbody_t* player,
    physics::rigidbody_t* other
) {
    auto* other_entity = (game::entity_t*) other->user_data;
    // gen_info(__FUNCTION__, "player hit: {} - id", other_entity->id);
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
    gen_warn(__FUNCTION__, "Loading {} meshes", results.count);
    results.meshes  = arena_alloc_ctor<gfx::mesh_view_t>(arena, results.count);

    for (size_t i = 0; i < results.count; i++) {
        std::string name = blob.deserialize<std::string>();
        gen_info(__FUNCTION__, "Mesh name: {}", name);
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
    const auto mate = blob.deserialize<u64>();
    assert(mate == utl::res::magic::mate);

    for (size_t i = 0; i < results.count; i++) {
        const auto& material = results.meshes[i].material = blob.deserialize<gfx::material_info_t>();
        gen_info(__FUNCTION__, "Loaded material: {}, albedo: {}, normal: {}", material.name, material.albedo, material.normal);
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
make_error_texture(arena_t* arena, u32 size) {
    auto* tex = arena_alloc<gfx::vul::texture_2d_t>(arena);

    tex->size[0] = tex->size[1] = size;
    tex->channels = 4;
    tex->pixels = arena_alloc_ctor<u8>(arena, size*size*4);
    u64 i = 0;
    range_u64(x, 0, size) {
        range_u64(y, 0, size) {
            auto n = std::sinf((f32)x/(f32)size*math::constants::pi32*10.0f) * std::cosf((f32)y/(f32)size*math::constants::pi32*10.0f);
            n = glm::step(n, 0.0f);
            tex->pixels[i++] = u8(n*255.0f);
            tex->pixels[i++] = 0;
            tex->pixels[i++] = u8(n*255.0f);
            tex->pixels[i++] = 255ui8;
        }
    }

    return tex;
}

inline static gfx::vul::texture_2d_t*
make_noise_texture(arena_t* arena, u32 size) {
    auto* tex = arena_alloc_ctor<gfx::vul::texture_2d_t>(arena);

    tex->size[0] = tex->size[1] = size;
    tex->channels = 4;
    tex->pixels = arena_alloc_ctor<u8>(arena, size*size*4);
    // tex->format = VK_FORMAT_R8G8B8A8_SRGB;
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
    return (app_t*)mem->arena.start;
}

inline static
game::rendering::lighting::ray_scene_t*
raytrace_test(app_memory_t* app_mem) {
    using namespace game::rendering;
    auto* app = get_app(app_mem);

    auto* scene = lighting::init_ray_scene(&app->main_arena, 64, 64);

    scene->camera.origin = v3f{0.0f, 0.0f, 10.0f};
    scene->camera.direction = v3f{0.0, 0.0, -1.0f};

    auto& teapot = get_mesh(app->render_system, "res/models/utah-teapot.obj");
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

        range_u64(m, 0, loaded_mesh.count) {
            u64 ids[2];
            u32 mask = app->render_system->texture_cache.load_material(app->render_system->arena, app->gfx, loaded_mesh.meshes[m].material, ids);
            loaded_mesh.meshes[m].material.albedo_id = (mask&0x1) ? ids[0] : std::numeric_limits<u64>::max();
            loaded_mesh.meshes[m].material.normal_id = (mask&0x2) ? ids[1] : std::numeric_limits<u64>::max();
        }
        game::rendering::add_mesh(app->render_system, file_name, loaded_mesh);
    }
    
    range_u64(i, 0, app->resource_file->file_count) {
        arena_t* arena = &app->mesh_arena;
        if (app->resource_file->table[i].file_type != utl::res::magic::skel) { 
            continue; 
        }

        const char* file_name = app->resource_file->table[i].name.c_data;
        std::byte*  file_data = utl::res::pack_file_get_file(app->resource_file, i);

        gfx::mesh_list_t results;
        utl::memory_blob_t blob{file_data};

        // read file meta data
        const auto meta = blob.deserialize<u64>();
        const auto vers = blob.deserialize<u64>();
        const auto mesh = blob.deserialize<u64>();

        assert(meta == utl::res::magic::meta);
        assert(vers == utl::res::magic::vers);
        assert(mesh == utl::res::magic::skel);

        results.count = blob.deserialize<u64>();
        gen_warn(__FUNCTION__, "Loading {} meshes", results.count);
        results.meshes  = arena_alloc_ctor<gfx::mesh_view_t>(arena, results.count);

        u64 total_vertex_count = 0;
        auto* vertices_ = new gfx::skinned_vertex_t[4'00'000];
        utl::pool_t<gfx::skinned_vertex_t> vertices{vertices_, 4'00'000};

        for (size_t j = 0; j < results.count; j++) {
            std::string name = blob.deserialize<std::string>();
            gen_info(__FUNCTION__, "Mesh name: {}", name);
            const size_t vertex_count = blob.deserialize<u64>();
            const size_t vertex_bytes = sizeof(gfx::skinned_vertex_t) * vertex_count;

            total_vertex_count += vertex_count;

            const u32 vertex_start = safe_truncate_u64(vertices.count);
            const u32 index_start = 0;

            results.meshes[j].vertex_count = safe_truncate_u64(vertex_count);
            results.meshes[j].vertex_start = vertex_start;
            results.meshes[j].index_start = index_start;

            gfx::skinned_vertex_t* v = vertices.allocate(vertex_count);

            std::memcpy(v, blob.read_data(), vertex_bytes);
            blob.advance(vertex_bytes);

            // no indices for skeletal meshes atm
            // const size_t index_count = blob.deserialize<u64>();
            // const size_t index_bytes = sizeof(u32) * index_count;
            // results.meshes[j].index_count = safe_truncate_u64(index_count);

            // u32* tris = indices->allocate(index_count);
            // std::memcpy(tris, blob.read_data(), index_bytes);
            // blob.advance(index_bytes);

            results.meshes[j].aabb = {};
            range_u64(k, 0, vertex_count) {
                results.meshes[j].aabb.expand(v[k].pos);
            }
        }

        results.aabb = {};
        range_u64(m, 0, results.count) {
            results.aabb.expand(results.meshes[m].aabb);
        }
    
        const auto anim = blob.deserialize<u64>();
        assert(anim == utl::res::magic::anim);    

        const auto anim_count = blob.deserialize<u64>();
        const auto total_anim_size = blob.deserialize<u64>();
        // const auto total_anim_size2 = blob.deserialize<u64>();
        // const auto anim_count = total_anim_size / sizeof(gfx::anim::animation_t);

        auto* animations = (gfx::anim::animation_t*) blob.read_data();
        blob.advance(total_anim_size);


        const auto skeleton_size = blob.deserialize<u64>();
        assert(skeleton_size == sizeof(gfx::anim::skeleton_t));
        auto* skeleton = (gfx::anim::skeleton_t*)blob.read_data();
        blob.advance(skeleton_size);

        // app_mem->message_box(
        //     fmt_sv("Asset {} has skeleton, {} vertices, {} animations\n{} - {}s {} ticks\nSkeleton: {} bones", 
        //     file_name, total_vertex_count, anim_count, 
        //     animations[0].name, animations[0].duration, animations[0].ticks_per_second,
        //     skeleton->bone_count).data());

        range_u64(a, 0, anim_count) {
            auto& animation = animations[a];
            gen_info(__FUNCTION__, "Animation: {}, size: {}", animation.name, sizeof(gfx::anim::animation_t));
            range_u64(n, 0, animation.node_count) {
                auto& node = animation.nodes[n];
                auto& timeline = node.bone;
                if (timeline) {
                    gen_info(__FUNCTION__, "Bone: {}, id: {}", timeline->name, timeline->id);
                }
            }
        }
        range_u64(b, 0, skeleton->bone_count) {
            auto& bone = skeleton->bones[b];
            gen_info(__FUNCTION__, "Bone: {}, parent: {}", bone.name_hash, bone.parent);
        }
    }

    app->gui_pipeline = gfx::vul::create_gui_pipeline(&app->mesh_arena, &vk_gfx, rs->render_passes[0]);
    app->sky_pipeline = gfx::vul::create_skybox_pipeline(&app->mesh_arena, &vk_gfx, rs->render_targets[0].render_pass);
    // app->mesh_pipeline = gfx::vul::create_mesh_pipeline(&app->mesh_arena, &vk_gfx, rs->render_targets[0].render_pass,
    //     gfx::vul::mesh_pipeline_info_t{
    //         vk_gfx.sporadic_uniform_buffer.buffer,
    //         app->render_system->job_storage_buffer().buffer,
    //         app->render_system->material_storage_buffer.buffer,
    //         app->render_system->environment_storage_buffer.buffer
    //     }
    // );
    // assert(app->mesh_pipeline);

    app->debug_pipeline = gfx::vul::create_debug_pipeline(
        &app->mesh_arena, 
        &vk_gfx, 
        rs->render_targets[0].render_pass
    );

    gfx::font_load(&app->texture_arena, &app->default_font, "./res/fonts/Go-Mono-Bold.ttf", 18.0f);
    gfx::font_load(&app->texture_arena, &app->large_font, "./res/fonts/Go-Mono-Bold.ttf", 24.0f);
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

    ui_textures[1] = &rs->frame_images[0].texture;
    // ui_textures[2] = &rs->frame_images[1].texture;
    
    // ui_textures[3] = make_noise_texture(&app->texture_arena, 128);
    // vk_gfx.load_texture_sampler(ui_textures[3], &app->texture_arena);

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

    // app->brick_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&app->texture_arena, 1);
    // {
    //     temp_arena_t ta = app->texture_arena;
    //     vk_gfx.load_texture_sampler(app->brick_texture, "./res/textures/rock_01_a.png", &ta);
    // }
    // app->brick_texture = make_error_texture(&app->texture_arena, 256);
    // vk_gfx.load_texture_sampler(app->brick_texture, &app->texture_arena);

    // app->brick_descriptor = vk_gfx.create_image_descriptor_set(
    //     vk_gfx.descriptor_pool,
    //     app->mesh_pipeline->descriptor_set_layouts[4],
    //     &app->brick_texture, 1);

    { //@test loading shader objects
        auto& mesh_pass = rs->frames[0].mesh_pass;
        auto& anim_pass = rs->frames[0].anim_pass;

        rs->shader_cache.load(
            app->main_arena, 
            vk_gfx,
            assets::shaders::simple_vert,
            mesh_pass.descriptor_layouts,
            mesh_pass.descriptor_count
        );
        rs->shader_cache.load(
            app->main_arena, 
            vk_gfx,
            assets::shaders::skinned_vert,
            anim_pass.descriptor_layouts,
            anim_pass.descriptor_count
        );
        rs->shader_cache.load(
            app->main_arena, 
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
            return game::rendering::create_material(
                app->render_system, 
                n, std::move(mat), 
                VK_NULL_HANDLE, 
                mesh_pass.pipeline_layout,
                shaders, array_count(shaders)
            );
        };

        make_material("default", gfx::material_t::plastic(gfx::color::v4::ray_white));
        make_material("red-plastic", gfx::material_t::plastic(gfx::color::v4::red));
        make_material("blue-plastic", gfx::material_t::plastic(gfx::color::v4::blue));
        make_material("gold-metal", gfx::material_t::plastic(gfx::color::v4::yellow));
        make_material("silver-metal", gfx::material_t::plastic(gfx::color::v4::white));
        make_material("sand", gfx::material_t::plastic(gfx::color::v4::sand));
        make_material("dirt", gfx::material_t::plastic(gfx::color::v4::dirt));

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::gui_vert,
            app->gui_pipeline->descriptor_set_layouts,
            app->gui_pipeline->descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::gui_frag,
            app->gui_pipeline->descriptor_set_layouts,
            app->gui_pipeline->descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::skybox_vert,
            app->sky_pipeline->descriptor_set_layouts,
            app->sky_pipeline->descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::skybox_frag,
            app->sky_pipeline->descriptor_set_layouts,
            app->sky_pipeline->descriptor_count
        );
    }
  

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
    TIMED_FUNCTION;
    app_t* app = get_app(app_mem);
    new (app) app_t{app_mem, app_mem->arena};

    app->app_mem = app_mem;
    
    arena_t* main_arena = &app->main_arena;
    arena_t* string_arena = &app->string_arena;

    app->temp_arena = arena_sub_arena(&app->main_arena, megabytes(4));
    app->string_arena = arena_sub_arena(&app->main_arena, megabytes(16));
    app->mesh_arena = arena_sub_arena(&app->main_arena, megabytes(512));
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

    app->resource_file = utl::res::load_pack_file(&app->mesh_arena, &app->string_arena, "./res/res.pack");

    
    physics::api_t* physics = app_mem->physics;
    if (physics) {
        gen_info("app_init", "Creating physics scene");
        assert(physics && physics->create_scene);
        physics->create_scene(physics, 0);
        gen_info("app_init", "Created physics scene");
    }
    app->game_world = game::world_init(&app->game_arena, app, physics);

    app_init_graphics(app_mem);

    {
        auto& rs = app->render_system;
        auto& vertices = rs->vertices.pool;
        auto& indices = rs->indices.pool;
        prcgen::mesh_builder_t builder{vertices, indices};

        builder.add_box({v3f{-1.0f}, v3f{1.0f}});
        game::rendering::add_mesh(rs, "unit_cube", builder.build(&app->mesh_arena));
    }

    using namespace gfx::color;

    app->scene.sporadic_buffer.mode = 1;
    app->scene.sporadic_buffer.use_lighting = 1;


    auto* player = game::spawn(app->game_world, app->render_system, game::db::characters::assassin, axis::up * 300.0f);
    player->physics.rigidbody->on_collision = player_on_collision;
    player->physics.rigidbody->linear_dampening = 9.0f;

    game::spawn(app->game_world, app->render_system,
        game::db::weapons::shotgun,
        v3f{-20,2,-10});
        // ->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
        //     puts("hello");
        // };

    auto* platform = game::spawn(app->game_world, app->render_system,
        game::db::misc::platform_3x3,
        v3f{8.0f,0.7f,0.0f});

    platform->physics.rigidbody->on_trigger = [](physics::rigidbody_t* trigger, physics::rigidbody_t* other) {
        auto* p = (game::entity_t*)trigger->user_data;
        p->coroutine->start();

        // puts("starting platform");
    };

    // game::spawn(app->game_world, app->render_system,
    //     game::db::load_from_file(app->main_arena, "res/entity/shaderball.entt"),
    //     v3f{-10,1,10});


    // game::spawn(app->game_world, app->render_system, game::db::rooms::room_01);

    
    // range_u64(i, 0, 1) {
    //     v3f pos = v3f{0.0f, 35.0f * f32(i), 0.0f};
    //     auto* r = game::spawn(app->game_world, app->render_system, game::db::rooms::tower_01, pos);
    //     r->transform.set_rotation(axis::up * math::constants::pi32 * f32(i));
    // }
    game::spawn(app->game_world, app->render_system, game::db::rooms::sponza);
    
    auto* tree = game::spawn(app->game_world, app->render_system, game::db::environmental::tree_01, v3f{20,0,20});
    constexpr u32 tree_count = 10'000;
    tree->gfx.instance_count = tree_count;
    tree->gfx.instance_buffer = arena_alloc_ctor<m44>(&app->game_world->arena, tree_count, 1.0f);
    for (size_t i = 0; i < tree_count; i++) {
        tree->gfx.instance_buffer[i] = math::transform_t{}.translate(900.0f * planes::xz * (utl::rng::random_s::randv() * 2.0f - 1.0f)).to_matrix();
    }

    utl::rng::random_t<utl::rng::xor64_random_t> rng;
    loop_iota_u64(i, 49) {
        auto* e = game::spawn(
            app->game_world, 
            app->render_system,
            game::db::misc::teapot,
            rng.randnv<v3f>() * 100.0f * planes::xz + axis::up * 8.0f
        );
        e->transform.set_scale(v3f{2.f});
        e->transform.set_rotation(rng.randnv<v3f>() * 1000.0f);
        e->gfx.material_id = rng.rand() % 6;
        // if (e->physics.rigidbody) {
        //     // e->physics.rigidbody->on_collision = teapot_on_collision;
        // }
    }

    gen_info("app", "player id: {} - {}", 
        app->game_world->player->id,
        (void*)game::find_entity_by_id(app->game_world, app->game_world->player->id)
    );
        
    gen_info("app", "world size: {}mb", GEN_TYPE_INFO(game::world_t).size/megabytes(1));
}

export_fn(void) 
app_on_deinit(app_memory_t* app_mem) {
    app_t* app = get_app(app_mem);

    game::rendering::cleanup(app->render_system);
    gen_info(__FUNCTION__, "Render System cleaned up");

    gfx::vul::state_t& vk_gfx = app->gfx;
    vk_gfx.cleanup();
    gen_info(__FUNCTION__, "Graphics API cleaned up");

    app->~app_t();
}

export_fn(void) 
app_on_unload(app_memory_t* app_mem) {
    app_t* app = get_app(app_mem);
    
    app->render_system->ticket.lock();
}

global_variable f32 gs_reload_time = 0.0f;
export_fn(void)
app_on_reload(app_memory_t* app_mem) {
    app_t* app = get_app(app_mem);
    gs_reload_time = 1.0f;
    app->render_system->ticket.unlock();
}

void 
camera_input(app_t* app, player_controller_t pc, f32 dt) {
    auto* world = app->game_world;
    auto* physics = app->game_world->physics;
    auto* player = world->player;
    auto* rigidbody = player->physics.rigidbody;
    const bool is_on_ground = rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
    const bool is_on_wall = rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;

    player->camera_controller.transform = 
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

    const f32 move_speed = 45.0f * (pc.sprint ? 1.75f : 1.0f);

    rigidbody->add_relative_force((forward * move.z + right * move.x + axis::up * move.y) * dt * move_speed);

    if(player->primary_weapon.entity) {
        player->primary_weapon.entity->transform.set_rotation(glm::quatLookAt(forward, axis::up));
    }
    rigidbody->angular_velocity = v3f{0.0f};


    if (pc.jump && (is_on_ground || is_on_wall)) {
        rigidbody->velocity.y = .3f;
    }
    
    if (pc.fire1 && player->primary_weapon.entity) {
        auto rd = forward;
        // auto ro = player->camera_controller.transform.origin;
        auto ro = player->global_transform().origin + rd * 1.7f;

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
                    //rb->inverse_transform_direction
                    auto hp = (ray.point - rb->position);
                    auto f = rb->inverse_transform_direction(rd);
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

    // if (input->pressed.keys['P']) {
    //     const game::entity_t player = *app->game_world->player;
    //     std::memcpy(app->game_world, &gs_saved_world, sizeof(game::world_t));
    //     *app->game_world->player = player;
    //     range_u64(i, 0, app->game_world->entity_count) {
    //         auto* e = app->game_world->entities + i;
    //         if (e->physics.flags == game::PhysicsEntityFlags_Dynamic &&
    //             e->physics.rigidbody
    //         ) {
    //             e->physics.rigidbody->velocity = v3f{0.0f};
    //             e->physics.rigidbody->angular_velocity = v3f{0.0f};
    //         }
    //     }
    // }

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

    input->dt *= app->time_scale;
    app_on_input(app, input);

    
    // TODO(ZACK): GAME CODE HERE

    if (app->render_system == nullptr) return;

    auto* world = app->game_world;

    if (world->player->global_transform().origin.y < -1000.0f) {
        app->app_mem->input.pressed.keys[key_id::F10] = 1;
    }

    local_persist f32 accum = 0.0f;
    const u32 sub_steps = 1;
    const f32 step = 1.0f/(60.0f*sub_steps);

    accum += f32(input->dt > 0.0f) * std::min(input->dt, step*15);

    // {
    //     local_persist gfx::trail_renderer_t tr{};
    //     v3f tr_pos = v3f{5.0f} + v3f{std::sinf(input->time), 0.0f, std::cosf(input->time)};
    //     tr.tick(input->dt, tr_pos);
    // }

    game::world_update_physics(app->game_world);

    {
        TIMED_BLOCK(PhysicsStep);
        if (accum >= step) {
            accum -= step;
            world->physics->simulate(world->physics, step);
        }
    }
        
    {
        TIMED_BLOCK(GameplayPostSimulate);

        for (size_t i{0}; i < app->game_world->entity_capacity; i++) {
            auto* e = app->game_world->entities + i;
            if (e->flags & game::EntityFlags_Breakpoint) {
                __debugbreak();
            }
            if (e->is_alive() == false) {
                continue;
            }

            const bool is_physics_object = e->physics.flags != game::PhysicsEntityFlags_None && e->physics.rigidbody;
            const bool is_pickupable = (e->flags & game::EntityFlags_Pickupable);
            const bool is_not_renderable = !e->is_renderable();
            // const bool is_running_coroutine = e->coroutine ? e->coroutine->is_running : false;

            // if (is_running_coroutine) {
            e->coroutine->run();
            // }

            if (is_physics_object) {
                app->game_world->physics->sync_rigidbody(0, e->physics.rigidbody);
                e->transform.origin = e->physics.rigidbody->position;
                e->transform.set_rotation(e->physics.rigidbody->orientation);
            }

            if (is_pickupable) {
                e->transform.rotate(axis::up, input->dt);
                if (math::intersect(math::sphere_t{e->global_transform().origin, 2.0f}, app->game_world->player->global_transform().origin)) {
                    app->game_world->player->primary_weapon.entity = e;
                    e->parent = app->game_world->player;
                    e->transform.origin = v3f{0.5, 0.0f, -0.5f};
                    e->flags &= ~game::EntityFlags_Pickupable;
                    // volatile int* crash{0}; *crash++;
                }
            }

            

            // app->debug.draw_aabb(e->global_transform().xform_aabb(e->aabb), gfx::color::v3::yellow);
        }
    }

    game::world_kill_free_queue(app->game_world);

    {
        TIMED_BLOCK(GameplayLock);
        app->render_system->ticket.lock();
    }

    TIMED_BLOCK(GameSubmitRenderJobs);
    game::rendering::begin_frame(app->render_system);
    app->debug.debug_vertices.pool.clear();

    app->game_world->player->camera_controller.transform = app->game_world->player->transform;
    app->game_world->player->camera_controller.translate(axis::up);

    for (size_t i{0}; i < app->game_world->entity_capacity; i++) {
        auto* e = app->game_world->entities + i;
        const bool is_not_renderable = !e->is_renderable();

        if (e->is_alive() == false) {
            continue;
        }

        if (is_not_renderable) {
            // gen_warn("render", "Skipping: {} - {}", (void*)e, e->flags);
            continue;
        }

        game::rendering::submit_job(
            app->render_system, 
            e->gfx.mesh_id, 
            e->gfx.material_id, 
            e->global_transform().to_matrix(),
            e->gfx.instance_count,
            e->gfx.instance_buffer
        );
    }

    app->render_system->ticket.unlock();
}

void
game_on_update(app_memory_t* app_mem) {
    // utl::profile_t p{"on_update"};
    app_t* app = get_app(app_mem);
    gs_dt = app_mem->input.dt;
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

    std::lock_guard lock{app->render_system->ticket};
    gfx::gui::ctx_clear(&app->gui.ctx, &app->gui.vertices[frame&1].pool, &app->gui.indices[frame&1].pool);
    
    const auto dt = app_mem->input.render_dt;

    app->time_text_anim -= dt;
    if ((gs_reload_time -= dt) > 0.0f) {
        gfx::gui::string_render(
            &app->gui.ctx, 
            string_t{}.view("Code Reloaded"),
            app->gui.ctx.screen_size/v2f{2.0f,4.0f} - gfx::font_get_size(&app->default_font, "Code Reloaded")/2.0f, 
            gfx::color::to_color32(v4f{0.80f, .9f, .70f, gs_reload_time}),
            &app->default_font
        );
    }
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

        local_persist bool show_entities = false;
        if (im::begin_panel(state, "Main UI"sv)) {
            local_persist bool show_player = false;
            local_persist bool show_stats = false;
            local_persist bool show_resources = false;
            local_persist bool show_window = false;
            local_persist bool show_gfx_cfg = false;
            local_persist bool show_perf = !true;
            local_persist bool show_gui = false;
            local_persist bool show_theme = false;
            local_persist bool show_colors= false;
            local_persist bool show_files[0xff];

            

            {
                local_persist f32 dt_accum{0};
                local_persist f32 dt_count{0};
                if (dt_count > 1000.0f) {
                    dt_count=dt_accum=0.0f;
                }
                dt_accum += app_mem->input.dt;
                dt_count += 1.0f;

                local_persist f32 rdt_accum{0};
                local_persist f32 rdt_count{0};
                if (rdt_count > 1000.0f) {
                    rdt_count=rdt_accum=0.0f;
                }
                rdt_accum += app_mem->input.render_dt;
                rdt_count += 1.0f;
                im::text(state, fmt_sv("Gameplay FPS: {:.2f} - {:.2f} ms", 1.0f / (dt_accum/dt_count), (dt_accum/dt_count) * 1000.0f));
                im::text(state, fmt_sv("Graphics FPS: {:.2f} - {:.2f} ms", 1.0f / (rdt_accum/rdt_count), (rdt_accum/rdt_count) * 1000.0f));
            }

#if GEN_INTERNAL
            local_persist bool show_record[128];
            if (im::text(state, "Profiling"sv, &show_perf)) {
                debug_table_t* tables[] {
                    &gs_debug_table, app_mem->physics->get_debug_table()
                };
                size_t table_sizes[]{
                    gs_main_debug_record_size, app_mem->physics->get_debug_table_size()
                };

                size_t record_count = 0;
                range_u64(t, 0, array_count(tables)) {
                    size_t size = table_sizes[t];
                    auto* table = tables[t];
                    range_u64(i, 0, size) {
                        auto* record = table->records + i;
                        const auto cycle = record->history[record->hist_id?record->hist_id-1:array_count(record->history)-1];

                        if (record->func_name == nullptr) continue;

                        im::same_line(state);
                        if (im::text(state,
                            fmt_sv("- {}:", 
                                record->func_name ? record->func_name : "<Unknown>"), 
                            show_record + record_count++
                        )) {
                            f32 hist[4096/32];
                            math::statistic_t debug_stat;
                            math::begin_statistic(debug_stat);
                            range_u64(j, 0, array_count(hist)) {
                                const auto ms = f64(record->history[j*32])/f64(1e+6);
                                math::update_statistic(debug_stat, ms);
                                hist[j] = (f32)ms;
                            }
    
                            if (im::text(state,
                                fmt_sv(" {:.2f}ms - [{:.3f}, {:.3f}]", cycle/1e+6, debug_stat.range.min, debug_stat.range.max)
                            )) {
                                record->set_breakpoint = !record->set_breakpoint;
                            }
                            math::end_statistic(debug_stat);

                            im::histogram(state, hist, array_count(hist), (f32)debug_stat.range.max, v2f{4.0f*128.0f, 32.0f});
                        } else {
                            im::text(state,
                                fmt_sv(" {:.2f}ms", cycle/1e+6)
                            );
                        }
                    }
                }
            }
#endif
            
            if (im::text(state, "Stats"sv, &show_stats)) {

                object_gui(state, app->render_system->stats);
                
                {
                    const auto [x,y] = app_mem->input.mouse.pos;
                    im::text(state, fmt_sv("- Mouse: [{:.2f}, {:.2f}]", x, y));
                }
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
                    if (im::text(state, fmt_sv("--- Cull Mode: {}", gs_cull_mode))) {
                        gs_cull_mode = (gs_cull_mode + 1) % array_count(gs_cull_modes);
                    }
                    if (im::text(state, fmt_sv("--- Polygon Mode: {}", gs_poly_mode))) {
                        gs_poly_mode = (gs_poly_mode + 1) % array_count(gs_poly_modes);
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
                    local_persist u64 file_start = 0;
                    im::same_line(state);
                    if (im::text(state, "Next")) file_start = (file_start+1) % app->resource_file->file_count;
                    if (im::text(state, "Prev")) file_start = file_start?file_start-1:app->resource_file->file_count-1;
                    range_u64(rf_i, 0, 10) {
                        u64 rf_ = file_start + rf_i;
                        u64 rf = rf_ % app->resource_file->file_count;
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
            local_persist bool show_textures = false;
            local_persist bool show_probes = false;
            local_persist bool show_env = false;
            local_persist bool show_mat_id[8] = {};
                local_persist bool show_sky = false;
            if (im::text(state, "Graphics"sv, &show_gfx)) { 
                if (im::text(state, "- Reload Shaders")) {
                    std::system("compile_shaders");
                    app->render_system->shader_cache.reload_all(
                        app->render_system->arena,
                        app->gfx
                    );
                }

                if (im::text(state, "- Texture Cache"sv, &show_textures)) {
                    auto& cache = app->render_system->texture_cache;

                    for(u64 i = cache.first();
                        i != cache.end();
                        i = cache.next(i)
                    ) {
                        if(im::text(state,
                            fmt_sv("--- {} -> {}: {} x {}, {} channels",
                                i, cache.textures[i].name, cache[i]->size.x, cache[i]->size.y, cache[i]->channels
                            )
                        )) {
                            // todo show texture here                            
                        }
                    }
                }

                if (im::text(state, "- Probes"sv, &show_probes)) { 
                    auto& probes = app->render_system->light_probes;
                    range_u64(i, 0, probes.probe_count) {
                        auto& p = probes.probes[i];
                        if (im::draw_circle_3d(state, vp, p.position, 0.1f, gfx::color::rgba::white)) {
                            auto push_theme = state.theme;
                            state.theme.border_radius = 1.0f;
                            if (im::begin_panel_3d(state, "probe"sv, vp, p.position)) {
                                im::text(state, fmt_sv("Probe ID: {}"sv, p.id));
                                im::end_panel(state);
                            }
                            state.theme = push_theme;
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
                            im::same_line(state);
                            im::text(state, "Metallic: ");
                            im::float_slider(state, &mat->metallic);

                            im::same_line(state);
                            im::text(state, "Roughness: ");
                            im::float_slider(state, &mat->roughness);

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
                const u64 entity_count = app->game_world->entity_count;

                im::text(state, fmt_sv("- Count: {}", entity_count));

                if (im::text(state, "- Kill All"sv)) {
                    range_u64(ei, 1, app->game_world->entity_capacity) {
                        if (app->game_world->entities[ei].is_alive()) {
                            app->game_world->entities[ei].queue_free();
                        }
                    }

                    // app->game_world->entities.clear();
                    // app->game_world->entity_count = 1;
                }
            }

            if (im::text(state, "Player"sv, &show_player)) {
                auto* player = app->game_world->player;
                const bool on_ground = app->game_world->player->physics.rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
                const bool on_wall = app->game_world->player->physics.rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;
                im::text(state, fmt_sv("- On Ground: {}", on_ground?"true":"false"));
                im::text(state, fmt_sv("- On Wall: {}", on_wall?"true":"false"));
                const auto v = player->physics.rigidbody->velocity;
                const auto p = player->global_transform().origin;
                im::text(state, fmt_sv("- Position: {}", p));
                im::text(state, fmt_sv("- Velocity: {}", v));
            }

            im::end_panel(state);
        }

        // for (game::entity_t* e = game_itr(app->game_world); e; e = e->next) {
        for (size_t i = 0; i < app->game_world->entity_capacity; i++) {
            auto* e = app->game_world->entities + i;
            if (e->is_alive() == false) { continue; }

            const v3f ndc = math::world_to_screen(vp, e->global_transform().origin);

            bool is_selected = widget_pos == &e->transform.origin;
            bool not_player = e != app->game_world->player;
            bool opened = false;
            if ((show_entities || is_selected) && im::begin_panel_3d(state, 
                e->name.c_data ? 
                fmt_sv("Entity: {}\0{}"sv, std::string_view{e->name}, (void*)e) :
                fmt_sv("Entity: {}", (void*)e),
                vp, e->global_transform().origin
            )) {
                opened = true;
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

                if (im::text(state, fmt_sv("Kill\0: {}"sv, (void*)e))) {
                    auto* te = e->next;
                    gen_info("gui", "Killing entity: {}", (void*)e);
                    e->queue_free();
                    // game::world_destroy_entity(app->game_world, e);
                    if (!te) {
                        im::end_panel(state);
                        break;
                    }
                    e = te;
                }

                if (check_for_debugger() && im::text(state, fmt_sv("Breakpoint\0{}"sv, e->id))) {
                    e->flags ^= game::EntityFlags_Breakpoint;
                }

                im::end_panel(state);
            }
            if (!opened && (not_player && im::draw_hiding_circle_3d(
                state, vp, e->global_transform().origin, 0.1f, 
                static_cast<u32>(utl::rng::fnv_hash_u64(e->id)), 2.0f, 4)) && state.ctx.input->mouse.buttons[0]) {
                widget_pos = &e->transform.origin;
            }

            const auto panel = im::get_last_panel(state);
            if (is_selected && opened && im::draw_circle(state, panel.max, 8.0f, gfx::color::rgba::red, 4)) {
                widget_pos = &default_widget_pos;
            }
        }

        im::gizmo(state, widget_pos, vp);

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
    TIMED_FUNCTION;
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
        gen_info(__FUNCTION__, "frame: {}", frame_count);
    }

    u32 imageIndex = wait_for_frame(app, frame_count);
    std::lock_guard lock{app->render_system->ticket};
    {
        app->render_system->camera_pos = app->game_world->camera.origin;
        app->render_system->set_view(app->game_world->camera.inverse().to_matrix(), app->width(), app->height());
    }

    *vk_gfx.sporadic_uniform_buffer.data = app->scene.sporadic_buffer;
    auto& command_buffer = vk_gfx.command_buffer[frame_count&1];
    vkResetCommandBuffer(command_buffer, 0);

    {
        auto& rs = app->render_system;
        auto& khr = vk_gfx.khr;
        auto& ext = vk_gfx.ext;

        auto command_buffer_begin_info = gfx::vul::utl::command_buffer_begin_info();
        VK_OK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));
    
        VkBuffer buffers[1] = { app->render_system->vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);

        vkCmdBindIndexBuffer(command_buffer,
            app->render_system->indices.buffer, 0, VK_INDEX_TYPE_UINT32
        );


        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            rs->frame_images[0].texture.image,
            // vk_gfx.swap_chain_images[imageIndex],
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
        
        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            rs->frame_images[1].texture.image,
            // vk_gfx.depth_stencil_texture.image,
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
            // colorAttachment.imageView = vk_gfx.swap_chain_image_views[imageIndex];
            colorAttachment.imageView = rs->frame_images[0].texture.image_view;
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.clearValue.color = { 0.0f,0.0f,0.0f,0.0f };

            // A single depth stencil attachment info can be used, but they can also be specified separately.
            // When both are specified separately, the only requirement is that the image view is identical.			
            VkRenderingAttachmentInfoKHR depthStencilAttachment{};
            depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            depthStencilAttachment.imageView = rs->frame_images[1].texture.image_view;
            // depthStencilAttachment.imageView = vk_gfx.depth_stencil_texture.image_view;
            depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };

            VkRenderingInfoKHR renderingInfo{};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            renderingInfo.renderArea = { 0, 0, rs->frame_images[0].width(), rs->frame_images[0].height() };
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;
            renderingInfo.pDepthAttachment = &depthStencilAttachment;
            renderingInfo.pStencilAttachment = &depthStencilAttachment;

            khr.vkCmdBeginRenderingKHR(command_buffer, &renderingInfo);
        }

            game::rendering::draw_skybox(
                rs, command_buffer, 
                assets::shaders::skybox_frag.filename,
                app->sky_pipeline->pipeline_layout,
                game::cam::get_direction(
                    app->game_world->player->camera_controller.yaw,
                    app->game_world->player->camera_controller.pitch
                ), app->scene.lighting.directional_light
            );

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
            VkBool32 fb_blend[1] { true };
            ext.vkCmdSetColorBlendEnableEXT(command_buffer,0, 1, fb_blend);
            

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

            build_shader_commands(
                rs, 
                command_buffer
            );
        khr.vkCmdEndRenderingKHR(command_buffer);
    
        {
            gfx::vul::utl::insert_image_memory_barrier(
                command_buffer,
                vk_gfx.swap_chain_images[imageIndex],
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
            colorAttachment.imageView = vk_gfx.swap_chain_image_views[imageIndex];
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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

            vkCmdBindDescriptorSets(command_buffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, app->gui_pipeline->pipeline_layout,
                0, 1, &app->default_font_descriptor, 0, nullptr);

            {
                VkVertexInputBindingDescription2EXT vertexInputBinding{};
                vertexInputBinding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
                vertexInputBinding.binding = 0;
                vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                vertexInputBinding.stride = sizeof(gfx::gui::vertex_t);
                vertexInputBinding.divisor = 1;

                VkVertexInputAttributeDescription2EXT vertexAttributes[] = {
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(gfx::gui::vertex_t, pos) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(gfx::gui::vertex_t, tex) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 2, 0, VK_FORMAT_R32_UINT, offsetof(gfx::gui::vertex_t, img) },
                    { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 3, 0, VK_FORMAT_R32_UINT, offsetof(gfx::gui::vertex_t, col) },
                };

                ext.vkCmdSetVertexInputEXT(command_buffer, 1, &vertexInputBinding, array_count(vertexAttributes), vertexAttributes);
            }

            vkCmdBindVertexBuffers(command_buffer,
                0, 1, &app->gui.vertices[frame_count&1].buffer, offsets);

            vkCmdBindIndexBuffer(command_buffer,
                app->gui.indices[frame_count&1].buffer, 0, VK_INDEX_TYPE_UINT32);

            VkShaderEXT gui_shaders[2] {
                *rs->shader_cache.get("./res/shaders/bin/gui.vert.spv"),
                *rs->shader_cache.get("./res/shaders/bin/gui.frag.spv"),
            };
            assert(gui_shaders[0] != VK_NULL_HANDLE);
            assert(gui_shaders[1] != VK_NULL_HANDLE);

            VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
            ext.vkCmdBindShadersEXT(command_buffer, 2, stages, gui_shaders);

            vkCmdDrawIndexed(command_buffer,
                (u32)app->gui.ctx.indices->count,
                1,
                0,
                0,
                0
            );
        }

        khr.vkCmdEndRenderingKHR(command_buffer);

        
        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            vk_gfx.swap_chain_images[imageIndex],
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
        
        VK_OK(vkEndCommandBuffer(command_buffer));
        present_frame(app, command_buffer, imageIndex, frame_count);
    }
}

void
game_on_render2(app_memory_t* app_mem) { 
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
    std::lock_guard lock{app->render_system->ticket};

    {
        app->render_system->camera_pos = app->game_world->camera.origin;
        app->render_system->set_view(app->game_world->camera.inverse().to_matrix(), app->width(), app->height());
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
            utl::str_hash_find(app->render_system->mesh_hash, "res/models/sphere.obj")
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


    // {
    //     vkCmdBindDescriptorSets(command_buffer, 
    //         VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline->pipeline_layout,
    //         0, 4, app->mesh_pipeline->descriptor_sets, 0, nullptr);

    //     vkCmdBindDescriptorSets(command_buffer, 
    //         VK_PIPELINE_BIND_POINT_GRAPHICS, app->mesh_pipeline->pipeline_layout,
    //         4, 1, &app->brick_descriptor, 0, nullptr);

    //     VkBuffer buffers[1] = { app->render_system->vertices.buffer };
    //     VkDeviceSize offsets[1] = { 0 };

    //     vkCmdBindVertexBuffers(command_buffer,
    //         0, 1, buffers, offsets);

    //     vkCmdBindIndexBuffer(command_buffer,
    //         app->render_system->indices.buffer, 0, VK_INDEX_TYPE_UINT32
    //     );
    //     game::rendering::build_commands(app->render_system, command_buffer);
    // }


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



inline entity_editor_t* get_entity_editor(app_memory_t* app) {
    local_persist entity_editor_t ee{get_app(app)};
    return &ee;
}

export_fn(void) 
app_on_render(app_memory_t* app_mem) {
    auto* app = get_app(app_mem);
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
        app_mem->input.gamepads->buttons[button_id::dpad_down].is_pressed) {
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
