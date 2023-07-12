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
#include "commdlg.h"
// #include "combaseapi.h"
// #include <Psapi.h>
// #include <winternl.h>
#undef near
#undef far

struct access_violation_exception : public std::exception {
    access_violation_exception(ULONG_PTR code_, void* address_) : code{code_}, address{address_} {}
    ULONG_PTR code;
    void* address;
    mutable std::string error;
    char const* what() const override final {
        error = "LOL YOU FUCKED UP.\n\n";
        switch(code) {
            case 0: error += "Access Violation (read).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 1: error += "Access Violation (write).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 8: error += "Access Violation (dep).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
        }
        error += fmt_str("\nTried to access virtual address {}", address);
        error += "\n\nPress retry to try to return to a previous save state\n\nGood luck";

        return error.c_str();
    }
};

void* win32_alloc(size_t size){
    return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}
template <typename T>
T* win32_alloc(){
    auto* t = (T*)win32_alloc(sizeof(T));
    new (t) T();
    return t;
}

app_memory_t* gs_app{0};
app_memory_t* gs_app_restore{0};
arena_t* gs_physics_arena;
arena_t* gs_physics_restore_arena;

FILETIME gs_game_dll_write_time;

FILETIME win32_last_write_time(const char* path){
	FILETIME time = {};
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (GetFileAttributesEx(path, GetFileExInfoStandard, &data))
		time = data.ftLastWriteTime;

	return time;
}

#endif

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// #define USE_SDL 1

#if USE_SDL
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_mixer.h"
#endif

#define MULTITHREAD_ENGINE

struct audio_cache_t {
    #if USE_SDL
    Mix_Chunk* sounds[1024];
    #endif
    u64 sound_count{0};
};

void 
create_meta_data_dir() {
    char* app_data_path;
    size_t path_size;
    const errno_t err = _dupenv_s(&app_data_path, &path_size, "APPDATA");
    if (err) {
        gen_error("save", "Get Env Error: {}, cant save", err);
        std::terminate(); // need to be warned of this
    }

    const std::string meta_path = fmt_str("{}/estella", std::string_view{app_data_path, path_size-1});
    
    gen_info("save", "Save File Path: {}", meta_path);
    if (!std::filesystem::exists(meta_path)) {
        gen_info("save", "Creating save directory");
        std::filesystem::create_directory(meta_path);
    }

    free(app_data_path);
}

void
load_graphics_config(app_memory_t* app_mem) {
    std::ifstream file{"gfx_config.bin", std::ios::binary};

    if(!file.is_open()) {
        gen_error("win32", "Failed to graphics config file");
        return;
    }

    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::byte* data = new std::byte[size];
    file.read((char*)data, size);
    utl::memory_blob_t loader{data};
    
    app_mem->config.graphics_config = loader.deserialize<app_config_t::graphic_config_t>();
    
    delete data;
}
void
save_graphics_config(app_memory_t* app_mem) {
    std::ofstream file{"./gfx_config.bin", std::ios::out | std::ios::binary};

    // std::byte* data = new std::byte[sizeof(app_config_t::graphic_config_t)];
    
    // utl::memory_blob_t loader{data};
    
    // loader.serialize(0, app_mem->config.graphics_config);

    file.write((const char*)&app_mem->config.graphics_config, sizeof(app_config_t::graphic_config_t));

    file.close();

    // delete data;

    gen_info("win32", "Saved graphics config");
}

