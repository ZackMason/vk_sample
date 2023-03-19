#ifndef APP_HPP
#define APP_HPP

#include "App/vk_state.hpp"
#include "App/gfx_pipelines.hpp"

#include "App/Game/Util/camera.hpp"

#include "App/Game/Physics/physics_world.hpp"

#include "App/Game/Rendering/render_system.hpp"

namespace game {
struct world_t;
};

struct player_controller_t {
    v2f move_input;
    v2f look_input;

    bool fire1;
    f32 iron_sight;

    bool jump;
    bool sprint;

};

struct debug_console_t;

struct app_t {
    app_memory_t*       app_mem{nullptr};

    // arenas
    arena_t             main_arena;
    arena_t             temp_arena;
    arena_t             string_arena;
    arena_t             mesh_arena;
    arena_t             texture_arena;
    arena_t             game_arena;

    utl::res::pack_file_t *     resource_file{0};

    gfx::vul::state_t   gfx;
    game::rendering::system_t* render_system{0};

    // todo(zack) clean this up
    gfx::font_t default_font;
    gfx::vul::texture_2d_t* default_font_texture{nullptr};
    VkDescriptorSet default_font_descriptor;

    gfx::vul::texture_2d_t* brick_texture{nullptr};
    VkDescriptorSet brick_descriptor;

    struct scene_t {
        gfx::vul::sporadic_buffer_t sporadic_buffer;

        gfx::vul::uniform_buffer_t<gfx::vul::scene_buffer_t>    debug_scene_uniform_buffer;

        struct lighting_t {
            v4f directional_light{1.0f, 2.0f, 3.0f, 1.0f};
        } lighting;
    } scene;
    
    gfx::vul::pipeline_state_t* gui_pipeline{0};
    gfx::vul::pipeline_state_t* debug_pipeline{0};
    gfx::vul::pipeline_state_t* mesh_pipeline{0};
    gfx::vul::pipeline_state_t* sky_pipeline{0};

    struct gui_state_t {
        gfx::gui::ctx_t ctx;
        gfx::vul::vertex_buffer_t<gfx::gui::vertex_t, 4'000'000> vertices;
        gfx::vul::index_buffer_t<6'000'000> indices;

        arena_t  arena;
    } gui;

    struct debugging_t {
        debug_console_t* console;

        bool show = false;
        u64 mesh_sub_index{0};
        u64 mesh_index{0};
        gfx::vul::vertex_buffer_t<gfx::vul::debug_line_vertex_t, 100'000> debug_vertices;

        void draw_aabb(const math::aabb_t<v3f>& aabb, gfx::color3 color) {
            const v3f size = aabb.size();
            const v3f sxxx = v3f{size.x,0,0};
            const v3f syyy = v3f{0,size.y,0};
            const v3f szzz = v3f{0,0,size.z};
            
            draw_line(aabb.min, aabb.min + sxxx, color);
            draw_line(aabb.min, aabb.min + syyy, color);
            draw_line(aabb.min, aabb.min + szzz, color);

            draw_line(aabb.min + sxxx + syyy, aabb.min + sxxx, color);
            draw_line(aabb.min + sxxx + syyy, aabb.min + syyy, color);
            draw_line(aabb.max - sxxx - syyy, aabb.max - sxxx, color);
            draw_line(aabb.max - sxxx - syyy, aabb.max - syyy, color);
            
            draw_line(aabb.min + sxxx, aabb.min + szzz + sxxx, color);
            draw_line(aabb.min + syyy, aabb.min + szzz + syyy, color);
            
            draw_line(aabb.max, aabb.max - sxxx, color);
            draw_line(aabb.max, aabb.max - syyy, color);
            draw_line(aabb.max, aabb.max - szzz, color);
        }

        void draw_line(v3f a, v3f b, gfx::color3 color) {
            auto* points = debug_vertices.pool.allocate(2);
            points[0].pos = a;
            points[1].pos = b;
            points[0].col = color;
            points[1].col = color;
        }
    } debug;

        
    game::world_t* game_world{0};
    game::phys::physx_state_t* physics{0};

    f32 time_scale = 1.0f;
    f32 time_text_anim = 0.0f;
};


struct debug_console_t {
    struct message_t {
        message_t* next{0};
        message_t* prev{0};

        string_t text;
        gfx::color32 color = gfx::color::rgba::white;

        void (*command)(void*){0};
        void* data{0};
    };

    message_t* messages{0};

    i32 scroll{0};

    char    text_buffer[1024]{'>'};
    size_t  text_size{0};
};

inline void
console_log(
    debug_console_t* console,
    string_t text,
    arena_t* arena
) {
    gen_info("console", text.c_data);
    auto* message = arena_alloc<debug_console_t::message_t>(arena);
    message->text = text;
    message->data = message->command = 0;
    
    node_push(message, console->messages);
}

inline void
console_log_ex(
    debug_console_t* console,
    string_t text,
    arena_t* arena,
    void (*on_click)(void*),
    void* data
) {
    gen_info("console", text.c_data);
    auto* message = arena_alloc<debug_console_t::message_t>(arena);
    message->text = text;
    message->data = data;
    message->command = on_click;
    
    node_push(message, console->messages);
}

inline void
draw_reverse(
    gfx::gui::im::state_t& imgui, 
    debug_console_t::message_t* message
) {
    using namespace gfx::gui;
    if (message == nullptr) {
        return;
    }
    draw_reverse(imgui, message->next);
    if (im::text(imgui, message->text) && message->command) {
        message->command(message->data);
    }
}

inline void
draw_console(
    gfx::gui::im::state_t& imgui, 
    debug_console_t* console,
    v2f pos
) {
    using namespace gfx::gui;
    using namespace std::string_view_literals;

    if (im::begin_panel(imgui, "Console"sv, pos)) {
        draw_reverse(imgui, console->messages);
        
        im::text(imgui, string_t{console->text_buffer});

        im::end_panel(imgui);
    }

}

#endif