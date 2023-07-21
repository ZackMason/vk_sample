#pragma once

#include "core.hpp"

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
        // math::aabb_t<v3f> as_aabb;
        math::ray_t as_ray;
    };
};

struct debug_state_t {
    f32& time;
    arena_t arena;

    f32 timeout{1.0f};
    v3f focus_point{0.0f};
    f32 focus_distance{10.0f};

    debug_variable_t* variables{0};
    debug_variable_t* first_free{0};

    void begin_frame() { 
        // variables = nullptr;

        // arena_clear(&arena);
    }

    void draw(gfx::gui::im::state_t& imgui, const m44& proj, const m44& view, const v4f& viewport) {
        using namespace gfx::gui;
        debug_variable_t* last = nullptr;
        debug_variable_t* curr = variables;
        while (curr && time - curr->time_stamp > timeout) {
            curr = variables = curr->next;
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
    debug_variable_t* add_time_variable(T val, std::string_view name) {
        auto* var = add_variable(val, name);
        var->time_stamp = time;

        return var;
    }

    template <typename T>
    debug_variable_t* add_variable(T val, std::string_view name) {
        auto* var = first_free ? first_free : arena_alloc<debug_variable_t>(&arena);
        if (first_free) { node_next(first_free); }
        var->name = name;
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

};

#define DEBUG_STATE (*gs_debug_state)
#define DEBUG_ADD_VARIABLE(var) DEBUG_STATE.add_time_variable((var), #var)
#define DEBUG_SET_FOCUS(point) DEBUG_STATE.focus_point = (point)
#define DEBUG_SET_TIMEOUT(timeout) DEBUG_STATE.timeout = (timeout)
#define DEBUG_STATE_DRAW(imgui, proj, view, viewport) DEBUG_STATE.draw(imgui, proj, view, viewport)

debug_state_t* gs_debug_state;
