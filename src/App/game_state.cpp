#include <zyy_core.hpp>

#include "App/game_state.hpp"

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

#include "App/Game/GUI/entity_editor.hpp"
#include "App/Game/Physics/player_movement.hpp"


#include "App/Game/Util/loading.hpp"


// global_variable f32 gs_dt;
global_variable f32 gs_reload_time = 0.0f;
global_variable f32 gs_jump_save_time = 0.0f;
global_variable f32 gs_jump_load_time = 0.0f;

global_variable b32 gs_show_console=false;
global_variable b32 gs_show_watcher=false;

global_variable zyy::cam::first_person_controller_t gs_debug_camera;
global_variable b32 gs_debug_camera_active;

global_variable gfx::anim::skeleton_t* gs_skeleton;
global_variable gfx::anim::animation_t* gs_animations;
global_variable u64 gs_animation_count;
global_variable gfx::anim::animator_t gs_animator;
global_variable gfx::mesh_list_t gs_skinned_mesh;

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

inline game_state_t*
get_game_state(game_memory_t* mem) {
    // note(zack): App should always be the first thing initialized in perm_memory
    return (game_state_t*)mem->arena.start;
}

void 
draw_game_gui(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    game_state_t* game_state = get_game_state(game_memory);
    auto* world = game_state->game_world;
    auto* player = world->player;

    if(!player || !world) return;

    auto& imgui = game_state->gui.state;

    if (player->primary_weapon.entity) {
        const auto& weapon = player->primary_weapon.entity->stats.weapon;
        const auto center = imgui.ctx.screen_size * 0.5f;
        gfx::gui::im::draw_circle(imgui, center, 2.0f, gfx::color::rgba::white);
        gfx::gui::im::draw_circle(imgui, center, 4.0f, gfx::color::rgba::light_gray);
        v2f weapon_display_start = imgui.ctx.screen_size * v2f{0.05f, 0.9f};
        gfx::gui::string_render(&imgui.ctx, fmt_sv("{} / {}", weapon.clip.current, weapon.clip.max), &weapon_display_start, gfx::color::rgba::white);
        gfx::gui::string_render(&imgui.ctx, fmt_sv("Chambered: {}", weapon.chamber_count), &weapon_display_start, gfx::color::rgba::white);
    }
}

#include "App/Game/GUI/debug_gui.hpp"

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
    // auto* tex = push_struct<gfx::vul::texture_2d_t>(arena);
    return make_grid_texture(arena,
							 tex,
							 size,
							 gfx::color::v3::black,
							 gfx::color::v3::purple);
}

inline static gfx::vul::texture_2d_t*
make_error_texture(arena_t* arena, u32 size) {
    auto* tex = push_struct<gfx::vul::texture_2d_t>(arena);
    return make_grid_texture(arena,
							 tex,
							 size,
							 gfx::color::v3::black,
							 gfx::color::v3::purple);
}

