#pragma once

struct dialog_window_t {
    u64 write_pos{0};
    char text_buffer[512];

    std::string_view title{};
    std::string_view description{};
    v2f position{0.0f};
    v2f size{100.0f};
    b32 done{0};
    b32 open{0};

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

    dialog_window_t& draw(gfx::gui::im::state_t& imgui) {
        using namespace gfx::gui;

        if (im::begin_panel(imgui, title, position, size)) {

            im::text(imgui, title);
            if (description.empty() == false) {
                im::text(imgui, description);
            }

            done = im::text_edit(imgui, text_buffer, &write_pos, "dialog_window::text_buffer"_sid);

            im::end_panel(imgui);
        }

        return *this;
    }

    void clear() {
        write_pos = 0;
        utl::memzero(text_buffer, array_count(text_buffer));
    }

    void into(f32& f) {
        if (done) {
            f = (f32)std::atof(text_buffer);
            clear();
        }
    }

    void into(i32& i) {
        if (done) {
            i = (i32)std::atoi(text_buffer);
            clear();
        }
    }

    void into(std::span<char> t) {
        if (done) {
            utl::copy(t.data(), text_buffer, std::min(t.size(), write_pos));
            clear();
        }
    }
};

u32
load_save_particle_dialog(
    gfx::gui::im::state_t& imgui,
    particle_system_t* system,
    u32 load
) {
    local_persist dialog_window_t dialog_box{};
    char file[512]{};
    utl::memzero(file, array_count(file));

    dialog_box
        .set_title(load?"Load Particle System":"Save Particle System")
        .set_description("Enter filename")
        .set_position(v2f{400.0f,300.0f})
        .draw(imgui)
        .into(file);

    if (!file[0]) {
        return false;
    }

    if (load) {
        particle_system_settings_load(system, file);
    } else {
        particle_system_settings_save(*system, file);
    }
    return true;
}

void draw_behavior(
    gfx::gui::im::state_t& imgui,
    bt::behavior_t* node
) {
    auto& rect = node->rect;

    v2f cursor = rect.min;
    const math::rect2d_t screen{v2f{0.0f}, v2f{imgui.ctx.screen_size}};


    if (auto* comp = dynamic_cast<bt::active_selector_t*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Active Selector", &cursor);
    }
    else if (auto* gtr = dynamic_cast<bt::greater_condition_t<f32>*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Greater Than (f32)", &cursor);
        gfx::gui::string_render(&imgui.ctx, gtr->name, &cursor);
    }
    else if (auto* alwys = dynamic_cast<bt::always_succeed_t*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Always Succeed", &cursor);
    }
    else if (auto* cond = dynamic_cast<bt::condition_t*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Condition", &cursor);
        gfx::gui::string_render(&imgui.ctx, cond->name, &cursor);
    }
    else if (auto* sele = dynamic_cast<bt::selector_t*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Selector", &cursor);
    }
    else if (auto* seq = dynamic_cast<bt::sequence_t*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Sequence", &cursor);
    }
    else if (auto* prt = dynamic_cast<print_t*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Print", &cursor);
        gfx::gui::string_render(&imgui.ctx, prt->text, &cursor);
    }
    else if (auto* move = dynamic_cast<move_toward_t*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Move", &cursor);
        gfx::gui::string_render(&imgui.ctx, move->text, &cursor);
    }
    else if (auto* flee = dynamic_cast<run_away_t*>(node)) {
        gfx::gui::string_render(&imgui.ctx, "Flee", &cursor);
        gfx::gui::string_render(&imgui.ctx, flee->text, &cursor);
    }
    else if (auto* brkpnt = dynamic_cast<breakpoint_t*>(node)) {
        auto temp_cursor = cursor;
        gfx::gui::string_render(&imgui.ctx, "Breakpoint", &temp_cursor);
    }
    else {
        gfx::gui::string_render(&imgui.ctx, "Node", &cursor);
    }

    rect.expand(cursor);

    auto text_size = gfx::font_get_size(imgui.ctx.font, "Active Selector wow");
    rect.expand(cursor+v2f{text_size.x,0.0f});

    gfx::gui::draw_round_rect(&imgui.ctx, rect, 4.0f, imgui.theme.bg_color);

    auto save_rect = rect;
    
    rect.pull(v2f{3.0f});
    gfx::color32 colors[]={
        gfx::color::rgba::green,
        gfx::color::rgba::red,
        gfx::color::rgba::white,
        gfx::color::rgba::purple,
        gfx::color::rgba::gray,
    };
    
    gfx::gui::draw_round_rect(&imgui.ctx, rect, 4.0f, colors[(u32)node->status]);
    rect = save_rect;

    auto draw_node = [&](auto* c, auto rect, auto&& i) {
        if (c->rect.min == v2f{0.0f}) {
            c->rect.min = math::bottom_middle(rect) + rect.size() * 1.3f * v2f{f32(i), 1.0f};
            c->rect.max = c->rect.min + v2f{120.0f, 32.0f};
        }

        draw_behavior(imgui, c);
        i += 1.0f;


        v2f p[4] = {
            math::bottom_middle(rect),
            v2f{0.0f, 200.0f},
            math::top_middle(c->rect),
            v2f{0.0f, 200.0f},
        };

        auto dist = glm::distance(p[0], p[2]);
        p[1][1] = dist;
        p[3][1] = dist;

        gfx::gui::draw_curve(&imgui.ctx, std::span<v2f,4>{p,4}, math::rect2d_t{v2f{0.0f}, v2f{1.0f}}, 32, 3.0f, colors[(u32)c->status]);
        gfx::gui::draw_circle(&imgui.ctx, p[0], 6.0f, colors[(u32)c->status]);
        gfx::gui::draw_curve(&imgui.ctx, std::span<v2f,4>{p,4}, math::rect2d_t{v2f{0.0f}, v2f{1.0f}}, 32, 3.0f, colors[(u32)c->status]);
        gfx::gui::draw_circle(&imgui.ctx, p[2], 6.0f, colors[(u32)c->status]);

    };

    if (auto* dec = dynamic_cast<bt::decorator_t*>(node)) {
        draw_node(dec->child, rect, 0.0f);
    }

    if (auto* comp = dynamic_cast<bt::composite_t*>(node)) {
        f32 i = -1.0f;
        for (auto* c = comp->head.next;
            c != &comp->head;
            c = c->next
        ) {
            draw_node(c, rect, i);
            // gfx::gui::draw_line(&imgui.ctx, c->rect.center(), rect.center(), 3.0f, colors[(u32)c->status]);
        }
    }
    auto save_size = rect.size();
    auto save_min = rect.min;
    auto save_max = rect.max;
    gfx::gui::im::point_edit(imgui, &rect.min, screen, screen, gfx::color::rgba::red);
    gfx::gui::im::point_edit(imgui, &rect.max, screen, screen, gfx::color::rgba::red);
    if (rect.min != save_min) {
        rect.max = rect.min + save_size;
    }
    if (rect.max != save_max) {
        rect.min = rect.max - save_size;
    }
    
}

