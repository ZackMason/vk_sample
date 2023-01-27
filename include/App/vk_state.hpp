// Author: Zackery Mason-Blaug
// Date: 2023-01-04

#pragma once

#include "core.hpp"

#include <vulkan\vulkan.h>

#include <vector>
#include <functional>

#define VK_OK(res) (res)

using VkDataBuffer = VkBuffer;


struct GLFWwindow;

namespace gfx::vul {

struct gpu_buffer_t {
	VkDataBuffer		buffer;
	VkDeviceMemory		vdm;
	VkDeviceSize		size;
};

using uniform_buffer_t = gpu_buffer_t;
using vertex_buffer_t = gpu_buffer_t;
using index_buffer_t = gpu_buffer_t;

struct texture_2d_t {
    i32 size[2];
    u8* pixels;
    VkImage image;
    VkImageView image_view;
    VkImageLayout image_layout;
    VkSampler sampler;
    VkDeviceMemory vdm;
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

    std::vector<VkFramebuffer> swap_chain_framebuffers;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
 
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;

    uniform_buffer_t scene_uniform_buffer;
    uniform_buffer_t object_uniform_buffer;
    uniform_buffer_t sporadic_uniform_buffer;

    texture_2d_t null_texture;

    VkDebugUtilsMessengerEXT debug_messenger;

    void init(app_config_t* info);
    void cleanup();

    void record_command_buffer(VkCommandBuffer buffer, u32 index, std::function<void(void)> user_fn);

    void create_instance(app_config_t* info);
    void create_debug_messenger();
    void create_surface(app_config_t* info);
    void find_device();
    void create_logical_device();
    void create_swap_chain(int width, int height);
    void create_image_views();
    void create_graphics_pipeline();
    void create_render_pass();
    void create_framebuffers();
    void create_command_pool();
    void create_command_buffer();
    void create_sync_objects();
    void create_descriptor_set_pool();
    void create_descriptor_set_layouts();
    void create_descriptor_sets();

    void transistion_image_layout(texture_2d_t* texture, VkImageLayout new_layout);

    void copy_buffer_to_image(gpu_buffer_t* buffer, texture_2d_t* texture);
    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);


    void load_texture_sampler(texture_2d_t* texture, std::string_view path, arena_t* arena);

    VkResult create_data_buffer(VkDeviceSize size, VkBufferUsageFlags usage, gpu_buffer_t* buffer);
    VkResult create_uniform_buffer(VkDeviceSize size, uniform_buffer_t* buffer);
    VkResult create_vertex_buffer(VkDeviceSize size, vertex_buffer_t* buffer);
    VkResult create_index_buffer(VkDeviceSize size, index_buffer_t* buffer);

    VkResult fill_data_buffer(uniform_buffer_t* buffer, void* data);
    VkResult fill_data_buffer(uniform_buffer_t* buffer, void* data, size_t size);

    void destroy_instance();
};

using index_t = u32;

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
};

};