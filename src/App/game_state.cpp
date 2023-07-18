#include <core.hpp>

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

#include "App/Game/Rendering/assets.hpp"

#include "App/Game/GUI/entity_editor.hpp"

#include "App/Game/Util/loading.hpp"

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



inline static gfx::vul::texture_2d_t*
make_grid_texture(arena_t* arena, gfx::vul::texture_2d_t* tex, u32 size, v3f c1, v3f c2, f32 grid_count = 10.0f) {
    *tex = {};
    tex->size[0] = tex->size[1] = size;
    tex->channels = 4;
    tex->pixels = arena_alloc_ctor<u8>(arena, size*size*4);
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
    // auto* tex = arena_alloc<gfx::vul::texture_2d_t>(arena);
    return make_grid_texture(arena, tex, size, gfx::color::v3::black, gfx::color::v3::purple);
}
inline static gfx::vul::texture_2d_t*
make_error_texture(arena_t* arena, u32 size) {
    auto* tex = arena_alloc<gfx::vul::texture_2d_t>(arena);
    return make_grid_texture(arena, tex, size, gfx::color::v3::black, gfx::color::v3::purple);
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

inline game_state_t*
get_game_state(game_memory_t* mem) {
    // note(zack): App should always be the first thing initialized in perm_memory
    return (game_state_t*)mem->arena.start;
}

void
app_init_graphics(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    game_state_t* game_state = get_game_state(game_memory);

    gfx::vul::state_t& vk_gfx = game_state->gfx;
    {
        temp_arena_t t = game_state->main_arena;
        game_state->gfx.init(&game_memory->config, &t);
    }

    make_grid_texture(&game_state->texture_arena, &vk_gfx.null_texture, 256, v3f{0.3f}, v3f{0.6f}, 2.0f);
    // texture_add_border(&vk_gfx.null_texture, gfx::color::v3::yellow, 4);
    // texture_add_border(&vk_gfx.null_texture, v3f{0.2f}, 4);
    texture_add_border(&vk_gfx.null_texture, v3f{0.9f}, 4);
    // make_error_texture(&game_state->texture_arena, &vk_gfx.null_texture, 256);
    vk_gfx.load_texture_sampler(&vk_gfx.null_texture);

    game_state->render_system = rendering::init<megabytes(256)>(vk_gfx, &game_state->main_arena);
    vk_gfx.create_vertex_buffer(&game_state->render_system->vertices);
    vk_gfx.create_index_buffer(&game_state->render_system->indices);
    vk_gfx.create_vertex_buffer(&game_state->debug.debug_vertices);
    vk_gfx.create_vertex_buffer(&game_state->gui.vertices[0]);
    vk_gfx.create_index_buffer(&game_state->gui.indices[0]);
    vk_gfx.create_vertex_buffer(&game_state->gui.vertices[1]);
    vk_gfx.create_index_buffer(&game_state->gui.indices[1]);
    auto* rs = game_state->render_system;

    // load all meshes from the resource file
    range_u64(i, 0, game_state->resource_file->file_count) {
        if (game_state->resource_file->table[i].file_type != utl::res::magic::mesh) { 
            continue; 
        }

        std::byte* file_data = utl::res::pack_file_get_file(game_state->resource_file, i);

        auto loaded_mesh = load_bin_mesh_data(
            &game_state->mesh_arena,
            file_data, 
            &game_state->render_system->vertices.pool,
            &game_state->render_system->indices.pool
        );
        const char* file_name = game_state->resource_file->table[i].name.c_data;

        range_u64(m, 0, loaded_mesh.count) {
            u64 ids[2];
            u32 mask = game_state->render_system->texture_cache.load_material(game_state->render_system->arena, game_state->gfx, loaded_mesh.meshes[m].material, ids);
            loaded_mesh.meshes[m].material.albedo_id = (mask&0x1) ? ids[0] : std::numeric_limits<u64>::max();
            loaded_mesh.meshes[m].material.normal_id = (mask&0x2) ? ids[1] : std::numeric_limits<u64>::max();
        }
        rendering::add_mesh(game_state->render_system, file_name, loaded_mesh);
    }
    
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

        auto* animations = (gfx::anim::animation_t*) blob.read_data();
        blob.advance(total_anim_size);


        const auto skeleton_size = blob.deserialize<u64>();
        assert(skeleton_size == sizeof(gfx::anim::skeleton_t));
        auto* skeleton = (gfx::anim::skeleton_t*)blob.read_data();
        blob.advance(skeleton_size);

        // game_memory->message_box(
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

    game_state->gui_pipeline = gfx::vul::create_gui_pipeline(&game_state->mesh_arena, &vk_gfx, rs->render_passes[0]);
    game_state->sky_pipeline = gfx::vul::create_skybox_pipeline(&game_state->mesh_arena, &vk_gfx, rs->render_targets[0].render_pass);
    // game_state->mesh_pipeline = gfx::vul::create_mesh_pipeline(&game_state->mesh_arena, &vk_gfx, rs->render_targets[0].render_pass,
    //     gfx::vul::mesh_pipeline_info_t{
    //         vk_gfx.sporadic_uniform_buffer.buffer,
    //         game_state->render_system->job_storage_buffer().buffer,
    //         game_state->render_system->material_storage_buffer.buffer,
    //         game_state->render_system->environment_storage_buffer.buffer
    //     }
    // );
    // assert(game_state->mesh_pipeline);

    game_state->debug_pipeline = gfx::vul::create_debug_pipeline(
        &game_state->mesh_arena, 
        &vk_gfx, 
        rs->render_targets[0].render_pass
    );

    gfx::font_load(&game_state->texture_arena, &game_state->default_font, "./res/fonts/Go-Mono-Bold.ttf", 18.0f);
    gfx::font_load(&game_state->texture_arena, &game_state->large_font, "./res/fonts/Go-Mono-Bold.ttf", 24.0f);
    game_state->default_font_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&game_state->texture_arena, 1);
    {
        temp_arena_t ta = game_state->texture_arena;
        vk_gfx.load_font_sampler(
            &ta,
            game_state->default_font_texture,
            &game_state->default_font);
    }

    gfx::vul::texture_2d_t* ui_textures[4096];
    for(size_t i = 0; i < array_count(ui_textures); i++) { ui_textures[i] = game_state->default_font_texture; }
    ui_textures[1] = &rs->frame_images[0].texture;

    game_state->default_font_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        game_state->gui_pipeline->descriptor_set_layouts[0],
        ui_textures, array_count(ui_textures));

    { //@test loading shader objects
        auto& mesh_pass = rs->frames[0].mesh_pass;
        auto& anim_pass = rs->frames[0].anim_pass;

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
            game_state->gui_pipeline->descriptor_set_layouts,
            game_state->gui_pipeline->descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::gui_frag,
            game_state->gui_pipeline->descriptor_set_layouts,
            game_state->gui_pipeline->descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::skybox_vert,
            game_state->sky_pipeline->descriptor_set_layouts,
            game_state->sky_pipeline->descriptor_count
        );

        rs->shader_cache.load(
            rs->arena, 
            vk_gfx,
            assets::shaders::skybox_frag,
            game_state->sky_pipeline->descriptor_set_layouts,
            game_state->sky_pipeline->descriptor_count
        );
    }
  

    game_state->gui.ctx.font = &game_state->default_font;
    game_state->gui.ctx.input = &game_memory->input;
    game_state->gui.ctx.screen_size = v2f{
        (f32)game_memory->config.window_size[0], 
        (f32)game_memory->config.window_size[1]
    };

    for (size_t i = 0; i < array_count(game_state->gui.ctx.dyn_font); i++) {
        game_state->gui.ctx.dyn_font[i] = arena_alloc<gfx::font_t>(&game_state->texture_arena);
        gfx::font_load(&game_state->texture_arena, game_state->gui.ctx.dyn_font[i], "./res/fonts/Go-Mono-Bold.ttf", (f32)i+1);
    }
}

