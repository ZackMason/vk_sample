// Author: Zackery Mason-Blaug
// Date: 2023-01-04

#pragma once

#include "core.hpp"

#include <vulkan\vulkan.h>

#include <vector>
#include <functional>

#define VK_OK(res) do { if (res != VK_SUCCESS) { gen_error("vk", "Vulkan Failed"); std::terminate(); } } while(0)

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

    struct gui_pipeline_t {
        VkPipeline pipeline;
        VkPipelineLayout pipeline_layout;
        VkFramebuffer framebuffer;
        std::vector<VkFramebuffer> swap_framebuffers;
        VkRenderPass render_pass;

        // descriptor for samplers
        VkDescriptorPool descriptor_pool;
        VkDescriptorSetLayout descriptor_set_layouts[1];
        VkDescriptorSet descriptor_sets[1];
    } gui_pipeline;

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
    void create_gui_pipeline();
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

    void destroy_instance();
};


};