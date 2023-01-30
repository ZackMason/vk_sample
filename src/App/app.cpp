#include <core.hpp>

#include "App/vk_state.hpp"
        

#include <thread>   

struct mesh_c : gfx::mesh_view_t, entity::component_t<mesh_c>, node_t<mesh_c> {
    using gfx::mesh_view_t::mesh_view_t;
    
    mesh_c(gfx::mesh_view_t mv)
        : gfx::mesh_view_t{mv}
    {}
};

using mesh_comp_deque = utl::deque<mesh_c>;

struct app_t {
    app_memory_t*       app_mem{nullptr};

    // arenas
    arena_t             main_arena;
    arena_t             temp_arena;
    arena_t             string_arena;
    arena_t             mesh_arena;
    arena_t             texture_arena;
    
    gfx::vul::state_t   gfx;

    gfx::font_t default_font;
    gfx::vul::texture_2d_t* default_font_texture{nullptr};
    VkDescriptorSet default_font_descriptor;

    gfx::vul::texture_2d_t* brick_texture{nullptr};
    VkDescriptorSet brick_descriptor;

    inline static constexpr size_t max_scene_vertex_count{1'000'000};
    inline static constexpr size_t max_scene_index_count{3'000'000};
    gfx::vul::vertex_buffer_t<gfx::vertex_t, max_scene_vertex_count> vertices;
    gfx::vul::index_buffer_t<max_scene_index_count> indices;

    struct scene_t {
        gfx::vul::scene_buffer_t scene_buffer;
        gfx::vul::object_buffer_t object_buffer;
        gfx::vul::sporadic_buffer_t sporadic_buffer;
    } scene;
    
    entity::component_t<v3f>::pool_t* position_components;
    entity::component_t<string_t>::pool_t* name_components;
    mesh_c::pool_t* mesh_components;    
    entity::world_t* world;

    u32 v_count;
    u32 i_count;

    struct gui_state_t {
        gfx::gui::ctx_t ctx;
        gfx::vul::vertex_buffer_t<gfx::gui::vertex_t, 40'000> vertices;
        gfx::vul::index_buffer_t<60'000> indices;

        arena_t  arena;
        gfx::gui::panel_t* main_panel{nullptr};
    } gui;

};

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

    app->world = entity::world_init(main_arena, megabytes(64));
    
    // setup graphics

    gfx::vul::state_t& vk_gfx = app->gfx;
    app->gfx.init(&app_mem->config);

    gfx::font_load(&app->texture_arena, &app->default_font, "./assets/fonts/Go-Mono-Bold.ttf", 18.0f);
    app->default_font_texture = arena_alloc_ctor<gfx::vul::texture_2d_t>(&app->texture_arena, 1);
    vk_gfx.load_font_sampler(
        &app->texture_arena,
        app->default_font_texture,
        &app->default_font);

    gfx::vul::texture_2d_t* ui_textures[32];
    for(size_t i = 0; i < array_count(ui_textures); i++) { ui_textures[i] = app->default_font_texture; }

    app->default_font_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        vk_gfx.gui_pipeline.descriptor_set_layouts[0],
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
    panel->max = v2f{400.0f, 300.0f};

    gfx::gui::panel_t* header = arena_alloc_ctor<gfx::gui::panel_t>(&app->gui.arena, 1, gfx::gui::panel_t{});
    header->min = v2f{1.0f};
    header->max = v2f{399.0f, 6.0f};
    header->theme.bg_color = "#3e3e3eff"_rgba;
    panel->next = header;

    gfx::gui::text_t* arena_text = arena_alloc_ctor<gfx::gui::text_t>(&app->gui.arena, 9, gfx::gui::text_t{});
    for (size_t i = 0; i < 9; i++) {
        arena_text[i].next = i != 8 ? arena_text + i + 1 : 0; 
        arena_text[i].font_size = 12;
        arena_text[i].offset = v2f{10.0f, 15.0f * i + 20.f};
        arena_text[i].theme.text_color = gfx::color::rgba::white;
        arena_text[i].theme.shadow_color = gfx::color::rgba::black;
        arena_text[i].theme.shadow_distance = 1;
    }

    panel->text_widgets = arena_text;


    std::thread{
        [=] {
            auto loaded_mesh = utl::pooled_load_mesh(
                &app->temp_arena, 
                &app->vertices.pool,
                &app->indices.pool,
                "./assets/models/utah-teapot.obj"
            );

            entity::entity_t e = entity::world_create_entity(app->world);
            mesh_c* mesh = entity::world_emplace_component<mesh_c>(app->world, e, loaded_mesh.meshes[0]);

            gen_info("mesh", "loaded: v start: {}\nv count: {}\ni start: {}\ni count: {}",
                mesh->vertex_start,
                mesh->vertex_count,
                mesh->index_start,
                mesh->index_count
            );

            assert(loaded_mesh.count == 1);

            app->v_count = safe_truncate_u64(loaded_mesh.meshes[0].vertex_count);
            app->i_count = safe_truncate_u64(loaded_mesh.meshes[0].index_count);
            arena_clear(&app->temp_arena);
        }
    }.detach();
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

    app->gfx.cleanup();
}