export_fn(void) 
app_on_init(game_memory_t* game_memory) {
    // utl::profile_t p{"game_state init"};
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

    using namespace std::string_view_literals;
#ifdef GEN_INTERNAL
    game_state->debug.console = arena_alloc<debug_console_t>(main_arena);

    console_log(game_state->debug.console, "Hello world");
    console_log(game_state->debug.console, "Hello world");
    console_log(game_state->debug.console, "Hello world");
    console_log(game_state->debug.console, "Hello world");
    console_log(game_state->debug.console, "does this work?", gfx::color::rgba::yellow, [](void* app_){
        game_state_t* game_state = (game_state_t*)app_;
        console_log(game_state->debug.console, "You tell me");
    }, game_state);

#endif

    game_state->resource_file = utl::res::load_pack_file(&game_state->mesh_arena, &game_state->string_arena, "./res/res.pack");

    
    physics::api_t* physics = game_memory->physics;
    physics->entity_transform_offset = offsetof(game::entity_t, transform);
    if (physics) {
        gen_info("app_init", "Creating physics scene");
        assert(physics && physics->create_scene);
        physics->create_scene(physics, 0);
        gen_info("app_init", "Created physics scene");
    }
    game_state->game_world = game::world_init(&game_state->game_arena, game_state, physics);

    app_init_graphics(game_memory);

    {
        auto& rs = game_state->render_system;
        auto& vertices = rs->vertices.pool;
        auto& indices = rs->indices.pool;
        prcgen::mesh_builder_t builder{vertices, indices};

        builder.add_box({v3f{-1.0f}, v3f{1.0f}});
        rendering::add_mesh(rs, "unit_cube", builder.build(&game_state->mesh_arena));
    }

    using namespace gfx::color;

    game_state->scene.sporadic_buffer.time = game_state->input().time;
    game_state->scene.sporadic_buffer.mode = 1;
    game_state->scene.sporadic_buffer.use_lighting = 1;

    world_0(game_state->game_world);

    gen_info("game_state", "world size: {}mb", GEN_TYPE_INFO(game::world_t).size/megabytes(1));
}

