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

#define USE_SDL 1

#if USE_SDL
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_mixer.h"
#endif

struct audio_cache_t {
    #if USE_SDL
    Mix_Chunk* sounds[1024];
    #endif
    u64 sound_count{0};
};

void __cdecl load_sound(const char* path) {

}

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
    app_input_t last_input;
    std::memcpy(&last_input, &app_mem->input, sizeof(app_input_t));

    app_input_reset(&app_mem->input);

    app_mem->input.time = (f32)(glfwGetTime());
    app_mem->input.dt = app_mem->input.time - last_input.time;

    f64 mouse_temp[2];
    glfwGetCursorPos(window, mouse_temp+0, mouse_temp+1);
    f32 mouse[2]; 
    mouse[0] = f32(mouse_temp[0]);
    mouse[1] = f32(mouse_temp[1]);

    app_mem->input.mouse.pos[0] = mouse[0];
    app_mem->input.mouse.pos[1] = mouse[1];

    app_mem->input.mouse.delta[0] = mouse[0] - last_input.mouse.pos[0];
    app_mem->input.mouse.delta[1] = mouse[1] - last_input.mouse.pos[1];

    for (int i = 0; i < array_count(app_mem->input.keys); i++) {
        if (i < array_count(app_mem->input.mouse.buttons)) {
            app_mem->input.mouse.buttons[i] = glfwGetMouseButton(window, i) == GLFW_PRESS;
        }

        if (i <= GLFW_JOYSTICK_LAST && i < array_count(app_mem->input.gamepads)) {
            const int present = glfwJoystickPresent(i);
            auto& gamepad = app_mem->input.gamepads[i];
            if (present) {
                GLFWgamepadstate state;
                if (glfwJoystickIsGamepad(i) && glfwGetGamepadState(i, &state)) {
                    loop_itoa(a, GLFW_GAMEPAD_AXIS_LAST + 1) {
                        if (std::fabs(state.axes[a]) < 0.1f) { // deadzone
                            state.axes[a] = 0.0f;
                        }
                        // gen_info("input", "axis[{}]:{}", a, axes_[a]);
                    }
                                    
                    gamepad.left_stick.x    = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
                    gamepad.left_stick.y    = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
                    gamepad.right_stick.x   = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
                    gamepad.right_stick.y   = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
                    gamepad.left_trigger    = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.5f ? 1.0f : 0.0f;
                    gamepad.right_trigger   = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.5f ? 1.0f : 0.0f;

                    gamepad.action_down.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_CROSS];
                    gamepad.action_right.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_CIRCLE];
                    gamepad.action_up.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_TRIANGLE];
                    gamepad.action_left.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_SQUARE];
                    
                    gamepad.dpad_down.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
                    gamepad.dpad_right.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
                    gamepad.dpad_up.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
                    gamepad.dpad_left.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];

                    gamepad.options.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_BACK];
                    gamepad.start.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_START];

                    gamepad.shoulder_left.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
                    gamepad.shoulder_right.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];

                    gamepad.guide.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE];

                    gamepad.left_thumb.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
                    gamepad.right_thumb.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];

                    gamepad.guide.is_held = state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE];

                } else {
                    gen_warn("input", "Joystick is not gamepad");
                }
            } else {
                gamepad = {};
            }
            gamepad.is_connected = present;
        }

        app_mem->input.keys[i] = glfwGetKey(window, i) == GLFW_PRESS;
        app_mem->input.pressed.keys[i] = app_mem->input.keys[i] && !last_input.keys[i];
        app_mem->input.released.keys[i] = !app_mem->input.keys[i] && last_input.keys[i];

        loop_itoa(g, array_count(app_mem->input.gamepads)) {
            loop_itoa(b, array_count(app_mem->input.gamepads[g].buttons)) {
                auto& button = app_mem->input.gamepads[g].buttons[b];
                button.is_pressed = !last_input.gamepads[g].buttons[b].is_held && button.is_held;
                button.is_released = last_input.gamepads[g].buttons[b].is_held && !button.is_held;
            }
        }
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

    // std::filesystem::copy(".\\build\\app_build.dll", ".\\build\\app_build_temp.dll");

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
    bool need_reload = std::filesystem::exists("./build/lock.tmp");
    if (need_reload) {
        utl::profile_t p{"dll reload time"};

        while (std::filesystem::exists("./build/lock.tmp")) {
            gen_warn("win32", "Game DLL Reload Detected");
            need_reload = true;
            if (app_dlls->dll) {
                while(FreeLibrary((HMODULE)app_dlls->dll)==0) {
                    gen_error("dll", "Error Freeing Library: {}", GetLastError());
                }
                app_dlls->dll = nullptr;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        reload_dlls(app_dlls);
        gen_warn("win32", "Game DLL has reloaded");
    }
}

int 
main(int argc, char* argv[]) {
    gen_info("win32", "Loading Platform Layer");
    app_memory_t app_mem{};
    app_mem.malloc = malloc;
    app_mem.free = free;
    app_mem.realloc = realloc;
    app_mem.perm_memory_size = gigabytes(4) + megabytes(32); // megabytes(256*2);
    app_mem.perm_memory = 

#if _WIN32
        VirtualAlloc(0, app_mem.perm_memory_size,  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else 
        
#endif



    assert(app_mem.perm_memory);

    auto& config = app_mem.config;
    config.window_size[0] = 800;
    config.window_size[1] = 600;

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

#if USE_SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        gen_info("sdl_mixer", "Couldn't initialize SDL: {}", SDL_GetError());
        return 255;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) < 0) {
        gen_info("sdl_mixer", "Couldn't open audio: {}", SDL_GetError());
        return 255;
    }
#endif

    audio_cache_t audio_cache{};
    utl::closure_t load_sound_closure;
    load_sound_closure.data = &audio_cache;
    load_sound_closure.func = reinterpret_cast<void(*)(void*)>(
        +[](void* data, const char* path) -> u64 {
            gen_info("sdl_mixer::load_sound", "Loading Sound: {}", path);

            auto* cache = (audio_cache_t*)data;
            #if USE_SDL
            cache->sounds[cache->sound_count] = Mix_LoadWAV(path);
            #endif

            gen_info("sdl_mixer::load_sound", "Sound Loaded: id = {}", cache->sound_count);
        
            return cache->sound_count++;
    });
    
    utl::closure_t play_sound_closure;
    play_sound_closure.data = &audio_cache;
    play_sound_closure.func = reinterpret_cast<void(*)(void*)>(
        +[](void* data, u64 id) -> void {
            gen_info("sdl_mixer::play_sound", "playing Sound: {}", id);

            auto* cache = (audio_cache_t*)data;

            #if USE_SDL
            Mix_PlayChannel(0, cache->sounds[id], 0);
            #endif
    });
    
    app_mem.audio.load_sound_closure = &load_sound_closure;
    app_mem.audio.play_sound_closure = &play_sound_closure;

    u64 sound = load_sound_closure.dispatch_request<u64>("./assets/audio/unlock.wav");
    gen_info("sdl_mixer::play_sound", "playing Sound: {}", sound);
    //play_sound_closure.dispatch(sound);

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

        if (app_mem.input.keys[256] || app_mem.input.gamepads[0].options.is_held) { // esc
            app_mem.running = false;
        }
    };
    app_dlls.on_deinit(&app_mem);


    gen_info("win32", "Closing Application");
    return 0;
}