GLFWwindow* 
init_glfw(app_memory_t* app_mem) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	
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
    
    loop_iota_i32(i, array_count(app_mem->input.keys)) {
        if (i < array_count(app_mem->input.mouse.buttons)) {
            app_mem->input.mouse.buttons[i] = glfwGetMouseButton(window, i) == GLFW_PRESS;
        }
        app_mem->input.keys[i] = glfwGetKey(window, i) == GLFW_PRESS;
        app_mem->input.pressed.keys[i] = app_mem->input.keys[i] && !last_input.keys[i];
        app_mem->input.released.keys[i] = !app_mem->input.keys[i] && last_input.keys[i];
    }

    loop_iota_i32(i, array_count(app_mem->input.gamepads)) {
        auto& gamepad = app_mem->input.gamepads[i];

        GLFWgamepadstate state;
        if (glfwJoystickIsGamepad(i) && glfwGetGamepadState(i, &state)) {
            gamepad.is_connected = true;
            loop_iota_u64(a, GLFW_GAMEPAD_AXIS_LAST + 1) {
                if (std::fabs(state.axes[a]) < 0.35f) { // deadzone
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
            
            gamepad.buttons[button_id::action_down].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_CROSS];
            gamepad.buttons[button_id::action_right].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_CIRCLE];
            gamepad.buttons[button_id::action_up].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_TRIANGLE];
            gamepad.buttons[button_id::action_left].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_SQUARE];

            gamepad.buttons[button_id::dpad_down].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
            gamepad.buttons[button_id::dpad_right].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
            gamepad.buttons[button_id::dpad_up].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
            gamepad.buttons[button_id::dpad_left].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];

            gamepad.buttons[button_id::options].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_BACK];
            gamepad.buttons[button_id::start].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_START];
            gamepad.buttons[button_id::guide].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE];

            gamepad.buttons[button_id::shoulder_left].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
            gamepad.buttons[button_id::shoulder_right].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];

            gamepad.buttons[button_id::left_thumb].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
            gamepad.buttons[button_id::right_thumb].is_held = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
        } else {
            // gen_warn("input", "Joystick is not gamepad");
            gamepad = {};
        }

    }

    loop_iota_u64(mb, array_count(app_mem->input.mouse.buttons)) {
        const u8 mouse_btn = app_mem->input.mouse.buttons[mb];
        const u8 last_mouse_btn = last_input.mouse.buttons[mb];
        app_mem->input.pressed.mouse_btns[mb] = !last_mouse_btn && mouse_btn;
        app_mem->input.released.mouse_btns[mb] = last_mouse_btn && !mouse_btn;
    }

    loop_iota_u64(g, array_count(app_mem->input.gamepads)) {
        loop_iota_u64(b, button_id::SIZE) {
            auto& button = app_mem->input.gamepads[g].buttons[b];
            button.is_pressed = !last_input.gamepads[g].buttons[b].is_held && button.is_held;
            button.is_released = last_input.gamepads[g].buttons[b].is_held && !button.is_held;
        }
    }
}

void 
load_dll_functions(app_dll_t* app_dlls) {
    assert(app_dlls->dll);

    app_dlls->on_update = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_update")
        );
    app_dlls->on_render = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_render")
        );
    app_dlls->on_init = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_init")
        );
    app_dlls->on_deinit = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_deinit")
        );
    app_dlls->on_unload = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_unload")
        );
    app_dlls->on_reload = 
        reinterpret_cast<app_func_t>(
            GetProcAddress((HMODULE)app_dlls->dll, "app_on_reload")
        );

    assert(app_dlls->dll);
}

void 
load_dlls(app_dll_t* app_dlls) {

    // std::filesystem::copy(".\\build\\app_build.dll", ".\\build\\app_build_temp.dll");

#if _WIN32
    CopyFile(".\\build\\app_build.dll", ".\\build\\game.dll", 0);
    app_dlls->dll = LoadLibrary(".\\build\\game.dll");
#else

#error "platform not imlemented"

#endif
    
    assert(app_dlls->dll);

    load_dll_functions(app_dlls);
}

void
update_dlls(app_dll_t* app_dlls, app_memory_t* app_mem) {
    bool need_reload = !std::filesystem::exists("./build/lock.tmp");
    if (need_reload) {
        utl::profile_t p{"dll reload time"};
        gen_warn("win32", "Game DLL Reload Detected");
        app_dlls->on_unload(app_mem);

        FreeLibrary((HMODULE)app_dlls->dll);
        *app_dlls={};

        load_dlls(app_dlls);

        if (app_dlls->dll == 0) {
            gen_error("win32", "Failed to load game");
        }

        gen_warn("win32", "Game DLL has reloaded");
        app_dlls->on_reload(app_mem);

        gs_game_dll_write_time = win32_last_write_time(".\\build\\app_build.dll");
    }
}

