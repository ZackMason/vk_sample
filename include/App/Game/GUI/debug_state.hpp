#pragma once

#include "zyy_core.hpp"
#include "App/Game/Entity/entity.hpp"

enum struct debug_priority_level {
    LOW, MEDIUM, HIGH
};

enum struct debug_variable_type {
    UINT32, INT32,
    FLOAT32, FLOAT64,
    VEC2, VEC3,
    AABB, RAY,
    COUNT
};

enum struct debug_watcher_type {
    UINT32, INT32,
    FLOAT32, FLOAT64,
    VEC2, VEC3,
    AABB, RAY,
    ENTITY, CSTR,
    COUNT
};

struct debug_watcher_t {
    debug_watcher_t* next{0};
    std::string_view name{};

    debug_watcher_type type{};

    union {
        char* as_cstr;
        b32* as_b32;
        u32* as_u32;
        i32* as_i32;
        f32* as_f32;
        f64* as_f64;
        v2f* as_v2f;
        v3f* as_v3f;
        // math::aabb_t<v3f> as_aabb;
        zyy::entity_t* as_entt;
        math::ray_t* as_ray;
    };

    union {
        u32 min_u32;
        i32 min_i32;
        f32 min_f32{0.0f};
        f64 min_f64;
        v2f min_v2f;
        v3f min_v3f;
    };

    union {
        u32 max_u32;
        i32 max_i32;
        f32 max_f32{1.0f};
        f64 max_f64;
        v2f max_v2f;
        v3f max_v3f;
    };
};

struct debug_variable_t {
    debug_variable_t* next{0};
    std::string_view name{};

    debug_variable_type type{};
    f32 time_stamp{0};

    union {
        u32 as_u32;
        i32 as_i32;
        f32 as_f32;
        f64 as_f64;
        v2f as_v2f;
        v3f as_v3f;
        math::aabb_t<v3f> as_aabb{};
        math::ray_t as_ray;
    };
};

struct debug_state_t {
    f32& time;
    arena_t arena;
    arena_t watch_arena;

    std::mutex ticket;

    f32 timeout{1.0f};
    v3f focus_point{0.0f};
    f32 focus_distance{0.10f};

    debug_variable_t* variables{0};
    debug_variable_t* first_free{0};

    debug_watcher_t* watcher{0};

    char watcher_needle[64];
    size_t watcher_wpos{0};

    void begin_frame() {
        watcher = nullptr;

        arena_clear(&watch_arena);
    }

    void show_entity(gfx::gui::im::state_t& imgui, debug_watcher_t* var) {
        using namespace gfx::gui;
        assert(var->type == debug_watcher_type::ENTITY);
        auto* entity = var->as_entt;
        if (entity->name.c_data) {
            im::text(imgui, entity->name.c_data);
        } else {
            im::text(imgui, fmt_sv("<entity_{}>", (void*)entity));
        }
        if (im::text(imgui, fmt_sv("- Kill\0{}"sv, entity->id))) {
            entity->queue_free();
        }
        im::text(imgui, fmt_sv("- Tag: {}", entity->tag));
        im::text(imgui, fmt_sv("- Flags: {}", entity->flags));

        im::same_line(imgui);
        im::text(imgui, "- Local Origin: ");
        im::vec3(imgui, entity->transform.origin);

        if (entity->stats.character.health.max > 0.0f) {
            auto [max, hp] = entity->stats.character.health;
            im::text(imgui, fmt_sv("- Health: {}/{}", max, hp));
        }
    }

    void draw_watch_window(gfx::gui::im::state_t& imgui) {
        std::lock_guard lock{ticket};
        using namespace gfx::gui;

        const auto theme = imgui.theme;
 
        if (im::begin_panel(imgui, "Watch Window", imgui.ctx.screen_size*0.2f)) {
            im::text(imgui, "Watch Window");

            im::text_edit(imgui, watcher_needle, &watcher_wpos, "watcher_text_box"_sid);

            u64 shown = 0;
            node_for(auto, watcher, var) {
                if (var->type == debug_watcher_type::CSTR) {
                    if ((std::strstr(var->as_cstr, watcher_needle) == nullptr) &&
                        (std::strstr(var->name.data(), watcher_needle) == nullptr)) continue;
                } else if (var->type == debug_watcher_type::ENTITY) {
                    if ((std::strstr(var->as_entt->name.c_data, watcher_needle) == nullptr) &&
                        (std::strstr(var->name.data(), watcher_needle) == nullptr)) continue;
                } else {
                    if (std::strstr(var->name.data(), watcher_needle) == nullptr) continue;
                }
                if (shown++ > 20) { break; }
                if (var->type == debug_watcher_type::ENTITY) {
                    show_entity(imgui, var);
                    continue;
                } else {
                    im::same_line(imgui);
                    im::text(imgui, fmt_sv("{}: ", var->name));
                }
                if (var->type == debug_watcher_type::FLOAT32) {
                    im::float_slider(imgui, var->as_f32, var->min_f32, var->max_f32);
                } else if (var->type == debug_watcher_type::UINT32) {
                    f32 t = (f32)*var->as_u32;
                    im::float_slider_id(imgui, (u64)var->as_u32, &t, (f32)var->min_u32, (f32)var->max_u32);
                    *var->as_u32 = (u32)t;
                } else if (var->type == debug_watcher_type::CSTR) {
                    im::text(imgui, var->as_cstr);
                } else if (var->type == debug_watcher_type::VEC3) {
                    im::vec3(imgui, *var->as_v3f, var->min_f32, var->max_f32);
                } else {
                    im::text(imgui, "Unsupported type");
                }
            }
            
            imgui.theme.bg_color = 0x0;
            imgui.theme.fg_color = 0x0;
            im::end_panel(imgui);
        }

        imgui.theme = theme;
    }

