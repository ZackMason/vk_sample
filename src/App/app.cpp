#include <core.hpp>

#include "App/vk_state.hpp"

#include "App/Game/Util/camera.hpp"

#include "App/Game/World/world.hpp"

#include <thread>   

#include "SDL_mixer.h"

struct mesh_cache_t {
    struct link_t : node_t<link_t> {
        utl::load_results_t mesh;
    };

    utl::deque<link_t> meshes;

    u64 add(arena_t* arena, const utl::load_results_t& r) {
        link_t* link = arena_alloc_ctor<link_t>(arena, 1);
        link->mesh = r;
        meshes.push_back(link);
        return meshes.size() - 1;
    }

    const utl::load_results_t& get(u64 id) const {
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
    
    gfx::vul::state_t   gfx;

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
    } scene;
    

    gfx::vul::pipeline_state_t* gui_pipeline{0};


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
    } debug;
        

    game::cam::camera_t camera;
    game::cam::first_person_controller_t first_person{&camera};

    game::world_t* game_world;
};

inline utl::load_results_t
load_bin_mesh_data(
    arena_t* arena,
    u8* data,
    utl::pool_t<gfx::vertex_t>* vertices,
    utl::pool_t<u32>* indices
) {
    utl::load_results_t results;

    utl::deserializer_t mouth{data};

    const auto meta = mouth.deserialize_u64();
    const auto vers = mouth.deserialize_u64();
    const auto mesh = mouth.deserialize_u64();

#if !NDEBUG
    assert(meta == utl::res::magic::meta);
    assert(vers == utl::res::magic::vers);
    assert(mesh == utl::res::magic::mesh);
#endif

    results.count = mouth.deserialize_u64();
    gen_warn("app::load_bin_mesh_data", "Loading {} meshes", results.count);
    results.meshes = arena_alloc_ctor<gfx::mesh_view_t>(arena, results.count);

    for (size_t i = 0; i < results.count; i++) {
        std::string name = mouth.deserialize_string();
        gen_info("app::load_bin_mesh_data", "Mesh name: {}", name);
        const size_t vertex_count = mouth.deserialize_u64();
        const size_t vertex_bytes = sizeof(gfx::vertex_t) * vertex_count;

        const u32 vertex_start = safe_truncate_u64(vertices->count);
        const u32 index_start = safe_truncate_u64(indices->count);

        results.meshes[i].vertex_count = safe_truncate_u64(vertex_count);
        results.meshes[i].vertex_start = vertex_start;
        results.meshes[i].index_start = index_start;

        gfx::vertex_t* v = vertices->allocate(vertex_count);

        std::memcpy(v, mouth.data(), vertex_bytes);
        mouth.advance(vertex_bytes);

        const size_t index_count = mouth.deserialize_u64();
        const size_t index_bytes = sizeof(u32) * index_count;
        results.meshes[i].index_count =  safe_truncate_u64(index_count);

        u32* tris = indices->allocate(index_count);
        std::memcpy(tris, mouth.data(), index_bytes);
        mouth.advance(index_bytes);
    }

    return results;
}

inline utl::load_results_t
load_bin_mesh(
    arena_t* arena,
    utl::res::pack_file_t* packed_file,
    std::string_view path,
    utl::pool_t<gfx::vertex_t>* vertices,
    utl::pool_t<u32>* indices
) {
    const size_t file_index = utl::res::pack_file_find_file(packed_file, path);
    
    u8* file_data = utl::res::pack_file_get_file(packed_file, file_index);

    utl::load_results_t results;
    
    if (file_data) {
        return load_bin_mesh_data(arena, file_data, vertices, indices);
    } else {
        gen_warn("app::load_bin_mesh", "Failed to open mesh file");
    }

    return results;
}

app_t*
get_app(app_memory_t* mem) {
    return (app_t*)mem->perm_memory;
}

