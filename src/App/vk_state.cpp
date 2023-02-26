// Author: Zackery Mason-Blaug
// Date: 2023-01-04

#include "App/vk_state.hpp"

#include <vendor/stb/stb_image.h>

#include <fstream>
#include <optional>
#include <span>
#include <set>


namespace gfx::vul {

namespace utl {
std::string error_string(VkResult errorCode) {
    switch (errorCode) {
#define STR(r) case VK_ ##r: return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
    default:
        return "UNKNOWN_ERROR";
    }
}
};

VkSampleCountFlagBits get_max_usable_sample_count(VkPhysicalDevice gpu_device) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(gpu_device, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

uint32_t find_memory_type(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

// note(zack): these params are switched compared to vk tut
u32
find_memory_by_flag_and_type(VkPhysicalDevice gpu_device, u32 memoryFlagBits, u32 memoryTypeBits)
{
	VkPhysicalDeviceMemoryProperties	vpdmp;
	vkGetPhysicalDeviceMemoryProperties( gpu_device, &vpdmp );
	for( u32 i = 0; i < vpdmp.memoryTypeCount; i++ ) {
		VkMemoryType vmt = vpdmp.memoryTypes[i];
		VkMemoryPropertyFlags vmpf = vmt.propertyFlags;
		if( ( memoryTypeBits & (1<<i) ) != 0 )
		{
			if( ( vmpf & memoryFlagBits ) != 0 )
			{
				gen_info("vk", "\n***** Found given memory flag ({}) and type ({}): i = {} *****\n", memoryFlagBits, memoryTypeBits, i );
				return i;
			}
		}
	}

	gen_error("vk", "\n***** Could not find given memory flag ({}) and type ({}) *****\n", memoryFlagBits, memoryTypeBits);
	
	return (u32)~0;
}

inline int
find_memory_that_is_host_visable(VkPhysicalDevice gpu_device, uint32_t memoryTypeBits) {
	return find_memory_by_flag_and_type(
        gpu_device, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        memoryTypeBits 
    );
}
inline int
find_memory_that_is_device_local(VkPhysicalDevice gpu_device, uint32_t memoryTypeBits) {
	return find_memory_by_flag_and_type(
        gpu_device, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        memoryTypeBits 
    );
}

struct queue_family_indices_t {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> compute_family;
    std::optional<uint32_t> present_family;

    [[nodiscard]] bool is_complete() const {
        return graphics_family.has_value() && present_family.has_value() && compute_family.has_value();
    }
};

queue_family_indices_t find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    queue_family_indices_t indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.compute_family = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.present_family = i;
        }

        if(indices.is_complete()) { break; }
        i++;
    }

    return indices;
}

struct swapchain_support_details_t {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

VkSurfaceFormatKHR chooseSwapSurfaceFormat(std::span<VkSurfaceFormatKHR> availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(std::span<VkPresentModeKHR> availablePresentModes) {
    return VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

swapchain_support_details_t querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    swapchain_support_details_t details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.present_modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.present_modes.data());
    }


    return details;
}


namespace internal {
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        // Message is important enough to show
        gen_error("vk::validation", "Validation Layer: {}", pCallbackData->pMessage);
        std::terminate();
    }

    return VK_FALSE;
}
    


#if NDEBUG
    constexpr bool enable_validation = false;
#else
    constexpr bool enable_validation = true;