LONG exception_filter(_EXCEPTION_POINTERS* exception_info) {
    auto [
        code,
        flags,
        record,
        address,
        param_count,
        info
    ] = *exception_info->ExceptionRecord;

    if (code == EXCEPTION_BREAKPOINT) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    std::string msg;
    switch(code) {
        case EXCEPTION_ACCESS_VIOLATION: msg = "Access Violation.\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: msg = "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking."; break;
        case EXCEPTION_BREAKPOINT: msg = "A breakpoint was encountered."; break;
        case EXCEPTION_DATATYPE_MISALIGNMENT: msg = "The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on."; break;
        case EXCEPTION_FLT_DENORMAL_OPERAND: msg = "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value."; break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: msg = "The thread tried to divide a floating-point value by a floating-point divisor of zero."; break;
        case EXCEPTION_FLT_INEXACT_RESULT: msg = "The result of a floating-point operation cannot be represented exactly as a decimal fraction."; break;
        case EXCEPTION_FLT_INVALID_OPERATION: msg = "This exception represents any floating-point exception not included in this list."; break;
        case EXCEPTION_FLT_OVERFLOW: msg = "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type."; break;
        case EXCEPTION_FLT_STACK_CHECK: msg = "The stack overflowed or underflowed as the result of a floating-point operation."; break;
        case EXCEPTION_FLT_UNDERFLOW: msg = "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type."; break;
        case EXCEPTION_ILLEGAL_INSTRUCTION: msg = "The thread tried to execute an invalid instruction."; break;
        case EXCEPTION_IN_PAGE_ERROR: msg = "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network."; break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO: msg = "The thread tried to divide an integer value by an integer divisor of zero."; break;
        case EXCEPTION_INT_OVERFLOW: msg = "The result of an integer operation caused a carry out of the most significant bit of the result."; break;
        case EXCEPTION_INVALID_DISPOSITION: msg = "An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception."; break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: msg = "The thread tried to continue execution after a noncontinuable exception occurred."; break;
        case EXCEPTION_PRIV_INSTRUCTION: msg = "The thread tried to execute an instruction whose operation is not allowed in the current machine mode."; break;
        case EXCEPTION_SINGLE_STEP: msg = "A trace trap or other single-instruction mechanism signaled that one instruction has been executed."; break;
        case EXCEPTION_STACK_OVERFLOW: msg = "The thread used up its stack."; break;
    }

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        switch(info[0]) {
            case 0: msg = "Access Violation (read).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 1: msg = "Access Violation (write).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 8: msg = "Access Violation (dep).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
        }

        msg += fmt::format("\nAttempted to access virtual address {}", (void*)info[1]);



    }
    if (code == EXCEPTION_IN_PAGE_ERROR) {
        switch(info[0]) {
            case 0: msg = "Page Violation (read).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 1: msg = "Page Violation (write).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 8: msg = "Page Violation (dep).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
        }

        msg += fmt::format("\nAttempted to access virtual address {}", (void*)info[1]);
        auto pressed = MessageBox(0, fmt::format("Exception at address {}\nCode: {}\nFlag: {}", address, msg!=""?msg:fmt::format("{}", code), flags).c_str(), 0, MB_ABORTRETRYIGNORE);
        if (pressed == IDRETRY) {
            // std::memcpy(gs_physics_arena->start, gs_physics_restore_arena->start, gs_physics_restore_arena->top);
            // *gs_app->physics = *gs_app_restore->physics;
            // gs_physics_arena->top = gs_physics_restore_arena->top;

            // std::memcpy(gs_app->arena.start, gs_app_restore->arena.start, gs_app_restore->arena.top);
            // gs_app->arena.top = gs_app_restore->arena.top;
            // std::memcpy(&gs_app->input, &gs_app_restore->input, sizeof(app_input_t));

            throw std::exception(msg.c_str());
        }
    }

    MessageBox(0, fmt::format("Exception at address {}\nCode: {}\nFlag: {}", address, msg!=""?msg:fmt::format("{}", code), flags).c_str(), 0, MB_ABORTRETRYIGNORE);

    gen_error(__FUNCTION__, "Exception at {} Code: {} Flag: {}", address, msg!=""?msg:fmt::format("{}", code), flags);

    return EXCEPTION_CONTINUE_SEARCH;
}


i32 message_box_proc(const char* text) {
    return MessageBox(0, text, 0, MB_OKCANCEL);
}

bool open_file_dialog(char* filename, size_t max_file_size) {
    OPENFILENAME open_file_name{};
    open_file_name.lStructSize = sizeof(OPENFILENAME);
    open_file_name.hwndOwner = 0;
    return false;
}

