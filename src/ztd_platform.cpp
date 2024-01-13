#include "ztd_core.hpp"

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
// #include "commdlg.h"

// #define MULTITHREAD_ENGINE
#define ENABLE_MEMORY_LOOP

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

constexpr umm OVERFLOW_PAD_SIZE = 8*5;//1024<<0;

struct win32_saved_block_t {
    u64 base;
    u64 size;
};

struct win32_memory_block_t {
    win32_memory_block_t* next{0};
    win32_memory_block_t* prev{0};
    // void* start;
    size_t size{0};

    u8 pad[OVERFLOW_PAD_SIZE] = {};
};

void* get_base_pointer(win32_memory_block_t* block) {
    return block+1;
}
win32_memory_block_t* get_block_pointer(void* ptr) {
    return (win32_memory_block_t*)ptr-1;
}

static_assert(sizeof(win32_memory_block_t) == 64);

global_variable HANDLE gs_loop_file;

global_variable win32_memory_block_t allocated_blocks;
global_variable win32_memory_block_t freed_blocks;
global_variable std::mutex allocation_ticket{};
global_variable std::mutex free_ticket{};

void free_all_used_blocks() {
    auto* head = &freed_blocks;
    std::lock_guard lock{free_ticket};

    for (auto* block = head->next;
        block != head;
        node_next(block)
    ) {
        auto* memory = block;
        block = block->next;
        dlist_remove(memory);
        auto result = VirtualFree(memory, 0, MEM_RELEASE);
        assert(result);
    }
}

HANDLE win32_begin_loop_file(std::string_view file_name) {
    HANDLE file = CreateFileA(
        file_name.data(), 
        GENERIC_WRITE, 
        0, 
        0, 
        CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL, 
        0);
    assert(file);
    return file;
}

HANDLE win32_end_loop_file(std::string_view file_name, HANDLE file) {
    if (file) {
        auto r = CloseHandle(file);
        assert(r);
        file = 0;
    }
    file = CreateFileA(
        file_name.data(), 
        GENERIC_READ, 
        0, 
        0, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        0);
    assert(file);
    return file;
}

void win32_save_blocks_to_file(HANDLE file, win32_memory_block_t* head) {
    bool result;
    DWORD bytes_written;
    DWORD total_bytes = 0;
    OVERLAPPED overlapped = {}; // ms says dont reuse, lets see what happens
    overlapped.OffsetHigh = overlapped.Offset = 0xffffffff;

    for (auto* block = head->next;
        block != head;
        node_next(block)
    ) {

        assert(block->size < std::numeric_limits<DWORD>::max());
        win32_saved_block_t saved = {};
        void* base = get_base_pointer(block);
        assert(base);
        saved.base = (u64)base;
        saved.size = block->size;
        result = WriteFile(file, &saved, sizeof(saved), &bytes_written, &overlapped);
        assert(result);
        result = WriteFile(file, base, (DWORD)block->size, &bytes_written, &overlapped);
        assert(result);

    }

    win32_saved_block_t terminator = {}; // I'll be (the) back
    result = WriteFile(file, &terminator, sizeof(terminator), &bytes_written, &overlapped);
    assert(result);
}

void win32_read_blocks_from_file(HANDLE file, win32_memory_block_t* head) {
    assert(file);
    bool result;
    DWORD bytes_read;
    u64 blocks_loaded = 0;
    u64 bytes_loaded = 0;
    head->prev = head->next = head;
    for (;;) {
        win32_saved_block_t saved = {};
        result = ReadFile(file, &saved, sizeof(saved), &bytes_read, 0);
        assert(result);
        if (saved.base) {
            blocks_loaded++;
            bytes_loaded += saved.size;

            void* base = (void*)saved.base;
            // ztd_info(__FUNCTION__, "Loading block: {} - {}Mb", base, saved.size/megabytes(1));
            auto* block = get_block_pointer(base);
            dlist_insert_as_last(head, block);
            result = ReadFile(file, base, (u32)saved.size, &bytes_read, 0);
            assert(result);
        } else {
            break;
        }
    }
    SetFilePointer(file, 0, NULL, FILE_BEGIN);

    ztd_info(__FUNCTION__, "Done Loading Loop: {} Blocks - {}Mb", blocks_loaded, bytes_loaded/megabytes(1));
}