#endif
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        // VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
        VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        //"VK_NV_shader_module_validation_cache"
    };

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pCallback);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, callback, pAllocator);
        }
    }

    bool check_validation_layer_support() {
        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
        gen_info("vulkan", "Layer count: {}", layerCount);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;
            gen_info("vulkan", "Layer name: {}", layerName);

            for (const auto& layerProperties : availableLayers) {
                gen_info("vulkan", "\t- Layer prop: {}", layerProperties.layerName);
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    return true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> get_required_extensions(bool enableValidationLayers, app_config_t* info) {
        std::vector<const char*> extensions(info->vk_exts, info->vk_exts + info->vk_ext_count);
        
        u32 numExtensionsAvailable;
		vkEnumerateInstanceExtensionProperties(nullptr, &numExtensionsAvailable, nullptr);
		auto* instance_extensions = new VkExtensionProperties[ numExtensionsAvailable ];
		vkEnumerateInstanceExtensionProperties(nullptr, &numExtensionsAvailable, instance_extensions);
		
		
		gen_info("vk::ext", "{} Instance Extensions actually available:", numExtensionsAvailable);
		for (u32 i = 0; i < numExtensionsAvailable; i++) {
			gen_info("vk::ext", "{:X}  '{}'", 
                instance_extensions[i].specVersion,
				instance_extensions[i].extensionName);
		}
        delete [] instance_extensions;

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // extensions.push_back(VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME);
        // extensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);

        for (const auto& e : extensions) {
            gen_info("vulkan", "loading extension: {}", e);
        }

        return extensions;
    }
};

VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void state_t::init(app_config_t* info) {
    create_instance(info);
    create_debug_messenger();
    create_surface(info);
    gen_info("vulkan", "created surface.");
    find_device();
    create_logical_device();
    gen_info("vulkan", "created device.");

    create_swap_chain(info->window_size[0], info->window_size[1]);
    gen_info("vulkan", "created swap chain.");

    create_depth_stencil_image(&depth_stencil_texture, info->window_size[0], info->window_size[1]);

    create_image_views();
    create_command_pool();

    create_uniform_buffer(&sporadic_uniform_buffer);

    
    arena_t t;
    t.start = new std::byte[2048*2048*4];
    t.size = 2048*2048*4;
    load_texture_sampler(&null_texture, "./assets/textures/null.png", &t);
    // load_texture_sampler(&null_texture, "./assets/textures/checker.png", &t);
    delete [] t.start;

    create_image(info->window_size[0], info->window_size[1], 1, 
        msaa_samples, swap_chain_image_format, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &color_buffer_texture);

    color_buffer_texture.image_view = create_image_view(device, color_buffer_texture.image, swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    create_descriptor_set_pool();

    create_command_buffer();
    
    create_sync_objects();

    gen_info("vulkan", "init finished");
}

void 
state_t::copy_buffer(
    VkBuffer srcBuffer, 
    VkBuffer dstBuffer, 
    VkDeviceSize size
) {
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    end_single_time_commands(commandBuffer);
}


VkCommandBuffer 
state_t::begin_single_time_commands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void 
state_t::end_single_time_commands(VkCommandBuffer cmd_buffer) {
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd_buffer;

    vkQueueSubmit(gfx_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(gfx_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &cmd_buffer);
}

struct quick_cmd_raii_t {
    state_t* _s;
    VkCommandBuffer command_buffer;

    auto& c() { return command_buffer; }

    quick_cmd_raii_t(state_t* gfx) 
        : _s{gfx}, command_buffer{gfx->begin_single_time_commands()}
    {
    }
    virtual ~quick_cmd_raii_t() {
        _s->end_single_time_commands(command_buffer);
    }
};


void 
begin_render_pass(
    state_t& state,
    VkRenderPass p_render_pass,
    VkFramebuffer framebuffer,
    VkCommandBuffer command_buffer, u32 index
) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = p_render_pass;
    renderPassInfo.framebuffer = framebuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = state.swap_chain_extent;

    VkClearColorValue			vccv;
		vccv.float32[0] = 0.0f;
		vccv.float32[1] = 0.0f;
		vccv.float32[2] = 0.0f;
		vccv.float32[3] = 1.0f;

	VkClearDepthStencilValue		vcdsv;
		vcdsv.depth = 1.f;
		vcdsv.stencil = 0;

    VkClearValue clearColor[2];
    clearColor[0].color = vccv;
    clearColor[1].depthStencil = vcdsv;

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;
    
    vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // set viewport and scissor because of dynamic pipeline state
    VkViewport viewport{vul::utl::viewport(
        static_cast<float>(state.swap_chain_extent.width),
        static_cast<float>(state.swap_chain_extent.height),
        0.0f,
        1.0f
    )};

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = state.swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}    

void state_t::record_pipeline_commands(
    VkPipeline p_pipeline, 
    VkRenderPass p_render_pass,
    VkFramebuffer framebuffer,
    VkCommandBuffer buffer, u32 index, std::function<void(void)> user_fn
) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = p_render_pass;
    renderPassInfo.framebuffer = framebuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swap_chain_extent;

    VkClearColorValue			vccv;
		vccv.float32[0] = 0.0f;
		vccv.float32[1] = 0.0f;
		vccv.float32[2] = 0.0f;
		vccv.float32[3] = 1.0f;

	VkClearDepthStencilValue		vcdsv;
		vcdsv.depth = 1.f;
		vcdsv.stencil = 0;

    VkClearValue clearColor[2];
    clearColor[0].color = vccv;
    clearColor[1].depthStencil = vcdsv;

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;
    
    vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline);

    // set viewport and scissor because of dynamic pipeline state
    VkViewport viewport{vul::utl::viewport(
        static_cast<float>(swap_chain_extent.width),
        static_cast<float>(swap_chain_extent.height),
        0.0f,
        1.0f
    )};

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    user_fn();

    vkCmdEndRenderPass(command_buffer);
}

void state_t::create_command_buffer() {

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &command_buffer) != VK_SUCCESS) {
        gen_error("vulkan", "failed to allocate command buffers!");
        std::terminate();
    }
}

