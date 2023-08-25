#pragma once

struct rt_compute_pass_t {
    VkPipelineLayout pipeline_layout{};
    VkDescriptorSet sporadic_descriptors{};
    VkDescriptorSet object_descriptors{};
    VkDescriptorSet material_descriptor{};
    VkDescriptorSet enviornment_descriptor{};
    VkDescriptorSet texture_descriptor{};
    VkDescriptorSet output_descriptor{};
    VkDescriptorSet vertex_descriptor{};
    VkDescriptorSet index_descriptor{};
    VkDescriptorSetLayout descriptor_layouts[8];
    u32 descriptor_count{8};
    VkDevice device;

    struct push_constants_t {
        m44 inv_view{1.0f};
        v4f cam_pos;
        u32 draw_count;
        u32 reserved;
    } push_constants;

    void build_layout(VkDevice device_) {
        device = device_;
        pipeline_layout = gfx::vul::create_pipeline_layout(device, descriptor_layouts, 8, sizeof(m44) + sizeof(v4f) + sizeof(u32) * 2, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    void build_buffer_sets(gfx::vul::descriptor_builder_t& builder, VkBuffer* buffers) {
        VkDescriptorBufferInfo buffer_info[8];
        u32 i = 0;
        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;//sizeof(gfx::vul::sporadic_buffer_t);

        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;//(sizeof(m44) + sizeof(u32) * 4 * 4) * 10'000; // hardcoded
        
        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;// (sizeof(m44)) * 2'000'000; // hardcoded

        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;//(sizeof(gfx::indirect_indexed_draw_t)) * 10'000; // hardcoded

        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;//sizeof(gfx::material_t) * 100; // hardcoded

        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;//sizeof(rendering::environment_t);

        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;

        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;

        using namespace gfx::vul;
        builder
            .bind_buffer(0, buffer_info + 0, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Uniform, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(sporadic_descriptors, descriptor_layouts[0]);
        builder
            .bind_buffer(0, buffer_info + 1, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_buffer(1, buffer_info + 2, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_buffer(2, buffer_info + 3, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(object_descriptors, descriptor_layouts[1]);
        builder
            .bind_buffer(0, buffer_info + 4, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(material_descriptor, descriptor_layouts[2]);
        builder
            .bind_buffer(0, buffer_info + 5, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(enviornment_descriptor, descriptor_layouts[3]);

        builder
            .bind_buffer(0, buffer_info + 6, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(vertex_descriptor, descriptor_layouts[6]);
        builder
            .bind_buffer(0, buffer_info + 7, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(index_descriptor, descriptor_layouts[7]);
    }

    void bind_images(
        gfx::vul::descriptor_builder_t& builder,
        texture_cache_t& texture_cache,
        gfx::vul::texture_2d_t* output_texture
    ) {
        using namespace gfx::vul;
        VkDescriptorImageInfo vdii[4096];
        VkDescriptorImageInfo ovdii[1];

        auto* null_texture = texture_cache["null"];
        range_u64(i, 0, array_count(texture_cache.textures)) {
            vdii[i].imageLayout = null_texture->image_layout;
            vdii[i].imageView = null_texture->image_view;
            vdii[i].sampler = null_texture->sampler;
        }

        u64 w{0};
        for(u64 i = texture_cache.first(); 
            i != texture_cache.end(); 
            i = texture_cache.next(i)
        ) {
            vdii[i].imageLayout = texture_cache[i]->image_layout;
            vdii[i].imageView = texture_cache[i]->image_view;
            vdii[i].sampler = texture_cache[i]->sampler;
        }

        ovdii[0].imageLayout = output_texture->image_layout;
        ovdii[0].imageView = output_texture->image_view;
        ovdii[0].sampler = output_texture->sampler;

        builder
            .bind_image(0, vdii, array_count(vdii), (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Sampler, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(texture_descriptor, descriptor_layouts[4]);
        builder
            .bind_image(0, ovdii, array_count(ovdii), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(output_descriptor, descriptor_layouts[5]);
    }
};
