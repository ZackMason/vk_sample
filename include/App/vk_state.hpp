// Author: Zackery Mason-Blaug
// Date: 2023-01-04

#pragma once

#include "core.hpp"

#include <vulkan\vulkan.h>

#include <vector>
#include <functional>

#define VK_OK(res) do { const auto r = (res); if ((r) != VK_SUCCESS) { gen_error("vk", utl::error_string(r)); std::terminate(); } } while(0)

using VkDataBuffer = VkBuffer;


struct GLFWwindow;

namespace gfx::vul {

struct gpu_buffer_t {
	VkDataBuffer		buffer;
	VkDeviceMemory		vdm;
	VkDeviceSize		size;
    virtual ~gpu_buffer_t() = default;
};

template <typename T>
struct uniform_buffer_t : public gpu_buffer_t {
    using gpu_buffer_t::gpu_buffer_t;
    T* data{nullptr};
};

template <typename T, size_t N>
struct vertex_buffer_t : public gpu_buffer_t {
    using gpu_buffer_t::gpu_buffer_t;
    utl::pool_t<T> pool{};
};


template <size_t N>
struct index_buffer_t : public gpu_buffer_t {
    using gpu_buffer_t::gpu_buffer_t;
    utl::pool_t<u32> pool{};
};


struct texture_2d_t {
    i32 size[2];
    u8* pixels;
    VkImage image;
    VkImageView image_view;
    VkImageLayout image_layout;
    VkSampler sampler;
    VkDeviceMemory vdm;
};

struct object_buffer_t {
    m44     model;
    v4f     color;
    float   shininess;
};

struct object_push_constants_t {
    m44         model;
    material_t  material;
};

struct scene_buffer_t {
    m44         proj;
    m44         view;
    m44         scene;
    v4f         light_pos;
    v4f         light_col;
    v4f         light_KaKdKs;
    float       time;
};

struct sporadic_buffer_t {
    int mode;
    int use_lighting;
    int num_instances;
    int padd;
};

struct pipeline_state_t {
    struct create_info_t {
        enum DescriptorFlag {
            DescriptorFlag_Uniform,
            DescriptorFlag_Sampler,
            DescriptorFlag_Invalid,
        };

        std::string_view vertex_shader{};
        std::string_view fragment_shader{};

        VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        VkPolygonMode       fill_mode{VK_POLYGON_MODE_FILL};
        VkCullModeFlagBits  cull_mode{VK_CULL_MODE_NONE};
        VkAttachmentDescription attachment_descriptions[6];
        u32                  attachment_count{2};

        VkVertexInputBindingDescription     vertex_input_binding_descriptions[8];
        u32                                 vertex_input_binding_count{0};
        VkVertexInputAttributeDescription   vertex_input_attribute_description[8];
        u32                                 vertex_input_attribute_count{0};

        u32                                 descriptor_count{0};
        DescriptorFlag                      descriptor_flags[8];
        VkDescriptorSetLayoutBinding        descriptor_set_layout_bindings[8];
        VkWriteDescriptorSet                write_descriptor_sets[8];

        u32                   push_constant_size{0};

        VkViewport          viewport{};
        VkRect2D            scissor{};

        f32                 line_width{1.0f};
    };

    VkPipeline          pipeline;
    VkPipelineLayout    pipeline_layout;
    VkRenderPass        render_passes[8];
    u32                 render_pass_count{1};
    VkFramebuffer*      framebuffers{0};
    u32              framebuffer_count{0};

    // descriptors
    u32                  descriptor_count{};
    VkDescriptorSetLayout   descriptor_set_layouts[5];
    VkDescriptorSet         descriptor_sets[5];
};

struct state_t {
    VkInstance instance;
    VkPhysicalDevice gpu_device;
    VkDevice device; // logical device in mb sample

    VkPhysicalDeviceFeatures device_features{};

    VkQueue gfx_queue;
    VkQueue present_queue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swap_chain;

    std::vector<VkImage> swap_chain_images;
    std::vector<VkImageView> swap_chain_image_views;

    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;

    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layouts[4];
    VkDescriptorSet descriptor_sets[4];

    std::vector<VkFramebuffer> world_framebuffers;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
 
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence[2];

    uniform_buffer_t<scene_buffer_t>    scene_uniform_buffer;
    uniform_buffer_t<object_buffer_t>   object_uniform_buffer;
    uniform_buffer_t<sporadic_buffer_t> sporadic_uniform_buffer;

    texture_2d_t null_texture;
    texture_2d_t depth_stencil_texture;

    VkDebugUtilsMessengerEXT debug_messenger;

    void init(app_config_t* info);
    void cleanup();

    void record_command_buffer(VkCommandBuffer buffer, u32 index, std::function<void(void)> user_fn);
    void record_pipeline_commands(
        VkPipeline pipeline, VkRenderPass render_pass, VkFramebuffer framebuffer,
        VkCommandBuffer buffer, u32 index, std::function<void(void)> user_fn, bool should_clear = true);

