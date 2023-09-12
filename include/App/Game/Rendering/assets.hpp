#ifndef APP_RENDERING_ASSETS_HPP
#define APP_RENDERING_ASSETS_HPP

#include "zyy_core.hpp"

#include "App/vk_state.hpp"

// TODO(Zack): Refactor until this file disappears (never),
// or refactor so it can be loaded from a dll?
#define ASSET_TYPE(name, type) constexpr global_variable type name = type

namespace assets::shaders {
    #define ASSET_SHADER(name) ASSET_TYPE(name, gfx::shader_description_t)
    #define SHADER_PATH_(name) "./res/shaders/bin/" ## name ## ".spv"
    #define SHADER_PATH(name) {SHADER_PATH_(name)}

    constexpr u32 CAMERA_CONSTANTS_SIZE = sizeof(m44) + sizeof(v4f);
    constexpr u32 NONE = 0;
    constexpr u32 VERT = VK_SHADER_STAGE_VERTEX_BIT;
    constexpr u32 FRAG = VK_SHADER_STAGE_FRAGMENT_BIT;
    constexpr u32 GEOM = VK_SHADER_STAGE_GEOMETRY_BIT;
    constexpr u32 COMP = VK_SHADER_STAGE_COMPUTE_BIT;

    ASSET_SHADER(rt_comp)
        SHADER_PATH("rt_compute.comp")
        .add_push_constant(CAMERA_CONSTANTS_SIZE + sizeof(u32) * 2)
        .set_stage(COMP);

    ASSET_SHADER(probe_integrate_comp)
        SHADER_PATH("probe_integrate.comp")
        .set_stage(COMP);
    ASSET_SHADER(probe_integrate_depth_comp)
        SHADER_PATH("probe_integrate_depth.comp")
        .set_stage(COMP);        

    ASSET_SHADER(screen_vert)
        SHADER_PATH("screen.vert")
        .set_stage(VERT)
        .add_next_stage(FRAG);

    ASSET_SHADER(tonemap_frag)
        SHADER_PATH("tonemap.frag")
        .set_stage(FRAG);        
    ASSET_SHADER(invert_frag)
        SHADER_PATH("invert.frag")
        .set_stage(FRAG);        

    ASSET_SHADER(gui_vert)
        SHADER_PATH("gui.vert")
        .set_stage(VERT)
        .add_next_stage(FRAG);
    ASSET_SHADER(gui_frag)
        SHADER_PATH("gui.frag")
        .set_stage(FRAG);

    ASSET_SHADER(simple_vert)
        SHADER_PATH("simple.vert")
        .set_stage(VERT)
        .add_next_stage(FRAG)
        .add_next_stage(GEOM)
        .add_push_constant(CAMERA_CONSTANTS_SIZE + sizeof(u32) * 2);
    ASSET_SHADER(simple_frag)
        SHADER_PATH("simple.frag")
        .set_stage(FRAG)
        .add_push_constant(CAMERA_CONSTANTS_SIZE)
        .add_uniform("uMaterial.uRoughness", "float")
        .add_uniform("uMaterial.uMetallic", "float")
        .add_uniform("uMaterial.uColor", "vec4");

    ASSET_SHADER(skinned_vert)
        SHADER_PATH("skeletal.vert")
        .set_stage(VERT)
        .add_next_stage(FRAG)
        .add_next_stage(GEOM)
        .add_push_constant(CAMERA_CONSTANTS_SIZE);

    ASSET_SHADER(trail_vert)
        SHADER_PATH("trail.vert")
        .set_stage(VERT)
        .add_next_stage(FRAG)
        .add_next_stage(GEOM)
        .add_push_constant(CAMERA_CONSTANTS_SIZE);
    ASSET_SHADER(trail_frag)
        SHADER_PATH("trail.frag")
        .set_stage(FRAG)
        .add_push_constant(CAMERA_CONSTANTS_SIZE);

    ASSET_SHADER(skybox_vert)
        SHADER_PATH("skybox.vert")
        .set_stage(VERT)
        .add_next_stage(FRAG)
        .add_push_constant(CAMERA_CONSTANTS_SIZE);
    ASSET_SHADER(skybox_frag)
        SHADER_PATH("skybox.frag")
        .set_stage(FRAG)
        .add_push_constant(CAMERA_CONSTANTS_SIZE);
    ASSET_SHADER(voidsky_frag)
        SHADER_PATH("voidsky.frag")
        .set_stage(FRAG)
        .add_push_constant(CAMERA_CONSTANTS_SIZE);
    
    #undef ASSET_SHADER
    #undef SHADER_PATH
    #undef SHADER_PATH_
};

struct sound_description_t {
    std::string_view    filename;
    f32                 volume{1.0f};
    u8                  loop{false};
    mutable u64         id{0};
};

namespace assets::sounds {
    #define ASSET_SOUND(name) ASSET_TYPE(name, sound_description_t)

    ASSET_SOUND(unlock){"./res/audio/unlock.wav"}; // 0

    ASSET_SOUND(walking_dirt_01){"./res/audio/Steps_dirt-002.wav"}; // 1

    ASSET_SOUND(reload){"./res/audio/1911-reload.wav"}; // 2
    ASSET_SOUND(pistol_shot){"./res/audio/9mm_firing_01.wav"}; // 3
    ASSET_SOUND(shotgun_shot){"./res/audio/shotgun_firing_01.wav"}; // 4

    #undef ASSET_SOUND

    #define LOAD_ASSET_SOUND(name) name.id = Platform.audio.load_sound.dispatch_request<u64>(name.filename.data())
    void load() {
        LOAD_ASSET_SOUND(unlock);
        LOAD_ASSET_SOUND(walking_dirt_01);
        LOAD_ASSET_SOUND(reload);
        LOAD_ASSET_SOUND(pistol_shot);
        LOAD_ASSET_SOUND(shotgun_shot);
    }
    #undef LOAD_ASSET_SOUND
};

#undef ASSET_TYPE

#endif