void state_t::create_sync_objects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &image_available_semaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &render_finished_semaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &in_flight_fence[0]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &in_flight_fence[1]) != VK_SUCCESS
    ) {
        gen_error("vulkan", "failed to create semaphores!");
        std::terminate();
    }

}


void state_t::create_command_pool() {

    auto queueFamilyIndices =  find_queue_families(gpu_device, surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphics_family.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &command_pool) != VK_SUCCESS) {
        gen_error("vulkan", "failed to create command pool!");
        std::terminate();
    }
}

void state_t::create_image_views() {
    swap_chain_image_views.resize(swap_chain_images.size());
    
    for (size_t i = 0; i < swap_chain_images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swap_chain_images[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swap_chain_image_format;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS) {
        gen_error("vulkan", "failed to create image views!");
            std::terminate();
        }
    }
}

void state_t::create_swap_chain(int width, int height) {
    auto swapChainSupport = querySwapChainSupport(gpu_device, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.present_modes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, width, height);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices = find_queue_families(gpu_device, surface);
    uint32_t queueFamilyIndices[] = {indices.graphics_family.value(), indices.present_family.value()};

    if (indices.graphics_family != indices.present_family) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    vkCreateSwapchainKHR(device, &createInfo, nullptr, &swap_chain);

    swap_chain_image_format = surfaceFormat.format;
    swap_chain_extent = extent;
    
    vkGetSwapchainImagesKHR(device, swap_chain, &imageCount, nullptr);
    swap_chain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swap_chain, &imageCount, swap_chain_images.data());
}

void state_t::create_surface(app_config_t* info) {

    info->create_vk_surface(&instance, &surface, info->window_handle);

    // VkWin32SurfaceCreateInfoKHR createInfo{};
    // createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    // createInfo.hwnd = (HWND)info->window_handle;
    // createInfo.hinstance = GetModuleHandle(nullptr);
    
    // // if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
    // if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
    //     gen_error("vulkan", "failed to create window surface!");
    //     std::terminate();
    // }
}

void state_t::create_debug_messenger() {
    // IS_VK_FUNCT;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};

    internal::populateDebugMessengerCreateInfo(createInfo);
    
    if (internal::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debug_messenger) != VK_SUCCESS) {
        gen_error("vulkan", "failed to set up debug messenger!");
        std::terminate();
    }
}

