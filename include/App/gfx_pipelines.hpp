#ifndef GFX_PIPELINES_HPP
#define GFX_PIPELINES_HPP

#include "core.hpp"
#include "vk_state.hpp"

namespace gfx::vul {

struct mesh_pipeline_info_t {
    VkBuffer scene_buffer;
    VkBuffer sporadic_buffer;
    VkBuffer object_buffer;

    std::string_view fragment_shader = "./assets/shaders/bin/simple.frag.spv";
    std::string_view vertex_shader = "./assets/shaders/bin/simple.vert.spv";
};

pipeline_state_t*
create_mesh_pipeline(arena_t* arena, state_t* state, const mesh_pipeline_info_t& pipeline_desc) {
    pipeline_state_t* pipeline = arena_alloc_ctor<pipeline_state_t>(arena, 1);

    pipeline->framebuffer_count = safe_truncate_u64(state->swap_chain_images.size());
    pipeline->framebuffers = arena_alloc_ctor<VkFramebuffer>(arena, state->swap_chain_images.size());

    const auto stack_mark = arena_get_mark(arena); 
    pipeline_state_t::create_info_t* create_info = arena_alloc_ctor<pipeline_state_t::create_info_t>(arena, 1);

    create_info->cull_mode = VK_CULL_MODE_BACK_BIT;

    create_info->vertex_shader = pipeline_desc.vertex_shader;
    create_info->fragment_shader = pipeline_desc.fragment_shader;

    create_info->push_constant_size = sizeof(gfx::vul::object_push_constants_t);

    create_info->attachment_count = 2;
    create_info->attachment_descriptions[0] = utl::attachment_description(
        state->swap_chain_image_format,
        VK_ATTACHMENT_LOAD_OP_LOAD,
        VK_ATTACHMENT_STORE_OP_STORE,
        // VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
    
    create_info->attachment_descriptions[1] = utl::attachment_description(
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    create_info->vertex_input_binding_count = 1;
    create_info->vertex_input_binding_descriptions[0] = utl::vertex_input_binding_description(
        0, sizeof(vertex_t), VK_VERTEX_INPUT_RATE_VERTEX
    );

    create_info->vertex_input_attribute_count = 4;
    create_info->vertex_input_attribute_description[0] = utl::vertex_input_attribute_description(
        0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_t, pos)
    );
    create_info->vertex_input_attribute_description[1] = utl::vertex_input_attribute_description(
        0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_t, nrm)
    );
    create_info->vertex_input_attribute_description[2] = utl::vertex_input_attribute_description(
        0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_t, col)
    );
    create_info->vertex_input_attribute_description[3] = utl::vertex_input_attribute_description(
        0, 3, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex_t, tex)
    );

    VkDescriptorImageInfo vdii[1];

    create_info->descriptor_count = 4;

    create_info->descriptor_flags[0] = pipeline_state_t::create_info_t::DescriptorFlag_Uniform;
    create_info->descriptor_flags[1] = pipeline_state_t::create_info_t::DescriptorFlag_Uniform;
    create_info->descriptor_flags[2] = pipeline_state_t::create_info_t::DescriptorFlag_Storage;

    create_info->descriptor_set_layout_bindings[0] = utl::descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0
    );
    create_info->descriptor_set_layout_bindings[1] = utl::descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0
    );
    create_info->descriptor_set_layout_bindings[2] = utl::descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0
    );

    create_info->descriptor_flags[3] = pipeline_state_t::create_info_t::DescriptorFlag_Sampler;
    create_info->descriptor_set_layout_bindings[3] = utl::descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, array_count(vdii)
    );

    state->create_pipeline_state_descriptors(pipeline, create_info);

    //todo(zack): fill out uniform pipeline_desc
    VkDescriptorBufferInfo buffer_info[3];
    buffer_info[0].buffer = pipeline_desc.sporadic_buffer;
    buffer_info[0].offset = 0; 
    buffer_info[0].range = sizeof(sporadic_buffer_t);

    buffer_info[1].buffer = pipeline_desc.scene_buffer;
    buffer_info[1].offset = 0; 
    buffer_info[1].range = sizeof(scene_buffer_t);

    buffer_info[2].buffer = pipeline_desc.object_buffer;
    buffer_info[2].offset = 0; 
    buffer_info[2].range = sizeof(m44) * 10'000; // hardcoded

    create_info->write_descriptor_sets[0] = utl::write_descriptor_set(
        pipeline->descriptor_sets[0], 
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
        0, buffer_info
    );
    create_info->write_descriptor_sets[1] = utl::write_descriptor_set(
        pipeline->descriptor_sets[1], 
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, buffer_info + 1
    );
    create_info->write_descriptor_sets[2] = utl::write_descriptor_set(
        pipeline->descriptor_sets[2], 
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, buffer_info + 2
    );

    for (size_t i = 0; i < array_count(vdii); i++) {
        vdii[i].sampler = state->null_texture.sampler;
        vdii[i].imageView = state->null_texture.image_view;
        vdii[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    create_info->write_descriptor_sets[3] = utl::write_descriptor_set(
        pipeline->descriptor_sets[3], 
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, vdii, array_count(vdii)
    );
    
    state->create_pipeline_state(pipeline, create_info);
    arena_set_mark(arena, stack_mark); 

    return pipeline;
}

pipeline_state_t*
create_skybox_pipeline(arena_t* arena, state_t* state) {
    pipeline_state_t* pipeline = arena_alloc_ctor<pipeline_state_t>(arena, 1);

    pipeline->framebuffer_count = safe_truncate_u64(state->swap_chain_images.size());
    pipeline->framebuffers = arena_alloc_ctor<VkFramebuffer>(arena, state->swap_chain_images.size());

    const auto stack_mark = arena_get_mark(arena); 
    pipeline_state_t::create_info_t* create_info = arena_alloc_ctor<pipeline_state_t::create_info_t>(arena, 1);

    create_info->cull_mode = VK_CULL_MODE_FRONT_BIT;

    create_info->vertex_shader = "./assets/shaders/bin/skybox.vert.spv";
    create_info->fragment_shader = "./assets/shaders/bin/skybox.frag.spv";

    create_info->push_constant_size = sizeof(m44) + sizeof(v4f);

    create_info->attachment_count = 2;
    create_info->attachment_descriptions[0] = utl::attachment_description(
        state->swap_chain_image_format,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
    
    create_info->attachment_descriptions[1] = utl::attachment_description(
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    create_info->vertex_input_binding_count = 1;
    create_info->vertex_input_binding_descriptions[0] = utl::vertex_input_binding_description(
        0, sizeof(vertex_t), VK_VERTEX_INPUT_RATE_VERTEX
    );

    create_info->vertex_input_attribute_count = 4;
    create_info->vertex_input_attribute_description[0] = utl::vertex_input_attribute_description(
        0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_t, pos)
    );
    create_info->vertex_input_attribute_description[1] = utl::vertex_input_attribute_description(
        0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_t, nrm)
    );
    create_info->vertex_input_attribute_description[2] = utl::vertex_input_attribute_description(
        0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex_t, col)
    );
    create_info->vertex_input_attribute_description[3] = utl::vertex_input_attribute_description(
        0, 3, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex_t, tex)
    );

    // state->create_pipeline_state_descriptors(pipeline, create_info);
    
    state->create_pipeline_state(pipeline, create_info);
    arena_set_mark(arena, stack_mark); 

    return pipeline;
}

