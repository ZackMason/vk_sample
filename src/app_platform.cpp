#include "core.hpp"

#include <thread>
#include <semaphore>
#include <iostream>
#include <filesystem>

#if !_WIN32 
    #error "No Linux platform"
#else

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#undef near
#undef far

#endif

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


struct work_queue_t {
    struct job_t {
        work_queue_t* queue;
        void* job;
    };

    volatile u32 write{0};
    volatile u32 read{0};
    volatile u32 completed{0};

    job_t jobs[256];

    std::counting_semaphore<256> signal;
};

GLFWwindow* 
init_glfw(app_memory_t* app_mem) {
    glfwInit();

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
	
    auto* window = 
        glfwCreateWindow(app_mem->config.window_size[0], app_mem->config.window_size[1], "Vk App", 0, 0);

	app_mem->config.vk_exts = (char**)glfwGetRequiredInstanceExtensions (&app_mem->config.vk_ext_count);
	printf("\nFound %d GLFW Required Instance Extensions:\n", app_mem->config.vk_ext_count);
	for( uint32_t i = 0; i < app_mem->config.vk_ext_count; i++ ) {
		printf( "\t%s\n", app_mem->config.vk_exts[ i ] );
	}

    app_mem->config.window_handle = glfwGetWin32Window(window);

    assert(window);

    return window;
}

void
update_input(app_memory_t* app_mem, GLFWwindow* window) {
    const auto last_input = app_mem->input;
    app_input_reset(&app_mem->input);

    app_mem->input.time = (f32)(glfwGetTime());
    app_mem->input.dt = app_mem->input.time - last_input.time;

    for (int i = 0; i < array_count(app_mem->input.keys); i++) {
        if (i < array_count(app_mem->input.mouse.buttons)) {
            app_mem->input.mouse.buttons[i] = glfwGetMouseButton(window, i) == GLFW_PRESS;
        }
        app_mem->input.keys[i] = glfwGetKey(window, i) == GLFW_PRESS;
    }
}

void 
load_dlls(app_dll_t* app_dlls) {
    assert(app_dlls->dll);

    app_dlls->on_update = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_update")
        );
    app_dlls->on_init = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_init")
        );
    app_dlls->on_deinit = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_deinit")
        );

    assert(app_dlls->dll);
}

void 
reload_dlls(app_dll_t* app_dlls) {
    
    if (app_dlls->dll) {
#if _WIN32
        FreeLibrary((HMODULE)app_dlls->dll);
#else
#error "No Linux"
#endif
        app_dlls->dll = nullptr;
    }

    app_dlls->dll = 

#if _WIN32
        LoadLibraryA(".\\build\\app_build.dll");
#else

#endif
    
    assert(app_dlls->dll);

    load_dlls(app_dlls);
}

void
update_dlls(app_dll_t* app_dlls) {
    bool need_reload = false;
    while (std::filesystem::exists("./build/lock.tmp")) {
        need_reload = true;
        if (app_dlls->dll) {
            FreeLibrary((HMODULE)app_dlls->dll);
            app_dlls->dll = nullptr;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (need_reload) {
        reload_dlls(app_dlls);
        gen_warn("win32", "Game DLL has reloaded");
    }
}

int 
main() {
    gen_info("win32", "Loading Platform Layer");
    app_memory_t app_mem{};
    app_mem.malloc = malloc;
    app_mem.free = free;
    app_mem.realloc = realloc;
    app_mem.perm_memory_size = gigabytes(4); // megabytes(256*2);
    app_mem.perm_memory = 

#if _WIN32
        VirtualAlloc(0, app_mem.perm_memory_size,  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else 

#endif

    assert(app_mem.perm_memory);

    auto& config = app_mem.config;
    config.window_size[0] = 640;
    config.window_size[1] = 480;

    app_mem.config.create_vk_surface = [](void* instance, void* surface, void* window_handle) {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = (HWND)window_handle;
        createInfo.hinstance = GetModuleHandle(nullptr);
        
        // if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        if (vkCreateWin32SurfaceKHR(*(VkInstance*)instance, &createInfo, nullptr, (VkSurfaceKHR*)surface) != VK_SUCCESS) {
            gen_error("vulkan", "failed to create window surface!");
            std::terminate();
        }
    };

    app_dll_t app_dlls;
    reload_dlls(&app_dlls);

    GLFWwindow* window = init_glfw(&app_mem);

    gen_info("win32", "Platform Initialization Completed");

    app_dlls.on_init(&app_mem);
    while(app_mem.running && !glfwWindowShouldClose(window)) {
        update_dlls(&app_dlls);
        
        if (app_dlls.on_update) {
            app_dlls.on_update(&app_mem);
        }

        glfwPollEvents();
        update_input(&app_mem, window);
        if (app_mem.input.keys[256]) {
            app_mem.running = false;
        }
    };
    app_dlls.on_deinit(&app_mem);


    gen_info("win32", "Closing Application");
    return 0;
}