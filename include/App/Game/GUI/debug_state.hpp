#pragma once

#include "zyy_core.hpp"
// #include "App/Game/Entity/entity.hpp"

enum struct debug_priority_level {
    LOW, MEDIUM, HIGH
};

enum struct debug_variable_type {
    UINT32, INT32,
    FLOAT32, FLOAT64,
    VEC2, VEC3,
    AABB, RAY, TEXT,
    COUNT
};

enum struct debug_watcher_type {
    UINT32, INT32,
    FLOAT32, FLOAT64,
    VEC2, VEC3,
    AABB, RAY,
    // ENTITY, 
    CSTR,
    COUNT
};

struct debug_watcher_t {
    debug_watcher_t* next{0};
    std::string_view name{};

    debug_watcher_type type{};

    union {
        char* as_cstr;
        void* as_vptr;
        b32* as_b32;
        u32* as_u32;
        i32* as_i32;
        f32* as_f32;
        f64* as_f64;
        v2f* as_v2f;
        v3f* as_v3f;
        // math::rect3d_t as_aabb;
        // zyy::entity_t* as_entt;
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
        math::rect3d_t as_aabb{};
        math::ray_t as_ray;
        stack_string<32> as_text;
    };
};

struct debug_alert_t {
    // debug_alert_t* next = 0;
    string_buffer message{};
    f32 time;

    void make(arena_t* arena, std::string_view text, f32 t) {
        time = t;

        message = {};
        message.push(arena, std::span((char*)text.data(), text.size()));
    }
};


struct debug_console_t;
struct debug_state_t {
    f32& time;
    arena_t arena;
    arena_t watch_arena;

    std::mutex ticket;

    f32 timeout{1.0f};
    v3f focus_point{0.0f};
    f32 focus_distance{0.10f};

    f32 debug_alert_show_time{10.0f};
    u64 active_alert_count = 0;
    buffer<debug_alert_t> debug_alerts = {};

    void alert(std::string_view text) {
        if (active_alert_count < debug_alerts.count) {
            debug_alerts.data[active_alert_count++].make(&arena, text, time);
        } else {
            debug_alert_t debug_alert = {};
            debug_alert.make(&arena, text, time);
            debug_alerts.push(&arena, debug_alert);
            active_alert_count++;
        }
    }

    void sort_active_alerts() {
        debug_alerts.sort([](auto& a, auto& b) {
            return a.time < b.time;
        });

        active_alert_count = 0;
        range_u64(i, 0, debug_alerts.count) {
            if (time - debug_alerts.data[i].time < debug_alert_show_time) {
                active_alert_count++;
            }
        }
    }

    b32 has_alert() {
        return active_alert_count > 0;
    }

    v3f* selection = 0;

    debug_variable_t* variables{0};
    debug_variable_t* first_free{0};

    debug_watcher_t* watcher{0};
    debug_watcher_t* free_watch{0};

    char watcher_needle[64];
    size_t watcher_wpos{0};

    debug_console_t* console;

    debug_watcher_t* create_watcher() {
        if (free_watch) {
            auto* v = free_watch;
            node_next(free_watch);
            new (v) debug_watcher_t;
            return v;
        } else {
            // return push_struct<debug_watcher_t>(&watch_arena);
            tag_struct(auto* result, debug_watcher_t, &watch_arena);
            return result;
        }
    }

    void begin_frame() {
        if (watcher) {
            if (free_watch) {
                auto* free_last = free_watch;
                while(free_last->next) { node_next(free_last); }
                node_push(free_last, watcher);
            } else {
                free_watch = watcher;
            }
        }
        debug_variable_t* last = 0;
        for(auto* var = variables; var;) {
            if (time - var->time_stamp > timeout) {
                if (last) {
                    last->next = var->next;
                    node_push(var, first_free);
                    var = last->next;
                    continue;
                } else {
                    node_next(variables);
                    auto* ll = var->next;
                    node_push(var, first_free);
                    var = ll;
                    continue;
                }
            }
            last = var;
            node_next(var);
        }
        watcher = nullptr;
        // arena_clear(&watch_arena);
    }

