#ifndef CVAR_HPP
#define CVAR_HPP

#include "core.hpp"

namespace cvar {
    enum struct type : u32 {
        int, float, str, color
    };
    enum struct flag : u32 {
        none = 0,
        no_edit = BIT(1),
        edit_read_only = BIT(2),
        advanced = BIT(3),

        edit_checkbox = BIT(8),
        edit_float_slider = BIT(9),
    };

    struct param_t {
        u64 index{};

        cvar::type  type{};
        cvar::flag  flags{};
        string_t    name{};
        string_t    description{};
    };

    template <typename T>
    struct storage_t {
        T initial;
        T current;
        cvar::param_t* parameter{0};
    };

    template <typename T>
    struct var_array_t {
        cvar::storage_t<T>* cvars{0};
        u64                 last_cvar{0};

        var_array_t(size_t size) {
            cvars = new cvar::storage_t<t>[size]();
        }
        virtual ~var_array_t() { 
            delete cvars;
        }

        const T& operator[](size_t i) const {
            return cvars[i].current;
        }

        T& operator[](size_t i) {
            return cvars[i].current;
        }

        u64 append(const T& value, cvar::param_t* param) {
            u64 index = last_cvar++;

            cvars[index].current = value;
            cvars[index].initial = value;
            cvars[index].parameter = param;

            return param->index = index;
        }
    };

    struct system_t {
        constexpr static size_t max_int_cvars = 100;
        cvar::var_array_t<i32> int_cvars{max_int_cvars};

        constexpr static size_t max_float_cvars = 100;
        cvar::var_array_t<f32> float_cvars{max_float_cvars};

        constexpr static size_t max_string_cvars = 100;
        cvar::var_array_t<string_t> string_cvars{max_string_cvars};

        constexpr static size_t max_color_cvars = 100;
        cvar::var_array_t<gfx::color32> color_cvars{max_color_cvars};

        utl::str_hash_t saved_cvars;

        system_t() {
            utl::str_hash_create(saved_cvars);
        }

        template <typename T>
        cvar::var_array_t<T>* get_array();

        template <>
        cvar::var_array_t<i32>* get_array() {
            return &int_cvars;
        }

        template <>
        cvar::var_array_t<f32>* get_array() {
            return &float_cvars;
        }

        template <>
        cvar::var_array_t<string_t>* get_array() {
            return &string_cvars;
        }

        template <>
        cvar::var_array_t<gfx::color32>* get_array() {
            return &color_cvars;
        }

        cvar::param_t* get_cvar(sid_t hash) {

        }

        cvar::param_t* init_cvar(std::string_view name, std::string_view description) {
            const sid_t name_hash = sid(name);
            if (get_cvar(name_hash)) {
                return nullptr;
            }

            utl::str_hash_add(saved_cvars, name, )
        }



        cvar::param_t* create_float_cvar(std::string_view name, std::string_view description, f32 default, f32 current) {

        }
    };
};

#endif