inline static gfx::vul::texture_2d_t*
make_noise_texture(arena_t* arena, u32 size) {
    auto* tex = push_struct<gfx::vul::texture_2d_t>(arena);
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

    game_state->render_system = rendering::init<megabytes(512)>(vk_gfx, &game_state->main_arena);
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
    
    range_u64(i, 0, game_state->resource_file->file_count) {
        arena_t* arena = &game_state->mesh_arena;
        if (game_state->resource_file->table[i].file_type != utl::res::magic::skel) { 
            continue; 
        }

        const char* file_name = game_state->resource_file->table[i].name.c_data;
        std::byte*  file_data = utl::res::pack_file_get_file(game_state->resource_file, i);

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
        zyy_warn(__FUNCTION__, "Loading {} meshes", results.count);
        results.meshes  = push_struct<gfx::mesh_view_t>(arena, results.count);

        u64 total_vertex_count = 0;
        utl::pool_t<gfx::skinned_vertex_t>& vertices{rs->skinned_vertices.pool};

        for (size_t j = 0; j < results.count; j++) {
            std::string name = blob.deserialize<std::string>();
            zyy_info(__FUNCTION__, "Mesh name: {}", name);
            const size_t vertex_count = blob.deserialize<u64>();
            const size_t vertex_bytes = sizeof(gfx::skinned_vertex_t) * vertex_count;

            total_vertex_count += vertex_count;

            const u32 vertex_start = safe_truncate_u64(vertices.count());
            const u32 index_start = 0;

            results.meshes[j].vertex_count = safe_truncate_u64(vertex_count);
            results.meshes[j].vertex_start = vertex_start;
            results.meshes[j].index_start = index_start;

            gfx::skinned_vertex_t* v = vertices.allocate(vertex_count);

            utl::copy(v, blob.read_data(), vertex_bytes);
            blob.advance(vertex_bytes);

            // no indices for skeletal meshes atm
            // const size_t index_count = blob.deserialize<u64>();
            // const size_t index_bytes = sizeof(u32) * index_count;
            // results.meshes[j].index_count = safe_truncate_u64(index_count);

            // u32* tris = indices->allocate(index_count);
            // utl::copy(tris, blob.read_data(), index_bytes);
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

        gs_animation_count = blob.deserialize<u64>();
        const auto total_anim_size = blob.deserialize<u64>();

        gs_animations = (gfx::anim::animation_t*) blob.read_data();
        blob.advance(total_anim_size);


        const auto skeleton_size = blob.deserialize<u64>();
        assert(skeleton_size == sizeof(gfx::anim::skeleton_t));
        gs_skeleton = (gfx::anim::skeleton_t*)blob.read_data();
        blob.advance(skeleton_size);

        // game_memory->message_box(
        //     fmt_sv("Asset {} has skeleton, {} vertices, {} animations\n{} - {}s {} ticks\nSkeleton: {} bones", 
        //     file_name, total_vertex_count, anim_count, 
        //     animations[0].name, animations[0].duration, animations[0].ticks_per_second,
        //     skeleton->bone_count).data());

        range_u64(a, 0, gs_animation_count) {
            auto& animation = gs_animations[a];
            zyy_info(__FUNCTION__, "Animation: {}, size: {}", animation.name, sizeof(gfx::anim::animation_t));
            range_u64(n, 0, animation.node_count) {
                auto& node = animation.nodes[n];
                auto& timeline = node.bone;
                if (timeline) {
                    zyy_info(__FUNCTION__, "Bone: {}, id: {}", timeline->name, timeline->id);
                }
            }
        }
        range_u64(b, 0, gs_skeleton->bone_count) {
            auto& bone = gs_skeleton->bones[b];
            zyy_info(__FUNCTION__, "Bone: {}, parent: {}", bone.name_hash, bone.parent);
        }
    }

    gs_animator.play_animation(gs_animations);
    range_u64(i, 0, 1000) {
        gs_animator.update(1.0f/60.0f);
    }

    {
        temp_arena_t ta = game_state->texture_arena;
        gfx::font_load(&ta, &game_state->large_font, "./res/fonts/Go-Mono-Bold.ttf", 24.0f);
        gfx::font_load(&ta, &game_state->default_font, "./res/fonts/Go-Mono-Bold.ttf", 18.0f);
        game_state->default_font_texture = push_struct<gfx::vul::texture_2d_t>(&game_state->texture_arena, 1);
        
        vk_gfx.load_font_sampler(
            &ta,
            game_state->default_font_texture,
            &game_state->default_font);


    }



    set_ui_textures(game_state);




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
                blood_texture->pixels[i++] = u8(n*255.0f);
                blood_texture->pixels[i++] = u8(n*255.0f * 0.3f);
                blood_texture->pixels[i++] = u8(n*255.0f * 0.3f);
                blood_texture->pixels[i++] = u8(n*255.0f);
            }
        }
        vk_gfx.load_texture_sampler(blood_texture);

        rs->texture_cache.insert("blood", *blood_texture);
    }
    
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
            unlit_mat.flags = gfx::material_lit;
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

    for (size_t i = 0; i < array_count(game_state->gui.ctx.dyn_font); i++) {
        game_state->gui.ctx.dyn_font[i] = push_struct<gfx::font_t>(&game_state->texture_arena);
        gfx::font_load(&game_state->texture_arena, game_state->gui.ctx.dyn_font[i], "./res/fonts/Go-Mono-Bold.ttf", (f32)i+1);
    }
}