    // void show_entity(gfx::gui::im::state_t& imgui, debug_watcher_t* var) {
    //     using namespace gfx::gui;
    //     assert(var->type == debug_watcher_type::ENTITY);
    //     auto* entity = var->as_entt;
    //     if (entity->name.c_data) {
    //         im::text(imgui, entity->name.c_data);
    //     } else {
    //         im::text(imgui, fmt_sv("<entity_{}>", (void*)entity));
    //     }
    //     if (im::text(imgui, fmt_sv("- Kill\0{}"sv, entity->id))) {
    //         entity->queue_free();
    //     }
    //     im::text(imgui, fmt_sv("- Tag: {}", entity->tag));
    //     im::text(imgui, fmt_sv("- Flags: {}", entity->flags));

    //     im::same_line(imgui);
    //     im::text(imgui, "- Local Origin: ");
    //     im::float3_drag(imgui, &entity->transform.origin);

    //     if (entity->stats.character.health.max > 0.0f) {
    //         auto [max, hp] = entity->stats.character.health;
    //         im::text(imgui, fmt_sv("- Health: {}/{}", max, hp));
    //     }
    // }

    void draw_watch_window(gfx::gui::im::state_t& imgui) {
        std::lock_guard lock{ticket};
        using namespace gfx::gui;

        const auto theme = imgui.theme;

        // imgui.theme.bg_color &= ~gfx::color::rgba::clear_alpha;
        // imgui.theme.bg_color = 0x0;
        // imgui.theme.fg_color = 0x0;

        local_persist v2f watch_pos{imgui.ctx.screen_size*0.2f};
        local_persist v2f watch_size{100.0f};
        local_persist v2f scroll_size{0.0f, 100.0f};
        local_persist f32 scroll;
        local_persist b32 watch_open{true};
 
        if (im::begin_panel(imgui, "Watch Window", &watch_pos, &watch_size, &watch_open)) {
            im::text(imgui, "Watch Window");

            im::text_edit(imgui, watcher_needle, &watcher_wpos, "watcher_text_box"_sid);

            im::begin_scroll_rect(imgui, &scroll, &scroll_size);
            

            u64 shown = 0;
            node_for(auto, watcher, var) {
                if (var->type == debug_watcher_type::CSTR) {
                    if ((std::strstr(var->as_cstr, watcher_needle) == nullptr) &&
                        (std::strstr(var->name.data(), watcher_needle) == nullptr)) continue;
                // } else if (var->type == debug_watcher_type::ENTITY) {
                //     if (var->as_entt->name.c_data==nullptr) continue;
                //     if ((std::strstr(var->as_entt->name.c_data, watcher_needle) == nullptr) &&
                //         (std::strstr(var->name.data(), watcher_needle) == nullptr)) continue;
                } else {
                    if (std::strstr(var->name.data(), watcher_needle) == nullptr) continue;
                }
                if (shown++ > 20) { break; }
                // if (var->type == debug_watcher_type::ENTITY) {
                    // show_entity(imgui, var);
                    // continue;
                // } else {
                    im::same_line(imgui);
                    im::text(imgui, fmt_sv("{}: ", var->name));
                // }
                if (var->type == debug_watcher_type::FLOAT32) {
                    im::float_input(imgui, var->as_f32);
                } else if (var->type == debug_watcher_type::UINT32) {
                    im::uint_input(imgui, var->as_u32);
                } else if (var->type == debug_watcher_type::CSTR) {
                    im::text(imgui, var->as_cstr);
                } else if (var->type == debug_watcher_type::VEC3) {
                    im::float3_input(imgui, var->as_v3f);
                } else {
                    im::text(imgui, "Unsupported type");
                }
            }
            im::end_scroll_rect(imgui, &scroll, &scroll_size);
            
            im::end_panel(imgui, &watch_pos, &watch_size);
        }

        imgui.theme = theme;
    }

