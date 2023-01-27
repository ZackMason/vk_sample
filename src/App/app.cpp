#include <core.hpp>

#include "App/vk_state.hpp"

struct mesh_builder_t {
    // note(zack): assumes nothing else will push to this arena during usage
    arena_t* arena; 
    vertex_t* vertices{nullptr};
    u32 vertex_count{0};
    gfx::vul::index_t* indices{nullptr};
    u32 index_count{0};

    mesh_builder_t(arena_t* _arena) 
        : arena{_arena}, 
        vertices{(vertex_t*)arena_get_top(arena)}
    {
    }
 
    mesh_builder_t& add_vertex(vertex_t v) {
        // note(zack):
        // must not have indices at this point
        // else the arrays are not linear
        assert(!indices);

        *(vertex_t*)arena_alloc(arena, sizeof(vertex_t)) = v;
        ++vertex_count;
        return *this;
    }

    mesh_builder_t& add_tri(
        gfx::vul::index_t p0,
        gfx::vul::index_t p1,
        gfx::vul::index_t p2
    ) {
        if (!indices) {
            indices = (u32*)arena->start;
        }
        auto* tri = arena_emplace<gfx::vul::index_t>(arena, 3, 0);
        tri[0] = p0;
        tri[1] = p1;
        tri[2] = p2;
        index_count += 3;
        return *this;
    }

    void finalize() {
        arena = nullptr;
    }
};

struct app_t {
    app_memory_t*       app_mem{nullptr};

    // arenas
    arena_t             main_arena;
    arena_t             temp_arena;
    arena_t             string_arena;
    arena_t             mesh_arena;
    arena_t             texture_arena;
    
    gfx::vul::state_t   gfx;

    gfx::vul::vertex_buffer_t vertices;
    gfx::vul::index_buffer_t indices;

    struct scene_t {
        gfx::vul::scene_buffer_t scene_buffer;
        gfx::vul::object_buffer_t object_buffer;
        gfx::vul::sporadic_buffer_t sporadic_buffer;
    } scene;

    string_t test;

    u32 v_count;
    u32 i_count;
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

    app->temp_arena = arena_sub_arena(&app->main_arena, megabytes(4));
    app->string_arena = arena_sub_arena(&app->main_arena, megabytes(16));
    app->mesh_arena = arena_sub_arena(&app->main_arena, megabytes(64));
    app->texture_arena = arena_sub_arena(&app->main_arena, megabytes(64));

    // app->test = (std::string*)arena_alloc(&app->main_arena, sizeof(std::string));
    // new (app->test) std::string;

    app->test.own(&app->string_arena, "hello world");

    gfx::vul::state_t& vk_gfx = app->gfx;
    app->gfx.init(&app_mem->config);

    auto loaded_mesh = utl::load_mesh(
        &app->temp_arena, 
        "./assets/models/utah-teapot.obj"
    );

    assert(loaded_mesh.count == 1);

    app->v_count = safe_truncate_u64(loaded_mesh.meshes[0].vertex_count);
    app->i_count = safe_truncate_u64(loaded_mesh.meshes[0].index_count);
    vk_gfx.create_vertex_buffer(
        loaded_mesh.meshes[0].vertex_count*sizeof(vertex_t), 
        &app->vertices
    );
    
    vk_gfx.fill_data_buffer(
        &app->vertices, 
        (void*)loaded_mesh.meshes[0].vertices, 
        loaded_mesh.meshes[0].vertex_count * sizeof(vertex_t)
    );

    vk_gfx.create_index_buffer(
        loaded_mesh.meshes[0].index_count*sizeof(u32), 
        &app->indices
    );
    
    vk_gfx.fill_data_buffer(
        &app->indices, 
        (void*)loaded_mesh.meshes[0].indices, 
        loaded_mesh.meshes[0].index_count * sizeof(u32)
    );

    arena_clear(&app->temp_arena);

    vk_gfx.fill_data_buffer(&vk_gfx.scene_uniform_buffer,    (void*)&app->scene.scene_buffer);
    vk_gfx.fill_data_buffer(&vk_gfx.object_uniform_buffer,   (void*)&app->scene.object_buffer);
    vk_gfx.fill_data_buffer(&vk_gfx.sporadic_uniform_buffer, (void*)&app->scene.sporadic_buffer);
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

    // update uniforms
    app->scene.sporadic_buffer.mode = 1;
    app->scene.sporadic_buffer.num_instances = 1;
    app->scene.sporadic_buffer.use_lighting = 0;
    vk_gfx.fill_data_buffer(
        &vk_gfx.sporadic_uniform_buffer,
        &app->scene.sporadic_buffer
    );

    const f32 aspect = (f32)app_mem->config.window_size[0] / (f32)app_mem->config.window_size[1];

    app->scene.scene_buffer.time = app_mem->input.time;
    app->scene.scene_buffer.view = glm::lookAt(v3f{0,0,-5}, v3f{}, v3f{0,1,0});
    app->scene.scene_buffer.proj = glm::perspective(45.0f, aspect, 0.1f, 100.0f);
    app->scene.scene_buffer.proj[1][1] *= -1.0f;

    app->scene.scene_buffer.scene = glm::rotate(glm::mat4{1.f}, app_mem->input.time * 1000.0f, v3f{0,1,0});
    app->scene.scene_buffer.light_pos = v4f{3,3,3,0};
    app->scene.scene_buffer.light_col = v4f{0.2f, 0.1f, 0.8f, 1.0f};
    app->scene.scene_buffer.light_KaKdKs = v4f{0.5f};

    vk_gfx.fill_data_buffer(
        &vk_gfx.scene_uniform_buffer,
        &app->scene.scene_buffer);

    app->scene.object_buffer.model = glm::scale(m44{1.0f}, v3f{0.1f});
    app->scene.object_buffer.color = v4f{1.0f};
    app->scene.object_buffer.shininess = 2.0f;

    vk_gfx.fill_data_buffer(
        &vk_gfx.object_uniform_buffer,
        &app->scene.object_buffer);

    // render
    
    vkWaitForFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk_gfx.device, 1, &vk_gfx.in_flight_fence);

    u32 imageIndex;
    vkAcquireNextImageKHR(vk_gfx.device, vk_gfx.swap_chain, UINT64_MAX, 
        vk_gfx.image_available_semaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(vk_gfx.command_buffer, 0);
    vk_gfx.record_command_buffer(vk_gfx.command_buffer, imageIndex, [&]{

        vkCmdBindDescriptorSets(vk_gfx.command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, vk_gfx.pipeline_layout,
            0, 4, vk_gfx.descriptor_sets, 0, nullptr);

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

        vkCmdDraw(vk_gfx.command_buffer,
            vertex_count,
            instance_count, 
            first_vertex,
            first_instance           
        );
    });

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

    if (vkQueueSubmit(vk_gfx.gfx_queue, 1, &submitInfo, vk_gfx.in_flight_fence) != VK_SUCCESS) {
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