export_fn(void) 
app_on_init(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    Platform = game_memory->platform;

    game_state_t* game_state = get_game_state(game_memory);
    new (game_state) game_state_t{game_memory, game_memory->arena};
    

    game_state->game_memory = game_memory;
    
    arena_t* main_arena = &game_state->main_arena;
    arena_t* string_arena = &game_state->string_arena;

    game_state->temp_arena = arena_sub_arena(&game_state->main_arena, megabytes(4));
    game_state->string_arena = arena_sub_arena(&game_state->main_arena, megabytes(16));
    game_state->mesh_arena = arena_sub_arena(&game_state->main_arena, megabytes(512));
    game_state->texture_arena = arena_sub_arena(&game_state->main_arena, megabytes(32));
    game_state->game_arena = arena_sub_arena(&game_state->main_arena, megabytes(512));
    game_state->gui.arena = arena_sub_arena(main_arena, megabytes(8));

    defer {
        arena_clear(&game_state->temp_arena);
    };

    assets::sounds::load();
    game_state->debug_state = push_struct<debug_state_t>(main_arena, 1, game_memory->input.time);
    game_state->debug_state->arena = arena_create();
    game_state->debug_state->watch_arena = arena_create(megabytes(4));


    gs_debug_state = game_state->debug_state;    

    using namespace std::string_view_literals;
#ifdef ZYY_INTERNAL
    game_state->debug.console = push_struct<debug_console_t>(main_arena);

    console_add_command(game_state->debug.console, "help", [](void* data) {
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
    }, game_state->debug.console);

    console_add_command(game_state->debug.console, "noclip", [](void* data) {
        // auto* console = (debug_console_t*)data;
        gs_debug_camera_active = !gs_debug_camera_active;
        
    }, game_state->debug.console);

    // time is broken during loops, this is a reminder
    console_add_command(game_state->debug.console, "rtime", [](void* data) {
        auto* game_state = (game_state_t*)data;
        console_log(game_state->debug.console, fmt_sv("Run Time: {} seconds", game_state->input().time));
    }, game_state);

    console_add_command(game_state->debug.console, "build", [](void* data) {
        // auto* console = (debug_console_t*)data;
        std::system("start /B ninja");
        
    }, game_state->debug.console);

    console_add_command(game_state->debug.console, "code", [](void* data) {
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
        
    }, game_state->debug.console);

    console_add_command(game_state->debug.console, "run", [](void* data) {
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
        
    }, game_state->debug.console);

    console_add_command(game_state->debug.console, "dtimeout", [](void* data) {
        auto* console = (debug_console_t*)data;
        const auto time = console->last_float();
        if (time) {
            console_log(console, fmt_sv("Setting Debug Timeout to {}", *time));
            DEBUG_SET_TIMEOUT(*time);
        } else {
            console_log(console, "Parse error", gfx::color::rgba::red);
        }
    }, game_state->debug.console);

    console_add_command(game_state->debug.console, "fdist", [](void* data) {
        auto* console = (debug_console_t*)data;
        const auto time = console->last_float();
        if (time) {
            DEBUG_SET_FOCUS_DISTANCE(*time);
            console_log(console, fmt_sv("Setting Debug Focus Distance to {}", *time));
        } else {
            console_log(console, "Parse error", gfx::color::rgba::red);
        }
    }, game_state->debug.console);

    // console_add_command(game_state->debug.console, "build", [](void* data) {
    //     // auto* console = (debug_console_t*)data;
    //     std::system("start /B build n game");
        
    // }, game_state->debug.console);

    console_log(game_state->debug.console, "Enter a command");

#endif

    game_state->resource_file = utl::res::load_pack_file(&game_state->mesh_arena, &game_state->string_arena, "./res/res.pack");

    
    physics::api_t* physics = game_memory->physics;
    physics->entity_transform_offset = offsetof(zyy::entity_t, transform);
    if (physics) {
        zyy_info("app_init", "Creating physics scene");
        assert(physics && physics->create_scene);
        physics->create_scene(physics, 0);
        zyy_info("app_init", "Created physics scene");
    }
    game_state->game_world = zyy::world_init(&game_state->game_arena, game_state, physics);

    app_init_graphics(game_memory);

    zyy::world_init_effects(game_state->game_world);

    {
        auto& rs = game_state->render_system;
        auto& vertices = rs->scene_context->vertices.pool;
        auto& indices = rs->scene_context->indices.pool;
        prcgen::mesh_builder_t builder{vertices, indices};

        builder.add_box({v3f{-1.0f}, v3f{1.0f}});
        rendering::add_mesh(rs, "unit_cube", builder.build(&game_state->mesh_arena));
    }

    using namespace gfx::color;

    game_state->scene.sporadic_buffer.time = game_state->input().time;
    game_state->scene.sporadic_buffer.mode = 1;
    game_state->scene.sporadic_buffer.use_lighting = 1;

    zyy_info("game_state", "world size: {}mb", GEN_TYPE_INFO(zyy::world_t).size/megabytes(1));
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
    game_state_t* game_state = get_game_state(game_memory);
    
    game_state->render_system->ticket.lock();
}