void win32_load_loop_file(HANDLE file) {
    //free_all_used_blocks();
    win32_read_blocks_from_file(file, &allocated_blocks);
}

void win32_save_loop_file(HANDLE file) {
    win32_save_blocks_to_file(file, &allocated_blocks);
}

void win32_free(void* ptr) {
    // return;
    auto* free_block = (win32_memory_block_t*)((u8*)ptr - sizeof(win32_memory_block_t));

    {
        std::lock_guard lock{allocation_ticket};
        dlist_remove(free_block);
    }

#ifdef ENABLE_MEMORY_LOOP
// #if ZTD_INTERNAL
    {
        auto* head = &freed_blocks;
        std::lock_guard lock{free_ticket};
        dlist_insert_as_last(head, free_block);
    }
#else
    auto result = VirtualFree(free_block, 0, MEM_RELEASE);

    assert(result);
#endif
}

void* win32_alloc(size_t size) {
#ifdef ENABLE_MEMORY_LOOP

    {
        // Note
        // theres a problem in that in the game layer 
        // we dont know the actually size of the block that we got
        auto* head = &freed_blocks;
        std::lock_guard lock{free_ticket};
        for (auto* block = head->next;
            block != head;
            node_next(block)
        ) {
            // skip blocks that are twice as big as what we are looking for
            if (block->size / 2 > size) { 
                continue;
            }
            if (block->size < size) {
                continue;
            }
            dlist_remove(block);
            {
                std::lock_guard alock{allocation_ticket};
                dlist_insert_as_last(&allocated_blocks, block);
            }
            utl::memzero(block+1, block->size);
            return block+1;
        }
    }
#endif

    ztd_info(__FUNCTION__, "Allocating: {}", size);
    void* memory = VirtualAlloc(0, size + sizeof(win32_memory_block_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(memory);

    auto* block = (win32_memory_block_t*)memory;
    new (block) win32_memory_block_t;
    block->size = size;
    
    auto* head = &allocated_blocks;
    {
        std::lock_guard lock{allocation_ticket};
        dlist_insert_as_last(head, block);
    }

    // return block+1;
    
    return (u8*)memory + sizeof(win32_memory_block_t);
}

template <typename T>
T* win32_alloc() {
    auto* t = (T*)win32_alloc(sizeof(T));
    new (t) T();
    return t;
}

bool check_buffer_overflow() {
    u8 sample[OVERFLOW_PAD_SIZE] = {};

    for(auto* block = allocated_blocks.next;
        block != &allocated_blocks;
        block = block->next
    ) {
        if (auto diff = utl::memdif(block->pad, sample, OVERFLOW_PAD_SIZE)) {
            ztd_error(__FUNCTION__, "Buffer Underflow detected: {} - {} bytes", (void*)block, diff);
            return true;
        }
    }
    return false;
}

FILETIME gs_game_dll_write_time;

FILETIME win32_last_write_time(const char* path) {
	FILETIME time = {};
	WIN32_FILE_ATTRIBUTE_DATA data;

	if (GetFileAttributesEx(path, GetFileExInfoStandard, &data))
		time = data.ftLastWriteTime;

	return time;
}

void
win32_unload_library(void* dll) {
    FreeLibrary((HMODULE)dll);
}
void*
win32_load_library(const char* file) {
    return LoadLibraryA(file);
}

void*
win32_load_module() {
    return GetModuleHandle(0);
}

void* 
win32_load_proc(void* lib, const char* name) {
    return GetProcAddress((HMODULE)lib, name);
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
        ztd_error("save", "Get Env Error: {}, cant save", err);
        std::terminate(); // need to be warned of this
    }

    const std::string meta_path = fmt_str("{}/estella", std::string_view{app_data_path, path_size-1});
    
    ztd_info("save", "Save File Path: {}", meta_path);
    if (!std::filesystem::exists(meta_path)) {
        ztd_info("save", "Creating save directory");
        std::filesystem::create_directory(meta_path);
    }

    free(app_data_path);
}

utl::config_t* load_dev_config(arena_t* arena, size_t* count) {
    std::ifstream file{"config.ini"};
    
    if(!file.is_open()) {
        ztd_error("win32", "Failed to graphics config file");
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
        ztd_error("win32", "Failed to graphics config file");
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

    ztd_info("win32", "Saved graphics config");
}

global_variable v2f gs_scroll;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    gs_scroll = v2f{f32(xoffset),f32(yoffset)};
}

void text_callback(GLFWwindow* window, unsigned int codepoint) {
    auto* input = (app_input_t*)glfwGetWindowUserPointer(window);

    if (input->text.active) {
        input->text.key = codepoint;
        input->text.fire = 1;
    }
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

    glfwSetWindowUserPointer(window, &game_memory->input);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCharCallback(window, text_callback);

    game_memory->config.window_handle = glfwGetWin32Window(window);

    assert(window);

    return window;
}



void
update_input(game_memory_t* game_memory, GLFWwindow* window) {
    app_input_t last_input;
    utl::copy(&last_input, &game_memory->input, sizeof(app_input_t));

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

    game_memory->input.mouse.scroll[0] = gs_scroll.x;
    game_memory->input.mouse.scroll[1] = gs_scroll.y;
    gs_scroll = {};
    
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
                // ztd_info("input", "axis[{}]:{}", a, axes_[a]);
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
            // ztd_warn("input", "Joystick is not gamepad");
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
    CopyFile(".\\build\\cultist.dll", ".\\build\\code.dll", 0);
    auto* dll = LoadLibraryA(".\\build\\code.dll");
    if (!dll) {
        DWORD err = GetLastError( );
        ztd_warn(__FUNCTION__, "LoadLibrary Failed to load dll: {}", err);
    }
    app_dlls->dll = dll;
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
    WIN32_FILE_ATTRIBUTE_DATA unused;
	if ( !GetFileAttributesEx( "./build/lock.tmp", GetFileExInfoStandard, &unused ) )
    {
        utl::profile_t p{"dll reload time"};
        ztd_warn("win32", "Game DLL Reload Detected");
        app_dlls->on_unload(game_memory);

        auto result = FreeLibrary((HMODULE)app_dlls->dll);
        if (result == 0) {
            ztd_warn(__FUNCTION__, "Failed to free library");
        }
            
        *app_dlls={};

        load_dlls(app_dlls);

        if (app_dlls->dll == 0) {
            ztd_error("win32", "Failed to load game");
        }

        ztd_warn("win32", "Game DLL has reloaded");
        app_dlls->on_reload(game_memory);

        gs_game_dll_write_time = win32_last_write_time(".\\build\\cultist.dll");
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
            
            throw std::exception(msg.c_str());
        }
    }

    MessageBox(0, fmt::format("Exception at address {}\nCode: {}\nFlag: {}", address, msg!=""?msg:fmt::format("{}", code), flags).c_str(), 0, MB_ABORTRETRYIGNORE);

    ztd_error(__FUNCTION__, "Exception at {} Code: {} Flag: {}", address, msg!=""?msg:fmt::format("{}", code), flags);

    return EXCEPTION_CONTINUE_SEARCH;
}