void 
draw_behavior_tree(
    gfx::gui::im::state_t& imgui,
    bt::behavior_tree_t* tree
) {
    auto* root = tree->root;
    
    if (root) {
        if (root->rect.min == v2f{0.0f}) {
            root->rect.min = v2f{500.0f};
            root->rect.max = v2f{600.0f, 532.0f};
        }
        draw_behavior(imgui, root);
    }

}

void
watch_game_state(game_state_t* game_state) {
    auto* rs = game_state->render_system;
    auto* time_scale = &game_state->time_scale;
    DEBUG_WATCH(time_scale)->max_f32 = 2.0f;
    // auto* window_size = &game_state->game_memory->config.graphics_config.window_size;
    // DEBUG_WATCH(window_size);
    DEBUG_WATCH(&game_state->gui.state.theme.shadow_distance);

    DEBUG_WATCH(&gs_rtx_on)->max_u32 = 2;
    auto* rtx_super_sample = &game_state->render_system->rt_cache->constants.super_sample;
    DEBUG_WATCH(rtx_super_sample)->max_u32 = 2;

    auto* pp_tonemap = &rs->postprocess_params.data[0];
    auto* pp_exposure = &rs->postprocess_params.data[1];
    auto* pp_contrast = &rs->postprocess_params.data[2];
    auto* pp_gamma = &rs->postprocess_params.data[3];
    DEBUG_WATCH(pp_tonemap)->max_f32 = 3.0f;
    DEBUG_WATCH(pp_exposure)->max_f32 = 2.0f;
    DEBUG_WATCH(pp_contrast);
    DEBUG_WATCH(pp_gamma)->max_f32 = 2.5f;

    auto& light_probe_settings = rs->light_probe_settings_buffer.pool[0];
    auto* probe_hysteresis = &light_probe_settings.hysteresis;
    auto* probe_gi_boost = &light_probe_settings.boost;
    auto* probe_depth_sharpness = &light_probe_settings.depth_sharpness;

    DEBUG_WATCH(probe_hysteresis)->max_f32 = 0.1f;
    DEBUG_WATCH(probe_gi_boost)->max_f32 = 2.0f;
    DEBUG_WATCH(probe_depth_sharpness)->max_f32 = 100.0f;

    DEBUG_WATCH(&bloom_filter_radius)->max_f32 = 0.01f;
}

void 
set_ui_textures(game_state_t* game_state) {
    auto* rs = game_state->render_system;
    auto& vk_gfx = *rs->vk_gfx;

    gfx::vul::texture_2d_t* ui_textures[4096];

    auto descriptor_set_layout_binding = gfx::vul::utl::descriptor_set_layout_binding(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, array_count(ui_textures)
    );
    for(size_t i = 0; i < array_count(ui_textures); i++) { ui_textures[i] = game_state->default_font_texture; }
    ui_textures[1] = &rs->frame_images[0].texture;
    ui_textures[2] = &rs->light_probes.irradiance_texture;
    ui_textures[3] = &rs->light_probes.visibility_texture;

    auto& gui_pipeline = game_state->render_system->pipelines.gui;
    
    gui_pipeline.set_count = 1;
    gfx::vul::create_descriptor_set_layouts(
        vk_gfx.device,
        &descriptor_set_layout_binding,
        gui_pipeline.set_count,
        gui_pipeline.set_layouts
    );

    gui_pipeline.layout = gfx::vul::create_pipeline_layout(
        vk_gfx.device,
        gui_pipeline.set_layouts, gui_pipeline.set_count, sizeof(m44)*2
    );


    game_state->default_font_descriptor = vk_gfx.create_image_descriptor_set(
        vk_gfx.descriptor_pool,
        gui_pipeline.set_layouts[0],
        ui_textures, array_count(ui_textures)
    );
    

}



