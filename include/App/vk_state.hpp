// Author: Zackery Mason-Blaug
// Date: 2023-01-04

#pragma once

#include "core.hpp"

#include <vulkan\vulkan.h>

#include <vector>
#include <functional>

#define VK_OK(res) do { const auto r = (res); if ((r) != VK_SUCCESS) { gen_error("vk", gfx::vul::utl::error_string(r)); std::terminate(); } } while(0)

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

template <typename T, size_t N>
struct storage_buffer_t : public gpu_buffer_t {
    using gpu_buffer_t::gpu_buffer_t;
    utl::pool_t<T> pool{};
};

struct texture_2d_t {
    i32 size[2];
    i32 channels{3};
    u8* pixels;
    VkImage image;
    VkImageView image_view;
    VkImageLayout image_layout;
    VkSampler sampler;
    VkDeviceMemory vdm;
};

struct cube_map_t {
    u32 size[2];
    VkImage image;
    VkImageView view;
    VkImageView face_views[6];
    VkImageLayout image_layout;
    VkSampler sampler;
    VkDeviceMemory memory;
};

struct object_buffer_t {
    v4f color;
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

struct framebuffer_attachment_t {
    framebuffer_attachment_t* next{0};
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkFormat format;
    VkImageSubresourceRange subresource_range;
    VkAttachmentDescription description;

    void destroy(VkDevice device) {
        vkDestroyImageView(device, view, nullptr);
        vkDestroyImage(device, image, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }

        bool has_depth() {
			switch (format)	{
				case VK_FORMAT_D16_UNORM:
				case VK_FORMAT_X8_D24_UNORM_PACK32:
				case VK_FORMAT_D32_SFLOAT:
				case VK_FORMAT_D16_UNORM_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
                    return true;
                default: break;
			};
			return false;
		}

		bool has_stencil() {
			switch (format)	{
				case VK_FORMAT_S8_UINT:
				case VK_FORMAT_D16_UNORM_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
                    return true;
                default: break;
			};
			return false;
		}
};

struct framebuffer_t {
    u32 width, height;
    VkFramebuffer framebuffer;
    framebuffer_attachment_t* attachments{0};
    VkRenderPass render_pass;
    VkSampler sampler;
};

struct framebuffer_attachment_create_info_t {
    u32 width, height;
    u32 layer=1;
    VkFormat format;
    VkImageUsageFlags usage;
    VkSampleCountFlagBits image_sample_count = VK_SAMPLE_COUNT_1_BIT;
};

struct debug_line_vertex_t {
    v3f pos{};
    v3f col{};
};

struct compute_pipeline_state_t {
    struct create_info_t {
        std::string_view shader{};
        u32 descriptor_count{0};
    };
    VkPipeline          pipeline;
    VkPipelineLayout    pipeline_layout;
};

struct pipeline_state_t {
    struct create_info_t {
        enum DescriptorFlag {
            DescriptorFlag_Uniform          = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            // DescriptorFlag_DynamicUniform   = VK_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
            DescriptorFlag_Storage          = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            DescriptorFlag_Sampler          = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            DescriptorFlag_Invalid,
        };

        std::string_view vertex_shader{};
        std::string_view fragment_shader{};

        VkPrimitiveTopology     topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        VkPolygonMode           fill_mode{VK_POLYGON_MODE_FILL};
        VkCullModeFlagBits      cull_mode{VK_CULL_MODE_NONE};
        // VkAttachmentDescription attachment_descriptions[6];
        // u32                     attachment_count{2};
        bool                        write_depth{true};
        bool                        test_depth{true};

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

        f32                     line_width{1.0f};
        VkSampleCountFlagBits   msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    };

    VkPipeline          pipeline;
    VkPipelineLayout    pipeline_layout;
    // VkRenderPass        render_passes[8];
    // u32                 render_pass_count{1};
    // VkFramebuffer*      framebuffers{0};
    // u32                 framebuffer_count{0};