i32 message_box_proc(const char* text) {
    return MessageBox(0, text, 0, MB_OKCANCEL);
}

int 
main(int argc, char* argv[]) {
    _set_error_mode(_OUT_TO_MSGBOX);
    // SetUnhandledExceptionFilter(exception_filter);

    dlist_init(&allocated_blocks);
    dlist_init(&freed_blocks);
    // gs_loop_file = win32_begin_loop_file("state1.ztd");

    create_meta_data_dir();

    ztd_info("win32", "Loading Platform Layer");
    defer {
        ztd_info("win32", "Closing Application");
    };

    Platform.allocate = win32_alloc;
    Platform.free = win32_free;
    Platform.message_box = message_box_proc;
    Platform.unload_library = win32_unload_library;
    Platform.load_library = win32_load_library;
    Platform.load_module = win32_load_module;
    Platform.load_proc = win32_load_proc;
    Platform.arguments = (const char**)argv;
    Platform.argument_count = argc;

    game_memory_t game_memory{};
    game_memory.platform = Platform;

    // constexpr size_t application_memory_size = gigabytes(2);
    game_memory.arena = arena_create(megabytes(8));

    // game_memory_t restore_point;
    // restore_point.arena.start = 0;
 
    // assert(game_memory.arena.start);

    auto& config = game_memory.config;
    load_graphics_config(&game_memory);

    utl::config_list_t dconfig{};
    dconfig.head = load_dev_config(&game_memory.arena, &dconfig.count);

    config.window_size[0] = utl::config_get_int(&dconfig, "width", game_memory.config.graphics_config.window_size.x);
    config.window_size[1] = utl::config_get_int(&dconfig, "height", game_memory.config.graphics_config.window_size.y);
    
    config.graphics_config.vsync = utl::config_get_int(&dconfig, "vsync", 1);


    game_memory.config.create_vk_surface = [](void* instance, void* surface, void* window_handle) {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = (HWND)window_handle;
        createInfo.hinstance = GetModuleHandle(nullptr);
        
        // if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        if (vkCreateWin32SurfaceKHR(*(VkInstance*)instance, &createInfo, nullptr, (VkSurfaceKHR*)surface) != VK_SUCCESS) {
            ztd_error("vulkan", "failed to create window surface!");
            std::terminate();
        }
    };

    void* physics_dll = LoadLibraryA(".\\build\\ztd_physics.dll");
    arena_t physics_arena = arena_create(megabytes(8));
    
    if (physics_dll)
    {
        ztd_info("win32", "Initializing Physics");
        game_memory.physics = push_struct<physics::api_t>(&physics_arena);
    
        game_memory.physics->collider_count =
        game_memory.physics->character_count =
        game_memory.physics->rigidbody_count = 0;
        assert(physics_dll && "Failing to load physics layer dll");
        physics::init_function init_physics = 
            reinterpret_cast<physics::init_function>(
                GetProcAddress((HMODULE)physics_dll, "physics_init_api")
            );
        init_physics(game_memory.physics, physics::backend_type::PHYSX, &Platform, &physics_arena);
        *game_memory.physics->Platform = Platform;
        ztd_info("win32", "Physics Loaded");
    }