void state_t::create_instance(app_config_t* info) {
    if (internal::enable_validation && !internal::check_validation_layer_support()) {
        assert(0  && "Failed to provide validation layers");
        gen_error("vulkan", "Failed to provide validation layers");
        std::terminate();
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1,0,0);
    app_info.pEngineName = "AAGE";
    app_info.engineVersion = VK_MAKE_VERSION(1,0,0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    auto extensions = internal::get_required_extensions(internal::enable_validation, info);
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
  
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if constexpr (internal::enable_validation) {
        create_info.enabledLayerCount = static_cast<uint32_t>(internal::validationLayers.size());
        create_info.ppEnabledLayerNames = internal::validationLayers.data();

        internal::populateDebugMessengerCreateInfo(debugCreateInfo);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        create_info.enabledLayerCount = 0;
    }

    VK_OK(vkCreateInstance(&create_info, nullptr, &instance));

    gen_info("vulkan", "{} extensions loaded", extensions.size());

    gen_info("vulkan", "instance created");
}


bool check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    gen_info("vulkan", "extensions available: - {}", availableExtensions.size());
    

    std::set<std::string> requiredExtensions(internal::deviceExtensions.begin(), internal::deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        //fmt::print("\t{}\n", extension.extensionName);

        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool is_device_poggers(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    auto indices = find_queue_families(device, surface);

    bool extensionsSupported = check_device_extension_support(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        auto swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
    }

    

    return  swapChainAdequate && extensionsSupported && indices.is_complete() &&
        deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader;
}

void state_t::find_device() {
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0) {
        gen_error("vulkan", "No Physical Device Found, You Must Be Poor LOL");
        std::terminate();
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for(const auto& phys_device: devices) {
        if (is_device_poggers(phys_device, surface)) {
            gpu_device = phys_device;
            msaa_samples = get_max_usable_sample_count(gpu_device);
            break;
        }
    }

    VkPhysicalDeviceProperties vpdp{};
    vkGetPhysicalDeviceProperties(gpu_device, &vpdp);

    gen_info("vk::device", "Selecting device: {}", vpdp.deviceName);

    if (gpu_device == VK_NULL_HANDLE) {
        gen_error("vulkan", "Failed to find a poggers gpu, oh no");
        std::terminate();
    }
}

void state_t::create_logical_device() {
    auto indices = find_queue_families(gpu_device, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphics_family.value(), indices.compute_family.value(), indices.present_family.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos =  queueCreateInfos.data();
    createInfo.queueCreateInfoCount = safe_truncate_u64(queueCreateInfos.size());

    createInfo.pEnabledFeatures = &device_features;

    createInfo.enabledExtensionCount = safe_truncate_u64(internal::deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = internal::deviceExtensions.data();
    

    if (internal::enable_validation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(internal::validationLayers.size());
        createInfo.ppEnabledLayerNames = internal::validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(gpu_device, &createInfo, nullptr, &device) != VK_SUCCESS) {
        gen_error("vulkan", "failed to create logical device!");
        std::terminate();
    } else {
        gen_info("vulkan", "created logical device");
    }

    graphics_index = indices.graphics_family.value();

    vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &gfx_queue);
    vkGetDeviceQueue(device, indices.compute_family.value(), 0, &compute_queue);
    vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

void state_t::destroy_instance() {
    vkDestroyInstance(instance, 0);
}

void state_t::cleanup() {
    vkDestroyImage(device, null_texture.image, nullptr);
    vkFreeMemory(device, null_texture.vdm, nullptr);

    vkDestroySemaphore(device, image_available_semaphore, nullptr);
    vkDestroySemaphore(device, render_finished_semaphore, nullptr);
    vkDestroyFence(device, in_flight_fence[0], nullptr);
    vkDestroyFence(device, in_flight_fence[1], nullptr);
    
    vkDestroyCommandPool(device, command_pool, nullptr);

    for (auto image_view : swap_chain_image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(device, swap_chain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);

    if constexpr (internal::enable_validation) {
        internal::DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }
    destroy_instance();
}

static std::vector<char> read_bin_file(std::string_view filename) {
    std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        gen_error("vulkan", "failed to open file!");
        assert(0);
    }
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

VkShaderModule create_shader_module(VkDevice device, std::span<char> code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        gen_error("vulkan", "failed to create shader module!");
    }
    return shaderModule;
}

VkResult 
state_t::fill_data_buffer(gpu_buffer_t* buffer, void* data, size_t size) {
    void* gpu_ptr;
	vkMapMemory(device, buffer->vdm, 0, VK_WHOLE_SIZE, 0, &gpu_ptr);	// 0 and 0 are offset and flags
	std::memcpy(gpu_ptr, data, size);
	vkUnmapMemory( device, buffer->vdm );
	return VK_SUCCESS;
}

VkResult 
state_t::fill_data_buffer(gpu_buffer_t* buffer, void* data) {
	return fill_data_buffer(buffer, data, (size_t)buffer->size);
}

VkResult
state_t::create_data_buffer(
    VkDeviceSize size, 
    VkBufferUsageFlags usage, 
    gpu_buffer_t* buffer
) {
	VkResult result = VK_SUCCESS;

	VkBufferCreateInfo  vbci;
		vbci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vbci.pNext = nullptr;
		vbci.flags = 0;
		vbci.size = size;
		vbci.usage = usage;
		vbci.queueFamilyIndexCount = 0;
		vbci.pQueueFamilyIndices = (const uint32_t *)nullptr;
		vbci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// can only use CONCURRENT if .queueFamilyIndexCount > 0

	buffer->size = size;
	result = vkCreateBuffer ( device, &vbci, 0, &buffer->buffer);
	
	VkMemoryRequirements			vmr;
	vkGetBufferMemoryRequirements( device, buffer->buffer, &vmr );		// fills vmr

	#if !NDEBUG
		gen_info("vk::create_data_buffer", "Buffer vmr.size = {}", vmr.size );
		gen_info("vk::create_data_buffer", "Buffer vmr.alignment = {}", vmr.alignment );
		gen_info("vk::create_data_buffer", "Buffer vmr.memoryTypeBits = {}", vmr.memoryTypeBits );
    #endif

	VkMemoryAllocateInfo			vmai;
		vmai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vmai.pNext = nullptr;
		vmai.allocationSize = vmr.size;
		vmai.memoryTypeIndex = find_memory_that_is_host_visable(gpu_device, vmr.memoryTypeBits );

	VkDeviceMemory				vdm;
	result = vkAllocateMemory( device, &vmai, 0, &vdm );
	
	buffer->vdm = vdm;

	result = vkBindBufferMemory( device, buffer->buffer, vdm, 0 );		// 0 is the offset

	return result;
}

void 
state_t::create_depth_stencil_image(
    texture_2d_t* texture,
    int width, 
    int height
) {
	VkExtent3D ve3d = { (u32)width, (u32)height, 1 };

	VkImageCreateInfo			vici;
		vici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		vici.pNext = nullptr;
		vici.flags = 0;
		vici.imageType = VK_IMAGE_TYPE_2D;
		vici.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		vici.extent = ve3d;
		vici.mipLevels = 1;
		vici.arrayLayers = 1;
		vici.samples = VK_SAMPLE_COUNT_1_BIT;
		vici.tiling = VK_IMAGE_TILING_OPTIMAL;
		vici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		vici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vici.queueFamilyIndexCount = 0;
		vici.pQueueFamilyIndices = (const uint32_t *)nullptr;
		vici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	vkCreateImage( device, &vici, 0, &texture->image);

	VkMemoryRequirements			vmr;
	vkGetImageMemoryRequirements( device, texture->image, &vmr );

	VkMemoryAllocateInfo			vmai;
		vmai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vmai.pNext = nullptr;
		vmai.allocationSize = vmr.size;
		vmai.memoryTypeIndex = find_memory_that_is_device_local( gpu_device, vmr.memoryTypeBits );

	VkDeviceMemory imageMemory;
	vkAllocateMemory( device, &vmai, 0, &imageMemory );
	
	vkBindImageMemory( device, texture->image, imageMemory, 0 );	// 0 is the offset

	VkImageViewCreateInfo			vivci;
		vivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vivci.pNext = nullptr;
		vivci.flags = 0;
		vivci.image = texture->image;
		vivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		vivci.format = vici.format;
		vivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		vivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		vivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		vivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		vivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		vivci.subresourceRange.baseMipLevel = 0;
		vivci.subresourceRange.levelCount = 1;
		vivci.subresourceRange.baseArrayLayer = 0;
		vivci.subresourceRange.layerCount = 1;

    VK_OK(vkCreateImageView( device, &vivci, 0, &texture->image_view));

    texture->size[0] = width;
    texture->size[1] = height;
}

void 
state_t::create_pipeline_state_descriptors(
    pipeline_state_t* pipeline, 
    pipeline_state_t::create_info_t* create_info
) {
    VkDescriptorPoolSize				vdps[8];
		for (u32 i = 0; i < create_info->descriptor_count; i++) {
            vdps[i].type = (VkDescriptorType)create_info->descriptor_flags[i];            
            vdps[i].descriptorCount = 1; // note(zack): not sure what this is for tbh
        }

    VkDescriptorSetLayoutCreateInfo			vdslc[8];
		for (u32 i = 0; i < create_info->descriptor_count; i++) {
            vdslc[i].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            vdslc[i].pNext = nullptr;
            vdslc[i].flags = 0;
            vdslc[i].bindingCount = 1;
            vdslc[i].pBindings = create_info->descriptor_set_layout_bindings + i;
        }
    pipeline->descriptor_count = create_info->descriptor_count;
    
    for (u32 i = 0; i < create_info->descriptor_count; i++) {
        VK_OK(vkCreateDescriptorSetLayout(device, vdslc + i, 0, pipeline->descriptor_set_layouts + i));
    }
    VkDescriptorSetAllocateInfo vdsai;
        vdsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        vdsai.pNext = nullptr;
        vdsai.descriptorPool = descriptor_pool;
        vdsai.descriptorSetCount = create_info->descriptor_count;
        vdsai.pSetLayouts = pipeline->descriptor_set_layouts;

    VK_OK(vkAllocateDescriptorSets(device, &vdsai,  pipeline->descriptor_sets));
}

void 
state_t::create_pipeline_state(
    pipeline_state_t* pipeline, 
    pipeline_state_t::create_info_t* create_info
) {
    u32 copy_count = 0;
    for (size_t i = 0; i < create_info->descriptor_count; i++) {
        vkUpdateDescriptorSets(device, 1, create_info->write_descriptor_sets + i, copy_count, nullptr);
    }

    VkAttachmentReference color_attachment_refs[6];
    VkAttachmentReference depth_attachment_refs[6];
    u32 color_count = 0;
    u32 depth_count = 0;
    for (u32 i = 0; i < create_info->attachment_count; i++) {
        if (create_info->attachment_descriptions[i].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            depth_attachment_refs[depth_count].attachment = i;
            depth_attachment_refs[depth_count].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_count++;
        } else {
            color_attachment_refs[color_count].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_attachment_refs[color_count].attachment = i;
            color_count++;
        }
    }
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subpass.colorAttachmentCount = color_count;
    subpass.pColorAttachments = color_attachment_refs;

    assert(depth_count <= 1);
    subpass.pDepthStencilAttachment = depth_count == 1 ? depth_attachment_refs : 0;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = create_info->attachment_count;
    renderPassInfo.pAttachments = create_info->attachment_descriptions;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    VkSubpassDependency dependency{};
    VkSubpassDependency depth_dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depth_dependency.dstSubpass = 0;
    depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.srcAccessMask = 0;
    depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency deps[] = {dependency, depth_dependency};

    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = deps;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &pipeline->render_passes[0]) != VK_SUCCESS) {
        gen_error("vulkan", "failed to create render pass!");
        std::terminate();
    }

    pipeline->render_pass_count++;

    // create pipeline
    auto vertShaderCode = read_bin_file(create_info->vertex_shader);
    auto fragShaderCode = read_bin_file(create_info->fragment_shader);
    
    VkShaderModule vertShaderModule = create_shader_module(device, vertShaderCode);
    VkShaderModule fragShaderModule = create_shader_module(device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = array_count(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;


    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = create_info->vertex_input_binding_count;
    vertexInputInfo.pVertexBindingDescriptions = create_info->vertex_input_binding_descriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = create_info->vertex_input_attribute_count;
    vertexInputInfo.pVertexAttributeDescriptions = create_info->vertex_input_attribute_description;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = create_info->topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swap_chain_extent.width;
    viewport.height = (float) swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = create_info->fill_mode;
    rasterizer.lineWidth = create_info->line_width;
    rasterizer.cullMode = create_info->cull_mode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = create_info->msaa_samples;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // colorBlendAttachment.blendEnable = VK_FALSE;
    // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;   

    VkStencilOpState					vsosf;	// front
		vsosf.failOp = VK_STENCIL_OP_KEEP;
		vsosf.passOp = VK_STENCIL_OP_KEEP;
		vsosf.depthFailOp = VK_STENCIL_OP_KEEP;
        vsosf.compareOp = VK_COMPARE_OP_NEVER;
		vsosf.compareMask = ~0ui32;
		vsosf.writeMask = ~0ui32;
		vsosf.reference = 0;


    VkStencilOpState					vsosb;	// back
		vsosb.failOp = VK_STENCIL_OP_KEEP;
		vsosb.passOp = VK_STENCIL_OP_KEEP;
		vsosb.depthFailOp = VK_STENCIL_OP_KEEP;
		vsosb.compareOp = VK_COMPARE_OP_NEVER;
		vsosb.compareMask = ~0ui32;
		vsosb.writeMask = ~0ui32;
		vsosb.reference = 0;

    VkPipelineDepthStencilStateCreateInfo			vpdssci;
		vpdssci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		vpdssci.pNext = nullptr;
		vpdssci.flags = 0;
		vpdssci.depthTestEnable = VK_TRUE;
		vpdssci.depthWriteEnable = VK_TRUE;
		vpdssci.depthCompareOp = VK_COMPARE_OP_LESS; 

        vpdssci.depthBoundsTestEnable = VK_FALSE;
		vpdssci.front = vsosf;
		vpdssci.back  = vsosb;
		vpdssci.minDepthBounds = 0.f;
		vpdssci.maxDepthBounds = 1.f;
		vpdssci.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPushConstantRange vpcr{};
        vpcr.offset = 0;
	    vpcr.size = create_info->push_constant_size;
        vpcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = pipeline->descriptor_count;
    pipelineLayoutInfo.pSetLayouts = pipeline->descriptor_set_layouts;
    pipelineLayoutInfo.pushConstantRangeCount = !!create_info->push_constant_size;
    pipelineLayoutInfo.pPushConstantRanges = create_info->push_constant_size ? &vpcr : 0;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline->pipeline_layout) != VK_SUCCESS) {
        gen_error("vulkan", "failed to create pipeline layout!");
        std::terminate();
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &vpdssci; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = pipeline->pipeline_layout;

    pipelineInfo.renderPass = pipeline->render_passes[0];
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->pipeline) != VK_SUCCESS) {
        gen_error("vulkan", "failed to create graphics pipeline!");
        std::terminate();
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        VkImageView attachments[] = {
            swap_chain_image_views[i],
            depth_stencil_texture.image_view
        };

        VkFramebufferCreateInfo vfci{};
        vfci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        vfci.renderPass = pipeline->render_passes[0];
        vfci.attachmentCount = array_count(attachments);
        vfci.pAttachments = attachments;
        vfci.width = swap_chain_extent.width;
        vfci.height = swap_chain_extent.height;
        vfci.layers = 1;

        if (vkCreateFramebuffer(device, &vfci, nullptr, &pipeline->framebuffers[i]) != VK_SUCCESS) {
            gen_error("vulkan", "failed to create framebuffer!");
            std::terminate();
        }
    }
}

void 
state_t::copy_buffer_to_image(
    gpu_buffer_t* buffer, 
    texture_2d_t* texture
) {
    quick_cmd_raii_t c{this};

    VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0,0,0};
        region.imageExtent = {
            (u32)texture->size[0],
            (u32)texture->size[1],
            1
        };
        
    vkCmdCopyBufferToImage(
        c.c(),
        buffer->buffer,
        texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
}

void 
state_t::transistion_image_layout(
    texture_2d_t* texture, 
    VkImageLayout new_layout
) {
    quick_cmd_raii_t c{this};

    VkImageMemoryBarrier barrier;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = nullptr;

        barrier.oldLayout = texture->image_layout;
        barrier.newLayout = new_layout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = texture->image;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        barrier.srcAccessMask = 0; // todo(zack)
        barrier.dstAccessMask = 0; // todo(zack)

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    auto oldLayout = texture->image_layout;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        gen_error("vk::image_layout_transistion", "unsupported layout transition!");
        std::terminate();
    }

    vkCmdPipelineBarrier(
        c.c(),
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    texture->image_layout = new_layout;
}

// todo(zack): support multiple texture formats
void 
state_t::load_font_sampler(
    arena_t* arena,
    texture_2d_t* texture, 
    font_t* font
) {
    // need to convert R32 to RGBA
    const size_t image_size = font->pixel_count*font->pixel_count*4;
    u8* t_pix = (u8*)arena_alloc(arena, image_size);
    for (size_t i = 0; i < font->pixel_count * font->pixel_count; i++) {
        t_pix[4*i+0] = font->bitmap[i];
        t_pix[4*i+1] = 0;
        t_pix[4*i+2] = 0;
        t_pix[4*i+3] = 0;
    }
    texture->pixels = t_pix;
    texture->size[0] = texture->size[1] = font->pixel_count;

    VkSamplerCreateInfo vsci;
        vsci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        vsci.pNext = nullptr;
        vsci.flags = 0;
        vsci.magFilter = VK_FILTER_LINEAR;
        vsci.minFilter = VK_FILTER_LINEAR;
        vsci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vsci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vsci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vsci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        vsci.mipLodBias = 0;
        vsci.anisotropyEnable = VK_FALSE;
        vsci.maxAnisotropy = 1;
        vsci.compareEnable = VK_FALSE;
        vsci.compareOp = VK_COMPARE_OP_NEVER;
        vsci.minLod = 0.;
        vsci.maxLod = 0.;
        vsci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK; 
        vsci.unnormalizedCoordinates = VK_FALSE;

    VK_OK(vkCreateSampler(device, &vsci, 0, &texture->sampler));

    gpu_buffer_t staging_buffer;
    
    create_data_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &staging_buffer);

    void* gpu_ptr;
    vkMapMemory(device, staging_buffer.vdm, 0, image_size, 0, &gpu_ptr);
    std::memcpy(gpu_ptr, texture->pixels, image_size);
    vkUnmapMemory(device, staging_buffer.vdm);

    VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = static_cast<uint32_t>(texture->size[0]);
        image_info.extent.height = static_cast<uint32_t>(texture->size[1]);
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;

        image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.flags = 0;

        VK_OK(vkCreateImage(device, &image_info, nullptr, &texture->image));

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device, texture->image, &mem_req);

    VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = find_memory_by_flag_and_type(gpu_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_req.memoryTypeBits);
        
    VK_OK(vkAllocateMemory(device, &alloc_info, nullptr, &texture->vdm));

    vkBindImageMemory(device, texture->image, texture->vdm, 0);

    texture->image_layout = image_info.initialLayout;

    transistion_image_layout(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(&staging_buffer, texture);
    transistion_image_layout(texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, staging_buffer.buffer, nullptr);
    vkFreeMemory(device, staging_buffer.vdm, nullptr);

    VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture->image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

    VK_OK(vkCreateImageView(device, &viewInfo, nullptr, &texture->image_view));
}