export_fn(void)
app_on_reload(app_memory_t* app_mem) {
    // s_app_mem = app_mem;
    app_t* app = get_app(app_mem);

    app->gfx.init(&app_mem->config);
}

export_fn(void) 
app_on_update2(app_memory_t* app_mem) {

}

void 
app_on_input(app_t* app, app_input_t* input) {
    if (input->keys['M']) {
        app->scene.sporadic_buffer.mode = 1;
    } else if (input->keys['N']) {
        app->scene.sporadic_buffer.mode = 0;
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
        app_mem->input.time
        , 
        10.0f
    );

    // draw gui 


    auto string_mark = arena_get_mark(&app->string_arena);

    arena_t* display_arenas[] = {
        &app->main_arena,
        &app->temp_arena,
        &app->string_arena,
        &app->mesh_arena,
        &app->texture_arena,
        &app->vertices.pool,
        &app->indices.pool,
        &app->gui.vertices.pool,
        &app->gui.indices.pool,
    };

    const char* display_arena_names[] = {
        "Main Arena",
        "Temp Arena",
        "String Arena",
        "Mesh Arena",
        "Texture Arena",
        "3D Vertex",
        "3D Index",
        "2D Vertex",
        "2D Index",
    };

    for (size_t i = 0; i < 9; i++) {
        app->gui.main_panel->text_widgets[i].text.own(&app->string_arena, arena_display_info(display_arenas[i], display_arena_names[i]).c_str());
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
    app->scene.scene_buffer.view = glm::lookAt(v3f{0,20,-25}, v3f{0.0f}, v3f{0,1,0});
    app->scene.scene_buffer.proj = glm::perspective(90.0f, aspect, 0.3f, 1000.0f);
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

        const u32 vertex_count = app->v_count;
        const u32 index_count = app->i_count;
        const u32 instance_count = 1;
        const u32 first_vertex = 0;
        const u32 first_index = 0;
        const u32 first_instance = 0;
        const u32 vertex_offset = 0;

        // vkCmdDrawIndexed(vk_gfx.command_buffer,
        //     vertex_count,
        //     instance_count, 
        //     first_vertex,
        //     first_instance           
        // );

        for (size_t i = 0; i < app->world->entities->count; i++) {
            entity::entity_t e = i;
            if (const auto* mesh = entity::world_get_component<mesh_c>(app->world, e)) {
                if (mesh->index_count) {
                    vkCmdDrawIndexed(vk_gfx.command_buffer,
                        mesh->index_count,
                        instance_count, 
                        mesh->index_start,
                        mesh->vertex_start,
                        first_instance           
                    );
                } else {
                    vkCmdDraw(vk_gfx.command_buffer,
                        mesh->vertex_count,
                        mesh->instance_count,
                        mesh->vertex_start,
                        mesh->instance_start
                    );
                }
            }

        }

        // vkCmdDrawIndexed(vk_gfx.command_buffer,
        //     index_count,
        //     instance_count, 
        //     first_index,
        //     vertex_offset,
        //     first_instance           
        // );
    });

    vk_gfx.record_pipeline_commands(
        vk_gfx.gui_pipeline.pipeline, vk_gfx.gui_pipeline.render_pass,
        vk_gfx.gui_pipeline.swap_framebuffers[imageIndex],
        vk_gfx.command_buffer, imageIndex, 
        [&]{
            vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, vk_gfx.gui_pipeline.pipeline_layout,
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