    void draw(gfx::gui::im::state_t& imgui, const m44& proj, const m44& view, const v4f& viewport) {
        std::lock_guard lock{ticket};
        using namespace gfx::gui;
        debug_variable_t* last = nullptr;
        debug_variable_t* curr = variables;
        while (curr && time - curr->time_stamp > timeout) {
            auto* next = curr->next;
            node_next(variables);
            node_push(curr, first_free);
            curr = next;
        }
        while (curr) {
            if (time - curr->time_stamp > timeout) {
                last->next = curr->next;
                node_push(curr, first_free);
                curr = last->next;

            } else {
                last = curr;
                node_next(curr);
            }
        }

        math::aabb_t<v3f> viewable{v3f{0.0f}, v3f{viewport.z, viewport.w, 1.0f}};
        node_for(auto, variables, var) {
            // im::text(imgui, var->name);
            if (var->type == debug_variable_type::VEC3) {
                if (glm::distance(var->as_v3f, focus_point) > focus_distance) continue;
                auto pos_ = math::world_to_screen(proj, view, viewport, var->as_v3f);
                if (viewable.contains(pos_)) {
                    auto pos_up = swizzle::xy(math::world_to_screen(proj, view, viewport, var->as_v3f + axis::up * 0.05f));
                    auto pos = swizzle::xy(pos_);
                    draw_circle(&imgui.ctx, pos, glm::distance(pos_up, pos) * 0.66f, gfx::color::rgba::light_gray);
                    draw_circle(&imgui.ctx, pos, glm::distance(pos_up, pos), gfx::color::rgba::light_blue);
                    string_render(&imgui.ctx, var->name, pos, gfx::color::rgba::light_blue);
                }
            } else if (var->type == debug_variable_type::AABB) {
                const auto aabb_distance = glm::distance(var->as_aabb.center(), focus_point);
                if (
                    aabb_distance > focus_distance || 
                    // var->as_aabb.contains(focus_point) || 
                    aabb_distance < 1.0f
                ) continue;

                auto pos = (math::world_to_screen(proj, view, viewport, var->as_aabb.center()));
                if (viewable.contains(pos)) {
                    // string_render(&imgui.ctx, var->name, swizzle::xy(pos), gfx::color::rgba::yellow);
                }
                draw_aabb(&imgui.ctx, var->as_aabb, proj * view, 2.0f, gfx::color::rgba::yellow);

            } else if (var->type == debug_variable_type::RAY) {
                if (glm::distance(var->as_ray.origin, focus_point) > focus_distance) continue;
                auto ro_ = math::world_to_screen(proj, view, viewport, var->as_ray.origin);
                auto rd_ = math::world_to_screen(proj, view, viewport, var->as_ray.origin + var->as_ray.direction);

                if (viewable.contains(ro_)) {
                    auto ro = swizzle::xy(ro_);
                    auto rd = swizzle::xy(rd_);
                    draw_line(&imgui.ctx, ro, rd, 2.0f, gfx::color::rgba::yellow);
                    string_render(&imgui.ctx, var->name, ro, gfx::color::rgba::yellow);
                }
            }
        }
    }

    template <typename T>
    debug_variable_t* add_time_variable(T val, std::string_view name, f32 force_life = -1.0f) {
        auto* var = add_variable(val, name);
        if (force_life != -1.0f) {
            const auto delta = timeout - force_life;
            var->time_stamp = time - delta;
        } else {
            var->time_stamp = time;
        }

        return var;
    }