void 
state_t::create_image(
    u32 w, u32 h, u32 mip_levels, 
    VkSampleCountFlagBits num_samples, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props,
    texture_2d_t* texture
) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = w;
    imageInfo.extent.height = h;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mip_levels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = num_samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &texture->image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, texture->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(gpu_device, memRequirements.memoryTypeBits, props);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &texture->vdm) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, texture->image, texture->vdm, 0);

    
}

void 
state_t::load_texture_sampler(
    texture_2d_t* texture, 
    std::string_view path, 
    arena_t* arena
) {
    gen_info("vk::load_texture_sampler", "Loading Texture:\n\t{}\n", path);

    stbi_set_flip_vertically_on_load(true);
    i32 channels = 0;
    u8* data = stbi_load(path.data(), texture->size + 0, texture->size + 1, &channels, 0);
    assert(data);
    const auto image_size = texture->size[0] * texture->size[1] * channels;
    texture->pixels = (u8*)arena_alloc(arena, image_size);
    std::memcpy(texture->pixels, data, image_size);
    stbi_image_free(data);
    gen_info("vk::load_texture_sampler", 
        "Texture Info:\n\twidth: {}\n\theight: {}\n\tchannels: {}", 
        texture->size[0], texture->size[1], channels    
    );

    VkSamplerCreateInfo vsci;
        vsci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        vsci.pNext = nullptr;
        vsci.flags = 0;
        vsci.magFilter = VK_FILTER_LINEAR;
        vsci.minFilter = VK_FILTER_LINEAR;
        vsci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vsci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vsci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vsci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        vsci.mipLodBias = 0;
        vsci.anisotropyEnable = VK_FALSE;
        vsci.maxAnisotropy = 1;
        vsci.compareEnable = VK_FALSE;
        vsci.compareOp = VK_COMPARE_OP_NEVER;
        vsci.minLod = 0.;
        vsci.maxLod = 0.;
        vsci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK; 
        vsci.unnormalizedCoordinates = VK_FALSE;

    VK_OK(vkCreateSampler(device, &vsci, 0, &texture->sampler));


    gpu_buffer_t staging_buffer;
    
    create_data_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &staging_buffer);

    void* gpu_ptr;
    vkMapMemory(device, staging_buffer.vdm, 0, image_size, 0, &gpu_ptr);
    std::memcpy(gpu_ptr, texture->pixels, image_size);
    vkUnmapMemory(device, staging_buffer.vdm);

    VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = static_cast<uint32_t>(texture->size[0]);
        image_info.extent.height = static_cast<uint32_t>(texture->size[1]);
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;

        image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.flags = 0;

        VK_OK(vkCreateImage(device, &image_info, nullptr, &texture->image));

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device, texture->image, &mem_req);

    VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = find_memory_by_flag_and_type(gpu_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_req.memoryTypeBits);
        
    VK_OK(vkAllocateMemory(device, &alloc_info, nullptr, &texture->vdm));

    vkBindImageMemory(device, texture->image, texture->vdm, 0);

    texture->image_layout = image_info.initialLayout;

    transistion_image_layout(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(&staging_buffer, texture);
    transistion_image_layout(texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, staging_buffer.buffer, nullptr);
    vkFreeMemory(device, staging_buffer.vdm, nullptr);

    VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture->image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

    VK_OK(vkCreateImageView(device, &viewInfo, nullptr, &texture->image_view));
}