export_fn(void) 
app_on_init(app_memory_t* app_mem) {
    // s_app_mem = app_mem;

    app_t* app = get_app(app_mem);
    new (app) app_t;


    app->app_mem = app_mem;
    app->main_arena.start = (u8*)app_mem->perm_memory + sizeof(app_t);
    app->main_arena.size = app_mem->perm_memory_size - sizeof(app_t);

    arena_t* main_arena = &app->main_arena;

    app->temp_arena = arena_sub_arena(&app->main_arena, megabytes(4));
    app->string_arena = arena_sub_arena(&app->main_arena, megabytes(16));
    app->mesh_arena = arena_sub_arena(&app->main_arena, megabytes(512));
    app->texture_arena = arena_sub_arena(&app->main_arena, megabytes(512));
    app->gui.arena = arena_sub_arena(main_arena, megabytes(8));

    app->resource_file = utl::res::load_pack_file(&app->mesh_arena, &app->string_arena, "./assets/res.pack");
    gen_info("app", "Pack file test: {}", utl::res::pack_file_find_file(
        app->resource_file, "assets\\models\\utah-teapot.obj"
    ));
    gen_info("app", "Pack file test: {}", utl::res::pack_file_find_file(
        app->resource_file, "assets\\models\\lucy.obj"
    ));


    app->game_world = game::world_init(main_arena, &app->camera);
    
    // setup graphics

    gfx::vul::state_t& vk_gfx = app->gfx;
    app->gfx.init(&app_mem->config);
    app->gui_pipeline = gfx::vul::create_gui_pipeline(&app->mesh_arena, &vk_gfx);

    gfx::font_load(&app->texture_arena, &app->default_font, "./assets/fonts/Go-Mono-Bold.ttf", 15.0f);
    app->default_font_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&app->texture_arena, 1);
    vk_gfx.load_font_sampler(
        &app->texture_arena,
        app->default_font_texture,
        &app->default_font);


    gfx::vul::texture_2d_t* ui_textures[32];
    for(size_t i = 0; i < array_count(ui_textures); i++) { ui_textures[i] = app->default_font_texture; }
    
    app->default_font_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        app->gui_pipeline->descriptor_set_layouts[0],
        ui_textures, 32);

    app->brick_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&app->texture_arena, 1);
    vk_gfx.load_texture_sampler(app->brick_texture, "./assets/textures/brick_01.png", &app->texture_arena);
    app->brick_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        vk_gfx.descriptor_set_layouts[3],
        &app->brick_texture, 1);

    vk_gfx.create_vertex_buffer(
        &app->vertices
    );
    vk_gfx.create_index_buffer(
        &app->indices
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

    for (size_t i = 0; i < app->resource_file->file_count; i++) {
        u8* file_data = utl::res::pack_file_get_file(app->resource_file, i);

        auto loaded_mesh = load_bin_mesh_data(
            &app->mesh_arena,
            file_data, //app->resource_file->resources[i].data,
            &app->vertices.pool,
            &app->indices.pool
        );
        app->mesh_cache.add(
            &app->mesh_arena,
            loaded_mesh
        );
    }

    // auto loaded_mesh = load_bin_mesh(
    //     &app->mesh_arena, 
    //     app->resource_file,
    //     "assets\\models\\utah-teapot.obj",
    //     &app->vertices.pool,
    //     &app->indices.pool
    // );

    const auto mesh_id = 0;
    // app->mesh_cache.add(
    //     &app->mesh_arena,
    //     loaded_mesh
    // );

    game::entity::entity_t* e2 = game::world_create_entity(app->game_world);
    e2->gfx.mesh_id = mesh_id;


    arena_clear(&app->temp_arena);


    app->camera.origin = v3f{0,20,-25};
    app->camera.look_at(v3f{0.0f});
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

export_fn(void) 
app_on_update2(app_memory_t* app_mem) {

}

void 
camera_input(app_t* app, app_input_t* input) {
    const v2f move = v2f{
        f32(input->keys['W']) - f32(input->keys['S']),
        f32(input->keys['D']) - f32(input->keys['A'])
    };

    const v3f forward = app->first_person.transform.basis[2] * input->dt;
    const v3f right = app->first_person.transform.basis[0] * input->dt;

    constexpr f32 camera_rot_speed{2000.0f};

    if (input->mouse.buttons[1]) {
        const v2f mouse_move = v2f{
            input->mouse.delta[0],
            input->mouse.delta[1]
        } / app->gui.ctx.screen_size;

        app->first_person.yaw += mouse_move.x * input->dt*camera_rot_speed;
        app->first_person.pitch -= mouse_move.y * input->dt*camera_rot_speed;
    }

    app->first_person.translate((forward * move.y + right * move.x)*10.0f);
}

void 
app_on_input(app_t* app, app_input_t* input) {
    camera_input(app, input);

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

export_fn(void) 
app_on_update(app_memory_t* app_mem) {
    app_t* app = get_app(app_mem);

    app_on_input(app, &app_mem->input);

    gfx::vul::state_t& vk_gfx = app->gfx;

    local_persist u32 frame_count = 0;
    if (frame_count++ < 3) {
        gen_info("app::on_update", "frame: {}", frame_count);
    }

    const f32 anim_time = glm::mod(
        app_mem->input.time, 10.0f
    );

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

    app->gui.main_panel->text_widgets[0].text.own(&app->string_arena, fmt_str(" {:.2f} - {:.2f} ms", 1.0f / app_mem->input.dt, app_mem->input.dt).c_str());
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

    gfx::gui::ctx_clear(
        &app->gui.ctx
    );

    gfx::gui::panel_render(
        &app->gui.ctx,
        app->gui.main_panel
    );

    arena_set_mark(&app->string_arena, string_mark);

    // update uniforms

    app->scene.sporadic_buffer.num_instances = 1;

    const f32 aspect = (f32)app_mem->config.window_size[0] / (f32)app_mem->config.window_size[1];

    app->scene.scene_buffer.time = app_mem->input.time;
    app->scene.scene_buffer.view = app->camera.to_matrix();
    app->scene.scene_buffer.proj = glm::perspective(45.0f, aspect, 0.3f, 1000.0f);
    app->scene.scene_buffer.proj[1][1] *= -1.0f;

    app->scene.scene_buffer.scene = glm::mat4{0.5f};
    app->scene.scene_buffer.light_pos = v4f{3,13,3,0};
    app->scene.scene_buffer.light_col = v4f{0.2f, 0.1f, 0.8f, 1.0f};
    app->scene.scene_buffer.light_KaKdKs = v4f{0.5f};

    app->scene.object_buffer.model = m44{1.0f};
    app->scene.object_buffer.color = v4f{1.0f};
    app->scene.object_buffer.shininess = 2.0f;

    *vk_gfx.scene_uniform_buffer.data = app->scene.scene_buffer;
    *vk_gfx.sporadic_uniform_buffer.data = app->scene.sporadic_buffer;
    *vk_gfx.object_uniform_buffer.data = app->scene.object_buffer;

    // render
        
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

    vk_gfx.record_command_buffer(vk_gfx.command_buffer, imageIndex, [&]{

        vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, vk_gfx.pipeline_layout,
            0, 3, vk_gfx.descriptor_sets, 0, nullptr);

        vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, vk_gfx.pipeline_layout,
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

        for (size_t pool = 0; pool < game::pool_count(); pool++)  {

            for (auto* e = game::pool_start(app->game_world, pool); e; e = e->next) {
                // const auto& meshes = app->mesh_cache.get(e->gfx.mesh_id);
                const auto& meshes = app->mesh_cache.get(app->debug.mesh_index%app->mesh_cache.meshes.size());

                gfx::vul::object_push_constants_t push_constants;
                push_constants.model = e->global_transform().to_matrix();
                push_constants.material.albedo = gfx::color::v4::green;

                // push constants cannot be bigger
                static_assert(sizeof(push_constants) <= 256);
                vkCmdPushConstants(vk_gfx.command_buffer, vk_gfx.pipeline_layout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(gfx::vul::object_push_constants_t), &push_constants
                );
            
                for (size_t j = 0; j < meshes.count; j++) {
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
            }
        }
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
    }, false);
    
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