export_fn(void) 
app_on_deinit(game_memory_t* game_memory) {
    game_state_t* game_state = get_game_state(game_memory);

    gfx::vul::state_t& vk_gfx = game_state->gfx;
    vkDeviceWaitIdle(vk_gfx.device);

    rendering::cleanup(game_state->render_system);
    gen_info(__FUNCTION__, "Render System cleaned up");

    vk_gfx.cleanup();
    gen_info(__FUNCTION__, "Graphics API cleaned up");

    game_state->~game_state_t();
}

export_fn(void) 
app_on_unload(game_memory_t* game_memory) {
    game_state_t* game_state = get_game_state(game_memory);
    
    game_state->render_system->ticket.lock();
}

global_variable f32 gs_reload_time = 0.0f;
export_fn(void)
app_on_reload(game_memory_t* game_memory) {
    Platform = game_memory->platform;
    game_state_t* game_state = get_game_state(game_memory);
    gs_reload_time = 1.0f;
    game_state->render_system->ticket.unlock();
}

void 
camera_input(game_state_t* game_state, player_controller_t pc, f32 dt) {
    auto* world = game_state->game_world;
    auto* physics = game_state->game_world->physics;
    auto* player = world->player;
    auto* rigidbody = player->physics.rigidbody;
    const bool is_on_ground = rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
    const bool is_on_wall = rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;

    player->camera_controller.transform = 
        game_state->game_world->player->transform;
    
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

        temp_arena_t fire_arena = world->frame_arena.get();
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
app_on_input(game_state_t* game_state, app_input_t* input) {
    camera_input(game_state, 
        input->gamepads->is_connected ? 
        gamepad_controller(input) : keyboard_controller(input),
        input->dt
    );

    // if (input->pressed.keys['P']) {
    //     const game::entity_t player = *game_state->game_world->player;
    //     std::memcpy(game_state->game_world, &gs_saved_world, sizeof(game::world_t));
    //     *game_state->game_world->player = player;
    //     range_u64(i, 0, game_state->game_world->entity_count) {
    //         auto* e = game_state->game_world->entities + i;
    //         if (e->physics.flags == game::PhysicsEntityFlags_Dynamic &&
    //             e->physics.rigidbody
    //         ) {
    //             e->physics.rigidbody->velocity = v3f{0.0f};
    //             e->physics.rigidbody->angular_velocity = v3f{0.0f};
    //         }
    //     }
    // }

    if (input->gamepads[0].buttons[button_id::dpad_left].is_released) {
        game_state->time_scale *= 0.5f;
        game_state->time_text_anim = 1.0f;
    }
    if (input->gamepads[0].buttons[button_id::dpad_right].is_released) {
        game_state->time_scale *= 2.0f;
        game_state->time_text_anim = 1.0f;
    }
}

void game_on_gameplay(game_state_t* game_state, app_input_t* input) {
    TIMED_FUNCTION;

    input->dt *= game_state->time_scale;
    app_on_input(game_state, input);

    
    // TODO(ZACK): GAME CODE HERE

    if (game_state->render_system == nullptr) return;

    auto* world = game_state->game_world;
    game::world_update(world, input->dt);

    if (world->player->global_transform().origin.y < -1000.0f) {
        game_state->game_memory->input.pressed.keys[key_id::F10] = 1;
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

    world->player->physics.rigidbody->integrate(input->dt, 9.81f * 0.16f);
    game::world_update_physics(game_state->game_world);

    {
        TIMED_BLOCK(PhysicsStep);
        if (accum >= step) {
            accum -= step;
            world->physics->simulate(world->physics, step);
        }
    }
        
    {
        TIMED_BLOCK(GameplayPostSimulate);

        for (size_t i{0}; i < game_state->game_world->entity_capacity; i++) {
            auto* e = game_state->game_world->entities + i;
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

            e->coroutine->run(world->frame_arena);

            if (is_pickupable) {
                e->transform.rotate(axis::up, input->dt);
                if (math::intersect(math::sphere_t{e->global_transform().origin, 2.0f}, game_state->game_world->player->global_transform().origin)) {
                    game_state->game_world->player->primary_weapon.entity = e;
                    e->parent = game_state->game_world->player;
                    e->transform.origin = v3f{0.5, 0.0f, -0.5f};
                    e->flags &= ~game::EntityFlags_Pickupable;
                    // volatile int* crash{0}; *crash++;
                }
            }

            // game_state->debug.draw_aabb(e->global_transform().xform_aabb(e->aabb), gfx::color::v3::yellow);
        }
    }

    game::world_kill_free_queue(game_state->game_world);

    // std::unique_lock<std::mutex> lock{game_state->render_system->ticket};
    std::lock_guard lock{game_state->render_system->ticket};
    {
        // TIMED_BLOCK(GameplayLock);
        // lock.lock();
    }

    TIMED_BLOCK(GameSubmitRenderJobs);
    rendering::begin_frame(game_state->render_system);
    // game_state->debug.debug_vertices.pool.clear();

    game_state->game_world->player->camera_controller.transform = game_state->game_world->player->transform;
    game_state->game_world->player->camera_controller.translate(axis::up);

    for (size_t i{0}; i < game_state->game_world->entity_capacity; i++) {
        auto* e = game_state->game_world->entities + i;
        const bool is_not_renderable = !e->is_renderable();

        if (e->is_alive() == false) {
            continue;
        }

        if (is_not_renderable) {
            // gen_warn("render", "Skipping: {} - {}", (void*)e, e->flags);
            continue;
        }

        if (e->gfx.particle_system) {
            particle_system_update(e->gfx.particle_system, game_state->input().dt);
            particle_system_build_matrices(e->gfx.particle_system, e->gfx.dynamic_instance_buffer, e->gfx.instance_count);
        }

        rendering::submit_job(
            game_state->render_system, 
            e->gfx.mesh_id, 
            e->gfx.material_id, 
            e->global_transform().to_matrix(),
            e->gfx.instance_count,
            e->gfx.instance_offset()
        );
    }

    // game_state->render_system->ticket.unlock();
}

void
game_on_update(game_memory_t* game_memory) {
    // utl::profile_t p{"on_update"};
    game_state_t* game_state = get_game_state(game_memory);
    gs_dt = game_memory->input.dt;
    game_on_gameplay(game_state, &game_memory->input);
}

void 
draw_game_gui(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    game_state_t* game_state = get_game_state(game_memory);
    auto* world = game_state->game_world;
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
draw_gui(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    game_state_t* game_state = get_game_state(game_memory);

    local_persist u64 frame{0}; frame++;
    auto string_mark = arena_get_mark(&game_state->string_arena);

    arena_t* display_arenas[] = {
        &game_state->main_arena,
        &game_state->temp_arena,
        &game_state->string_arena,
        &game_state->mesh_arena,
        &game_state->texture_arena,
        &game_state->game_arena,
        &game_state->game_world->arena,
        &game_state->game_world->frame_arena.arena[0],
        &game_state->game_world->frame_arena.arena[1],
        // &game_state->physics->default_allocator.arena,
        // &game_state->physics->default_allocator.heap_arena,
        &game_state->render_system->arena,
        &game_state->render_system->frame_arena,
        &game_state->render_system->vertices.pool,
        &game_state->render_system->indices.pool,
        &game_state->gui.vertices[!(frame&1)].pool,
        &game_state->gui.indices[!(frame&1)].pool,
    };

    const char* display_arena_names[] = {
        "- Main Arena",
        "- Temp Arena",
        "- String Arena",
        "- Mesh Arena",
        "- Texture Arena",
        "- Game Arena",
        "- World Arena",
        "- World Frame 0",
        "- World Frame 1",
        // "- Physics Arena",
        // "- Physics Heap",
        "- Rendering Arena",
        "- Rendering Frame Arena",
        "- 3D Vertex",
        "- 3D Index",
        "- 2D Vertex",
        "- 2D Index",
    };

    // std::lock_guard lock{game_state->render_system->ticket};
    gfx::gui::ctx_clear(&game_state->gui.ctx, &game_state->gui.vertices[frame&1].pool, &game_state->gui.indices[frame&1].pool);
    
    const auto dt = game_memory->input.render_dt;

    game_state->time_text_anim -= dt;
    if ((gs_reload_time -= dt) > 0.0f) {
        gfx::gui::string_render(
            &game_state->gui.ctx, 
            string_t{}.view("Code Reloaded"),
            game_state->gui.ctx.screen_size/v2f{2.0f,4.0f} - gfx::font_get_size(&game_state->default_font, "Code Reloaded")/2.0f, 
            gfx::color::to_color32(v4f{0.80f, .9f, .70f, gs_reload_time}),
            &game_state->default_font
        );
    }
    if (game_state->time_text_anim > 0.0f) {
        gfx::gui::string_render(
            &game_state->gui.ctx, 
            string_t{}.view(fmt_str("Time Scale: {}", game_state->time_scale).c_str()),
            game_state->gui.ctx.screen_size/2.0f, gfx::color::to_color32(v4f{0.80f, .9f, 1.0f, game_state->time_text_anim})
        );
    }

    local_persist gfx::gui::im::state_t state{
        .ctx = game_state->gui.ctx,
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
            game_state->render_system->vp;

        // draw_console(state, game_state->debug.console, v2f{0.0, 800.0f});
        

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
                dt_accum += game_memory->input.dt;
                dt_count += 1.0f;

                local_persist f32 rdt_accum{0};
                local_persist f32 rdt_count{0};
                if (rdt_count > 1000.0f) {
                    rdt_count=rdt_accum=0.0f;
                }
                rdt_accum += game_memory->input.render_dt;
                rdt_count += 1.0f;
                im::text(state, fmt_sv("Gameplay FPS: {:.2f} - {:.2f} ms", 1.0f / (dt_accum/dt_count), (dt_accum/dt_count) * 1000.0f));
                im::text(state, fmt_sv("Graphics FPS: {:.2f} - {:.2f} ms", 1.0f / (rdt_accum/rdt_count), (rdt_accum/rdt_count) * 1000.0f));
            }

#if GEN_INTERNAL
            local_persist bool show_record[128];
            if (im::text(state, "Profiling"sv, &show_perf)) {
                debug_table_t* tables[] {
                    &gs_debug_table, game_memory->physics->get_debug_table()
                };
                size_t table_sizes[]{
                    gs_main_debug_record_size, game_memory->physics->get_debug_table_size()
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

                object_gui(state, game_state->render_system->stats);
                
                {
                    const auto [x,y] = game_memory->input.mouse.pos;
                    im::text(state, fmt_sv("- Mouse: [{:.2f}, {:.2f}]", x, y));
                }
                if (im::text(state, "- GFX Config"sv, &show_gfx_cfg)) {
                    auto& config = game_memory->config.graphics_config;

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
                    im::text(state, fmt_sv("--- Width: {}", game_memory->config.window_size[0]));
                    im::text(state, fmt_sv("--- Height: {}", game_memory->config.window_size[1]));
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

                if (im::text(state, fmt_sv("- Light Mode: {}", game_state->scene.sporadic_buffer.use_lighting))) {
                    game_state->scene.sporadic_buffer.use_lighting = !game_state->scene.sporadic_buffer.use_lighting;
                }
                if (im::text(state, fmt_sv("- Mode: {}", game_state->scene.sporadic_buffer.mode))) {
                    game_state->scene.sporadic_buffer.mode = ++game_state->scene.sporadic_buffer.mode % 4;
                }
                if (im::text(state, fmt_sv("- Resource File Count: {}", game_state->resource_file->file_count), &show_resources)) {
                    local_persist u64 file_start = 0;
                    im::same_line(state);
                    if (im::text(state, "Next")) file_start = (file_start+1) % game_state->resource_file->file_count;
                    if (im::text(state, "Prev")) file_start = file_start?file_start-1:game_state->resource_file->file_count-1;
                    range_u64(rf_i, 0, 10) {
                        u64 rf_ = file_start + rf_i;
                        u64 rf = rf_ % game_state->resource_file->file_count;
                        assert(rf < array_count(show_files));
                        if (im::text(state, fmt_sv("--- File Name: {}", game_state->resource_file->table[rf].name.c_data), show_files + rf)) {
                            im::text(state, fmt_sv("----- Size: {} bytes", game_state->resource_file->table[rf].size));
                            im::text(state, fmt_sv("----- Type: {:X}", game_state->resource_file->table[rf].file_type));
                        }
                    }
                }
            }

            if (im::text(state, "Debug"sv, &game_state->debug.show)) {
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
                if (im::text(state, "- Reload Shaders"sv)) {
                    std::system("compile_shaders");
                    game_state->render_system->shader_cache.reload_all(
                        game_state->render_system->arena,
                        game_state->gfx
                    );
                }

                if (im::text(state, "- Texture Cache"sv, &show_textures)) {
                    auto& cache = game_state->render_system->texture_cache;

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
                    auto& probes = game_state->render_system->light_probes;
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
                    auto& env = game_state->render_system->environment_storage_buffer.pool[0];
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
                    loop_iota_u64(i, game_state->render_system->materials.size()) {
                        auto* mat = game_state->render_system->materials[i];
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

                            game_state->render_system->material_storage_buffer.pool[i] = *mat;

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
                        im::float_slider(state, &game_state->scene.lighting.directional_light.x, -1.0f, 1.0f);
                        im::float_slider(state, &game_state->scene.lighting.directional_light.y, -1.0f, 1.0f);
                        im::float_slider(state, &game_state->scene.lighting.directional_light.z, -1.0f, 1.0f);
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
                const u64 entity_count = game_state->game_world->entity_count;

                im::text(state, fmt_sv("- Count: {}", entity_count));

                if (im::text(state, "- Kill All"sv)) {
                    range_u64(ei, 1, game_state->game_world->entity_capacity) {
                        if (game_state->game_world->entities[ei].is_alive()) {
                            game_state->game_world->entities[ei].queue_free();
                        }
                    }

                    // game_state->game_world->entities.clear();
                    // game_state->game_world->entity_count = 1;
                }
            }

            if (im::text(state, "Player"sv, &show_player)) {
                auto* player = game_state->game_world->player;
                const bool on_ground = game_state->game_world->player->physics.rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
                const bool on_wall = game_state->game_world->player->physics.rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;
                im::text(state, fmt_sv("- On Ground: {}", on_ground?"true":"false"));
                im::text(state, fmt_sv("- On Wall: {}", on_wall?"true":"false"));
                const auto v = player->physics.rigidbody->velocity;
                const auto p = player->global_transform().origin;
                im::text(state, fmt_sv("- Position: {}", p));
                im::text(state, fmt_sv("- Velocity: {}", v));
            }

            im::end_panel(state);
        }

        // for (game::entity_t* e = game_itr(game_state->game_world); e; e = e->next) {
        for (size_t i = 0; i < game_state->game_world->entity_capacity; i++) {
            auto* e = game_state->game_world->entities + i;
            if (e->is_alive() == false) { continue; }

            const v3f ndc = math::world_to_screen(vp, e->global_transform().origin);

            bool is_selected = widget_pos == &e->transform.origin;
            bool not_player = e != game_state->game_world->player;
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
                    // game::world_destroy_entity(game_state->game_world, e);
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
                static_cast<u32>(utl::rng::fnv_hash_u64(e->id)), 2.0f)) && state.ctx.input->pressed.mouse_btns[0]
            ) {
                widget_pos = &e->transform.origin;
            }

            const auto panel = im::get_last_panel(state);
            if (is_selected && opened && im::draw_circle(state, panel.max, 8.0f, gfx::color::rgba::red, 4)) {
                widget_pos = &default_widget_pos;
            }
        }

        im::gizmo(state, widget_pos, vp);

        draw_game_gui(game_memory);

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

    arena_set_mark(&game_state->string_arena, string_mark);
}

inline static u32
wait_for_frame(game_state_t* game_state, u64 frame_count) {
    TIMED_FUNCTION;
    gfx::vul::state_t& vk_gfx = game_state->gfx;
    vkWaitForFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[frame_count&1], VK_TRUE, UINT64_MAX);
    vkResetFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence[frame_count&1]);
    u32 imageIndex;
    vkAcquireNextImageKHR(vk_gfx.device, vk_gfx.swap_chain, UINT64_MAX, 
        vk_gfx.image_available_semaphore[frame_count&1], VK_NULL_HANDLE, &imageIndex);
    return imageIndex;
}

inline static void
present_frame(game_state_t* game_state, VkCommandBuffer command_buffer, u32 imageIndex, u64 frame_count) {
    gfx::vul::state_t& vk_gfx = game_state->gfx;
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
game_on_render(game_memory_t* game_memory, u32 imageIndex, u32 frame_count) { 
    TIMED_FUNCTION;
    
    game_state_t* game_state = get_game_state(game_memory);

    gfx::vul::state_t& vk_gfx = game_state->gfx;


    game_state->render_system->camera_pos = game_state->game_world->camera.origin;
    game_state->render_system->set_view(game_state->game_world->camera.inverse().to_matrix(), game_state->width(), game_state->height());

    game_state->scene.sporadic_buffer.time = game_state->input().time;
    *vk_gfx.sporadic_uniform_buffer.data = game_state->scene.sporadic_buffer;
    auto& command_buffer = vk_gfx.command_buffer[frame_count&1];
    vkResetCommandBuffer(command_buffer, 0);

    {
        auto& rs = game_state->render_system;
        auto& khr = vk_gfx.khr;
        auto& ext = vk_gfx.ext;

        auto command_buffer_begin_info = gfx::vul::utl::command_buffer_begin_info();
        VK_OK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));
    
        VkBuffer buffers[1] = { game_state->render_system->vertices.buffer };
        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);

        vkCmdBindIndexBuffer(command_buffer,
            game_state->render_system->indices.buffer, 0, VK_INDEX_TYPE_UINT32
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

            rendering::draw_skybox(
                rs, command_buffer, 
                assets::shaders::skybox_frag.filename,
                game_state->sky_pipeline->pipeline_layout,
                game::cam::get_direction(
                    game_state->game_world->player->camera_controller.yaw,
                    game_state->game_world->player->camera_controller.pitch
                ), game_state->scene.lighting.directional_light
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
                VK_PIPELINE_BIND_POINT_GRAPHICS, game_state->gui_pipeline->pipeline_layout,
                0, 1, &game_state->default_font_descriptor, 0, nullptr);

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
                0, 1, &game_state->gui.vertices[frame_count&1].buffer, offsets);

            vkCmdBindIndexBuffer(command_buffer,
                game_state->gui.indices[frame_count&1].buffer, 0, VK_INDEX_TYPE_UINT32);

            VkShaderEXT gui_shaders[2] {
                *rs->shader_cache.get("./res/shaders/bin/gui.vert.spv"),
                *rs->shader_cache.get("./res/shaders/bin/gui.frag.spv"),
            };
            assert(gui_shaders[0] != VK_NULL_HANDLE);
            assert(gui_shaders[1] != VK_NULL_HANDLE);

            VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
            ext.vkCmdBindShadersEXT(command_buffer, 2, stages, gui_shaders);

            vkCmdDrawIndexed(command_buffer,
                (u32)game_state->gui.ctx.indices->count,
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
    auto* game_state = get_game_state(game_memory);
    local_persist u32 frame_count = 0;
    // std::lock_guard lock{game_state->render_system->ticket};
    switch (scene_state) {
        case 0:{
    
            std::lock_guard lock{game_state->render_system->ticket};
            entity_editor_render(get_entity_editor(game_memory));
            u32 image_index = wait_for_frame(game_state, ++frame_count);
            game_on_render(game_memory, image_index, frame_count);
        }   break;
        case 1:{
            // Game           
    
            u32 image_index = wait_for_frame(game_state, ++frame_count);
            std::lock_guard lock{game_state->render_system->ticket};
            draw_gui(game_memory);
            game_on_render(game_memory, image_index, frame_count);
        
        }   break;
        default:
            gen_warn("scene::render", "Unknown scene: {}", scene_state);
            break;
        //     scene_state = 1;
    }
}

export_fn(void) 
app_on_update(game_memory_t* game_memory) {
    if (game_memory->input.pressed.keys[key_id::F3] || 
        game_memory->input.gamepads->buttons[button_id::dpad_down].is_pressed) {
        scene_state = !scene_state;
    }
    switch (scene_state) {
        case 0: // Editor
            entity_editor_update(get_entity_editor(game_memory));
            break;
        case 1: // Game
            game_on_update(game_memory);
            break;
        default:
            gen_warn("scene::update", "Unknown scene: {}", scene_state);
            break;
        //     scene_state = 1;
    }
}