void state_t::create_descriptor_set_pool() {

    VkDescriptorPoolSize				vdps[5];
		vdps[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vdps[0].descriptorCount = 1;

		vdps[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vdps[1].descriptorCount = 1;
        
		vdps[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vdps[2].descriptorCount = 1;

		vdps[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vdps[3].descriptorCount = 1;

		vdps[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		vdps[4].descriptorCount = 1;

    VkDescriptorPoolCreateInfo			vdpci;
		vdpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		vdpci.pNext = nullptr;
		vdpci.flags = 0;

        vdpci.maxSets = 400;
		vdpci.poolSizeCount = array_count(vdps);
		vdpci.pPoolSizes = &vdps[0];

	VK_OK(vkCreateDescriptorPool(device, &vdpci, 0, &descriptor_pool));
	gen_info("vk", "vkCreateDescriptorPool");

}


VkDescriptorSet 
state_t::create_image_descriptor_set(
    VkDescriptorPool p_descriptor_pool,
    VkDescriptorSetLayout p_descriptor_set_layout,
    texture_2d_t** texture,
    size_t count
) {
    VkDescriptorSet descriptor_set;

    VkDescriptorSetAllocateInfo vdsai;
        vdsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        vdsai.pNext = nullptr;
        vdsai.descriptorPool = p_descriptor_pool;
        vdsai.descriptorSetCount = 1;
        vdsai.pSetLayouts = &p_descriptor_set_layout;

    VK_OK(vkAllocateDescriptorSets(device, &vdsai, &descriptor_set));

    VkDescriptorImageInfo vdii[64];
        for (size_t i = 0; i < count; i++) {
            vdii[i].sampler = texture[i]->sampler;
            vdii[i].imageView = texture[i]->image_view;
            vdii[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    
    VkWriteDescriptorSet vwds[1];
        vwds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vwds[0].pNext = nullptr;
        vwds[0].dstSet = descriptor_set;
        vwds[0].dstBinding = 0;
        vwds[0].dstArrayElement = 0;
        vwds[0].descriptorCount = safe_truncate_u64(count);
        vwds[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        vwds[0].pBufferInfo = nullptr;
        vwds[0].pImageInfo = vdii;
        vwds[0].pTexelBufferView = nullptr;

    u32 copy_count = 0;
    vkUpdateDescriptorSets(device, 1, vwds, copy_count, nullptr);

    return descriptor_set;
}

};