void 
draw_gui(game_memory_t* game_memory) {
    TIMED_FUNCTION;
    game_state_t* game_state = get_game_state(game_memory);
    auto* input = &game_memory->input;

    local_persist f32 test_float = 0.0f;
    local_persist dialog_window_t dialog{
        .open = 0,
    };

    watch_game_state(game_state);

    auto* render_system = game_state->render_system;

    const u64 frame{++game_state->gui.ctx.frame};
    auto string_mark = arena_get_mark(&game_state->string_arena);

    arena_t* display_arenas[] = {
        &game_state->main_arena,
        &game_state->temp_arena,
        &game_state->string_arena,
        &game_state->mesh_arena,
        &game_state->texture_arena,
#ifdef DEBUG_STATE
        &DEBUG_STATE.arena,
        &DEBUG_STATE.watch_arena,
#endif
        &game_state->game_world->arena,
        &game_state->game_world->particle_arena,
        &game_state->game_world->frame_arena.arena[0],
        &game_state->game_world->frame_arena.arena[1],
        // &game_state->game_world->physics->default_allocator.arena,
        // &game_state->game_world->physics->default_allocator.heap_arena,
        &game_state->render_system->arena,
        &game_state->render_system->frame_arena,
    };
    utl::pool_base_t* display_pools[] = {
        &game_state->render_system->scene_context->vertices.pool,
        &game_state->render_system->scene_context->indices.pool,
        &game_state->gui.vertices[!(frame&1)].pool,
        &game_state->gui.indices[!(frame&1)].pool,
        &game_state->render_system->instance_storage_buffer.pool,
        &game_state->render_system->scene_context->entities.pool,
        &game_state->render_system->job_storage_buffers[(frame&1)].pool,
    };

    const char* display_arena_names[] = {
        "- Main Arena",
        "- Temp Arena",
        "- String Arena",
        "- Mesh Arena",
        "- Texture Arena",
#ifdef DEBUG_STATE
        "- Debug Arena",
        "- Debug Watch Arena",
#endif
        "- World Arena",
        "- World Particle Arena",
        "- World Frame 0",
        "- World Frame 1",
        // "- Physics Arena",
        // "- Physics Heap",
        "- Rendering Arena",
        "- Rendering Frame Arena",
    };
    const char* display_pool_names[] = {
        "- 3D Vertex",
        "- 3D Index",
        "- 2D Vertex",
        "- 2D Index",
        "- Instance Buffer",
        "- Entity Buffer",
        "- Render Job Buffer",
    };

    // std::lock_guard lock{game_state->render_system->ticket};
    gfx::gui::ctx_clear(&game_state->gui.ctx, 
        &game_state->gui.vertices[(frame&1)].pool, &game_state->gui.indices[(frame&1)].pool);
    
    const auto dt = game_memory->input.dt;

    game_state->time_text_anim -= dt;
    if ((gs_reload_time -= dt) > 0.0f) {
        gfx::gui::string_render(
            &game_state->gui.ctx, 
            string_t{}.view("Code Reloaded"),
            game_state->gui.ctx.screen_size/v2f{2.0f,4.0f} - gfx::font_get_size(&game_state->default_font, "Code Reloaded")/2.0f, 
            gfx::color::to_color32(v4f{0.80f, .9f, .70f, gs_reload_time}),
            &game_state->default_font
        );
    }
    if ((gs_jump_load_time -= dt) > 0.0f) {
        gfx::gui::string_render(
            &game_state->gui.ctx, 
            string_t{}.view("State Reset"),
            game_state->gui.ctx.screen_size/v2f{2.0f,4.0f} - gfx::font_get_size(&game_state->default_font, "State Reset")/2.0f, 
            gfx::color::to_color32(v4f{0.80f, .9f, .70f, gs_jump_load_time}),
            &game_state->default_font
        );
    }
    if ((gs_jump_save_time -= dt) > 0.0f) {
        gfx::gui::string_render(
            &game_state->gui.ctx, 
            string_t{}.view("State Saved"),
            game_state->gui.ctx.screen_size/v2f{2.0f,4.0f} - gfx::font_get_size(&game_state->default_font, "State Saved")/2.0f, 
            gfx::color::to_color32(v4f{0.80f, .9f, .70f, gs_jump_save_time}),
            &game_state->default_font
        );
    }
    if (game_state->time_text_anim > 0.0f) {
        gfx::gui::string_render(
            &game_state->gui.ctx, 
            string_t{}.view(fmt_str("Time Scale: {}", game_state->time_scale).c_str()),
            game_state->gui.ctx.screen_size/2.0f, gfx::color::to_color32(v4f{0.80f, .9f, 1.0f, game_state->time_text_anim})
        );
    }

    auto& state = game_state->gui.state;


#ifdef DEBUG_STATE 
        DEBUG_STATE_DRAW(state, render_system->projection, render_system->view, render_system->viewport());
        if (gs_show_watcher) {
            DEBUG_STATE_DRAW_WATCH_WINDOW(state);
        }
#endif

    // state.theme.bg_color = 
    //    gfx::color::rgba::dark_gray & ( ~(u32(f32(0xff) * gs_panel_opacity) << 24) );

    {
        using namespace std::string_view_literals;
        using namespace gfx::gui;
        im::clear(state);

        const m44 vp = 
            game_state->render_system->vp;

#ifdef DEBUG_STATE
        if (gs_show_console) {
            draw_console(state, DEBUG_STATE.console, v2f{400.0, 0.0f});
        }
#endif

        local_persist b32 load_system{0};
        local_persist particle_system_t* saving_system{0};
        local_persist v3f default_widget_pos{10.0f};
        local_persist zyy::entity_t* selected_entity{0};
        local_persist v3f* widget_pos = &default_widget_pos;
        local_persist bool show_probes = false;

        if (saving_system) {
            if (load_save_particle_dialog(state, saving_system, load_system)) {
                saving_system = 0;
            }
        }


        auto* world = game_state->game_world;
        if (world->world_generator == nullptr) {
            const math::rect2d_t screen{v2f{0.0f}, v2f{state.ctx.screen_size}};
            local_persist math::rect2d_t world_rect{v2f{550,350}, v2f{800, 600}};

            im::point_edit(state, &world_rect.min, screen, screen, gfx::color::rgba::red);
            im::point_edit(state, &world_rect.max, screen, screen, gfx::color::rgba::red);
        
            im::begin_panel(state, "World Select"sv, world_rect.min, world_rect.size());

            #define WORLD_GUI(name) if (im::text(state, #name)) {world->world_generator = generate_##name(&world->arena); }
            WORLD_GUI(world_maze);
            WORLD_GUI(room_03);
            WORLD_GUI(world_0);
            WORLD_GUI(world_1);
            WORLD_GUI(forest);
            WORLD_GUI(probe_test);
            WORLD_GUI(world_test);
            WORLD_GUI(homebase);
            WORLD_GUI(sponza);
            WORLD_GUI(particle_test);
            WORLD_GUI(crash_test);
            #undef WORLD_GUI
            im::end_panel(state);
        }

        if (world->world_generator && world->world_generator->is_done() == false) {
            auto* generator = world->world_generator;
            im::begin_panel(state, "Loading"sv, v2f{350,350}, v2f{800, 600});
            im::text(state, fmt_sv("Loading World {}/{}", generator->completed_count, generator->step_count));
            const auto theme = state.theme;
            state.theme.text_color = gfx::color::rgba::green;
            auto* step = generator->first_step;
            for (size_t i = 0; i < generator->completed_count && step; i++) {
                im::text(state, fmt_sv("Step {}: {}", i, step->name));
                node_next(step);
            }
            state.theme.text_color = gfx::color::rgba::purple;
            if (generator->completed_count < generator->step_count) {
                im::text(state, fmt_sv("Step {}: {}", generator->completed_count, step->name));
            }
            state.theme = theme;

            state.hot = 0; state.active = 0; // @hack to clear ui state
            im::end_panel(state);
        }

        local_persist bool show_entities = false;
        if (im::begin_panel(state, "Main UI"sv)) {
            local_persist bool show_player = false;
            local_persist bool show_stats = false;
            local_persist bool show_resources = false;
            local_persist bool show_window = false;
            local_persist bool show_gfx_cfg = false;
            local_persist bool show_perf = !true;
            local_persist bool show_gui = false;
            local_persist bool show_theme = false;
            local_persist bool show_colors= false;
            local_persist bool show_files[0xff];

            

            {
                local_persist f32 dt_accum{0};
                local_persist f32 dt_count{0};
                if (dt_count > 1000.0f) {
                    dt_count=dt_accum=0.0f;
                }
                dt_accum += game_memory->input.dt;
                dt_count += 1.0f;

                local_persist f32 rdt_accum{0};
                local_persist f32 rdt_count{0};
                if (rdt_count > 1000.0f) {
                    rdt_count=rdt_accum=0.0f;
                }
                rdt_accum += game_memory->input.render_dt;
                rdt_count += 1.0f;
                im::text(state, fmt_sv("Gameplay FPS: {:.2f} - {:.2f} ms", 1.0f / (dt_accum/dt_count), (dt_accum/dt_count) * 1000.0f));
                im::text(state, fmt_sv("Graphics FPS: {:.2f} - {:.2f} ms", 1.0f / (rdt_accum/rdt_count), (rdt_accum/rdt_count) * 1000.0f));
            }

#if ZYY_INTERNAL
            local_persist bool show_record[128];
            if (im::text(state, "Profiling"sv, &show_perf)) {
                debug_table_t* tables[] {
                    &gs_debug_table, game_memory->physics->get_debug_table()
                };
                size_t table_sizes[]{
                    gs_main_debug_record_size, game_memory->physics->get_debug_table_size()
                };

                size_t record_count = 0;
                range_u64(t, 0, array_count(tables)) {
                    size_t size = table_sizes[t];
                    auto* table = tables[t];
                    range_u64(i, 0, size) {
                        auto* record = table->records + i;
                        const auto cycle = record->history[record->hist_id?record->hist_id-1:array_count(record->history)-1];

                        if (record->func_name == nullptr) continue;

                        im::same_line(state);
                        if (im::text(state,
                            fmt_sv("- {}:", 
                                record->func_name ? record->func_name : "<Unknown>"), 
                            show_record + record_count++
                        )) {
                            f32 hist[4096/32];
                            math::statistic_t debug_stat;
                            math::begin_statistic(debug_stat);
                            range_u64(j, 0, array_count(hist)) {
                                const auto ms = f64(record->history[j*32])/f64(1e+6);
                                math::update_statistic(debug_stat, ms);
                                hist[j] = (f32)ms;
                            }
    
            
                            if (im::text(state,
                                fmt_sv(" {:.2f}ms - [{:.3f}, {:.3f}]", cycle/1e+6, debug_stat.range.min, debug_stat.range.max)
                            )) {
                                record->set_breakpoint = !record->set_breakpoint;
                            }
                            math::end_statistic(debug_stat);

                            im::histogram(state, hist, array_count(hist), (f32)debug_stat.range.max, v2f{4.0f*128.0f, 32.0f});
                        } else {
                            im::text(state,
                                fmt_sv(" {:.2f}ms", cycle/1e+6)
                            );
                        }
                    }
                }
            }
#endif
            
            if (im::text(state, "Stats"sv, &show_stats)) {

                object_gui(state, game_state->render_system->stats);
                
                {
                    const auto [x,y] = game_memory->input.mouse.pos;
                    im::text(state, fmt_sv("- Mouse: [{:.2f}, {:.2f}]", x, y));
                }
                if (im::text(state, "- GFX Config"sv, &show_gfx_cfg)) {
                    auto& config = game_memory->config.graphics_config;

                    local_persist u64 _win_size = 0;

                    const v2i _sizes[] = {
                        v2i{1920, 1080}, // 120
                        v2i{1600, 900}, // 100
                        v2i{1280, 720}, // 80
                        v2i{960,  540}, // 60
                        v2i{640,  360}, // 40
                    };
                    f32 closest = 1000000.0f;
                    range_u64(i, 0, array_count(_sizes)) {
                        const auto dist = glm::distance(v2f(_sizes[i]), v2f(config.window_size));
                        if (dist < closest) {
                            closest = dist;
                            _win_size = i;
                        }
                    }

                    if (im::text(state, "--- Next"sv)) {
                        _win_size = (_win_size + 1) % array_count(_sizes);
                        config.window_size = _sizes[_win_size];
                    }

                    im::text(state, fmt_sv("--- Width: {}", config.window_size.x));
                    im::text(state, fmt_sv("--- Height: {}", config.window_size.y));


                    if (im::text(state, fmt_sv("--- V-Sync: {}", config.vsync))) {
                        config.vsync = !config.vsync;
                    }
                    
                    if (im::text(state, fmt_sv("--- Fullscreen: {}", config.fullscreen))) {
                        config.fullscreen = !config.fullscreen;
                    }
                    if (im::text(state, fmt_sv("--- Cull Mode: {}", gs_cull_mode))) {
                        gs_cull_mode = (gs_cull_mode + 1) % array_count(gs_cull_modes);
                    }
                    if (im::text(state, fmt_sv("--- Polygon Mode: {}", gs_poly_mode))) {
                        gs_poly_mode = (gs_poly_mode + 1) % array_count(gs_poly_modes);
                    }
                }

                if (im::text(state, "- Window"sv, &show_window)) {
                    im::text(state, fmt_sv("--- Width: {}", game_memory->config.window_size[0]));
                    im::text(state, fmt_sv("--- Height: {}", game_memory->config.window_size[1]));
                }

                if (im::text(state, "- GUI"sv, &show_gui)) {
                    im::text(state, fmt_sv("--- Active: {:X}", state.active.id));
                    im::text(state, fmt_sv("--- Hot: {:X}", state.hot.id));
                    if (im::text(state, "--- Theme"sv, &show_theme)) {
                        im::text(state, fmt_sv("----- Margin: {}", state.theme.margin));
                        im::text(state, fmt_sv("----- Padding: {}", state.theme.padding));
                        im::text(state, fmt_sv("----- Shadow Offset: {}", state.theme.shadow_distance));
                        im::float_slider(state, &state.theme.shadow_distance, 0.0f, 16.0f);
                        if (im::text(state, "----- Color"sv, &show_colors)) {
                            local_persist gfx::color32* edit_color = &state.theme.fg_color;

                            im::color_edit(state, edit_color);

                            if (im::text(state, fmt_sv("------- Foreground: {:X}", state.theme.fg_color))) {
                                edit_color = &state.theme.fg_color;
                            }
                            if (im::text(state, fmt_sv("------- Background: {:X}", state.theme.bg_color))) {
                                edit_color = &state.theme.bg_color;
                            }
                            if (im::text(state, fmt_sv("------- Text: {:X}", state.theme.text_color))) {
                                edit_color = &state.theme.text_color;
                            }
                            if (im::text(state, fmt_sv("------- Border: {:X}", state.theme.border_color))) {
                                edit_color = &state.theme.border_color;
                            }
                            if (im::text(state, fmt_sv("------- Active: {:X}", state.theme.active_color))) {
                                edit_color = &state.theme.active_color;
                            }
                            if (im::text(state, fmt_sv("------- Disabled: {:X}", state.theme.disabled_color))) {
                                edit_color = &state.theme.disabled_color;
                            }
                            if (im::text(state, fmt_sv("------- Shadow: {:X}", state.theme.shadow_color))) {
                                edit_color = &state.theme.shadow_color;
                            }
                        }
                    }
                }

                if (im::text(state, fmt_sv("- Light Mode: {}", game_state->scene.sporadic_buffer.use_lighting))) {
                    game_state->scene.sporadic_buffer.use_lighting = !game_state->scene.sporadic_buffer.use_lighting;
                }
                if (im::text(state, fmt_sv("- Mode: {}", game_state->scene.sporadic_buffer.mode))) {
                    game_state->scene.sporadic_buffer.mode = ++game_state->scene.sporadic_buffer.mode % 4;
                }
                if (im::text(state, fmt_sv("- Resource File Count: {}", game_state->resource_file->file_count), &show_resources)) {
                    local_persist u64 file_start = 0;
                    im::same_line(state);
                    if (im::text(state, "Next")) file_start = (file_start+1) % game_state->resource_file->file_count;
                    if (im::text(state, "Prev")) file_start = file_start?file_start-1:game_state->resource_file->file_count-1;
                    range_u64(rf_i, 0, 10) {
                        u64 rf_ = file_start + rf_i;
                        u64 rf = rf_ % game_state->resource_file->file_count;
                        assert(rf < array_count(show_files));
                        if (im::text(state, fmt_sv("--- File Name: {}", game_state->resource_file->table[rf].name.c_data), show_files + rf)) {
                            im::text(state, fmt_sv("----- Size: {} bytes", game_state->resource_file->table[rf].size));
                            im::text(state, fmt_sv("----- Type: {:X}", game_state->resource_file->table[rf].file_type));
                        }
                    }
                }
            }

            if (im::text(state, "Debug"sv, &game_state->debug.show)) {
                if(im::text(state, "- Assert"sv)) {
                    assert(0);
                }
                if(im::text(state, "- Breakpoint"sv)) {
                    volatile u32 _tx=0;
                    _tx += 1;
                }
            }

            local_persist bool show_gfx = !false;
            local_persist bool show_mats = false;
            local_persist bool show_textures = false;
            local_persist bool show_env = !false;
            local_persist bool show_mat_id[8] = {};
            local_persist bool show_sky = false;
            if (im::text(state, "Graphics"sv, &show_gfx)) { 
                if (im::text(state, "- Reload Shaders"sv)) {
                    std::system("ninja");
                    game_state->render_system->shader_cache.reload_all(
                        game_state->render_system->arena,
                        game_state->gfx
                    );
                }

                if (im::text(state, "- Texture Cache"sv, &show_textures)) {
                    auto& cache = game_state->render_system->texture_cache;

                    for(u64 i = cache.first();
                        i != cache.end();
                        i = cache.next(i)
                    ) {
                        if(im::text(state,
                            fmt_sv("--- {} -> {}: {} x {}, {} channels",
                                i, cache.textures[i].name, cache[i]->size.x, cache[i]->size.y, cache[i]->channels
                            )
                        )) {
                            // todo show texture here                            
                        }
                    }
                }

                if (im::text(state, "- Probes"sv, &show_probes)) { 
                    auto* probes = &game_state->render_system->probe_storage_buffer.pool[0];
                    auto* probe_box = &game_state->render_system->light_probes;
                    auto& probe_settings = game_state->render_system->light_probe_settings_buffer.pool[0];
                    im::text(state, fmt_sv("--- AABB: {} - {}", probe_settings.aabb_min, probe_settings.aabb_max));
                    im::text(state, fmt_sv("--- Dim: {} x {} x {}", probe_settings.dim.x, probe_settings.dim.y, probe_settings.dim.z));
                    u32 probe_count = probe_settings.dim.x * probe_settings.dim.y * probe_settings.dim.z;
                    range_u32(i, 0, probe_count) {
                        auto& p = probes[i];
                        // im::text(state, fmt_sv("P[{}].SH[2]: {}", p.id, p.irradiance.h[2]));
                        // im::text(state, fmt_sv("P[{}].SH[5]: {}", p.id, p.irradiance.h[5]));
                        auto color = (p.is_off()) ?
                            gfx::color::rgba::red : gfx::color::rgba::white;
                        if (im::draw_circle_3d(state, vp, rendering::lighting::probe_position(probe_box, &p, i), 0.1f, color)) {
                            auto push_theme = state.theme;
                            state.theme.border_radius = 1.0f;
                            if (im::begin_panel_3d(state, "probe"sv, vp, rendering::lighting::probe_position(probe_box, &p, i))) {
                                im::text(state, fmt_sv("Probe ID: {}"sv, i));
                                im::text(state, fmt_sv("Pos: {}", p.position));
                                im::text(state, fmt_sv("Ray Count: {}", p.ray_count()));
                                im::text(state, fmt_sv("Backface Count: {}", p.backface_count()));
                                
                                im::end_panel(state);
                            }
                            state.theme = push_theme;
                        }
                    }
                }
                if (im::text(state, "- Environment"sv, &show_env)) { 
                    auto& env = game_state->render_system->environment_storage_buffer.pool[0];
                    auto* point_lights = &game_state->render_system->point_light_storage_buffer.pool[0];
                    local_persist bool show_amb = false;
                    if (im::text(state, "--- Ambient"sv, &show_amb)) { 
                        gfx::color32 tc = gfx::color::to_color32(env.ambient_color);
                        im::color_edit(state, &tc);
                        env.ambient_color = gfx::color::to_color4(tc);
                    }
                    local_persist bool show_fog = false;
                    if (im::text(state, "--- Fog"sv, &show_fog)) { 
                        gfx::color32 tc = gfx::color::to_color32(env.fog_color);
                        im::color_edit(state, &tc);
                        env.fog_color = gfx::color::to_color4(tc);
                        // im::float_slider(state, &env.fog_density);
                        im::float_edit(state, &env.fog_density);
                    }
                    local_persist bool show_lights = !false;
                    local_persist bool light[512]{true};
                    if (im::text(state, "--- Point Lights"sv, &show_lights)) { 
                        if (im::text(state, "----- Add Light"sv)) {
                            rendering::create_point_light(game_state->render_system, v3f{0.0f, 3.0f, 0.0f}, 1.0f, 10.0f, v3f{0.1f});
                        }
                        range_u64(i, 0, env.light_count) {
                            if (im::text(state, fmt_sv("----- Light[{}]"sv, i), light+i)) { 
                                // gfx::color32 color = gfx::color::to_color32(point_lights[i].col);
                                // im::color_edit(state, &color);
                                // point_lights[i].col = gfx::color::to_color4(color);
                                
                                im::vec3(state, point_lights[i].pos, -25.0f, 25.0f);
                                im::vec3(state, point_lights[i].col, 0.0f, 3.0f);

                                im::float_drag(state, &point_lights[i].range, 1.0f);
                                im::float_drag(state, &point_lights[i].power, 1.0f);

                                widget_pos = (v3f*)&point_lights[i].pos;
                            }
                        }
                    }
                }
                if (im::text(state, "- Materials"sv, &show_mats)) { 
                    loop_iota_u64(i, game_state->render_system->materials.size()) {
                        auto* mat = game_state->render_system->materials[i];
                        if (im::text(state, fmt_sv("--- Name: {}", std::string_view{mat->name}), show_mat_id + i)) {
                            gfx::color32 color = gfx::color::to_color32(mat->albedo);
                            im::color_edit(state, &color);
                            mat->albedo = gfx::color::to_color4(color);

                            im::same_line(state);
                            im::text(state, "Ambient: ");
                            im::float_slider(state, &mat->ao);

                            im::same_line(state);
                            im::text(state, "Metallic: ");
                            im::float_slider(state, &mat->metallic);

                            im::same_line(state);
                            im::text(state, "Roughness: ");
                            im::float_slider(state, &mat->roughness);

                            im::same_line(state);
                            im::text(state, "Emission: ");
                            im::float_slider(state, &mat->emission, 0.0f, 30.0f);

                            im::same_line(state);
                            if (im::text(state, "Reflectivity: ")) {
                                mat->reflectivity = 0.0f;
                            }
                            im::float_drag(state, &mat->reflectivity, 0.05f);

                            game_state->render_system->material_storage_buffer.pool[i] = *mat;

                        }
                    }
                }
                if (im::text(state, "- SkyBox"sv, &show_sky)) { 
                    local_persist bool show_dir = false;
                    local_persist bool show_color = false;
                    auto* env = &game_state->render_system->environment_storage_buffer.pool[0];
                    if (im::text(state, "--- Sun Color"sv, &show_color)) {
                        auto color = gfx::color::to_color32(env->sun.color);
                        im::color_edit(state, &color);
                        env->sun.color = gfx::color::to_color4(color);
                    }
                    if (im::text(state, "--- Sun Direction"sv, &show_dir)) { 
                        im::vec3(state, env->sun.direction, -2.0f, 2.0f);
                    }
                }
            }

            local_persist bool show_arenas = false;
            local_persist bool open_arena[256];
            local_persist u32 offset[256];
            if (im::text(state, "Memory"sv, &show_arenas)) {
                for (size_t i = 0; i < array_count(display_arenas); i++) {
                    if (im::text(state, arena_display_info(display_arenas[i], display_arena_names[i]), open_arena+i)) {
                        auto tag_count = node_count(display_arenas[i]->tags);
                        im::text(state, fmt_sv("- Tags: {}", tag_count));
                        if (im::text(state, "Next")) {
                            offset[i] = (offset[i]+1)%tag_count;
                        }
                        auto* start = display_arenas[i]->tags;
                        u32 skip=0;
                        for(;start;node_next(start)) {
                            if (skip++ == offset[i]) break;
                        }
                        skip = 0;
                        node_for(auto, start, tag) {
                            if (skip++ > 10) { break; }                            
                            if (im::text(state, fmt_sv("--- {}[{}]: {}() | {}({})", tag->type_name, tag->count, tag->function_name, utl::trim_filename(tag->file_name), tag->line_number))) {
                                std::system(fmt_sv("code -g {}:{}:0", tag->file_name, tag->line_number).data());
                            }
                        }
                        if (tag_count > 10) {
                            node_for(auto, display_arenas[i]->tags, tag) {
                                if (skip++ > 10) { break; }                            
                                if (im::text(state, fmt_sv("--- {}[{}]: {}() | {}({})", tag->type_name, tag->count, tag->function_name, utl::trim_filename(tag->file_name), tag->line_number))) {
                                    std::system(fmt_sv("code -g {}:{}:0", tag->file_name, tag->line_number).data());
                                }
                            }
                        }
                    }
                }
                for (size_t i = 0; i < array_count(display_pools); i++) {
                    im::text(state, pool_display_info(display_pools[i], display_pool_names[i]));
                }
            }

            if (im::text(state, "Entities"sv, &show_entities)) {
                const u64 entity_count = game_state->game_world->entity_count;

                im::text(state, fmt_sv("- Count: {}", entity_count));

                if (im::text(state, "- Kill All"sv)) {
                    range_u64(ei, 1, game_state->game_world->entity_capacity) {
                        if (game_state->game_world->entities[ei].is_alive()) {
                            game_state->game_world->entities[ei].queue_free();
                        }
                    }

                    // game_state->game_world->entities.clear();
                    // game_state->game_world->entity_count = 1;
                }
            }

            auto* player = game_state->game_world->player;
            if (player && im::text(state, "Player"sv, &show_player)) {
                const bool on_ground = game_state->game_world->player->physics.rigidbody->flags & physics::rigidbody_flags::IS_ON_GROUND;
                const bool on_wall = game_state->game_world->player->physics.rigidbody->flags & physics::rigidbody_flags::IS_ON_WALL;
                im::text(state, fmt_sv("- On Ground: {}", on_ground?"true":"false"));
                im::text(state, fmt_sv("- On Wall: {}", on_wall?"true":"false"));
                const auto v = player->physics.rigidbody->velocity;
                const auto p = player->global_transform().origin;
                im::text(state, fmt_sv("- Position: {}", p));
                im::text(state, fmt_sv("- Velocity: {}", v));
                im::text(state, fmt_sv("- Speed: {}", glm::length(v)));
            }

            im::end_panel(state);
        }

        // for (zyy::entity_t* e = game_itr(game_state->game_world); e; e = e->next) {
        for (size_t i = 0; i < game_state->game_world->entity_capacity; i++) {
            auto* e = game_state->game_world->entities + i;
            if (e->is_alive() == false) { continue; }

            const v3f ndc = math::world_to_screen(vp, e->global_transform().origin);

            bool is_selected = widget_pos == &e->transform.origin;
            if (is_selected) {
                selected_entity = e;
                if (e->gfx.mesh_id) {
                    // gfx::gui::draw_mesh(
                    //     &state.ctx,
                    //     &game_state->render_system->mesh_cache.get(e->gfx.mesh_id),
                    //     &game_state->render_system->scene_context->vertices.pool[0],
                    //     &game_state->render_system->scene_context->indices.pool[0],
                    //     e->global_transform().to_matrix(),
                    //     gfx::color::to_color32(v4f{1.0f, 1.0f, 0.1f, 0.5f})
                    //     // gfx::color::rgba::yellow
                    // );
                }
            }
            bool not_player = e != game_state->game_world->player;
            bool opened = false;
            if ((show_entities || is_selected) && im::begin_panel_3d(state, 
                e->name.c_data ? 
                std::string_view{fmt_sv("Entity: {}\0{}"sv, std::string_view{e->name}, (void*)e)}:
                std::string_view{fmt_sv("Entity: {}", (void*)e)},
                vp, e->global_transform().origin
            )) {

                opened = true;
                im::text(state, 
                    e->name.c_data ? 
                    fmt_sv("Entity: {}", std::string_view{e->name}) :
                    fmt_sv("Entity: {}", (void*)e)
                );

                if (e->parent && im::text(state, fmt_sv("Parent: {}",(void*)e->parent))) {
                    widget_pos = &e->parent->transform.origin; 
                    im::end_panel(state);
                    break;
                }
                im::text(state, 
                    fmt_sv("Children[{}]", zyy::entity_child_count(e))
                );

#if ZYY_INTERNAL
                auto _DEBUG_meta = e->_DEBUG_meta;
                auto spawn_delta = game_state->game_world->time() - _DEBUG_meta.game_time;
                
                im::text(state, "DEBUG_meta:");
                im::text(state, fmt_sv("- Spawn Time: {} - (T {:+}s)", _DEBUG_meta.game_time, spawn_delta));
                if (_DEBUG_meta.prefab_name) im::text(state, fmt_sv("- Prefab Name: {}", _DEBUG_meta.prefab_name));
                if (_DEBUG_meta.function) im::text(state, fmt_sv("- Function: {}", _DEBUG_meta.function));
                if (_DEBUG_meta.file_name) {
                    if (im::text(state, fmt_sv("- Filename: {}:({})", utl::trim_filename(_DEBUG_meta.file_name), _DEBUG_meta.line_number))) {
                        std::system(fmt_sv("code -g {}:{}:0", _DEBUG_meta.file_name, _DEBUG_meta.line_number).data());
                    }
                }
#endif

                // im::text(state, 
                //     fmt_sv("Screen Pos: {:.2f} {:.2f}", ndc.x, ndc.y)
                // );
                im::text(state, 
                    fmt_sv("Mesh ID: {}", e->gfx.mesh_id)
                );

                if (im::text(state,
                    fmt_sv("Origin: {:.2f} {:.2f} {:.2f}", 
                        e->global_transform().origin.x,
                        e->global_transform().origin.y,
                        e->global_transform().origin.z
                    )
                )) {
                    widget_pos = &e->transform.origin;
                }

                switch(e->physics.flags) {
                    case zyy::PhysicsEntityFlags_None:
                        im::text(state, "Physics Type: None");
                        break;
                    case zyy::PhysicsEntityFlags_Character:
                        im::text(state, "Physics Type: Character");
                        break;
                    case zyy::PhysicsEntityFlags_Kinematic:
                        im::text(state, "Physics Type: Kinematic");
                        break;
                    case zyy::PhysicsEntityFlags_Static:
                        im::text(state, "Physics Type: Static");
                        break;
                    case zyy::PhysicsEntityFlags_Dynamic:
                        im::text(state, "Physics Type: Dynamic");
                        break;
                }
                if (e->physics.rigidbody) {
                    im::text(state, fmt_sv("Velocity: {}", e->physics.rigidbody->velocity));
                }

                switch(e->type) {
                    case zyy::entity_type::weapon: {
                        auto stats = e->stats.weapon;
                        im::text(state, "Type: Weapon");
                        im::text(state, "- Stats");
                        im::text(state, fmt_sv("--- Damage: {}", stats.stats.damage));
                        im::text(state, fmt_sv("--- Pen: {}", stats.stats.pen));
                        im::text(state, fmt_sv("- Fire Rate: {}", stats.fire_rate));
                        im::text(state, fmt_sv("- Load Speed: {}", stats.load_speed));
                        im::text(state, "- Ammo");
                        im::text(state, fmt_sv("--- {}/{}", stats.clip.current, stats.clip.max));
                    }   break;
                }

                im::text(state, fmt_sv("AABB: {} {}", e->aabb.min, e->aabb.max));

                if (im::text(state, fmt_sv("Kill\0: {}"sv, (void*)e))) {
                    auto* te = e->next;
                    zyy_info("gui", "Killing entity: {}", (void*)e);
                    e->queue_free();
                    // zyy::world_destroy_entity(game_state->game_world, e);
                    if (!te) {
                        im::end_panel(state);
                        break;
                    }
                    e = te;
                }

                if (e->brain_id != uid::invalid_id) {
                    if (e->brain.type == brain_type::person) {
                        im::float_slider(state, &e->brain.person.fear);
                        draw_behavior_tree(state, &e->brain.person.tree);
                    }
                }

                if (check_for_debugger() && im::text(state, fmt_sv("Breakpoint\0{}"sv, e->id))) {
                    e->flags ^= zyy::EntityFlags_Breakpoint;
                }

                if (e->gfx.particle_system) {
                    auto* ps = e->gfx.particle_system;
                    im::text(state, fmt_sv("Particle System - {} / {}", ps->live_count, ps->max_count));
                    auto* alloc_info = get_allocation_tag(ps);
                    if (alloc_info) {
                        im::text(state, fmt_sv("- Particle Alloc Info: {}({})", alloc_info->file_name, alloc_info->line_number));
                    }
                    im::text(state, "- Particle Template");

                    if (im::text(state, "--- Save: ")) {
                        load_system = 0;
                        saving_system = ps;
                    }
                    if (im::text(state, "--- Load: ")) {
                        load_system = 1;
                        saving_system = ps;
                    }

                    im::same_line(state);
                    im::text(state, "--- life_time: ");
                    im::float_slider(state, &ps->template_particle.life_time, 0.0f, 2.0f);
                    
                    im::same_line(state);
                    im::text(state, "--- velocity: ");
                    im::vec3(state, ps->template_particle.velocity, -1.0f, 1.0f);

                    im::same_line(state);
                    im::text(state, "- acceleration: ");
                    im::vec3(state, ps->acceleration, -1.0f, 1.0f);

                    im::same_line(state);
                    im::text(state, "- spawn_rate: ");
                    im::float_slider(state, &ps->spawn_rate, 0.0f, 1.0f);

                    im::same_line(state);
                    im::text(state, "- scale_over_life_time.min: ");
                    im::float_slider(state, &ps->scale_over_life_time.min, 0.0f, 4.0f);
                    
                    im::same_line(state);
                    im::text(state, "- scale_over_life_time.max: ");
                    im::float_slider(state, &ps->scale_over_life_time.max, 0.0f, 4.0f);

                    im::same_line(state);
                    im::text(state, "- velocity_random.min: ");
                    im::vec3(state, ps->velocity_random.min, -1.0f, 1.0f);
                    
                    im::same_line(state);
                    im::text(state, "- velocity_random.max: ");
                    im::vec3(state, ps->velocity_random.max, -1.0f, 1.0f);


                    bool is_box = ps->emitter_type == particle_emitter_type::box;
                    im::same_line(state);
                    im::text(state, "- emitter_type == box: ");
                    im::checkbox(state, &is_box);
                    ps->emitter_type = is_box ? particle_emitter_type::box : particle_emitter_type::sphere;
                    if (is_box) {
                        im::same_line(state);
                        im::text(state, "--- min: ");
                        im::vec3(state, ps->box.min, -4.0f, 4.0f);
                        im::same_line(state);
                        im::text(state, "--- max: ");
                        im::vec3(state, ps->box.max, -4.0f, 4.0f);
                    } else {
                        im::same_line(state);
                        im::text(state, "--- origin: ");
                        im::vec3(state, ps->sphere.origin, -4.0f, 4.0f);
                        im::same_line(state);
                        im::text(state, "--- radius: ");
                        im::float_slider(state, &ps->sphere.radius, 0.0f, 10.0f, v2f{128.0f, 16.0f});
                    }
                }
                    
                im::end_panel(state);
            }
            if (!opened && !is_selected && (not_player && im::draw_hiding_circle_3d(
                state, vp, e->global_transform().origin, 0.1f, 
                static_cast<u32>(utl::rng::fnv_hash_u64(e->id)), 2.0f)) && state.ctx.input->pressed.mouse_btns[0]
            ) {
                widget_pos = &e->transform.origin;
            }

            const auto panel = im::get_last_panel(state);
            if (is_selected && opened && im::draw_circle(state, panel.max, 8.0f, gfx::color::rgba::red, 4)) {
                widget_pos = &default_widget_pos;
            }
        }

        if (widget_pos == &default_widget_pos) {
            selected_entity = 0;
        }

        im::gizmo(state, widget_pos, vp);

        if (selected_entity && 
            selected_entity->physics.rigidbody //&&
            // (selected_entity->physics.rigidbody->type == physics::rigidbody_type::KINEMATIC ||
            //  selected_entity->physics.rigidbody->type == physics::rigidbody_type::STATIC 
            // )
        ) {
            selected_entity->physics.rigidbody->set_transform(selected_entity->global_transform().to_matrix());
        }

        local_persist math::rect2d_t depth_uv{v2f{0.0}, v2f{0.04, 0.2}};
        local_persist math::rect2d_t color_uv{v2f{0.0}, v2f{0.04, 0.2}};
        // local_persist math::rect2d_t color_uv{v2f{0.0}, v2f{0.01, 0.15}};

        if (input->keys[key_id::UP]) { depth_uv.add(v2f{0.0f, -0.01}); }
        if (input->keys[key_id::DOWN]) { depth_uv.add(v2f{0.0f, 0.01}); }
        if (input->keys[key_id::RIGHT]) { depth_uv.add(v2f{0.01f, 0.0}); }
        if (input->keys[key_id::LEFT]) { depth_uv.add(v2f{-0.01f, 0.0}); }
        if (input->keys[key_id::MINUS]) { depth_uv.scale(v2f{0.99f});}
        if (input->keys[key_id::EQUAL]) { depth_uv.scale(v2f{1.01f});}

        if (input->keys[key_id::UP]) { color_uv.add(v2f{0.0f, -0.01}); }
        if (input->keys[key_id::DOWN]) { color_uv.add(v2f{0.0f, 0.01}); }
        if (input->keys[key_id::RIGHT]) { color_uv.add(v2f{0.01f, 0.0}); }
        if (input->keys[key_id::LEFT]) { color_uv.add(v2f{-0.01f, 0.0}); }
        if (input->keys[key_id::MINUS]) { color_uv.scale(v2f{0.99f});}
        if (input->keys[key_id::EQUAL]) { color_uv.scale(v2f{1.01f});}

        if (show_probes) {
            im::image(state, 2, {v2f{0.0f, 800}, v2f{state.ctx.screen_size.x * 0.3f, state.ctx.screen_size.y}}, color_uv);
            im::image(state, 3, {v2f{state.ctx.screen_size.x *  0.7f, 800}, v2f{state.ctx.screen_size}}, depth_uv, 0xff0000ff);
        }

        draw_game_gui(game_memory);

        local_persist viewport_t viewport{};
        viewport.images[0] = 1;
        viewport.images[1] = 2;
        viewport.images[2] = 1;
        viewport.images[3] = 2;

        if constexpr (false) {

        local_persist v2f p0{0.125f};
        local_persist v2f p1{0.25f};
        local_persist v2f p2{0.50f};
        local_persist v2f p3{0.750f};
        local_persist v2f p4{0.650f};
        local_persist v2f p5{0.150f};
        
        local_persist math::rect2d_t point_screen{v2f{600.0f,200.0f}, v2f{1100.0f,300.0f}};
        const math::rect2d_t screen{v2f{0.0f}, v2f{state.ctx.screen_size}};
        const math::rect2d_t prange{v2f{0.0f}, v2f{1.0f}};

        im::point_edit(state, &point_screen.min, screen, screen, gfx::color::rgba::blue);
        im::point_edit(state, &point_screen.max, screen, screen, gfx::color::rgba::blue);
        im::point_edit(state, &p0, point_screen, prange, gfx::color::rgba::white);
        im::derivative_edit(state, p0, &p1, point_screen, prange, gfx::color::rgba::white);
        im::point_edit(state, &p2, point_screen, prange, gfx::color::rgba::white);
        im::derivative_edit(state, p2, &p3, point_screen, prange, gfx::color::rgba::white);
        im::point_edit(state, &p4, point_screen, prange, gfx::color::rgba::white);
        im::derivative_edit(state, p4, &p5, point_screen, prange, gfx::color::rgba::white);

        gfx::gui::draw_curve(&state.ctx, std::span<v2f,4>{&p0,4}, point_screen, 32, 2.0f, gfx::color::rgba::white);
        gfx::gui::draw_curve(&state.ctx, std::span<v2f,4>{&p0,4}, point_screen, 32, 6.0f, gfx::color::rgba::yellow);

        gfx::gui::draw_curve(&state.ctx, std::span<v2f,4>{&p2,4}, point_screen, 32, 2.0f, gfx::color::rgba::white);
        gfx::gui::draw_curve(&state.ctx, std::span<v2f,4>{&p2,4}, point_screen, 32, 6.0f, gfx::color::rgba::yellow);

        gfx::gui::draw_line(&state.ctx, point_screen.sample(p0),point_screen.sample(p0+p1), 2.0f, gfx::color::rgba::white);
        gfx::gui::draw_line(&state.ctx, point_screen.sample(p2),point_screen.sample(p2+p3), 2.0f, gfx::color::rgba::white);
        gfx::gui::draw_line(&state.ctx, point_screen.sample(p4),point_screen.sample(p4+p5), 2.0f, gfx::color::rgba::white);
        gfx::gui::draw_line(&state.ctx, point_screen.sample(p0),point_screen.sample(p0+p1), 6.0f, gfx::color::rgba::black);
        gfx::gui::draw_line(&state.ctx, point_screen.sample(p2),point_screen.sample(p2+p3), 6.0f, gfx::color::rgba::black);
        gfx::gui::draw_line(&state.ctx, point_screen.sample(p4),point_screen.sample(p4+p5), 6.0f, gfx::color::rgba::black);
        // gfx::gui::draw_line(&state.ctx, point_screen.sample(p1),point_screen.sample(p2), 2.0f, gfx::color::rgba::white);
        // gfx::gui::draw_line(&state.ctx, point_screen.sample(p2),point_screen.sample(p3), 2.0f, gfx::color::rgba::white);

        // gfx::gui::draw_line(&state.ctx, point_screen.sample(p0),point_screen.sample(p1), 6.0f, gfx::color::rgba::black);
        // gfx::gui::draw_line(&state.ctx, point_screen.sample(p1),point_screen.sample(p2), 6.0f, gfx::color::rgba::black);
        // gfx::gui::draw_line(&state.ctx, point_screen.sample(p2),point_screen.sample(p3), 6.0f, gfx::color::rgba::black);

        gfx::gui::draw_rect(&state.ctx, point_screen, gfx::color::rgba::gray);
        }

        // viewport.h_split[0] = 0.5f;
        // viewport.v_split[0] = 0.5f;

        // draw_viewport(state, &viewport);

        // const math::rect2d_t screen{v2f{0.0f}, state.ctx.screen_size};
        // im::image(state, 2, math::rect2d_t{v2f{state.ctx.screen_size.x - 400, 0.0f}, v2f{state.ctx.screen_size.x, 400.0f}});
    }

    arena_set_mark(&game_state->string_arena, string_mark);
}