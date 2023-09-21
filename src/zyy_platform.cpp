#include "zyy_core.hpp"

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
#undef near
#undef far

// #define MULTITHREAD_ENGINE
platform_api_t Platform;

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

void win32_free(void* ptr){
    VirtualFree(ptr, 0, MEM_RELEASE);
}

void* win32_alloc(size_t size){
    return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

template <typename T>
T* win32_alloc(){
    auto* t = (T*)win32_alloc(sizeof(T));
    new (t) T();
    return t;
}

game_memory_t* gs_app{0};
game_memory_t* gs_app_restore{0};
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

void 
create_meta_data_dir() {
    char* app_data_path;
    size_t path_size;
    const errno_t err = _dupenv_s(&app_data_path, &path_size, "APPDATA");
    if (err) {
        zyy_error("save", "Get Env Error: {}, cant save", err);
        std::terminate(); // need to be warned of this
    }

    const std::string meta_path = fmt_str("{}/estella", std::string_view{app_data_path, path_size-1});
    
    zyy_info("save", "Save File Path: {}", meta_path);
    if (!std::filesystem::exists(meta_path)) {
        zyy_info("save", "Creating save directory");
        std::filesystem::create_directory(meta_path);
    }

    free(app_data_path);
}

utl::config_t* load_dev_config(arena_t* arena, size_t* count) {
    std::ifstream file{"config.ini"};
    
    if(!file.is_open()) {
        zyy_error("win32", "Failed to graphics config file");
        return 0;
    }
    auto text = (std::stringstream{} << file.rdbuf()).str();

    utl::config_t* config{0};
    utl::read_config(arena, text, &config, count);
    return config;
}

void
load_graphics_config(game_memory_t* game_memory) {
    std::ifstream file{"gfx_config.bin", std::ios::binary};

    if(!file.is_open()) {
        zyy_error("win32", "Failed to graphics config file");
        return;
    }

    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::byte* data = new std::byte[size];
    file.read((char*)data, size);
    utl::memory_blob_t loader{data};
    
    game_memory->config.graphics_config = loader.deserialize<app_config_t::graphic_config_t>();
    
    delete data;
}
void
save_graphics_config(game_memory_t* game_memory) {
    std::ofstream file{"./gfx_config.bin", std::ios::out | std::ios::binary};

    // std::byte* data = new std::byte[sizeof(app_config_t::graphic_config_t)];
    
    // utl::memory_blob_t loader{data};
    
    // loader.serialize(0, game_memory->config.graphics_config);

    file.write((const char*)&game_memory->config.graphics_config, sizeof(app_config_t::graphic_config_t));

    file.close();

    // delete data;

    zyy_info("win32", "Saved graphics config");
}

GLFWwindow* 
init_glfw(game_memory_t* game_memory) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	
    auto* window = 
        glfwCreateWindow(game_memory->config.window_size[0], game_memory->config.window_size[1], "Vk App", 0, 0);

	game_memory->config.vk_exts = (char**)glfwGetRequiredInstanceExtensions (&game_memory->config.vk_ext_count);
	printf("\nFound %d GLFW Required Instance Extensions:\n", game_memory->config.vk_ext_count);
	for( uint32_t i = 0; i < game_memory->config.vk_ext_count; i++ ) {
		printf( "\t%s\n", game_memory->config.vk_exts[ i ] );
	}

    game_memory->config.window_handle = glfwGetWin32Window(window);

    assert(window);

    return window;
}