    template <typename T>
    debug_variable_t* add_variable(T val, std::string_view name) {
        std::lock_guard lock{ticket};
        auto* var = first_free ? first_free : arena_alloc<debug_variable_t>(&arena);
        if (first_free) { 
            node_next(first_free); 
            new (var) debug_variable_t;
        }
        var->name = name;
        if constexpr (std::is_same_v<T, zyy::entity_t>) { var->type = debug_variable_type::ENTITY; }
        if constexpr (std::is_same_v<T, u32>) { var->type = debug_variable_type::UINT32; }
        if constexpr (std::is_same_v<T, i32>) { var->type = debug_variable_type::INT32; }
        if constexpr (std::is_same_v<T, f32>) { var->type = debug_variable_type::FLOAT32; }
        if constexpr (std::is_same_v<T, f64>) { var->type = debug_variable_type::FLOAT64; }
        if constexpr (std::is_same_v<T, v2f>) { var->type = debug_variable_type::VEC2; }
        if constexpr (std::is_same_v<T, v3f>) { var->type = debug_variable_type::VEC3; }
        if constexpr (std::is_same_v<T, math::ray_t>) { var->type = debug_variable_type::RAY; }
        if constexpr (std::is_same_v<T, math::aabb_t<v3f>>) { var->type = debug_variable_type::AABB; }
        
        std::memcpy(&var->as_u32, &val, sizeof(val));

        node_push(var, variables);
        return var;
    }

    debug_watcher_t* has_watch_variable(void* ptr) {
        node_for(auto, watcher, var) {
            if ((void*)var->as_u32 == ptr) {
                return var;
            }
        }
        return 0;
    }

    template <typename T>
    debug_watcher_t* watch_variable(T* val, std::string_view name) {
        std::lock_guard lock{ticket};
        if (auto ovar = has_watch_variable((void*)val)) return ovar;
        auto* var = arena_alloc<debug_watcher_t>(&watch_arena);
        var->name = name;
        if constexpr (std::is_same_v<T, zyy::entity_t>) { var->type = debug_watcher_type::ENTITY; }
        if constexpr (std::is_same_v<T, const char>) { var->type = debug_watcher_type::CSTR; }
        if constexpr (std::is_same_v<T, u32>) { var->type = debug_watcher_type::UINT32; }
        if constexpr (std::is_same_v<T, i32>) { var->type = debug_watcher_type::INT32; }
        if constexpr (std::is_same_v<T, f32>) { var->type = debug_watcher_type::FLOAT32; }
        if constexpr (std::is_same_v<T, f64>) { var->type = debug_watcher_type::FLOAT64; }
        if constexpr (std::is_same_v<T, v2f>) { var->type = debug_watcher_type::VEC2; }
        if constexpr (std::is_same_v<T, v3f>) { var->type = debug_watcher_type::VEC3; }
        if constexpr (std::is_same_v<T, math::ray_t>) { var->type = debug_watcher_type::RAY; }
        if constexpr (std::is_same_v<T, math::aabb_t<v3f>>) { var->type = debug_watcher_type::AABB; }
        
        std::memcpy(&var->as_u32, &val, sizeof(val));

        node_push(var, watcher);
        return var;
    }

    template <typename T>
    void set_watch_variable(T val, std::string_view name) {
        std::lock_guard lock{ticket};
        node_for(auto, watcher, n) {
            if (n->name == name) {
                if constexpr (std::is_same_v<T, zyy::entity_t>) { assert(n->type == debug_watcher_type::ENTITY); }
                if constexpr (std::is_same_v<T, const char>) { assert(n->type == debug_watcher_type::CSTR); }
                if constexpr (std::is_same_v<T, u32>) { assert(n->type == debug_watcher_type::UINT32); }
                if constexpr (std::is_same_v<T, i32>) { assert(n->type == debug_watcher_type::INT32); }
                if constexpr (std::is_same_v<T, f32>) { assert(n->type == debug_watcher_type::FLOAT32); }
                if constexpr (std::is_same_v<T, f64>) { assert(n->type == debug_watcher_type::FLOAT64); }
                if constexpr (std::is_same_v<T, v2f>) { assert(n->type == debug_watcher_type::VEC2); }
                if constexpr (std::is_same_v<T, v3f>) { assert(n->type == debug_watcher_type::VEC3); }
                if constexpr (std::is_same_v<T, math::ray_t>) { assert(n->type == debug_watcher_type::RAY); }
                if constexpr (std::is_same_v<T, math::aabb_t<v3f>>) { assert(n->type == debug_watcher_type::AABB); }
                std::memcpy(n->as_u32, &val, sizeof(T));
            }
        }
    }
};


struct debug_console_t {
    struct command_t {
        void (*command)(void*){0};
        void* data{0};
    };

    struct message_t {
        char text[256]{};
        gfx::color32 color = gfx::color::rgba::white;
        command_t command{};
    };

    struct console_command_t {
        console_command_t* next{0};
        char name[64]{};
        command_t command{};
    };

