#ifndef LIGHTING_HPP
#define LIGHTING_HPP

#include "core.hpp"
#include "uid.hpp"

namespace game::rendering::lighting {
    struct cube_map_pass_t {
		int32_t width, height;
		std::array<VkFramebuffer, 6> framebuffers;
        
        VkRenderPass renderPass;
		VkSampler sampler;
		// VkDescriptorImageInfo descriptor;
	};


    DEFINE_TYPED_ID(probe_id);
    struct probe_t {
        probe_t* next{};
        v3f position{};
        probe_id id{uid::invalid_id};
        gfx::vul::framebuffer_t framebuffer{};
    };

    struct probe_box_t : public math::aabb_t<v3f> {
        using math::aabb_t<v3f>::aabb_t;
        f32 grid_size{16.0f};
        probe_t* probes{0};
        u64 probe_count{0};
    };

    inline static void
    setup_framebuffer(gfx::vul::state_t& vk_gfx, probe_t* probe, arena_t* arena) {
        auto& fb = probe->framebuffer;
        fb.width = fb.height = 64;
        vk_gfx.create_framebuffer(&fb);
        {
            gfx::vul::framebuffer_attachment_create_info_t fbaci{};
            fbaci.width = fb.width; fbaci.height = fb.height;
            fbaci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            fbaci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            vk_gfx.add_framebuffer_attachment(arena, &fb, fbaci);
        }
        {
            gfx::vul::framebuffer_attachment_create_info_t fbaci{};
            fbaci.width = fb.width; fbaci.height = fb.height;
            fbaci.format = VK_FORMAT_D24_UNORM_S8_UINT;
            fbaci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            vk_gfx.add_framebuffer_attachment(arena, &fb, fbaci);
        }

        vk_gfx.create_framebuffer_sampler(&fb, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        vk_gfx.framebuffer_create_renderpass(&fb);
    }

    inline static void 
    set_probes(gfx::vul::state_t& vk_gfx, probe_box_t* probe_box, arena_t* arena) {
        const v3f step_size = v3f{probe_box->grid_size};
        const v3u probe_count = v3u{glm::floor(probe_box->size() / probe_box->grid_size)};
        const u64 total_probe_count = probe_count.x * probe_count.y * probe_count.z;
        probe_box->probes = arena_alloc_ctor<probe_t>(arena, total_probe_count);
        probe_box->probe_count = total_probe_count;

        u64 i = 0;
        range_u32(x, 0, probe_count.x) {
            range_u32(y, 0, probe_count.y) {
                range_u32(z, 0, probe_count.z) {
                    auto* probe = probe_box->probes + i++;
                    probe->position = probe_box->min + step_size * v3f{x,y,z};
                    probe->id = i;
                    // setup_framebuffer(vk_gfx, probe, arena);
                }
            }
        }
    }       

};

#endif