// #if USE_SDL
//     if (SDL_Init(SDL_INIT_AUDIO) < 0) {
//         ztd_info("sdl_mixer", "Couldn't initialize SDL: {}", SDL_GetError());
//         return 255;
//     }

//     if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) < 0) {
//         ztd_info("sdl_mixer", "Couldn't open audio: {}", SDL_GetError());
//         return 255;
//     }
    
// #endif

//     audio_cache_t audio_cache{};
//     closure_t& load_sound_closure = Platform.audio.load_sound;
//     load_sound_closure.data = &audio_cache;
//     load_sound_closure.func = reinterpret_cast<void(*)(void*)>(
//         +[](void* data, const char* path) -> u64 {
//             auto* cache = (audio_cache_t*)data;

// #if USE_SDL
//             ztd_info("sdl_mixer::load_sound", "Loading Sound: {}", path);
//             cache->sounds[cache->sound_count] = Mix_LoadWAV(path);
//             ztd_info("sdl_mixer::load_sound", "Sound Loaded: id = {}", cache->sound_count);
// #endif

//             return cache->sound_count++;
//     });
    
//     closure_t& play_sound_closure = Platform.audio.play_sound;
//     play_sound_closure.data = &audio_cache;
//     play_sound_closure.func = reinterpret_cast<void(*)(void*)>(
//         +[](void* data, u64 id) -> void {

//             auto* cache = (audio_cache_t*)data;