pipeline_state_t*
create_gui_pipeline(arena_t* arena, state_t* state) {
    pipeline_state_t* pipeline = arena_alloc_ctor<pipeline_state_t>(arena, 1);

    pipeline->framebuffer_count = safe_truncate_u64(state->swap_chain_images.size());
    pipeline->framebuffers = arena_alloc_ctor<VkFramebuffer>(arena, state->swap_chain_images.size());

    const auto stack_mark = arena_get_mark(arena); 
    pipeline_state_t::create_info_t* create_info = arena_alloc_ctor<pipeline_state_t::create_info_t>(arena, 1);

    create_info->vertex_shader = "./assets/shaders/bin/gui.vert.spv";
    create_info->fragment_shader = "./assets/shaders/bin/gui.frag.spv";

    create_info->attachment_count = 2;
    create_info->attachment_descriptions[0] = utl::attachment_description(
        state->swap_chain_image_format,
        VK_ATTACHMENT_LOAD_OP_LOAD,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
    
    create_info->attachment_descriptions[1] = utl::attachment_description(
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    create_info->vertex_input_binding_count = 1;
    create_info->vertex_input_binding_descriptions[0] = utl::vertex_input_binding_description(
        0, sizeof(gui::vertex_t), VK_VERTEX_INPUT_RATE_VERTEX
    );
    create_info->vertex_input_attribute_count = 4;
    create_info->vertex_input_attribute_description[0] = utl::vertex_input_attribute_description(
        0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(gui::vertex_t, pos)
    );
    create_info->vertex_input_attribute_description[1] = utl::vertex_input_attribute_description(
        0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(gui::vertex_t, tex)
    );
    create_info->vertex_input_attribute_description[2] = utl::vertex_input_attribute_description(
        0, 2, VK_FORMAT_R32_UINT, offsetof(gui::vertex_t, img)
    );
    create_info->vertex_input_attribute_description[3] = utl::vertex_input_attribute_description(
        0, 3, VK_FORMAT_R32_UINT, offsetof(gui::vertex_t, col)
    );

    VkDescriptorImageInfo vdii[64];

    create_info->descriptor_count = 1;
    create_info->descriptor_flags[0] = pipeline_state_t::create_info_t::DescriptorFlag_Sampler;
    create_info->descriptor_set_layout_bindings[0] = utl::descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, array_count(vdii)
    );

    state->create_pipeline_state_descriptors(pipeline, create_info);

    for (size_t i = 0; i < array_count(vdii); i++) {
        vdii[i].sampler = state->null_texture.sampler;
        vdii[i].imageView = state->null_texture.image_view;
        vdii[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    create_info->write_descriptor_sets[0] = utl::write_descriptor_set(
        pipeline->descriptor_sets[0], 
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, vdii, array_count(vdii)
    );
    
    state->create_pipeline_state(pipeline, create_info);
    arena_set_mark(arena, stack_mark); 

    return pipeline;
}

pipeline_state_t*
create_debug_pipeline(arena_t* arena, state_t* state, VkBuffer scene_buffer, size_t scene_buffer_size) {
    pipeline_state_t* pipeline = arena_alloc_ctor<pipeline_state_t>(arena, 1);

    pipeline->framebuffer_count = safe_truncate_u64(state->swap_chain_images.size());
    pipeline->framebuffers = arena_alloc_ctor<VkFramebuffer>(arena, state->swap_chain_images.size());

    const auto stack_mark = arena_get_mark(arena); 
    pipeline_state_t::create_info_t* create_info = arena_alloc_ctor<pipeline_state_t::create_info_t>(arena, 1);

    create_info->topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;    
    create_info->line_width = 1.0f;

    create_info->vertex_shader = "./assets/shaders/bin/debug_line.vert.spv";
    create_info->fragment_shader = "./assets/shaders/bin/debug_line.frag.spv";

    create_info->attachment_count = 2;
    create_info->attachment_descriptions[0] = utl::attachment_description(
        state->swap_chain_image_format,
        VK_ATTACHMENT_LOAD_OP_LOAD,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    
    create_info->attachment_descriptions[1] = utl::attachment_description(
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_ATTACHMENT_LOAD_OP_LOAD,
        VK_ATTACHMENT_STORE_OP_STORE,
        // VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    create_info->vertex_input_binding_count = 1;
    create_info->vertex_input_binding_descriptions[0] = utl::vertex_input_binding_description(
        0, sizeof(debug_line_vertex_t), VK_VERTEX_INPUT_RATE_VERTEX
    );
    create_info->vertex_input_attribute_count = 2;
    create_info->vertex_input_attribute_description[0] = utl::vertex_input_attribute_description(
        0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(debug_line_vertex_t, pos)
    );
    create_info->vertex_input_attribute_description[1] = utl::vertex_input_attribute_description(
        0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(debug_line_vertex_t, col)
    );

    create_info->descriptor_count = 1;
    create_info->descriptor_flags[0] = pipeline_state_t::create_info_t::DescriptorFlag_Uniform;
    create_info->descriptor_set_layout_bindings[0] = utl::descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1
    );

    state->create_pipeline_state_descriptors(pipeline, create_info);

    VkDescriptorBufferInfo buffer_info[1];
    buffer_info[0].buffer = scene_buffer;
    buffer_info[0].offset = 0;
    buffer_info[0].range = scene_buffer_size;

    create_info->write_descriptor_sets[0] = utl::write_descriptor_set(
        pipeline->descriptor_sets[0], 
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, buffer_info, array_count(buffer_info) // todo(zack): replace with 1
    );
    
    state->create_pipeline_state(pipeline, create_info);
    arena_set_mark(arena, stack_mark); 

    return pipeline;
}

};

#endif