    void create_instance(app_config_t* info);
    void create_debug_messenger();
    void create_surface(app_config_t* info);
    void find_device();
    void create_logical_device();
    void create_swap_chain(int width, int height);
    void create_image_views();
    void create_depth_stencil_image(texture_2d_t* texture, int width, int height);
    void create_graphics_pipeline();
    void create_render_pass();
    void create_framebuffers();
    void create_command_pool();
    void create_command_buffer();
    void create_sync_objects();
    
    void create_gui_descriptor_sets();
    void create_descriptor_set_pool();
    void create_descriptor_set_layouts();
    void create_descriptor_sets();

    void transistion_image_layout(texture_2d_t* texture, VkImageLayout new_layout);

    void copy_buffer_to_image(gpu_buffer_t* buffer, texture_2d_t* texture);
    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);

    VkDescriptorSet create_image_descriptor_set(
        VkDescriptorPool descriptor_pool,
        VkDescriptorSetLayout descriptor_set_layout,
        texture_2d_t** texture,
        size_t count
    );

    void load_texture_sampler(texture_2d_t* texture, std::string_view path, arena_t* arena);
    void load_font_sampler(arena_t* arena, texture_2d_t* texture, font_t* font);

    template <typename T>
    VkResult create_uniform_buffer(uniform_buffer_t<T>* buffer) {
        const auto r = create_data_buffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, buffer);
        void* gpu_ptr{0};
        vkMapMemory(device, buffer->vdm, 0, sizeof(T), 0, &gpu_ptr);
        buffer->data = (T*)gpu_ptr;
        return r;
    }

    template <typename T, size_t N>
    VkResult create_vertex_buffer(vertex_buffer_t<T, N>* buffer) {
        const auto r = create_data_buffer(sizeof(T) * N, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, buffer);
        void* gpu_ptr{0};
        vkMapMemory(device, buffer->vdm, 0, sizeof(T) * N, 0, &gpu_ptr);
        buffer->pool = utl::pool_t<T>{(T*)gpu_ptr, N};
        return r;
    }

    template <size_t N>
    VkResult create_index_buffer(index_buffer_t<N>* buffer) {
        const auto r = create_data_buffer(sizeof(u32) * N, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, buffer);
        void* gpu_ptr{0};
        vkMapMemory(device, buffer->vdm, 0, sizeof(u32) * N, 0, &gpu_ptr);
        buffer->pool = utl::pool_t<u32>{(u32*)gpu_ptr, N};
        return r;
    }

    VkResult create_data_buffer(VkDeviceSize size, VkBufferUsageFlags usage, gpu_buffer_t* buffer);
    // VkResult create_uniform_buffer(VkDeviceSize size, uniform_buffer_t* buffer);
    // VkResult create_vertex_buffer(VkDeviceSize size, vertex_buffer_t* buffer);
    // VkResult create_index_buffer(VkDeviceSize size, index_buffer_t* buffer);

    VkResult fill_data_buffer(gpu_buffer_t* buffer, void* data);
    VkResult fill_data_buffer(gpu_buffer_t* buffer, void* data, size_t size);

    void create_pipeline_state(pipeline_state_t* pipeline, pipeline_state_t::create_info_t* create_info);
    void create_pipeline_state_descriptors(pipeline_state_t* pipeline, pipeline_state_t::create_info_t* create_info);

    void destroy_instance();
};

namespace utl {


inline VkPipelineLayoutCreateInfo pipeline_layout_create_info(
    const VkDescriptorSetLayout* pSetLayouts,
    uint32_t setLayoutCount = 1)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
    pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
    return pipelineLayoutCreateInfo;
}

inline VkDescriptorSetLayoutBinding descriptor_set_layout_binding(
    VkDescriptorType type,
    VkShaderStageFlags stageFlags,
    uint32_t binding,
    uint32_t descriptorCount = 1)
{
    VkDescriptorSetLayoutBinding setLayoutBinding {};
    setLayoutBinding.descriptorType = type;
    setLayoutBinding.stageFlags = stageFlags;
    setLayoutBinding.binding = binding;
    setLayoutBinding.descriptorCount = descriptorCount;
    return setLayoutBinding;
}

inline VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(
    const VkDescriptorSetLayoutBinding* pBindings,
    uint32_t bindingCount)
{
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pBindings = pBindings;
    descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
    return descriptorSetLayoutCreateInfo;
}

inline VkDescriptorSetAllocateInfo descriptor_set_allocate_info(
    VkDescriptorPool descriptorPool,
    const VkDescriptorSetLayout* pSetLayouts,
    uint32_t descriptorSetCount)
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
    descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
    return descriptorSetAllocateInfo;
}