    console_command_t console_commands[64]{};
    u32 command_count{0};

    message_t messages[256]{};
    size_t message_top = 0;

    i32 scroll{0};

    const message_t& last_message() const {
        return messages[message_top?message_top-1:array_count(messages)-1];
    }

    std::optional<f32> last_float() const {
        const auto& message = last_message();
        auto i = std::string_view{message.text}.find_first_of("0123456789.+");
        if (i != std::string_view::npos) {
            std::stringstream ss{std::string_view{message.text}.substr(i).data()};
            float f;
            ss >> f;
            return f;
        } else {
            return std::nullopt;
        }
    }

    char    text_buffer[1024]{};
    size_t  text_size{0};
};

inline static void 
console_add_command(
    debug_console_t* console,
    std::string_view command_name,
    void (*command)(void*),
    void* user_data
) {
    assert(console->command_count < array_count(console->console_commands));
    debug_console_t::console_command_t& new_command = console->console_commands[console->command_count++];

    std::memcpy(new_command.name, command_name.data(), std::min(array_count(new_command.name), command_name.size()));
    // strncpy_s(new_command.name, array_count(new_command.name), command_name.data(), command_name.size()+1);
    new_command.command.command = command;
    new_command.command.data = user_data;
}

inline static void 
console_try_command(
    debug_console_t* console,
    std::string_view command_name
) {
    range_u64(i, 0, console->command_count) {
        if (std::strstr(command_name.data(), console->console_commands[i].name)) {
            console->console_commands[i].command.command(
                console->console_commands[i].command.data
            );
            return;
        }
    }
}

inline static void
console_log(
    debug_console_t* console,
    std::string_view text,
    gfx::color32 color = gfx::color::rgba::white,
    void (*on_click)(void*) = 0,
    void* user_data = 0
) {
    size_t text_size = text.size();
    while(text_size > 0) {
        auto* message = console->messages + console->message_top;
        console->message_top = (console->message_top + 1) % array_count(console->messages);
        message->color = color;
        const auto write_count = std::min(array_count(message->text), text_size);
        utl::memzero(message->text, array_count(message->text));
        std::memcpy(message->text, text.data(), write_count);
        text_size -= write_count;

        message->command.data = user_data;
        message->command.command = on_click;
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

    const auto theme = imgui.theme;
    imgui.theme.border_radius = 1.0f;

    if (im::begin_panel(imgui, "Console"sv, pos, v2f{400.0f, 0.0f})) {
        // draw_reverse(imgui, console->messages);

        for (u64 i = 10; i <= 10; i--) {
            u64 index = (console->message_top - 1 - i) % array_count(console->messages);
            auto* message = console->messages + index;
            if (message->text[0]==0) {
                continue;
            }
            imgui.theme.text_color = message->color;
            if (im::text(imgui, fmt_sv("[{}] {}", index, message->text)) && message->command.command) {
                message->command.command(message->command.data);
            }
        }
        
        if (im::text_edit(imgui, console->text_buffer, &console->text_size, "console_text_box"_sid)) {
            console_log(console, console->text_buffer);
            console_try_command(console, console->text_buffer);
            utl::memzero(console->text_buffer, array_count(console->text_buffer));
            console->text_size = 0;
        }

        im::end_panel(imgui);
    }

    imgui.theme = theme;
}

#define DEBUG_STATE (*gs_debug_state)

#ifdef DEBUG_STATE
    #define DEBUG_WATCH(var) DEBUG_STATE.watch_variable((var), #var)
    #define DEBUG_ADD_VARIABLE(var) DEBUG_STATE.add_time_variable((var), #var)
    #define DEBUG_ADD_VARIABLE_(var, time) DEBUG_STATE.add_time_variable((var), #var, (time))
    #define DEBUG_SET_FOCUS(point) DEBUG_STATE.focus_point = (point)
    #define DEBUG_SET_FOCUS_DISTANCE(distance) DEBUG_STATE.focus_distance = (distance)
    #define DEBUG_SET_TIMEOUT(time) DEBUG_STATE.timeout = (time)
    #define DEBUG_STATE_DRAW(imgui, proj, view, viewport) DEBUG_STATE.draw(imgui, proj, view, viewport)
    #define DEBUG_STATE_DRAW_WATCH_WINDOW(imgui) DEBUG_STATE.draw_watch_window(imgui)
#else
    #define DEBUG_ADD_VARIABLE(var) 
    #define DEBUG_ADD_VARIABLE(var, time)
    #define DEBUG_SET_FOCUS(point) 
    #define DEBUG_SET_TIMEOUT(timeout)
    #define DEBUG_STATE_DRAW(imgui, proj, view, viewport)
    #define DEBUG_STATE_DRAW_WATCH_WINDOW(imgui)
#endif

debug_state_t* gs_debug_state;
