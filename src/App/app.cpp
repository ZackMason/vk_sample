#include <core.hpp>

#include "App/vk_state.hpp"
        

struct app_t {
    app_memory_t*       app_mem{nullptr};

    // arenas
    arena_t             main_arena;
    arena_t             temp_arena;
    arena_t             string_arena;
    arena_t             mesh_arena;
    arena_t             texture_arena;
    
    gfx::vul::state_t   gfx;


    inline static constexpr size_t max_scene_vertex_count{1'000'000};
    inline static constexpr size_t max_scene_index_count{3'000'000};
    gfx::vul::vertex_buffer_t<gfx::vertex_t, max_scene_vertex_count> vertices;
    gfx::vul::index_buffer_t<max_scene_index_count> indices;

    struct scene_t {
        gfx::vul::scene_buffer_t scene_buffer;
        gfx::vul::object_buffer_t object_buffer;
        gfx::vul::sporadic_buffer_t sporadic_buffer;
    } scene;

    utl::anim::anim_pool_t<v3f>* pos_anim_pool;
    utl::anim::anim_pool_t<v3f>* scale_anim_pool;
    utl::anim::anim_pool_t<v3f>* rot_anim_pool;
    
    entity::entity_pool_t* entities;
    entity::component_t<v3f>::pool_t* position_components;
    entity::component_t<string_t>::pool_t* name_components;
    entity::world_t* world;

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

    arena_t* main_arena = &app->main_arena;

    app->temp_arena = arena_sub_arena(&app->main_arena, megabytes(4));
    app->string_arena = arena_sub_arena(&app->main_arena, megabytes(16));
    app->mesh_arena = arena_sub_arena(&app->main_arena, megabytes(512));
    app->texture_arena = arena_sub_arena(&app->main_arena, megabytes(64));


    app->pos_anim_pool      = arena_alloc_ctor<utl::anim::anim_pool_t<v3f>>(main_arena, 1, utl::anim::anim_pool_t<v3f>{main_arena, 64});
    app->scale_anim_pool    = arena_alloc_ctor<utl::anim::anim_pool_t<v3f>>(main_arena, 1, utl::anim::anim_pool_t<v3f>{main_arena, 64});
    app->rot_anim_pool      = arena_alloc_ctor<utl::anim::anim_pool_t<v3f>>(main_arena, 1, utl::anim::anim_pool_t<v3f>{main_arena, 64});

    // app->entities           = arena_alloc_ctor<entity::entity_pool_t>(main_arena, 1, entity::entity_pool_t{main_arena, 64'000});
    // app->name_components    = arena_alloc_ctor<entity::component_t<string_t>::pool_t>(main_arena, 1, entity::component_t<string_t>::pool_t{main_arena, 64'000});
    // app->position_components= arena_alloc_ctor<entity::component_t<v3f>::pool_t>(main_arena, 1, entity::component_t<v3f>::pool_t{main_arena, 64'000});
    
    // app->entities->allocate(64'000);
    // app->name_components->allocate(10);
    // (*app->name_components)[0].own(&app->string_arena, "wow");

    // gen_info("app::init", "ent name: {}", 
    //     (*app->name_components)[0].c_str()
    // );

    // app->world = entity::world_init(main_arena, megabytes(256+64));

    // struct health_c : entity::component_t<health_c> {
    //     i32 max, current;

    //     health_c(i32 _max, i32 _cur)
    //     : max{_max}, current{_cur}
    //     {
    //     }
    // };

    // const auto e = entity::world_create_entity(app->world);
    // health_c* health = entity::world_emplace_component<health_c>(app->world, e, 100, 69);
    // for (int i = 0; i<1000; i++) {
    //     const auto ne = entity::world_create_entity(app->world);
    //     health = entity::world_emplace_component<health_c>(app->world, ne, 100, 420);
    // }
    
    // gen_info("app::entity", "entity health: {} / {}", health->current, health->max );



    // utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 0.0f, v3f{-3.0f, 0.0f, 0.0f});
    // utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 2.0f, v3f{0.0f, 0.0f, 3.0f});
    // utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 4.0f, v3f{3.0f, 0.0f, 0.0f});
    // utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 6.0f, v3f{0.0f, 0.0f, -3.0f});
    
    utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 0.0f, v3f{1.0f});
    utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 2.0f, v3f{0.5f});
    utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 4.0f, v3f{1.0f});
    utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 6.0f, v3f{2.0f});
    utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 8.0f, v3f{1.0f});
    utl::anim::anim_pool_add_time_value(app->pos_anim_pool, 10.0f, v3f{0.5f});
    
    utl::anim::anim_pool_add_time_value(app->rot_anim_pool, 0.0f, v3f{1.0f, 0.0f, .0f});
    utl::anim::anim_pool_add_time_value(app->rot_anim_pool, 2.0f, v3f{0.5f, 0.0f, 0.0f});
    utl::anim::anim_pool_add_time_value(app->rot_anim_pool, 4.0f, v3f{0.0f, 1.0f, .0f});
    utl::anim::anim_pool_add_time_value(app->rot_anim_pool, 6.0f, v3f{0.0f, 2.0f, 0.0f});
    utl::anim::anim_pool_add_time_value(app->rot_anim_pool, 8.0f, v3f{0.0f, 0.0f, 1.0f});
    utl::anim::anim_pool_add_time_value(app->rot_anim_pool, 10.0f, v3f{0.0f, 0.0f, 0.5f});
    
    utl::anim::anim_pool_add_time_value(app->scale_anim_pool, 0.0f, v3f{1.0f});
    utl::anim::anim_pool_add_time_value(app->scale_anim_pool, 2.0f, v3f{2.5f});
    utl::anim::anim_pool_add_time_value(app->scale_anim_pool, 4.0f, v3f{1.0f});
    utl::anim::anim_pool_add_time_value(app->scale_anim_pool, 6.0f, v3f{0.5f});
    utl::anim::anim_pool_add_time_value(app->scale_anim_pool, 8.0f, v3f{0.5f, 6.0f, 0.5f});
    utl::anim::anim_pool_add_time_value(app->scale_anim_pool, 10.0f, v3f{1.5f});

    
    // setup graphics

    gfx::vul::state_t& vk_gfx = app->gfx;
    app->gfx.init(&app_mem->config);

    vk_gfx.create_vertex_buffer(
        &app->vertices
    );
    vk_gfx.create_index_buffer(
        &app->indices
    );

    auto loaded_mesh = utl::pooled_load_mesh(
        &app->temp_arena, 
        &app->vertices.pool,
        &app->indices.pool,
        "./assets/models/lucy.obj"
    );

    assert(loaded_mesh.count == 1);

    app->v_count = safe_truncate_u64(loaded_mesh.meshes[0].vertex_count);
    app->i_count = safe_truncate_u64(loaded_mesh.meshes[0].index_count);


    arena_clear(&app->temp_arena);
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

    // update uniforms

    app->scene.sporadic_buffer.num_instances = 1;

    const f32 aspect = (f32)app_mem->config.window_size[0] / (f32)app_mem->config.window_size[1];

    app->scene.scene_buffer.time = app_mem->input.time;
    app->scene.scene_buffer.view = glm::lookAt(v3f{0,30,-65}, v3f{40.0f, 0.0f, 0.0f}, v3f{0,1,0});
    app->scene.scene_buffer.proj = glm::perspective(90.0f, aspect, 0.3f, 1000.0f);
    app->scene.scene_buffer.proj[1][1] *= -1.0f;

    app->scene.scene_buffer.scene = glm::mat4{0.5f};
    app->scene.scene_buffer.light_pos = v4f{3,13,3,0};
    app->scene.scene_buffer.light_col = v4f{0.2f, 0.1f, 0.8f, 1.0f};
    app->scene.scene_buffer.light_KaKdKs = v4f{0.5f};

    app->scene.object_buffer.model = 
        glm::translate(m44{1.0f}, 
            utl::anim::anim_pool_get_time_value(app->pos_anim_pool, anim_time)
        )
        * 
        glm::rotate(m44{1.0f}, anim_time * 360.0f * 0.0015f, 
            utl::anim::anim_pool_get_time_value(app->rot_anim_pool, anim_time)
        ) 
        *
        glm::scale(m44{1.0f}, 
            utl::anim::anim_pool_get_time_value(app->scale_anim_pool, anim_time) * 0.015f
        ) 
        ;

    app->scene.object_buffer.color = v4f{1.0f};
    app->scene.object_buffer.shininess = 2.0f;

    *vk_gfx.scene_uniform_buffer.data = app->scene.scene_buffer;
    *vk_gfx.sporadic_uniform_buffer.data = app->scene.sporadic_buffer;
    *vk_gfx.object_uniform_buffer.data = app->scene.object_buffer;

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

        // vkCmdDrawIndexed(vk_gfx.command_buffer,
        //     vertex_count,
        //     instance_count, 
        //     first_vertex,
        //     first_instance           
        // );

        vkCmdDrawIndexed(vk_gfx.command_buffer,
            index_count,
            instance_count, 
            first_index,
            vertex_offset,
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