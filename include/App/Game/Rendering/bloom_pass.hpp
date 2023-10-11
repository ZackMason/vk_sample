#pragma once

#include "zyy_core.hpp"
#include "App/vk_state.hpp"

struct bloom_mip_t {
    v2i size;
    gfx::vul::texture_2d_t* texture{0};

    v2f fsize() const {
        return v2f{size};
    };
};

struct bloom_pass_t {
    arena_t arena = arena_create(1024);

    VkPipelineLayout pipeline_layout{};
    VkDescriptorSet texture_descriptor{};
    VkDescriptorSetLayout descriptor_layouts[1];
    pp_material_t parameters{};
    u32 descriptor_count{1};
    VkDevice device;

    u32 texture_count{0};
    gfx::vul::texture_2d_t* textures[13];
    gfx::vul::texture_2d_t* output{0};

    v2i size{};

    bloom_mip_t mip_chain[12];
    u32 mip_count{0};

    void initialize(gfx::vul::state_t& gfx, v2i size_, u32 count) {
        size = size_;
        range_u32(i, 0, count) {
            size_ >>= 1;
            tag_struct(auto* texture, gfx::vul::texture_2d_t, &arena);
            texture->format = VK_FORMAT_R16G16B16A16_SFLOAT;
            texture->sampler_tiling_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            gfx.create_texture(texture, size_.x, size_.y, 4, 0, 0, sizeof(float)*2);
            gfx.load_texture_sampler(texture);
            add_mip(size_, texture);
        }
    }

    void fill_textures(gfx::vul::texture_2d_t* texture) {
        texture_count = 0;
        textures[texture_count++] = texture;
        range_u32(i, 0, mip_count) {
            textures[texture_count++] = mip_chain[i].texture;
        }
    }

    bloom_mip_t& last_mip() {
        return mip_chain[mip_count-1];
    }

    bloom_mip_t& add_mip(v2i size_, gfx::vul::texture_2d_t* texture) {
        assert(mip_count<12);
        return mip_chain[mip_count++] = bloom_mip_t {
            .size = size_,
            .texture = texture,
        };
    }
        
    void build_layout(gfx::vul::state_t& vk_state) {
        utl::memzero(parameters.data.data(), sizeof(f32)*16);
        device = vk_state.device;
        pipeline_layout = gfx::vul::create_pipeline_layout(vk_state.device, descriptor_layouts, 1, sizeof(parameters));
    }
    
    void push_constants(VkCommandBuffer command_buffer) {
        vkCmdPushConstants(command_buffer, pipeline_layout,
            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 
            0, sizeof(parameters), &parameters
        );
    }

    void bind_descriptors(VkCommandBuffer command_buffer) {
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &texture_descriptor, 0, nullptr);
    }

    void bind_images(gfx::vul::descriptor_builder_t& builder, gfx::vul::texture_2d_t* texture) {
        using namespace gfx::vul;
        VkDescriptorImageInfo vdii[1];
            vdii[0].imageLayout = texture->image_layout;
            vdii[0].imageView = texture->image_view;
            vdii[0].sampler = texture->sampler;

        builder
            .bind_image(0, vdii, array_count(vdii), (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Sampler, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(texture_descriptor, descriptor_layouts[0]);
    }

    void bind_images(gfx::vul::descriptor_builder_t& builder) {
        using namespace gfx::vul;
        VkDescriptorImageInfo vdii[13];

        range_u32(i, 0, texture_count) {
            vdii[i].imageLayout = textures[i]->image_layout;
            vdii[i].imageView = textures[i]->image_view;
            vdii[i].sampler = textures[i]->sampler;
        }

        builder
            .bind_image(0, vdii, texture_count, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Sampler, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(texture_descriptor, descriptor_layouts[0]);
    }

};