    // descriptors
    u32                     descriptor_count{0};
    VkDescriptorSetLayout   descriptor_set_layouts[5];
    VkDescriptorSet         descriptor_sets[5];
};

inline void
end_render_pass(
    VkCommandBuffer cmd
) {
    vkCmdEndRenderPass(cmd);
}

struct state_t {
    VkInstance instance;
    VkPhysicalDevice gpu_device;
    VkDevice device; // logical device in mb sample

    VkPhysicalDeviceFeatures device_features{};
    VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT;

    u32 graphics_index;

    VkQueue gfx_queue;
    VkQueue compute_queue;
    VkQueue present_queue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swap_chain;

    std::vector<VkImage> swap_chain_images;
    std::vector<VkImageView> swap_chain_image_views;

    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;

    VkDescriptorPool descriptor_pool;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer[2];
 
    VkSemaphore image_available_semaphore[2];
    VkSemaphore render_finished_semaphore[2];
    VkFence in_flight_fence[2];

    uniform_buffer_t<sporadic_buffer_t> sporadic_uniform_buffer;

    texture_2d_t null_texture;
    texture_2d_t depth_stencil_texture;
    texture_2d_t color_buffer_texture;

    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDeviceShaderObjectFeaturesEXT enabled_shader_object_features_EXT{};
	VkPhysicalDeviceDynamicRenderingFeaturesKHR enabled_dynamic_rendering_features_KHR{};
        
    struct khr_functions_t {
        // VK_EXT_shader_objects requires render passes to be dynamic
        PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
        PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;
    } khr;

    struct ext_functions_t {
        PFN_vkCreateShadersEXT vkCreateShadersEXT;
        PFN_vkCmdBindShadersEXT vkCmdBindShadersEXT;
        PFN_vkGetShaderBinaryDataEXT vkGetShaderBinaryDataEXT;
        // With VK_EXT_shader_object pipeline state must be set at command buffer creation using these functions
        // VK_EXT_dynamic_state
        PFN_vkCmdSetViewportWithCountEXT vkCmdSetViewportWithCountEXT;
        PFN_vkCmdSetScissorWithCountEXT vkCmdSetScissorWithCountEXT;
        PFN_vkCmdSetDepthCompareOpEXT vkCmdSetDepthCompareOpEXT;
        PFN_vkCmdSetCullModeEXT vkCmdSetCullModeEXT;
        PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT;
        PFN_vkCmdSetDepthWriteEnableEXT vkCmdSetDepthWriteEnableEXT;
        PFN_vkCmdSetFrontFaceEXT vkCmdSetFrontFaceEXT;
        PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT;
        // VK_EXT_vertex_input_dynamic_state
        PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInputEXT;
    } ext;

    void init(app_config_t* info, arena_t* temp_arena);
    void cleanup();

    void record_command_buffer(VkCommandBuffer buffer, u32 index, std::function<void(void)> user_fn);
    void record_pipeline_commands(
        VkPipeline pipeline, VkRenderPass render_pass, VkFramebuffer framebuffer,
        VkCommandBuffer buffer, u32 index, std::function<void(void)> user_fn);

    void create_instance(app_config_t* info);
    void create_debug_messenger();
    void create_surface(app_config_t* info);
    void find_device(arena_t* arena);
    void create_logical_device();
    void create_swap_chain(int width, int height, bool vsync);
    void create_image_views();
    void create_depth_stencil_image(texture_2d_t* texture, int width, int height);
    
    void create_command_pool();
    void create_command_buffer();
    void create_sync_objects();
    
    void create_descriptor_set_pool();

    void transistion_image_layout(texture_2d_t* texture, VkImageLayout new_layout);

    void copy_buffer_to_image(gpu_buffer_t* buffer, texture_2d_t* texture);
    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void create_cube_map(cube_map_t* cube_map, VkFormat format);

    void create_framebuffer(framebuffer_t* fb);
    void add_framebuffer_attachment(arena_t* arena, framebuffer_t* fb, framebuffer_attachment_create_info_t fbaci);
    VkResult create_framebuffer_sampler(framebuffer_t* fb, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode);
    VkResult framebuffer_create_renderpass(framebuffer_t* fb);
    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);

    VkDescriptorSet create_image_descriptor_set(
        VkDescriptorPool descriptor_pool,
        VkDescriptorSetLayout descriptor_set_layout,
        texture_2d_t** texture,
        size_t count
    );

