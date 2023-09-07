#ifndef LIGHTING_HPP
#define LIGHTING_HPP

#include "zyy_core.hpp"
#include "uid.hpp"

namespace rendering::lighting {
    
    struct sh9_irradiance_t {
        v3f h[9];
        v2f d[9];
    };

    struct probe_t {
        sh9_irradiance_t irradiance;
        v3f position{};
        u32 id{0};
        u32 samples{0};
    };

    template <umm Size>
    using probe_buffer_t = gfx::vul::storage_buffer_t<probe_t, Size>;

    struct probe_settings_t {
        v3f aabb_min;
        v3f aabb_max;
        // math::aabb_t<v3f> aabb;
        v3u dim{100,10,100};
        i32 sample_max{100};
    };

    struct probe_box_t {
        math::aabb_t<v3f> aabb;
        // f32 grid_size{5.0f};
        // f32 grid_size{1.0f + 1.618033f};
        f32 grid_size{1.618033f};
        probe_t* probes{0};
        u32 probe_count{0};

        probe_settings_t settings{};

        gfx::vul::texture_2d_t irradiance_texture;
        gfx::vul::texture_2d_t visibility_texture;
    };

    constexpr u32 PROBE_MAX_COUNT = 10'000;
    constexpr u32 PROBE_IRRADIANCE_DIM = 8;
    constexpr u32 PROBE_VISIBILITY_DIM = 16;
    constexpr u32 PROBE_PADDING = 1;
    constexpr u32 PROBE_IRRADIANCE_TOTAL = PROBE_IRRADIANCE_DIM + 2 * PROBE_PADDING;
    constexpr u32 PROBE_VISIBILITY_TOTAL = PROBE_VISIBILITY_DIM + 2 * PROBE_PADDING;

    static void
    init_textures(gfx::vul::state_t& gfx, probe_box_t* probe_box) {
        // probe_box->irradiance_texture.format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        const u32 PROBE_MAX_COUNT_SQRT = (u32)std::sqrt(PROBE_MAX_COUNT);
        probe_box->irradiance_texture.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        probe_box->visibility_texture.format = VK_FORMAT_R16G16B16A16_SFLOAT;//VK_FORMAT_R16G16_SFLOAT;

        // todo: optimization with half texture height and glossy GI
        
        // const u32 irradiance_width = probe_box->settings.dim.x * probe_box->settings.dim.y * PROBE_IRRADIANCE_TOTAL;
        // const u32 irradiance_height = probe_box->settings.dim.z * PROBE_IRRADIANCE_TOTAL;
        const u32 irradiance_width = PROBE_MAX_COUNT_SQRT * PROBE_IRRADIANCE_TOTAL;
        const u32 irradiance_height = PROBE_MAX_COUNT_SQRT * PROBE_IRRADIANCE_TOTAL;
        const u32 visibility_width = PROBE_MAX_COUNT_SQRT * PROBE_VISIBILITY_TOTAL;
        const u32 visibility_height = PROBE_MAX_COUNT_SQRT * PROBE_VISIBILITY_TOTAL;
        
        gfx.create_texture(&probe_box->irradiance_texture, irradiance_width, irradiance_height, 4, 0, 0, 2*4);
        gfx.create_texture(&probe_box->visibility_texture, visibility_width, visibility_height, 4, 0, 0, 2*4);
        gfx.load_texture_sampler(&probe_box->irradiance_texture, true);
        gfx.load_texture_sampler(&probe_box->visibility_texture, true);
    }


    inline static void 
    set_probes(gfx::vul::state_t& vk_gfx, probe_box_t* probe_box, arena_t* arena = 0, probe_t* probes=0) {
        const v3f step_size = v3f{probe_box->grid_size};
        const v3u probe_count = v3u{glm::floor(probe_box->aabb.size() / probe_box->grid_size)};
        probe_box->settings.dim = probe_count;
        const u32 total_probe_count = probe_count.x * probe_count.y * probe_count.z;
        probe_box->probes = arena?arena_alloc_ctor<probe_t>(arena, total_probe_count):probes?probes:0;
        probe_box->probe_count = total_probe_count;
        probe_box->settings.aabb_min = probe_box->aabb.min;
        probe_box->settings.aabb_max = probe_box->aabb.max;

        zyy_info(__FUNCTION__, "Creating Light Probe Box");
        zyy_info(__FUNCTION__, "-- Total Probe Count: {}", total_probe_count);
        zyy_info(__FUNCTION__, "-- Probe Dimensions: {} x {} x {}", probe_count.x, probe_count.y, probe_count.z);
        zyy_info(__FUNCTION__, "-- Probe AABB: {} - {}", probe_box->settings.aabb_min, probe_box->settings.aabb_max);

        u32 i = 0;
        range_u32(z, 0, probe_count.z) {
            range_u32(y, 0, probe_count.y) {
                range_u32(x, 0, probe_count.x) {
                    auto* probe = probe_box->probes + i;
                    probe->position = probe_box->aabb.min + step_size * v3f{x,y,z};
                    probe->id = i++;
                }
            }
        }
    }       
    inline static void 
    update_probe_positions(probe_box_t* probe_box) {
        const v3f step_size = v3f{probe_box->grid_size};
        const v3u probe_count = v3u{glm::floor(probe_box->aabb.size() / probe_box->grid_size)};
        probe_box->settings.dim = probe_count;
        const u32 total_probe_count = probe_count.x * probe_count.y * probe_count.z;
        probe_box->probe_count = total_probe_count;
        probe_box->settings.aabb_min = probe_box->aabb.min;
        probe_box->settings.aabb_max = probe_box->aabb.max;


        zyy_info(__FUNCTION__, "Updating Light Probe Box");
        zyy_info(__FUNCTION__, "-- Total Probe Count: {}", total_probe_count);
        zyy_info(__FUNCTION__, "-- Probe Dimensions: {} x {} x {}", probe_count.x, probe_count.y, probe_count.z);
        zyy_info(__FUNCTION__, "-- Probe AABB: {} - {}", probe_box->settings.aabb_min, probe_box->settings.aabb_max);

        u32 i = 0;
        range_u32(z, 0, probe_count.z) {
            range_u32(y, 0, probe_count.y) {
                range_u32(x, 0, probe_count.x) {
                    auto* probe = probe_box->probes + i;
                    probe->position = probe_box->aabb.min + step_size * v3f{x,y,z};
                    probe->id = i++;
                    probe->samples = 0;
                    utl::memzero(&probe->irradiance, sizeof(probe->irradiance));
                }
            }
        }
    }       
};

#endif