int 
main(int argc, char* argv[]) {
    _set_error_mode(_OUT_TO_MSGBOX);
    SetUnhandledExceptionFilter(exception_filter);
    create_meta_data_dir();

    gen_info("win32", "Loading Platform Layer");
    defer {
        gen_info("win32", "Closing Application");
    };

    app_memory_t app_mem{};
    app_mem.message_box = message_box_proc;
    constexpr size_t application_memory_size = gigabytes(4);
    app_mem.arena = arena_create(VirtualAlloc(0, application_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE), application_memory_size);

    app_memory_t restore_point;
    restore_point.arena.start = 0;

    gs_app = &app_mem;
    gs_app_restore = &restore_point;
 
    assert(app_mem.arena.start);

    auto& config = app_mem.config;
    load_graphics_config(&app_mem);

    config.window_size[0] = app_mem.config.graphics_config.window_size.x;
    config.window_size[1] = app_mem.config.graphics_config.window_size.y;

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

    void* physics_dll = LoadLibraryA(".\\build\\app_physics.dll");
    arena_t physics_arena = arena_create(win32_alloc(gigabytes(2)), gigabytes(2));
    arena_t restore_physics_arena = arena_create(win32_alloc(gigabytes(2)), gigabytes(2));
    gs_physics_arena = &physics_arena;
    gs_physics_restore_arena = &restore_physics_arena;
    if (physics_dll)
    {
        gen_info("win32", "Initializing Physics");
        app_mem.physics = win32_alloc<physics::api_t>();
        restore_point.physics = win32_alloc<physics::api_t>();
        restore_point.physics->arena = &restore_physics_arena;

        app_mem.physics->collider_count =
        app_mem.physics->character_count =
        app_mem.physics->rigidbody_count = 0;
        assert(physics_dll && "Failing to load physics layer dll");
        physics::init_function init_physics = 
            reinterpret_cast<physics::init_function>(
                GetProcAddress((HMODULE)physics_dll, "physics_init_api")
            );
        init_physics(app_mem.physics, physics::backend_type::PHYSX, &physics_arena);
    }

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

    u64 sound = load_sound_closure.dispatch_request<u64>("./res/audio/unlock.wav");
    gen_info("sdl_mixer::play_sound", "playing Sound: {}", sound);
    //play_sound_closure.dispatch(sound);

    app_dll_t app_dlls;
    load_dlls(&app_dlls);

    GLFWwindow* window = init_glfw(&app_mem);

    gen_info("win32", "Platform Initialization Completed");

    app_dlls.on_init(&app_mem);

#ifdef MULTITHREAD_ENGINE
    std::atomic_bool reload_flag = false;
    std::atomic_bool rendering_flag = false;
    auto render_thread = std::thread([&]{
        while(app_mem.running && !glfwWindowShouldClose(window)) {
            while(reload_flag);
            if (app_dlls.on_render) {
                f32 last_time = app_mem.input.render_time;
                
                app_mem.input.render_time = (f32)(glfwGetTime());
                app_mem.input.render_dt = app_mem.input.render_time - last_time;

                rendering_flag = true;
                app_dlls.on_render(&app_mem);
                rendering_flag = false;
            }
        }
    });
#endif
    
    app_mem.input.pressed.keys[key_id::F9] = 1;

    _set_se_translator([](unsigned int u, EXCEPTION_POINTERS *pExp) {
        std::string error = "SE Exception: ";
        bool want_to_throw = true;
        
        switch (u) {
        case EXCEPTION_BREAKPOINT: want_to_throw = false; break;
        case 0xC0000005: 
            // error += fmt_str("Access Violation at {}", pExp->ExceptionRecord->ExceptionInformation[1]);
          
            throw access_violation_exception{ pExp->ExceptionRecord->ExceptionInformation[0], (void*)pExp->ExceptionRecord->ExceptionInformation[1]};

        default:
            char result[11];
            sprintf_s(result, 11, "0x%08X", u);
            error += result;
        };
        if (want_to_throw) throw std::exception(error.c_str());
    });

    while(app_mem.running && !glfwWindowShouldClose(window)) {
        utl::profile_t* p = 0;

        if (app_mem.input.pressed.keys[key_id::F7]) {
            try {
                int* crash{0}; *crash = 0xf;
            } catch (std::exception & e) {
                puts(e.what());
            };
        }
        if (app_mem.input.pressed.keys[key_id::F8]) {
            const auto dif = utl::memdif((const u8*)physics_arena.start, (const u8*)restore_physics_arena.start, restore_physics_arena.top);
            const auto undif = restore_physics_arena.top - dif;
            gen_info("memdif", "dif {} bytes, undif {} megabytes", dif, undif/megabytes(1));
        }

        if (app_mem.input.pressed.keys[key_id::F9]) {
#ifdef MULTITHREAD_ENGINE
            reload_flag = true;
            while(rendering_flag);
#endif 
            std::memcpy(restore_physics_arena.start, physics_arena.start, physics_arena.top);
            *restore_point.physics = *app_mem.physics;
            restore_physics_arena.top = physics_arena.top;

            if (restore_point.arena.start) {
                VirtualFree(restore_point.arena.start, 0, MEM_RELEASE);
                restore_point.arena.start = 0;
            }
            size_t copy_size = app_mem.arena.top;
            restore_point.arena = arena_create(VirtualAlloc(0, copy_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE), copy_size);
            std::memcpy(restore_point.arena.start, app_mem.arena.start, copy_size);
            restore_point.arena.top = app_mem.arena.top;
            std::memcpy(&restore_point.input, &app_mem.input, sizeof(app_input_t));
#ifdef MULTITHREAD_ENGINE
                reload_flag = false;
#endif 
        }
        if (app_mem.input.pressed.keys[key_id::F10]) {
#ifdef MULTITHREAD_ENGINE
            reload_flag = true;
            while(rendering_flag);
#endif 
            std::memcpy(physics_arena.start, restore_physics_arena.start, restore_physics_arena.top);
            *app_mem.physics = *restore_point.physics;
            physics_arena.top = restore_physics_arena.top;

            std::memcpy(app_mem.arena.start, restore_point.arena.start, restore_point.arena.top);
            app_mem.arena.top = restore_point.arena.top;
            std::memcpy(&app_mem.input, &restore_point.input, sizeof(app_input_t));
#ifdef MULTITHREAD_ENGINE
                reload_flag = false;
#endif 
        }
        if (app_mem.input.pressed.keys[key_id::P]) {
            p = new utl::profile_t{"win32::loop"};
        }

        {
            FILETIME game_time = win32_last_write_time(".\\build\\app_build.dll");
            local_persist bool first = true;
            if (first) {
                gs_game_dll_write_time = game_time;
            }
            if (CompareFileTime(&game_time, &gs_game_dll_write_time)) {
#ifdef MULTITHREAD_ENGINE
                reload_flag = true;
                while(rendering_flag);
#endif 
                update_dlls(&app_dlls, &app_mem);
#ifdef MULTITHREAD_ENGINE
                reload_flag = false;
#endif 
            }
            first = false;
        }
        
        while((f32)(glfwGetTime())-app_mem.input.time<1.0f/60.0f);
        update_input(&app_mem, window);
        if (app_dlls.on_update) {
            try {
                app_dlls.on_update(&app_mem);

            } catch (access_violation_exception& e) {
                auto pressed = MessageBox(0, e.what(), 0, MB_ABORTRETRYIGNORE);
                if (pressed == IDRETRY) {
         #ifdef MULTITHREAD_ENGINE
            reload_flag = true;
            while(rendering_flag);
#endif 
            std::memcpy(physics_arena.start, restore_physics_arena.start, restore_physics_arena.top);
            *app_mem.physics = *restore_point.physics;
            physics_arena.top = restore_physics_arena.top;

            std::memcpy(app_mem.arena.start, restore_point.arena.start, restore_point.arena.top);
            app_mem.arena.top = restore_point.arena.top;
            std::memcpy(&app_mem.input, &restore_point.input, sizeof(app_input_t));
#ifdef MULTITHREAD_ENGINE
                reload_flag = false;
#endif 
         
                }
            } catch (std::exception & e) {
                gen_error("exception", "{}", e.what());
            }
        }
#ifndef MULTITHREAD_ENGINE
        if (app_dlls.on_render) {
            app_dlls.on_render(&app_mem);
        }
#endif 

        glfwPollEvents();

        if (app_mem.input.keys[key_id::ESCAPE] || 
            app_mem.input.gamepads[0].buttons[button_id::options].is_held) {
            app_mem.running = false;
        }

        delete p;

    };

#ifdef MULTITHREAD_ENGINE
    render_thread.join();
#endif 

    save_graphics_config(&app_mem);

    app_dlls.on_deinit(&app_mem);

    return 0;
}