void
update_input(game_memory_t* game_memory, GLFWwindow* window) {
    app_input_t last_input;
    std::memcpy(&last_input, &game_memory->input, sizeof(app_input_t));

    app_input_reset(&game_memory->input); // CLEARS ENTIRE INPUT STRUCT (!INCLUDING TIME!!!!!!)

    local_persist f32 last_time = (f32)(glfwGetTime());
    local_persist f32 time_accum = 0.0f;
    f32 time = (f32)(glfwGetTime());

    game_memory->input.dt = std::min(time - last_time, 0.5f);
    game_memory->input.time = time_accum += game_memory->input.dt;
    last_time = time;

    f64 mouse_temp[2];
    glfwGetCursorPos(window, mouse_temp+0, mouse_temp+1);
    f32 mouse[2]; 
    mouse[0] = f32(mouse_temp[0]);
    mouse[1] = f32(mouse_temp[1]);

    game_memory->input.mouse.pos[0] = mouse[0];
    game_memory->input.mouse.pos[1] = mouse[1];

    game_memory->input.mouse.delta[0] = mouse[0] - last_input.mouse.pos[0];
    game_memory->input.mouse.delta[1] = mouse[1] - last_input.mouse.pos[1];
    
    loop_iota_i32(i, array_count(game_memory->input.keys)) {
        if (i < array_count(game_memory->input.mouse.buttons)) {
            game_memory->input.mouse.buttons[i] = glfwGetMouseButton(window, i) == GLFW_PRESS;
        }
        game_memory->input.keys[i] = glfwGetKey(window, i) == GLFW_PRESS;
        game_memory->input.pressed.keys[i] = game_memory->input.keys[i] && !last_input.keys[i];
        game_memory->input.released.keys[i] = !game_memory->input.keys[i] && last_input.keys[i];
    }

    loop_iota_i32(i, array_count(game_memory->input.gamepads)) {
        auto& gamepad = game_memory->input.gamepads[i];

        GLFWgamepadstate state;
        if (glfwJoystickIsGamepad(i) && glfwGetGamepadState(i, &state)) {
            gamepad.is_connected = true;
            loop_iota_u64(a, GLFW_GAMEPAD_AXIS_LAST + 1) {
                if (std::fabs(state.axes[a]) < 0.35f) { // deadzone
                    state.axes[a] = 0.0f;
                }
                // zyy_info("input", "axis[{}]:{}", a, axes_[a]);
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
            // zyy_warn("input", "Joystick is not gamepad");
            gamepad = {};
        }
    }

    loop_iota_u64(mb, array_count(game_memory->input.mouse.buttons)) {
        const u8 mouse_btn = game_memory->input.mouse.buttons[mb];
        const u8 last_mouse_btn = last_input.mouse.buttons[mb];
        game_memory->input.pressed.mouse_btns[mb] = !last_mouse_btn && mouse_btn;
        game_memory->input.released.mouse_btns[mb] = last_mouse_btn && !mouse_btn;
    }

    loop_iota_u64(g, array_count(game_memory->input.gamepads)) {
        loop_iota_u64(b, button_id::SIZE) {
            auto& button = game_memory->input.gamepads[g].buttons[b];
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
    CopyFile(".\\build\\zyy_build.dll", ".\\build\\code.dll", 0);
    app_dlls->dll = LoadLibrary(".\\build\\code.dll");
#else

#error "platform not imlemented"

#endif
    
    assert(app_dlls->dll);

    load_dll_functions(app_dlls);
}

void
update_dlls(app_dll_t* app_dlls, game_memory_t* game_memory) {
    // bool need_reload = !std::filesystem::exists("./build/lock.tmp");
    // if (1) 
    {
        utl::profile_t p{"dll reload time"};
        zyy_warn("win32", "Game DLL Reload Detected");
        app_dlls->on_unload(game_memory);

        FreeLibrary((HMODULE)app_dlls->dll);
        *app_dlls={};

        load_dlls(app_dlls);

        if (app_dlls->dll == 0) {
            zyy_error("win32", "Failed to load game");
        }

        zyy_warn("win32", "Game DLL has reloaded");
        app_dlls->on_reload(game_memory);

        gs_game_dll_write_time = win32_last_write_time(".\\build\\zyy_build.dll");
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

    zyy_error(__FUNCTION__, "Exception at {} Code: {} Flag: {}", address, msg!=""?msg:fmt::format("{}", code), flags);

    return EXCEPTION_CONTINUE_SEARCH;
}

i32 message_box_proc(const char* text) {
    return MessageBox(0, text, 0, MB_OKCANCEL);
}

int 
main(int argc, char* argv[]) {
    _set_error_mode(_OUT_TO_MSGBOX);
    SetUnhandledExceptionFilter(exception_filter);
    create_meta_data_dir();

    zyy_info("win32", "Loading Platform Layer");
    defer {
        zyy_info("win32", "Closing Application");
    };

    Platform.allocate = win32_alloc;
    Platform.free = win32_free;
    Platform.message_box = message_box_proc;

    game_memory_t game_memory{};
    game_memory.platform = Platform;
    constexpr size_t application_memory_size = gigabytes(2);
    game_memory.arena = arena_create(VirtualAlloc(0, application_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE), application_memory_size);

    game_memory_t restore_point;
    restore_point.arena.start = 0;

    gs_app = &game_memory;
    gs_app_restore = &restore_point;
 
    assert(game_memory.arena.start);

    auto& config = game_memory.config;
    load_graphics_config(&game_memory);

    utl::config_list_t dconfig{};
    dconfig.head = load_dev_config(&game_memory.arena, &dconfig.count);

    config.window_size[0] = game_memory.config.graphics_config.window_size.x;
    config.window_size[1] = game_memory.config.graphics_config.window_size.y;

    config.window_size[0] = utl::config_get_int(&dconfig, "width", 1920);
    config.window_size[1] = utl::config_get_int(&dconfig, "height", 1080);
    
    config.graphics_config.vsync = utl::config_get_int(&dconfig, "vsync", 1);
    

    game_memory.config.create_vk_surface = [](void* instance, void* surface, void* window_handle) {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = (HWND)window_handle;
        createInfo.hinstance = GetModuleHandle(nullptr);
        
        // if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        if (vkCreateWin32SurfaceKHR(*(VkInstance*)instance, &createInfo, nullptr, (VkSurfaceKHR*)surface) != VK_SUCCESS) {
            zyy_error("vulkan", "failed to create window surface!");
            std::terminate();
        }
    };

    void* physics_dll = LoadLibraryA(".\\build\\zyy_physics.dll");
    arena_t physics_arena = arena_create(win32_alloc(gigabytes(2)), gigabytes(2));
    arena_t restore_physics_arena = arena_create(win32_alloc(gigabytes(2)), gigabytes(2));
    gs_physics_arena = &physics_arena;
    gs_physics_restore_arena = &restore_physics_arena;
    if (physics_dll)
    {
        zyy_info("win32", "Initializing Physics");
        game_memory.physics = win32_alloc<physics::api_t>();
        restore_point.physics = win32_alloc<physics::api_t>();
        restore_point.physics->arena = &restore_physics_arena;

        game_memory.physics->collider_count =
        game_memory.physics->character_count =
        game_memory.physics->rigidbody_count = 0;
        assert(physics_dll && "Failing to load physics layer dll");
        physics::init_function init_physics = 
            reinterpret_cast<physics::init_function>(
                GetProcAddress((HMODULE)physics_dll, "physics_init_api")
            );
        init_physics(game_memory.physics, physics::backend_type::PHYSX, &physics_arena);
    }

#if USE_SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        zyy_info("sdl_mixer", "Couldn't initialize SDL: {}", SDL_GetError());
        return 255;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) < 0) {
        zyy_info("sdl_mixer", "Couldn't open audio: {}", SDL_GetError());
        return 255;
    }
    
#endif

    audio_cache_t audio_cache{};
    closure_t& load_sound_closure = Platform.audio.load_sound;
    load_sound_closure.data = &audio_cache;
    load_sound_closure.func = reinterpret_cast<void(*)(void*)>(
        +[](void* data, const char* path) -> u64 {
            zyy_info("sdl_mixer::load_sound", "Loading Sound: {}", path);

            auto* cache = (audio_cache_t*)data;
#if USE_SDL
            // if (utl::has_extension(path, "wav")) {
            cache->sounds[cache->sound_count] = Mix_LoadWAV(path);
            // } else if (utl::has_extension(path, "ogg")) {
                // cache->sounds[cache->sound_count] = Mix_LoadMUS(path);
            // }
#endif

            zyy_info("sdl_mixer::load_sound", "Sound Loaded: id = {}", cache->sound_count);
        
            return cache->sound_count++;
    });
    
    closure_t& play_sound_closure = Platform.audio.play_sound;
    play_sound_closure.data = &audio_cache;
    play_sound_closure.func = reinterpret_cast<void(*)(void*)>(
        +[](void* data, u64 id) -> void {
            zyy_info("sdl_mixer::play_sound", "playing Sound: {}", id);

            auto* cache = (audio_cache_t*)data;

#if USE_SDL
            Mix_PlayChannel(-1, cache->sounds[id], 0);
#endif
    });

    game_memory.platform = Platform;

    app_dll_t app_dlls;
    load_dlls(&app_dlls);

    GLFWwindow* window = init_glfw(&game_memory);

    zyy_info("win32", "Platform Initialization Completed");

    app_dlls.on_init(&game_memory);

#ifdef MULTITHREAD_ENGINE
    // std::atomic_bool reload_flag = false;
    // std::atomic_bool rendering_flag = false;
    utl::spinlock_t rendering_lock{};
    auto render_thread = std::thread([&]{
        while(game_memory.running && !glfwWindowShouldClose(window)) {
            // while(reload_flag);
            if (app_dlls.on_render) {
                f32 last_time = game_memory.input.render_time;
                
                game_memory.input.render_time = (f32)(glfwGetTime());
                game_memory.input.render_dt = game_memory.input.render_time - last_time;

                // rendering_flag = true;
                rendering_lock.lock();
                app_dlls.on_render(&game_memory);
                rendering_lock.unlock();
            }
        }
    });
#endif
    
    // game_memory.input.pressed.keys[key_id::F9] = 1;

    _set_se_translator([](unsigned int u, EXCEPTION_POINTERS *pExp) {
        std::string error = "SE Exception: ";
        
        switch (u) {
        case EXCEPTION_BREAKPOINT: break; // ignore
        case EXCEPTION_ACCESS_VIOLATION: 
            throw access_violation_exception{ 
                pExp->ExceptionRecord->ExceptionInformation[0], 
                (void*)pExp->ExceptionRecord->ExceptionInformation[1]
            };

        default:
            char result[11];
            sprintf_s(result, 11, "0x%08X", u);
            error += result;
            throw std::runtime_error(error.c_str());
        };
    });

    while(game_memory.running && !glfwWindowShouldClose(window)) {
        utl::profile_t* p = 0;
        if (game_memory.input.pressed.keys[key_id::O]) {
            local_persist bool captured = false;
            glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            captured = !captured;
        }

        if (game_memory.input.pressed.keys[key_id::F8]) {
            const auto dif = utl::memdif((const u8*)physics_arena.start, (const u8*)restore_physics_arena.start, restore_physics_arena.top);
            const auto undif = restore_physics_arena.top - dif;
            zyy_info("memdif", "dif {} bytes, undif {} megabytes", dif, undif/megabytes(1));
        }

        if (game_memory.input.pressed.keys[key_id::F9]) {
#ifdef MULTITHREAD_ENGINE
            rendering_lock.lock();
#endif 
            std::memcpy(restore_physics_arena.start, physics_arena.start, physics_arena.top);
            *restore_point.physics = *game_memory.physics;
            restore_physics_arena.top = physics_arena.top;

            if (restore_point.arena.start) {
                VirtualFree(restore_point.arena.start, 0, MEM_RELEASE);
                restore_point.arena.start = 0;
            }
            size_t copy_size = game_memory.arena.top;
            restore_point.arena = arena_create(VirtualAlloc(0, copy_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE), copy_size);
            std::memcpy(restore_point.arena.start, game_memory.arena.start, copy_size);
            restore_point.arena.top = game_memory.arena.top;
            std::memcpy(&restore_point.input, &game_memory.input, sizeof(app_input_t));
#ifdef MULTITHREAD_ENGINE
            rendering_lock.unlock();
#endif 
        }
        if (game_memory.input.pressed.keys[key_id::F10]) {
#ifdef MULTITHREAD_ENGINE
            rendering_lock.lock();
#endif 
            std::memcpy(physics_arena.start, restore_physics_arena.start, restore_physics_arena.top);
            *game_memory.physics = *restore_point.physics;
            physics_arena.top = restore_physics_arena.top;

            std::memcpy(game_memory.arena.start, restore_point.arena.start, restore_point.arena.top);
            game_memory.arena.top = restore_point.arena.top;
            std::memcpy(&game_memory.input, &restore_point.input, sizeof(app_input_t));
#ifdef MULTITHREAD_ENGINE
            rendering_lock.unlock();
#endif 
        }
        if (game_memory.input.pressed.keys[key_id::P]) {
            p = new utl::profile_t{"win32::loop"};
        }

        {
            FILETIME game_time = win32_last_write_time(".\\build\\zyy_build.dll");
            local_persist bool first = true;
            if (first) {
                gs_game_dll_write_time = game_time;
            }
            if (CompareFileTime(&game_time, &gs_game_dll_write_time)) {
#ifdef MULTITHREAD_ENGINE
                rendering_lock.lock();
#endif 
                update_dlls(&app_dlls, &game_memory);
#ifdef MULTITHREAD_ENGINE
                rendering_lock.unlock();
#endif 
            }
            first = false;
        }
        // limit gameplay fps
        local_persist f64 last_time = glfwGetTime();
        // while((f32)(glfwGetTime())-last_time<1.0f/60.0f);
        // if (glfwGetTime() - last_time >= 1.0f/60.0f) 
        {
        last_time = glfwGetTime();
        update_input(&game_memory, window);
        if (app_dlls.on_update) {
            try {
                app_dlls.on_update(&game_memory);

            } catch (access_violation_exception& e) {
                zyy_error("exception", "{}", e.what());
                auto pressed = MessageBox(0, e.what(), 0, MB_ABORTRETRYIGNORE);
                if (pressed == IDRETRY) {
#ifdef MULTITHREAD_ENGINE
                    rendering_lock.lock();
#endif 
                    std::memcpy(physics_arena.start, restore_physics_arena.start, restore_physics_arena.top);
                    *game_memory.physics = *restore_point.physics;
                    physics_arena.top = restore_physics_arena.top;

                    std::memcpy(game_memory.arena.start, restore_point.arena.start, restore_point.arena.top);
                    game_memory.arena.top = restore_point.arena.top;
                    std::memcpy(&game_memory.input, &restore_point.input, sizeof(app_input_t));
#ifdef MULTITHREAD_ENGINE
                    rendering_lock.unlock();
#endif 
                } else {
                    std::terminate();
                }
            } catch (std::exception & e) {
                zyy_error("exception", "{}", e.what());
                std::terminate();
            }
        }
#ifndef MULTITHREAD_ENGINE
        if (app_dlls.on_render) {
            app_dlls.on_render(&game_memory);
        }
#endif 
        }

        glfwPollEvents();

        if (game_memory.input.keys[key_id::ESCAPE] || 
            game_memory.input.gamepads[0].buttons[button_id::options].is_held) {
            game_memory.running = false;
        }

        delete p;

    };

#ifdef MULTITHREAD_ENGINE
    render_thread.join();
#endif 

    save_graphics_config(&game_memory);

    app_dlls.on_deinit(&game_memory);

    return 0;
}