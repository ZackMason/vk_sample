#pragma once

static void 
begin_rt_pass(
    system_t* rs,
    VkCommandBuffer command_buffer
) {
    auto& queue = rs->vk_gfx->compute_queue;
    auto& ext = rs->vk_gfx->ext;
    auto& khr = rs->vk_gfx->khr;
        
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

    auto& pass = rs->get_frame_data().rt_compute_pass;

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 0, 1, &pass.sporadic_descriptors, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 1, 3, &pass.object_descriptors, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 2, 1, &pass.material_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 3, 1, &pass.enviornment_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 4, 1, &pass.texture_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 5, 1, &pass.output_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 6, 1, &pass.vertex_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.pipeline_layout, 7, 1, &pass.index_descriptor, 0, nullptr);

    u32 width = (u32)rs->width;
    u32 height = (u32)rs->height;

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