export_fn(void)
app_on_reload(game_memory_t* game_memory) {
    Platform = game_memory->platform;
    game_state_t* game_state = get_game_state(game_memory);
    gs_debug_state = game_state->debug_state;
    gs_reload_time = 1.0f;
    game_state->render_system->ticket.unlock();
}

void 
app_on_input(game_state_t* game_state, app_input_t* input) {
    if (input->pressed.keys[key_id::F10]) {
        gs_jump_load_time = 1.0f;
    }
    if (input->pressed.keys[key_id::F9]) {
        gs_jump_save_time = 1.0f;
    }

    if (input->pressed.keys[key_id::F1]) {
        if (auto b = gs_show_watcher = !gs_show_watcher) {
            game_state->gui.state.active = "watcher_text_box"_sid;
        } else {
            game_state->gui.state.hot = 
            game_state->gui.state.active = 0;
        }
    }
    if (input->pressed.keys[key_id::F2]) {
        if (auto b = gs_show_console = !gs_show_console) {
            game_state->gui.state.active = "console_text_box"_sid;
        } else {
            game_state->gui.state.hot = 
            game_state->gui.state.active = 0;
        }
    }
    if (input->pressed.keys[key_id::F5]) {
        zyy::world_destroy_all(game_state->game_world);
        game_state->game_world->world_generator = nullptr;
        game_state->game_world->player = nullptr;
        game_state->game_world->effects.blood_splat_count = 0;
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
}

void game_on_gameplay(game_state_t* game_state, app_input_t* input, f32 dt) {
    TIMED_FUNCTION;
 
    // TODO(ZACK): GAME CODE HERE

    if (game_state->render_system == nullptr) return;

    auto* rs = game_state->render_system;
    auto* world = game_state->game_world;
    
    zyy::world_update(world, dt);

    DEBUG_DIAGRAM_(v3f{axis::right * 50.0f}, 0.01f);

    if (world->player && world->player->global_transform().origin.y < -100.0f) {
        if (world->player->parent) {
            world->player->transform.origin.y = world->player->parent->global_transform().inv_xform(v3f{0.0f}).y;
        } else {
            world->player->transform.origin.y = 0.0f;
        }
        // zyy_warn("killz", "Reset players vertical position");
        // game_state->game_memory->input.pressed.keys[key_id::F10] = 1;
    }


    // {
    //     local_persist gfx::trail_renderer_t tr{};
    //     v3f tr_pos = v3f{5.0f} + v3f{std::sinf(input->time), 0.0f, std::cosf(input->time)};
    //     tr.tick(dt, tr_pos);
    // }
    
        
    {
        TIMED_BLOCK(GameplayUpdatePostSimulate);

        for (size_t i{0}; i < game_state->game_world->entity_capacity; i++) {
            auto* e = game_state->game_world->entities + i;
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

            if (brain_id != uid::invalid_id) {
                world_update_brain(world, e, dt);
            }

            if (is_pickupable) {
                e->transform.rotate(axis::up, std::min(0.5f, dt));
            }

            // @debug

            // const auto entity_aabb = e->global_transform().xform_aabb(e->aabb);
            // if (e->type != zyy::entity_type::player) {
            //     DEBUG_DIAGRAM_(entity_aabb, 0.0000f);
            // }
            // if (e->gfx.particle_system) {
            //     const auto particle_aabb = e->gfx.particle_system->aabb;
            //     DEBUG_DIAGRAM_(particle_aabb, 0.0000f);
            // } else if (e->gfx.mesh_id != -1) {
            //     auto& mesh = world->render_system()->mesh_cache.get(e->gfx.mesh_id);
            //     range_u64(j, 0, mesh.count) {
            //         const auto mesh_aabb = e->global_transform().xform_aabb(mesh.meshes[i].aabb);
            //         DEBUG_DIAGRAM_(mesh_aabb, 0.0000f);
            //     }
            // }
        }
    }
    app_on_input(game_state, input);

    zyy::world_kill_free_queue(game_state->game_world);

    zyy::world_update_kinematic_physics(game_state->game_world);

    // local_persist f32 accum = 0.0f;
    // const u32 sub_steps = 1;
    // const f32 step = 1.0f/(60.0f*sub_steps);

    // accum += f32(dt > 0.0f) * std::min(dt, step*15);
    // if (dt > 0.0f)
    {
        TIMED_BLOCK(PhysicsStep);

        // if (game_state->time_scale == 1.0f) {
            // world->physics->simulate(world->physics, step);
        //     accum = 0.0f;
        // } else {
            // if (accum >= step) {
                // accum -= step;
                world->physics->simulate(world->physics, dt);
            // }
        // }
    }

    for (size_t i{0}; i < game_state->game_world->entity_capacity; i++) {
        auto* e = game_state->game_world->entities + i;

        if (e->is_alive() == false) {
            continue;
        }

        // if (e->physics.rigidbody && accum > 0.0f) {
        //     // if (e->physics.rigidbody->type == physics::rigidbody_type::CHARACTER) {
        //         // continue;
        //     // }
        //     auto* rb = e->physics.rigidbody;
        //     e->transform.origin = rb->position + rb->velocity * accum;
        //     auto orientation = rb->orientation;
        //     orientation += (orientation * glm::quat(0.0f, rb->angular_velocity)) * (0.5f * accum);
        //     orientation = glm::normalize(orientation);
        //     // e->transform.set_rotation(orientation);
        // }

        if(e->primary_weapon.entity) {
            auto forward = -e->primary_weapon.entity->transform.basis[2];
        //     e->primary_weapon.entity->transform.set_rotation(glm::quatLookAt(forward, axis::up));
        //     e->primary_weapon.entity->transform.set_scale(v3f{3.0f});
            e->primary_weapon.entity->transform.origin =
                e->global_transform().origin +
                forward + axis::up * 0.3f;
        //         //  0.5f * (right + axis::up);
        }
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
        DEBUG_WATCH(&env->sun.color);
        DEBUG_WATCH((v3f*)&env->sun.direction);
    }

    TIMED_BLOCK(GameSubmitRenderJobs);
    // rendering::begin_frame(game_state->render_system);
    // game_state->debug.debug_vertices.pool.clear();

    draw_gui(game_state->game_memory);

    world->camera.reset_priority();


    if (world->player) {
        world->player->camera_controller.transform.origin = world->player->physics.rigidbody->position + 
            axis::up * world->player->camera_controller.head_height + axis::up * world->player->camera_controller.head_offset;
        world->player->camera_controller.translate(v3f{0.0f});
    
        DEBUG_SET_FOCUS(world->player->global_transform().origin);
    }
    
    game_state->render_system->camera_pos = game_state->game_world->camera.origin;
    game_state->render_system->set_view(game_state->game_world->camera.inverse().to_matrix(), game_state->width(), game_state->height());

    // make sure not to allocate from buffer during sweep


    arena_begin_sweep(&world->render_system()->instance_storage_buffer.pool);
    // arena_begin_sweep(&world->particle_arena);

    for (size_t i{0}; i < game_state->game_world->entity_capacity; i++) {
        auto* e = game_state->game_world->entities + i;
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
            arena_sweep_keep(&world->render_system()->instance_storage_buffer.pool, e->gfx.instance_end());
        }

        if (e->gfx.particle_system) {
            auto* ps = e->gfx.particle_system;
            // arena_sweep_keep(&world->particle_arena, (std::byte*)(ps->particles + ps->max_count));
            particle_system_update(e->gfx.particle_system, e->global_transform(), dt);
            particle_system_build_matrices(
                e->gfx.particle_system, 
                e->global_transform(),
                e->gfx.dynamic_instance_buffer, 
                e->gfx.instance_count()
            );
        }

        v3f size = e->aabb.size()*0.5f;
        v4f bounds{e->aabb.center(), glm::max(glm::max(size.x, size.y), size.z) };

        // if (e->type == zyy::entity_type::player) continue;

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
    }

    arena_sweep_keep(&world->render_system()->instance_storage_buffer.pool, (std::byte*)(world->effects.blood_splats + world->effects.blood_splat_max));
    zyy::world_render_bloodsplat(world);

    // game_state->render_system->ticket.unlock();
}

