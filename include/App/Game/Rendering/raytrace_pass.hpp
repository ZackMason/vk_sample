#pragma once

static void 
begin_rt_pass(
    system_t* rs,
    VkCommandBuffer command_buffer,
    u32 frame_count 
) {
    TIMED_FUNCTION;
    auto& gfx = *rs->vk_gfx;
    auto& queue = rs->vk_gfx->compute_queue;
    auto& ext = rs->vk_gfx->ext;
    auto& khr = rs->vk_gfx->khr;
    auto& pass = rs->get_frame_data().rt_compute_pass;
    auto& cache = *rs->rt_cache;
    u32 width = (u32)rs->width;
    u32 height = (u32)rs->height;

    pass.set_camera(gfx, rs->view, rs->projection);


    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties{};
    ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 device_properties{};
    device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    device_properties.pNext = &ray_tracing_pipeline_properties;
    vkGetPhysicalDeviceProperties2(gfx.gpu_device, &device_properties);
    
    local_persist uint32_t handle_size_aligned = align_2n(ray_tracing_pipeline_properties.shaderGroupHandleSize, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);

    VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
    raygen_shader_sbt_entry.deviceAddress = gfx.get_buffer_device_address(cache.raygen_shader_binding_table.buffer);
    raygen_shader_sbt_entry.stride        = handle_size_aligned;
    raygen_shader_sbt_entry.size          = handle_size_aligned;

    VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
    miss_shader_sbt_entry.deviceAddress = gfx.get_buffer_device_address(cache.miss_shader_binding_table.buffer);
    miss_shader_sbt_entry.stride        = handle_size_aligned;
    miss_shader_sbt_entry.size          = handle_size_aligned;

    VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
    hit_shader_sbt_entry.deviceAddress = gfx.get_buffer_device_address(cache.hit_shader_binding_table.buffer);
    hit_shader_sbt_entry.stride        = handle_size_aligned;
    hit_shader_sbt_entry.size          = handle_size_aligned;

    VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

    /*
        Dispatch the ray tracing commands
    */
    if (pass.instance_count > 0) {
        pass.build_tlas(*rs->vk_gfx, *rs->rt_cache, 
            pass.tlas.buffer.buffer != VK_NULL_HANDLE
        );

        pass.build_descriptors(
            gfx, 
            rs->texture_cache, 
            &cache.mesh_data_buffer, 
            gfx::vul::descriptor_builder_t::begin(
                rs->descriptor_layout_cache, 
                rs->get_frame_data().dynamic_descriptor_allocator
            ),
            &rs->frame_images[frame_count%2].texture);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, cache.pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, cache.pipeline_layout, 0, 1, &pass.descriptor_sets[0], 0, 0);

        gfx.khr.vkCmdTraceRaysKHR(
            command_buffer,
            &raygen_shader_sbt_entry,
            &miss_shader_sbt_entry,
            &hit_shader_sbt_entry,
            &callable_shader_sbt_entry,
            width,
            height,
            1);
    }
    return;
        
    { // copy vertex buffers
        u32 count = (u32)rs->vertices.pool.count();
        range_u64(i, 0, count) {
            rs->vertex_storage_buffer.pool[i].p = rs->vertices.pool[i].pos;
            rs->vertex_storage_buffer.pool[i].n = rs->vertices.pool[i].nrm;
            rs->vertex_storage_buffer.pool[i].c = rs->vertices.pool[i].col;
            rs->vertex_storage_buffer.pool[i].t = rs->vertices.pool[i].tex;
        }
        // std::memcpy(
        //     sizeof(gfx::vertex_t) * rs->vertices.pool.count()
        // );
        std::memcpy(
            &rs->index_storage_buffer.pool[0],
            &rs->indices.pool[0],
            sizeof(u32) * rs->indices.pool.count()
        );
    }

    VkBuffer buffers[]{
        rs->vk_gfx->sporadic_uniform_buffer.buffer,
        rs->job_storage_buffer().buffer,
        rs->instance_storage_buffer.buffer,
        rs->get_frame_data().indexed_indirect_storage_buffer.buffer,                    
        rs->material_storage_buffer.buffer,
        rs->environment_storage_buffer.buffer,
        rs->vertex_storage_buffer.buffer,
        rs->index_storage_buffer.buffer,
    };
    auto builder = gfx::vul::descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->get_frame_data().dynamic_descriptor_allocator);
    rs->get_frame_data().rt_compute_pass.build_buffer_sets(builder, buffers);
    rs->get_frame_data().rt_compute_pass.bind_images(builder, rs->texture_cache, &rs->frame_images[6].texture);

    auto* shader = rs->shader_cache[assets::shaders::rt_comp.filename];
    VkShaderStageFlagBits stages[1] = { VK_SHADER_STAGE_COMPUTE_BIT };
    ext.vkCmdBindShadersEXT(command_buffer, 1, stages, shader);


    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 0, 1, &pass.sporadic_descriptors, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 1, 3, &pass.object_descriptors, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 2, 1, &pass.material_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 3, 1, &pass.enviornment_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 4, 1, &pass.texture_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 5, 1, &pass.output_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 6, 1, &pass.vertex_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 7, 1, &pass.index_descriptor, 0, nullptr);


    pass.push_constants.cam_pos = v4f{rs->camera_pos, 1.0};
    pass.push_constants.inv_view = glm::inverse(rs->vp);
    pass.push_constants.draw_count = (u32)rs->get_frame_data().indexed_indirect_storage_buffer.pool.count();
    // zyy_info("dc", "DrawCount: {}", pass.push_constants.draw_count);

    vkCmdPushConstants(command_buffer, pass.pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(pass.push_constants), &pass.push_constants
    );

    vkCmdDispatch(command_buffer, width / 16, height / 16, 1);
}