    void load_texture(texture_2d_t* texture, std::string_view path, arena_t* arena);
    void load_texture_sampler(texture_2d_t* texture, arena_t* arena);
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

    // todo(zack): need to select proper memory type
    template <typename T, size_t N>
    VkResult create_storage_buffer(storage_buffer_t<T, N>* buffer) {
        const auto r = create_data_buffer(sizeof(T) * N, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer);
        void* gpu_ptr{0};
        vkMapMemory(device, buffer->vdm, 0, sizeof(T) * N, 0, &gpu_ptr);
        buffer->pool = utl::pool_t<T>{(T*)gpu_ptr, N};
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
    VkResult fill_data_buffer(gpu_buffer_t* buffer, void* data);
    VkResult fill_data_buffer(gpu_buffer_t* buffer, void* data, size_t size);

    void create_pipeline_state(pipeline_state_t* pipeline, pipeline_state_t::create_info_t* create_info, VkRenderPass render_pass);
    void create_pipeline_state_descriptors(pipeline_state_t* pipeline, pipeline_state_t::create_info_t* create_info);

    void destroy_instance();

    void create_image(
        u32 w, u32 h, u32 mip_levels, 
        VkSampleCountFlagBits num_samples, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props,
        texture_2d_t* texture
    );
};

void 
begin_render_pass(
    state_t& state,
    VkRenderPass p_render_pass,
    VkFramebuffer framebuffer,
    VkCommandBuffer command_buffer
);

namespace utl {

inline VkCommandPoolCreateInfo 
command_pool_create_info() {
    VkCommandPoolCreateInfo cmdPoolCreateInfo {};
    cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    return cmdPoolCreateInfo;
}

inline VkCommandBufferBeginInfo command_buffer_begin_info(

) {
    VkCommandBufferBeginInfo cmdBufferBeginInfo {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    return cmdBufferBeginInfo;
}

inline VkCommandBufferAllocateInfo command_buffer_allocate_info(
    VkCommandPool commandPool, 
    uint32_t bufferCount)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = bufferCount;
    return commandBufferAllocateInfo;
}

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
inline VkImageMemoryBarrier image_memory_barrier()
{
    VkImageMemoryBarrier imageMemoryBarrier {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    return imageMemoryBarrier;
}
inline VkMemoryAllocateInfo memory_allocate_info()
{
    VkMemoryAllocateInfo memAllocInfo {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    return memAllocInfo;
}

inline VkImageCreateInfo image_create_info()
{
    VkImageCreateInfo imageCreateInfo {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    return imageCreateInfo;
}

inline VkSamplerCreateInfo sampler_create_info()
{
    VkSamplerCreateInfo samplerCreateInfo {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    return samplerCreateInfo;
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

inline VkImageViewCreateInfo image_view_create_info()
{
    VkImageViewCreateInfo imageViewCreateInfo {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    return imageViewCreateInfo;
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

inline void
buffer_host_memory_barrier(
    VkCommandBuffer command_buffer,
    VkBuffer buffer
) {
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.buffer = buffer;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    
    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        0, 0, nullptr, 1, &barrier, 0, nullptr
    );
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
void insert_image_memory_barrier(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkImageSubresourceRange subresourceRange)
{
    VkImageMemoryBarrier imageMemoryBarrier = image_memory_barrier();
    imageMemoryBarrier.srcAccessMask = srcAccessMask;
    imageMemoryBarrier.dstAccessMask = dstAccessMask;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    vkCmdPipelineBarrier(
        cmdbuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);
}

std::string error_string(VkResult errorCode);




}; // namespace utl

// extensions
void create_shader_objects_from_files(
    arena_t arena, state_t& state,
    VkDescriptorSetLayout* descriptor_set_layout,
    u32 descriptor_set_layout_count,
    u32 push_constant_size,
    const VkShaderStageFlagBits* const stages,
    const VkShaderStageFlagBits* const next_stages,
    const char** const filenames,
    const u32 shader_count,
    VkShaderEXT* const shaders /* out */,
    SpvReflectShaderModule* const reflection = 0 /* out */
);


}; // namespace gfx::vul