    void draw(gfx::gui::im::state_t& imgui, const m44& proj, const m44& view, const v4f& viewport) {
        std::lock_guard lock{ticket};
        using namespace gfx::gui;
        debug_variable_t* last = nullptr;
        debug_variable_t* curr = variables;
        // while (curr && time - curr->time_stamp > timeout) {
        //     auto* next = curr->next;
        //     node_next(variables);
        //     node_push(curr, first_free);
        //     curr = next;
        // }
        // while (curr) {
        //     if (time - curr->time_stamp > timeout) {
        //         last->next = curr->next;
        //         node_push(curr, first_free);
        //         curr = last->next;
        //     } else {
        //         last = curr;
        //         node_next(curr);
        //     }
        // }

        imgui.begin_free_drawing();
        defer {
            imgui.end_free_drawing();
        };

        math::rect3d_t viewable{v3f{0.0f}, v3f{viewport.z, viewport.w, 1.0f}};
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
        // auto* var = first_free ? first_free : push_struct<debug_variable_t>(&arena);
        auto* var = first_free;
        if (first_free) { 
            node_next(first_free); 
            new (var) debug_variable_t;
        } else {
            tag_struct(var, debug_variable_t, &arena);
        }
        var->name = name;
        // if constexpr (std::is_same_v<T, zyy::entity_t>) { var->type = debug_variable_type::ENTITY; }
        if constexpr (std::is_same_v<T, u32>) { var->type = debug_variable_type::UINT32; }
        if constexpr (std::is_same_v<T, i32>) { var->type = debug_variable_type::INT32; }
        if constexpr (std::is_same_v<T, f32>) { var->type = debug_variable_type::FLOAT32; }
        if constexpr (std::is_same_v<T, f64>) { var->type = debug_variable_type::FLOAT64; }
        if constexpr (std::is_same_v<T, v2f>) { var->type = debug_variable_type::VEC2; }
        if constexpr (std::is_same_v<T, v3f>) { var->type = debug_variable_type::VEC3; }
        if constexpr (std::is_same_v<T, math::ray_t>) { var->type = debug_variable_type::RAY; }
        if constexpr (std::is_same_v<T, math::rect3d_t>) { var->type = debug_variable_type::AABB; }
        if constexpr (std::is_same_v<T, std::string_view>) { 
            var->type = debug_variable_type::TEXT; 
            utl::copy(&var->as_text.buffer, val.data(), val.size());
        } else {
            utl::copy(&var->as_u32, &val, sizeof(val));
        }

        node_push(var, variables);
        return var;
    }

    debug_watcher_t* has_watch_variable(void* ptr) {
        node_for(auto, watcher, var) {
            if (var->as_vptr == ptr) {
                return var;
            }
        }
        return 0;
    }

    template <typename T>
    debug_watcher_t* watch_variable(T* val, std::string_view name) {
        std::lock_guard lock{ticket};
        if (auto cached = has_watch_variable((void*)val)) return cached;
        auto* var = create_watcher();
        var->name = name;
        // if constexpr (std::is_same_v<T, zyy::entity_t>) { var->type = debug_watcher_type::ENTITY; }
        if constexpr (std::is_same_v<T, const char>) { var->type = debug_watcher_type::CSTR; }
        if constexpr (std::is_same_v<T, u32>) { var->type = debug_watcher_type::UINT32; }
        if constexpr (std::is_same_v<T, i32>) { var->type = debug_watcher_type::INT32; }
        if constexpr (std::is_same_v<T, f32>) { var->type = debug_watcher_type::FLOAT32; }
        if constexpr (std::is_same_v<T, f64>) { var->type = debug_watcher_type::FLOAT64; }
        if constexpr (std::is_same_v<T, v2f>) { var->type = debug_watcher_type::VEC2; }
        if constexpr (std::is_same_v<T, v3f>) { var->type = debug_watcher_type::VEC3; }
        if constexpr (std::is_same_v<T, math::ray_t>) { var->type = debug_watcher_type::RAY; }
        if constexpr (std::is_same_v<T, math::rect3d_t>) { var->type = debug_watcher_type::AABB; }
        
        utl::copy(&var->as_vptr, &val, sizeof(val));

        node_push(var, watcher);
        return var;
    }