// #if USE_SDL
//             ztd_info("sdl_mixer::play_sound", "playing Sound: {}", id);
//             Mix_PlayChannel(-1, cache->sounds[id], 0);
// #endif
//     });

    game_memory.platform = Platform;

    app_dll_t app_dlls;
    load_dlls(&app_dlls);

    GLFWwindow* window = init_glfw(&game_memory);

    ztd_info("win32", "Platform Initialization Completed");

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

    // _set_se_translator([](unsigned int u, EXCEPTION_POINTERS *pExp) {
    //     std::string error = "SE Exception: ";
        
    //     switch (u) {
    //     case EXCEPTION_BREAKPOINT: break; // ignore
    //     case EXCEPTION_ACCESS_VIOLATION: 
    //         throw access_violation_exception{ 
    //             pExp->ExceptionRecord->ExceptionInformation[0], 
    //             (void*)pExp->ExceptionRecord->ExceptionInformation[1]
    //         };

    //     default:
    //         char result[11];
    //         sprintf_s(result, 11, "0x%08X", u);
    //         error += result;
    //         throw std::runtime_error(error.c_str());
    //     };
    // });

    while(game_memory.running && !glfwWindowShouldClose(window)) {
        if (check_buffer_overflow()) { 
            ztd_error("memory", "buffer overflow detected");
        }
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            // glfwPollEvents();
            continue;
        }
        utl::profile_t* p = 0;
        if (game_memory.input.pressed.keys[key_id::O]) {
            local_persist bool captured = false;
            glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            captured = !captured;
        }

        
#ifdef ENABLE_MEMORY_LOOP
        if (game_memory.input.pressed.keys[key_id::F9]) {
#ifdef MULTITHREAD_ENGINE
            rendering_lock.lock();
#endif 
            if (gs_loop_file) {
                CloseHandle(gs_loop_file);
            }
            gs_loop_file = win32_begin_loop_file("state1.replay");
            win32_save_loop_file(gs_loop_file);
            
#ifdef MULTITHREAD_ENGINE
            rendering_lock.unlock();
#endif 
        }
#endif 

#ifdef ENABLE_MEMORY_LOOP
        if (game_memory.input.pressed.keys[key_id::F10]) {
#ifdef MULTITHREAD_ENGINE
            rendering_lock.lock();
#endif 
            if (gs_loop_file) {
                gs_loop_file = win32_end_loop_file("state1.replay", gs_loop_file);
                win32_load_loop_file(gs_loop_file);

                //@keep Do this for loops probably
                // CloseHandle(gs_loop_file);
                // gs_loop_file = 0;
            } else {
                ztd_warn("loop", "No loop file");
            }
#ifdef MULTITHREAD_ENGINE
            rendering_lock.unlock();
#endif 
        }
#endif 
        if (game_memory.input.pressed.keys[key_id::P]) {
            p = new utl::profile_t{"win32::loop"};
        }

        {
            FILETIME game_time = win32_last_write_time(".\\build\\cultist.dll");
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
            // try {
                app_dlls.on_update(&game_memory);

//             } catch (access_violation_exception& e) {
//                 ztd_error("exception", "{}", e.what());
//                 auto pressed = MessageBox(0, e.what(), 0, MB_ABORTRETRYIGNORE);
//                 if (pressed == IDRETRY) {
// #ifdef MULTITHREAD_ENGINE
//                     rendering_lock.lock();
// #endif 
//                     // utl::copy(physics_arena.start, restore_physics_arena.start, restore_physics_arena.top);
//                     // *game_memory.physics = *restore_point.physics;
//                     // physics_arena.top = restore_physics_arena.top;

//                     // utl::copy(game_memory.arena.start, restore_point.arena.start, restore_point.arena.top);
//                     // game_memory.arena.top = restore_point.arena.top;
//                     // utl::copy(&game_memory.input, &restore_point.input, sizeof(app_input_t));
// #ifdef MULTITHREAD_ENGINE
//                     rendering_lock.unlock();
// #endif 
//                 } else {
//                     std::terminate();
//                 }
            // } catch (std::exception & e) {
            //     ztd_error("exception", "{}", e.what());
            //     std::terminate();
            // }
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