void
game_on_update(game_memory_t* game_memory) {
    game_state_t* game_state = get_game_state(game_memory);
    game_state->time += game_memory->input.dt * game_state->time_scale;

    auto* world_generator = game_state->game_world->world_generator;
    if (world_generator && world_generator->is_done() == false) {
        world_generator->execute(game_state->game_world, [&](){draw_gui(game_memory);});
        std::lock_guard lock{game_state->render_system->ticket};
        set_ui_textures(game_state);
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

    // vkWaitForFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[frame_count%2], VK_TRUE, UINT64_MAX);
    // vkResetFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[frame_count%2]);
    // u32 imageIndex;
    // vkAcquireNextImageKHR(vk_gfx.device, vk_gfx.swap_chain, UINT64_MAX, 
    //     vk_gfx.image_available_semaphore[frame_count%2], VK_NULL_HANDLE, &imageIndex);
    // return imageIndex;
}

inline static void
present_frame(game_state_t* game_state, VkCommandBuffer command_buffer, u32 image_index, u64 frame_count) {
    
    rendering::present_frame(game_state->render_system, image_index);

    // gfx::vul::state_t& vk_gfx = game_state->gfx;
    // VkSubmitInfo submitInfo{};
    // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // VkSemaphore waitSemaphores[] = {vk_gfx.image_available_semaphore[frame_count%2]};
    // VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    // submitInfo.waitSemaphoreCount = 1;
    // submitInfo.pWaitSemaphores = waitSemaphores;
    // submitInfo.pWaitDstStageMask = waitStages;

    // VkSemaphore signalSemaphores[] = {vk_gfx.render_finished_semaphore[frame_count%2]};
    // submitInfo.signalSemaphoreCount = 1;
    // submitInfo.pSignalSemaphores = signalSemaphores;

    // submitInfo.commandBufferCount = 1;
    // submitInfo.pCommandBuffers = &command_buffer;


    // if (vkQueueSubmit(vk_gfx.gfx_queue, 1, &submitInfo, vk_gfx.in_flight_fence[frame_count%2]) != VK_SUCCESS) {
    //     zyy_error(__FUNCTION__, "failed to submit draw command buffer!");
    //     std::terminate();
    // }

    // VkPresentInfoKHR presentInfo{};
    // presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // presentInfo.waitSemaphoreCount = 1;
    // presentInfo.pWaitSemaphores = signalSemaphores;

    // VkSwapchainKHR swapChains[] = {vk_gfx.swap_chain};
    // presentInfo.swapchainCount = 1;
    // presentInfo.pSwapchains = swapChains;
    // presentInfo.pImageIndices = &imageIndex;

    // presentInfo.pResults = nullptr; // Optional

    // vkQueuePresentKHR(vk_gfx.present_queue, &presentInfo);
}

void
game_on_render(game_memory_t* game_memory, u32 imageIndex) { 
    TIMED_FUNCTION;
    
    game_state_t* game_state = get_game_state(game_memory);
    u64 frame_count = game_state->render_system->frame_count%2;

    gfx::vul::state_t& vk_gfx = game_state->gfx;

    game_state->scene.sporadic_buffer.time = game_state->input().time;
    *vk_gfx.sporadic_uniform_buffer.data = game_state->scene.sporadic_buffer;

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
        
        rendering::memory_barriers(rs, command_buffer);
        const u64 gui_frame = game_state->gui.ctx.frame&1;
        auto& gui_vertices = game_state->gui.vertices[gui_frame];
        auto& gui_indices = game_state->gui.indices[gui_frame];
        gui_vertices.insert_memory_barrier(command_buffer);
        gui_indices.insert_memory_barrier(command_buffer);
        
        // if (gs_rtx_on) 
        {
            rendering::begin_rt_pass(game_state->render_system, command_buffer);
        // } else {
            // game_state->render_system->rt_cache->frame = 0;
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
                // colorAttachment.imageView = vk_gfx.swap_chain_image_views[imageIndex];
                colorAttachment.imageView = rs->frame_images[frame_count%2].texture.image_view;
                colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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
            auto view_dir = game_state->game_world->player ? zyy::cam::get_direction(
                    game_state->game_world->player->camera_controller.yaw,
                    game_state->game_world->player->camera_controller.pitch
                ) : axis::forward;
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
            VkColorBlendEquationEXT blend_fn[1];
            blend_fn[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            blend_fn[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_fn[0].colorBlendOp = VK_BLEND_OP_ADD;
            blend_fn[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blend_fn[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_fn[0].alphaBlendOp = VK_BLEND_OP_ADD;
            // ext.vkCmdSetColorBlendEquationEXT(command_buffer, 0, 1, blend_fn);
            VkBool32 fb_blend[1] { true };
            ext.vkCmdSetColorBlendEnableEXT(command_buffer,0, 1, fb_blend);
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
            // if (vk_gfx.compute_index != vk_gfx.graphics_index) {
            // if (gs_rtx_on) {
            //     gfx::vul::utl::insert_image_memory_barrier(
            //         command_buffer,
            //         rs->frame_images[6].texture.image,
            //         0,
            //         VK_ACCESS_SHADER_READ_BIT,
            //         VK_IMAGE_LAYOUT_GENERAL,
            //         VK_IMAGE_LAYOUT_GENERAL,
            //         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            //         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            //         VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            //         vk_gfx.compute_index,
            //         vk_gfx.graphics_index
            //     );
            // }
            // } else {
            //     gfx::vul::utl::insert_image_memory_barrier(
            //         command_buffer,
            //         rs->frame_images[frame_count%2].texture.image,
            //         VK_ACCESS_SHADER_WRITE_BIT,
            //         VK_ACCESS_SHADER_READ_BIT,
            //         VK_IMAGE_LAYOUT_GENERAL,
            //         VK_IMAGE_LAYOUT_GENERAL,
            //         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            //         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            //         VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            //     );
            // }
            gfx::vul::begin_debug_marker(command_buffer, "Post Process Pass");

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

            ext.vkCmdSetDepthTestEnableEXT(command_buffer, VK_FALSE);
            ext.vkCmdSetDepthWriteEnableEXT(command_buffer, VK_FALSE);

            {   // tonemap
                rendering::pp_material_t parameters = rs->postprocess_params;
                
                rendering::draw_postprocess(
                    rs, command_buffer,
                    assets::shaders::tonemap_frag.filename,
                    gs_rtx_on && 0 ?
                    &rs->frame_images[6].texture :
                    &rs->frame_images[frame_count%2].texture,
                    parameters
                );
            }
            gfx::vul::end_debug_marker(command_buffer);
            gfx::vul::begin_debug_marker(command_buffer, "UI Pass");

            ext.vkCmdSetDepthTestEnableEXT(command_buffer, VK_TRUE);
            ext.vkCmdSetDepthWriteEnableEXT(command_buffer, VK_TRUE);

            vkCmdBindDescriptorSets(command_buffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, game_state->render_system->pipelines.gui.layout,
                0, 1, &game_state->default_font_descriptor, 0, nullptr);

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

            vkCmdDrawIndexed(command_buffer,
                (u32)gui_indices.pool.count(),
                1,
                0,
                0,
                0
            );
        }

        khr.vkCmdEndRenderingKHR(command_buffer);
        gfx::vul::end_debug_marker(command_buffer);

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
        present_frame(game_state, command_buffer, imageIndex, frame_count);
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
    if (game_memory->input.pressed.keys[key_id::F3] || 
        game_memory->input.gamepads->buttons[button_id::dpad_down].is_pressed) {
        scene_state = !scene_state;
    }
    u32 image_index = wait_for_frame(game_state);
    rendering::begin_frame(game_state->render_system);

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
}