    template <typename T>
    void set_watch_variable(T val, std::string_view name) {
        std::lock_guard lock{ticket};
        node_for(auto, watcher, n) {
            if (n->name == name) {
                // if constexpr (std::is_same_v<T, zyy::entity_t>) { assert(n->type == debug_watcher_type::ENTITY); }
                if constexpr (std::is_same_v<T, const char>) { assert(n->type == debug_watcher_type::CSTR); }
                if constexpr (std::is_same_v<T, u32>) { assert(n->type == debug_watcher_type::UINT32); }
                if constexpr (std::is_same_v<T, i32>) { assert(n->type == debug_watcher_type::INT32); }
                if constexpr (std::is_same_v<T, f32>) { assert(n->type == debug_watcher_type::FLOAT32); }
                if constexpr (std::is_same_v<T, f64>) { assert(n->type == debug_watcher_type::FLOAT64); }
                if constexpr (std::is_same_v<T, v2f>) { assert(n->type == debug_watcher_type::VEC2); }
                if constexpr (std::is_same_v<T, v3f>) { assert(n->type == debug_watcher_type::VEC3); }
                if constexpr (std::is_same_v<T, math::ray_t>) { assert(n->type == debug_watcher_type::RAY); }
                if constexpr (std::is_same_v<T, math::rect3d_t>) { assert(n->type == debug_watcher_type::AABB); }
                utl::copy(n->as_vptr, &val, sizeof(T));
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

    void* user_data = 0; // store game_state

    console_command_t console_commands[64]{};
    u32 command_count{0};

    message_t messages[256]{};
    size_t message_top = 0;

    // i32 scroll{0};
    f32 open_percent = 0.0f;

    const message_t& top_message() const {
        return messages[message_top];
    }

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

    std::optional<std::string_view> get_args() const {
        const auto& message = last_message();
        auto i = std::string_view{message.text}.find_first_of(" ");
        if (i != std::string_view::npos) {
            return std::string_view{message.text}.substr(i);
        } else {
            return std::nullopt;
        }
    }

    char    text_buffer[1024]{};
    size_t  text_size{0};
    size_t  text_position{0};
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

    utl::copy(new_command.name, command_name.data(), std::min(array_count(new_command.name), command_name.size()));
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
    void* user_data = 0,
    b32 execute = 0
) {
    size_t text_size = text.size();

    while(text_size > 0) {
        auto* message = console->messages + console->message_top;
        console->message_top = (console->message_top + 1) % array_count(console->messages);
        message->color = color;
        const auto write_count = std::min(array_count(message->text), text_size);
        utl::memzero(message->text, array_count(message->text));
        utl::copy(message->text, text.data(), write_count);
        text_size -= write_count;

        message->command.data = user_data;
        message->command.command = on_click;
    }
    if (execute) console_try_command(console, text);
}

inline void
draw_console(
    gfx::gui::im::state_t& imgui, 
    debug_console_t* console,
    v2f* pos
) {
    using namespace gfx::gui;
    auto* c = &imgui.ctx;

    imgui.begin_free_drawing();
    auto* save_font = c->font;
    c->font = c->dyn_font[24];
    defer {
        // imgui.end_free_drawing();
        c->font = save_font;
    };

    imgui.hot =
    imgui.active = "DEBUG_console"_sid;


    f32 open_prc = console->open_percent;
    auto mouse = imgui.ctx.input->mouse.pos2();
    auto clicked = imgui.ctx.input->mouse.buttons[0];
    auto entered = imgui.ctx.input->pressed.keys[key_id::ENTER];
    auto deleted = imgui.ctx.input->pressed.keys[key_id::BACKSPACE];
    auto ctrl = imgui.ctx.input->keys[key_id::LEFT_CONTROL];
    auto left = imgui.ctx.input->pressed.keys[key_id::LEFT];
    auto right = imgui.ctx.input->pressed.keys[key_id::RIGHT];

    const auto test_font_size = gfx::font_get_size(imgui.ctx.font, "Hello World!");

    auto text_box_color = gfx::color::rgba::dark_red;

    auto r = imgui.ctx.screen_rect();
    math::rect2d_t text_box;
    math::rect2d_t _t;

    r.add(-math::height2 * r.size().y * (1.0f-open_prc));

    draw_rect(c, r, imgui.theme.bg_color);


    {
        auto text_color = gfx::color::rgba::cream;

        std::tie(text_box, r) = math::cut_bottom(r, test_font_size.y);
        std::tie(_t, r) = math::cut_left(r, 8.0f);
        
        draw_rect(c, text_box, text_box_color);

        string_render(c, console->text_buffer, text_box.min, text_color);

        auto pos_size = gfx::font_get_size(c->font, std::string_view{console->text_buffer, console->text_position});
        auto [before_cursor, after_cursor] = math::cut_left(text_box, pos_size.x);

        math::rect2d_t cursor;
        cursor.min = math::top_right(before_cursor);
        cursor.max = before_cursor.max;
        cursor.min.x -= 3.0f;
        draw_rect(c, cursor, gfx::color::rgba::yellow);
    
        auto key = c->input->keyboard_input();
        if (left) { 
            if (console->text_position) {
                console->text_position--;
            }
        } else if (right) {
            if (console->text_position < console->text_size) {
                console->text_position++;
            }
        }
        if (key > 0) {
            if (console->text_size + 1 < array_count(console->text_buffer)) {
                if (console->text_position == console->text_size) {
                    console->text_buffer[console->text_position++] = key;
                } else {
                    utl::copy(console->text_buffer + console->text_position + 1, console->text_buffer + console->text_position, console->text_size - console->text_position - 1);
                    console->text_buffer[console->text_position++] = key;
                }
                console->text_size++;
            }
        }
        if (entered) {
            console_log(console, console->text_buffer, gfx::color::rgba::white, 0, 0, 1);
            
            utl::memzero(console->text_buffer, array_count(console->text_buffer));

            console->text_position = 
            console->text_size = 0;
        }
        if (deleted) {
            if (ctrl) {
                utl::memzero(console->text_buffer, console->text_position+1);

                utl::copy(console->text_buffer, console->text_buffer + console->text_position, console->text_size - console->text_position);

                console->text_size -= console->text_position;
                console->text_position = 0;
            } else if (console->text_position > 0) {
                if (console->text_position == console->text_size) {
                    console->text_position -= 1;
                    console->text_buffer[console->text_position] = 0;
                } else {
                    console->text_position -= 1;
                    utl::copy(console->text_buffer + console->text_position, console->text_buffer + console->text_position + 1, console->text_size - console->text_position + 1);
                }
                console->text_size -= 1;
            }
        }
    }

    r = c->screen_rect().clip(r);

    local_persist f32 scroll;
    local_persist f32 scroll_v;

    scroll_v += c->input->mouse.scroll.y * 3.0f;
    scroll   += scroll_v * c->input->dt;
    scroll_v = tween::damp(scroll_v, 0.0f, 0.95f, c->input->dt);

    imgui.end_free_drawing();
    imgui.scissor(r);
    imgui.begin_free_drawing();

    r.add(math::height2 * scroll);

    defer {
        imgui.end_free_drawing();
        imgui.end_scissor();
    };

    // for (u64 i = 10; i <= 10; i--) {
    for (u64 i = 0; i < array_count(console->messages); i++) {
        u64 index = (console->message_top - 1 - i) % array_count(console->messages);
        auto* message = console->messages + index;
        if (message->text[0]==0) {
            continue;
        }
        auto text_color = message->color;

        std::tie(text_box, r) = math::cut_bottom(r, test_font_size.y);
        // std::tie(text_box, r) = math::cut_top(r, test_font_size.y);
        string_render(c, message->text, text_box.min, text_color);

        if (text_box.contains(mouse) && clicked) {
            message->command.command(message->command.data);
        }
    }
}

inline void
draw_console2(
    gfx::gui::im::state_t& imgui, 
    debug_console_t* console,
    v2f* pos
) {
    using namespace gfx::gui;
    

    const auto theme = imgui.theme;
    imgui.theme.border_radius = 1.0f;
    v2f size={};
    local_persist b32 open=1;

    if (im::begin_panel(imgui, "Console"sv, pos, &size, &open)) {
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
            console_log(console, console->text_buffer, gfx::color::rgba::white, 0, 0, 1);
            
            utl::memzero(console->text_buffer, array_count(console->text_buffer));
            console->text_size = 0;
        }

        im::end_panel(imgui, pos, &size);
    }

    imgui.theme = theme;
}

#define DEBUG_STATE (*gs_debug_state)

#ifdef DEBUG_STATE
    #define DEBUG_WATCH(var) DEBUG_STATE.watch_variable((var), #var)
    #define DEBUG_DIAGRAM(var) DEBUG_STATE.add_time_variable((var), #var)
    #define DEBUG_DIAGRAM_(var, time) DEBUG_STATE.add_time_variable((var), #var, (time))
    #define DEBUG_SET_FOCUS(point) DEBUG_STATE.focus_point = (point)
    #define DEBUG_SET_FOCUS_DISTANCE(distance) DEBUG_STATE.focus_distance = (distance)
    #define DEBUG_SET_TIMEOUT(time) DEBUG_STATE.timeout = (time)
    #define DEBUG_STATE_DRAW(imgui, proj, view, viewport) DEBUG_STATE.draw(imgui, proj, view, viewport)
    #define DEBUG_STATE_DRAW_WATCH_WINDOW(imgui) DEBUG_STATE.draw_watch_window(imgui)
    #define DEBUG_ALERT(msg) DEBUG_STATE.alert(msg)
    #define CLOG(text) console_log(DEBUG_STATE.console, (text), gfx::color::rgba::white, 0, 0, 1)
    #define FLOG(text, ...) console_log(DEBUG_STATE.console, fmt_sv((text), __VA_ARGS__), gfx::color::rgba::white, 0, 0, 1)
    
    #define aslert(expr) do { if (!(expr)) { zyy_warn(__FUNCTION__, "{}", #expr); DEBUG_STATE.alert(fmt_sv("Assertion Failed: {}:{} - {}", ::utl::trim_filename(__FILE__), __LINE__, #expr)); } } while(0)
    #undef assert
    #define assert(expr) aslert(expr)

    #define assert_false(expr) do { assert((expr)); if(!(expr)) return false; } while (0)
    #define assert_true(expr) do { assert((expr)); if(!(expr)) return true; } while (0)
    #define assert_continue(expr) if(!(expr)) { assert(!#expr); continue; }

#else
    // #define DEBUG_STATE 
    #define DEBUG_WATCH(var) 
    #define DEBUG_DIAGRAM(var) 
    #define DEBUG_DIAGRAM_(var, time) 
    #define DEBUG_SET_FOCUS(point) 
    #define DEBUG_SET_FOCUS_DISTANCE(distance)
    #define DEBUG_SET_TIMEOUT(timeout)
    #define DEBUG_STATE_DRAW(imgui, proj, view, viewport)
    #define DEBUG_STATE_DRAW_WATCH_WINDOW(imgui)
    #define DEBUG_ALERT(msg)
    #define CLOG(text) 
    #define FLOG(text, ...)


    #define assert_false(expr) do { assert((expr)); if(!(expr)) return false; } while (0)
    #define assert_true(expr) do { assert((expr)); if(!(expr)) return true; } while (0)
    #define assert_continue(expr) if(!(expr)) { assert(!#expr); continue; }
#endif

debug_state_t* gs_debug_state;
