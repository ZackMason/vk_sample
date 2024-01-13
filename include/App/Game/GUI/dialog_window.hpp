#pragma once

#include "ztd_core.hpp"

struct dialog_window_t {
    u64 write_pos{0};
    char text_buffer[512];

    std::string_view title{};
    std::string_view description{};
    v2f position{0.0f};
    v2f size{100.0f};
    u32 done{0}; // 1 == done, 2 == closed
    b32 open{1};

    dialog_window_t& set_title(std::string_view title_) {
        title = title_;
        return *this;
    }

    dialog_window_t& set_description(std::string_view description_) {
        description = description_;
        return *this;
    }

    dialog_window_t& set_position(v2f position_) {
        position = position_;
        return *this;
    }

    dialog_window_t& set_size(v2f size_) {
        size = size_;
        return *this;
    }

    dialog_window_t& draw(gfx::gui::im::state_t& imgui, std::function<void(void)> fn = []{}) {
        using namespace gfx::gui;
        
        auto [open_, want_to_close] = im::begin_panel(imgui, title, &position, &size, &open);
        if (open_) {
            im::horizontal_text_center(imgui, title);
            im::text(imgui, title);
            if (description.empty() == false) {
                im::text(imgui, description);
            }

            fn();

            done = im::text_edit(imgui, text_buffer, &write_pos, "dialog_window::text_buffer"_sid);

            im::end_panel(imgui, &position, &size);
        }
        if (want_to_close) {
            done = 2;
        }

        return *this;
    }

    void clear() {
        write_pos = 0;
        utl::memzero(text_buffer, array_count(text_buffer));
    }

    void into(f32& f) {
        if (done == 1) {
            f = (f32)std::atof(text_buffer);
            clear();
        }
    }

    void into(i32& i) {
        if (done == 1) {
            i = (i32)std::atoi(text_buffer);
            clear();
        }
    }

    void into(std::span<char> t) {
        if (done == 1) {
            utl::copy(t.data(), text_buffer, std::min(t.size(), write_pos));
            clear();
        }
    }
};
