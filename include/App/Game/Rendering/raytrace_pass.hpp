#pragma once

static void 
begin_rt_pass(
    system_t* rs,
    VkCommandBuffer command_buffer
    // u32 frame_count 
) {
    TIMED_FUNCTION;
    auto& gfx = *rs->vk_gfx;
    auto device = gfx.device;
    auto& queue = rs->vk_gfx->compute_queue;
    auto& ext = rs->vk_gfx->ext;
    auto& khr = rs->vk_gfx->khr;
    auto& pass = rs->get_frame_data().rt_compute_pass;
    auto& cache = *rs->rt_cache;
    u32 width = (u32)rs->width;
    u32 height = (u32)rs->height;



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
        pass.build_tlas(*rs->vk_gfx, *rs->rt_cache, command_buffer,
            pass.tlas.buffer.buffer != VK_NULL_HANDLE
        );

        // rs->scene_context->get_scene().tlas = pass.tlas.device_address;

        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        VkMemoryBarrier barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, 0, 0};
        vkCmdPipelineBarrier(command_buffer, srcStageMask, dstStageMask, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        local_persist u32 frame=0;
        if (frame++ < 3) {
            zyy_info("rt", "Frame: {}", frame);
        }

        v3f axis = glm::normalize(utl::rng::random_s::randnv());
        float angle = utl::rng::random_s::randf()*360.0f;
        cache.constants.random_rotation = glm::toMat3(glm::angleAxis(angle, axis));
        cache.constants.scene = rs->scene_context->scene_address();

        // local_persist b32 once = true;
        
    #if 1
        // if (pass.descriptor_set_layouts[0] == VK_NULL_HANDLE) 
        {
            // once = false;
            pass.build_descriptors(
                gfx, 
                // command_buffer,
                // cache.pipeline_layout,
                rs->texture_cache, 
                &rs->scene_context->entities,
                // &cache.mesh_data_buffer, 
                &rs->probe_storage_buffer,
                &rs->light_probe_settings_buffer,
                &rs->light_probe_ray_buffer,
                &rs->environment_storage_buffer,
                &rs->point_light_storage_buffer,
                gfx::vul::descriptor_builder_t::begin(
                    rs->descriptor_layout_cache, 
                    rs->get_frame_data().dynamic_descriptor_allocator
                ),
                &rs->light_probes.irradiance_texture,
                &rs->light_probes.visibility_texture
            );
        }
    #endif

        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            rs->light_probes.irradiance_texture.image,
            0,
            VK_ACCESS_SHADER_READ_BIT,
            // VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            // VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            rs->light_probes.visibility_texture.image,
            0,
            VK_ACCESS_SHADER_READ_BIT,
            // VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            // VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        width = rs->light_probes.probe_count;

        // rs->light_probe_ray_buffer.insert_memory_barrier(
        //     command_buffer, 
        //     VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 
        //     VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 
        //     0,
        //     VK_ACCESS_SHADER_WRITE_BIT
        // );

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, cache.pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, cache.pipeline_layout, 0, 1, &pass.descriptor_sets[0], 0, 0);

        vkCmdPushConstants(command_buffer, 
            cache.pipeline_layout,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            0, sizeof(cache.constants), &cache.constants
        );

        gfx.khr.vkCmdTraceRaysKHR(
            command_buffer,
            &raygen_shader_sbt_entry,
            &miss_shader_sbt_entry,
            &hit_shader_sbt_entry,
            &callable_shader_sbt_entry,
            width,
            128,
            // 64,
            1);


        // vkCmdPipelineBarrier(command_buffer, srcStageMask, dstStageMask, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        // dstStageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

        srcStageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(command_buffer, srcStageMask, dstStageMask, 0, 1, &barrier, 0, nullptr, 0, nullptr);


        
        rs->light_probe_ray_buffer.insert_memory_barrier(
            command_buffer, 
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT
        );
        
        VkBuffer buffers[]{
            rs->vk_gfx->sporadic_uniform_buffer.buffer,
            rs->light_probe_settings_buffer.buffer,
            rs->light_probe_ray_buffer.buffer,
            rs->probe_storage_buffer.buffer,
        };

        auto builder = gfx::vul::descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->get_frame_data().dynamic_descriptor_allocator);
        pass.build_integrate_pass(builder, buffers, &rs->light_probes.irradiance_texture, &rs->light_probes.visibility_texture);
        pass.build_integrate_pass(builder, buffers, &rs->light_probes.irradiance_texture, &rs->light_probes.visibility_texture, true);

        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            rs->light_probes.irradiance_texture.image,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            rs->light_probes.visibility_texture.image,
            VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        {
            auto* shader = rs->shader_cache[assets::shaders::probe_integrate_comp.filename];
            VkShaderStageFlagBits stages[1] = { VK_SHADER_STAGE_COMPUTE_BIT };
            ext.vkCmdBindShadersEXT(command_buffer, 1, stages, shader);


            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout[0], 0, 1, &pass.descriptor_sets[1], 0, nullptr);

            vkCmdPushConstants(command_buffer, 
                pass.pipeline_layout[0],
                VK_SHADER_STAGE_COMPUTE_BIT,
                0, sizeof(u32), &cache.constants.frame
            );

            vkCmdDispatch(command_buffer, width, 1, 1);
        }
        {
            auto* shader = rs->shader_cache[assets::shaders::probe_integrate_depth_comp.filename];
            VkShaderStageFlagBits stages[1] = { VK_SHADER_STAGE_COMPUTE_BIT };
            ext.vkCmdBindShadersEXT(command_buffer, 1, stages, shader);


            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout[1], 0, 1, &pass.descriptor_sets[2], 0, nullptr);

            vkCmdPushConstants(command_buffer, 
                pass.pipeline_layout[1],
                VK_SHADER_STAGE_COMPUTE_BIT,
                0, sizeof(u32), &cache.constants.frame
            );

            vkCmdDispatch(command_buffer, width, 1, 1);
        }
        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            rs->light_probes.irradiance_texture.image,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

        gfx::vul::utl::insert_image_memory_barrier(
            command_buffer,
            rs->light_probes.visibility_texture.image,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });


        // generate_mipmaps(&gfx, &rs->light_probes.irradiance_texture, command_buffer);
        // generate_mipmaps(&gfx, &rs->light_probes.visibility_texture, command_buffer);
        cache.constants.frame++;
    }
}