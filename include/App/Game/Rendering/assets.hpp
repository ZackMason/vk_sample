#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "core.hpp"

#include "App/vk_state.hpp"

#define ASSET_SHADER(name) global_variable gfx::shader_description_t name
namespace assets::shaders {
    ASSET_SHADER(gui_vert){"./assets/shaders/bin/gui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    ASSET_SHADER(gui_frag){"./assets/shaders/bin/gui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, 0};

    ASSET_SHADER(simple_vert){"./assets/shaders/bin/simple.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    ASSET_SHADER(simple_frag){"./assets/shaders/bin/simple.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, 0};

    ASSET_SHADER(skybox_vert){"./assets/shaders/bin/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    ASSET_SHADER(skybox_frag){"./assets/shaders/bin/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, 0};
};



#endif