inline VkDescriptorImageInfo descriptor_image_info(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
{
    VkDescriptorImageInfo descriptorImageInfo {};
    descriptorImageInfo.sampler = sampler;
    descriptorImageInfo.imageView = imageView;
    descriptorImageInfo.imageLayout = imageLayout;
    return descriptorImageInfo;
}

inline VkDescriptorImageInfo descriptor_image_info(texture_2d_t* texture) {
    return descriptor_image_info(texture->sampler, texture->image_view, texture->image_layout);
}

inline VkVertexInputBindingDescription vertex_input_binding_description(
    uint32_t binding,
    uint32_t stride,
    VkVertexInputRate inputRate)
{
    VkVertexInputBindingDescription vInputBindDescription {};
    vInputBindDescription.binding = binding;
    vInputBindDescription.stride = stride;
    vInputBindDescription.inputRate = inputRate;
    return vInputBindDescription;
}

inline VkVertexInputAttributeDescription vertex_input_attribute_description(
    uint32_t binding,
    uint32_t location,
    VkFormat format,
    uint32_t offset)
{
    VkVertexInputAttributeDescription vInputAttribDescription {};
    vInputAttribDescription.location = location;
    vInputAttribDescription.binding = binding;
    vInputAttribDescription.format = format;
    vInputAttribDescription.offset = offset;
    return vInputAttribDescription;
}

inline VkGraphicsPipelineCreateInfo pipeline_create_info(
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    VkPipelineCreateFlags flags = 0)
{
    VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = layout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.flags = flags;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    return pipelineCreateInfo;
}

inline VkWriteDescriptorSet write_descriptor_set(
    VkDescriptorSet dstSet,
    VkDescriptorType type,
    uint32_t binding,
    VkDescriptorBufferInfo* bufferInfo,
    uint32_t descriptorCount = 1)
{
    VkWriteDescriptorSet writeDescriptorSet {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pBufferInfo = bufferInfo;
    writeDescriptorSet.descriptorCount = descriptorCount;
    return writeDescriptorSet;
}

inline VkWriteDescriptorSet write_descriptor_set(
    VkDescriptorSet dstSet,
    VkDescriptorType type,
    uint32_t binding,
    VkDescriptorImageInfo *imageInfo,
    uint32_t descriptorCount = 1)
{
    VkWriteDescriptorSet writeDescriptorSet {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pImageInfo = imageInfo;
    writeDescriptorSet.descriptorCount = descriptorCount;
    return writeDescriptorSet;
}

inline VkComputePipelineCreateInfo compute_pipeline_create_info(
    VkPipelineLayout layout, 
    VkPipelineCreateFlags flags = 0)
{
    VkComputePipelineCreateInfo computePipelineCreateInfo {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = layout;
    computePipelineCreateInfo.flags = flags;
    return computePipelineCreateInfo;
}

inline VkPushConstantRange push_constant_range(
    VkShaderStageFlags stageFlags,
    uint32_t size,
    uint32_t offset)
{
    VkPushConstantRange pushConstantRange {};
    pushConstantRange.stageFlags = stageFlags;
    pushConstantRange.offset = offset;
    pushConstantRange.size = size;
    return pushConstantRange;
}

inline VkAttachmentDescription attachment_description(
    VkFormat                        format,
    VkAttachmentLoadOp              loadOp,
    VkAttachmentStoreOp             storeOp,
    VkImageLayout                   initialLayout,
    VkImageLayout                   finalLayout
) {
    VkAttachmentDescription vad{};
    vad.format = format;
    vad.samples = VK_SAMPLE_COUNT_1_BIT;

    vad.loadOp = loadOp;
    vad.storeOp = storeOp;

    vad.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vad.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    vad.initialLayout = initialLayout;
    vad.finalLayout = finalLayout;
    vad.flags = 0;
    return vad;
}
inline VkSubmitInfo submit_info()
{
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    return submitInfo;
}

inline VkViewport viewport(
    float width,
    float height,
    float minDepth,
    float maxDepth)
{
    VkViewport viewport {};
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    return viewport;
}

inline VkRect2D rect2D(
    int32_t width,
    int32_t height,
    int32_t offsetX,
    int32_t offsetY)
{
    VkRect2D rect2D {};
    rect2D.extent.width = width;
    rect2D.extent.height = height;
    rect2D.offset.x = offsetX;
    rect2D.offset.y = offsetY;
    return rect2D;
}

std::string error_string(VkResult errorCode);

}; // namespace utl

pipeline_state_t*
create_gui_pipeline(arena_t* arena, state_t* state) {
    pipeline_state_t* pipeline = arena_alloc_ctor<pipeline_state_t>(arena, 1);

    pipeline->framebuffer_count = safe_truncate_u64(state->world_framebuffers.size());
    pipeline->framebuffers = arena_alloc_ctor<VkFramebuffer>(arena, state->world_framebuffers.size());

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

    create_info->descriptor_count = 1;
    create_info->descriptor_flags[0] = pipeline_state_t::create_info_t::DescriptorFlag_Sampler;
    create_info->descriptor_set_layout_bindings[0] = utl::descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 32
    );

    state->create_pipeline_state_descriptors(pipeline, create_info);

    VkDescriptorImageInfo vdii[32];
    for (size_t i = 0; i < array_count(vdii); i++) {
        vdii[i].sampler = state->null_texture.sampler;
        vdii[i].imageView = state->null_texture.image_view;
        vdii[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    create_info->write_descriptor_sets[0] = utl::write_descriptor_set(
        pipeline->descriptor_sets[0], 
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, vdii, 32
    );
    
    state->create_pipeline_state(pipeline, create_info);
    arena_set_mark(arena, stack_mark); 

    return pipeline;
}


};