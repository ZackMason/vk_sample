#ifndef LIGHTING_HPP
#define LIGHTING_HPP

#include "ztd_core.hpp"
#include "uid.hpp"
#include "App/vk_state.hpp"

namespace rendering::lighting {
    struct directional_light_t {
        v4f direction{};
        v4f color{};
    };

    struct point_light_t {
        v3f pos; // position
        f32 range;
        v3f col; // color
        f32 power{1.0f};
    };
    
    struct environment_t {
        directional_light_t sun{};

        v4f ambient_color{0.9569f, 0.8941f, 0.5804f, 1.0f};

        v4f fog_color{};

        // point_light_t point_lights[32];
        
        f32 fog_density{};

        // color correction
        f32 ambient_strength{1.0f};
        f32 contrast{};
        u32 light_count{};
        v4f more_padding{};
        
    };

    
    struct sh9_irradiance_t {
        v3f h[9];
        // v2f d[9];
    };

    struct probe_t {
        // sh9_irradiance_t irradiance;
        v3f position{};
        // u32 id{0};
        u32 ray_back_count{0};

        u16 ray_count() {
            return ray_back_count&0xffff;
        }
        u16 backface_count() {
            return (ray_back_count>>16)&0xffff;
        }

        bool is_off() {
            return backface_count() > (ray_count()>>1);
        }
        // u32 backface_count{0};
    };


    struct probe_ray_result_t {
        v2u direction_depth;
        v2u radiance;
    };

    struct probe_settings_t {
        v3f aabb_min;
        v3f aabb_max;

        v3i scroll_offset;

        // math::rect3d_t aabb;
        v3u dim{100,10,100};
        v3f grid_size{0.0f};
        f32 hysteresis{0.01f};
        f32 boost{1.0f};
        f32 depth_sharpness{32.0f};
    };

    struct probe_box_t {
        math::rect3d_t aabb;
        // f32 grid_size{4.0f};
        // f32 grid_size{15.0f};
        // f32 grid_size{1.0f + 1.618033f};
        f32 grid_size{1.618033f};
        // f32 grid_size{1.0f};
        probe_t* probes{0};
        u32 probe_count{0};
        

        probe_settings_t settings{};

        gfx::vul::texture_2d_t irradiance_texture;
        gfx::vul::texture_2d_t visibility_texture;
        gfx::vul::texture_2d_t filter_texture;
    };

    v3u index_3d(v3u dim, u32 idx) {
        const u32 z = idx / (dim.x * dim.y);
        idx -= (z * dim.x * dim.y);
        const u32 y = idx / dim.x;
        const u32 x = idx % dim.x;
        return  v3u(x, y, z);
    }

    u32 index_1d(v3u dim, v3u idx) {
        return idx.x + idx.y * dim.x + idx.z * dim.x * dim.y;
    }

    constexpr u32 PROBE_RAY_MAX = 256;
    constexpr u32 PROBE_MAX_COUNT = 10'000;
    constexpr u32 PROBE_IRRADIANCE_DIM = 8;
    constexpr u32 PROBE_VISIBILITY_DIM = 16;
    // constexpr u32 PROBE_VISIBILITY_DIM = 24;
    constexpr u32 PROBE_PADDING = 1;
    constexpr u32 PROBE_IRRADIANCE_TOTAL = PROBE_IRRADIANCE_DIM + 2 * PROBE_PADDING;
    constexpr u32 PROBE_VISIBILITY_TOTAL = PROBE_VISIBILITY_DIM + 2 * PROBE_PADDING;

    static void
    init_textures(gfx::vul::state_t& gfx, probe_box_t* probe_box, arena_t* arena = 0) {

        if (probe_box->filter_texture.image == 0) {
            if (arena) {
                // gfx.load_texture_sampler(&probe_box->filter_texture, "res/textures/painted_noise.png", arena);
                gfx.load_texture_sampler(&probe_box->filter_texture, "res/textures/white.png", arena);
            } else {
                ztd_warn(__FUNCTION__, "Tried to load ddgi filter but no arena");
            }
        }

        // probe_box->irradiance_texture.format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        const u32 PROBE_MAX_COUNT_SQRT = (u32)std::sqrt(PROBE_MAX_COUNT);
        probe_box->irradiance_texture.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        probe_box->visibility_texture.format = VK_FORMAT_R16G16B16A16_SFLOAT;//VK_FORMAT_R16G16_SFLOAT;
        // probe_box->irradiance_texture.filter = VK_FILTER_NEAREST;

        // todo: optimization with half texture height and glossy GI
        
        const u32 irradiance_width = probe_box->settings.dim.x * probe_box->settings.dim.y * PROBE_IRRADIANCE_TOTAL;
        const u32 irradiance_height = probe_box->settings.dim.z * PROBE_IRRADIANCE_TOTAL;
        
        const u32 visibility_width = probe_box->settings.dim.x * probe_box->settings.dim.y * PROBE_VISIBILITY_TOTAL;
        const u32 visibility_height = probe_box->settings.dim.z * PROBE_VISIBILITY_TOTAL;

        gfx.create_texture(&probe_box->irradiance_texture, irradiance_width, irradiance_height, 4, 0, 0, 2*4);
        gfx.create_texture(&probe_box->visibility_texture, visibility_width, visibility_height, 4, 0, 0, 2*4);
        gfx.load_texture_sampler(&probe_box->irradiance_texture, true);
        gfx.load_texture_sampler(&probe_box->visibility_texture, true);

        gfx::vul::set_object_name(gfx.device, (u64)probe_box->irradiance_texture.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "DDGI Irradiance Image");
        gfx::vul::set_object_name(gfx.device, (u64)probe_box->irradiance_texture.image_view, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, "DDGI Irradiance Image View");
        gfx::vul::set_object_name(gfx.device, (u64)probe_box->irradiance_texture.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "DDGI Irradiance Sampler"); 
        gfx::vul::set_object_name(gfx.device, (u64)probe_box->irradiance_texture.vdm,  VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, "DDGI Irradiance Device Memory");
        
        gfx::vul::set_object_name(gfx.device, (u64)probe_box->visibility_texture.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "DDGI Visibility Image");
        gfx::vul::set_object_name(gfx.device, (u64)probe_box->visibility_texture.image_view, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, "DDGI Visibility Image View");
        gfx::vul::set_object_name(gfx.device, (u64)probe_box->visibility_texture.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "DDGI Visibility Sampler"); 
        gfx::vul::set_object_name(gfx.device, (u64)probe_box->visibility_texture.vdm,  VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, "DDGI Visibility Device Memory");

        const umm irradiance_size = irradiance_width * irradiance_height * 2 * sizeof(f32);
        const umm visibility_size = visibility_width * visibility_height * 2 * sizeof(f32);
        ztd_info(__FUNCTION__, "Irradiance Texture Size: {}Kb", irradiance_size/kilobytes(1));
        ztd_info(__FUNCTION__, "Visibility Texture Size: {}Kb", visibility_size/kilobytes(1));
    }

    static void 
    destroy_textures(gfx::vul::state_t& gfx, probe_box_t* probe_box) {
        gfx.destroy_texture(&probe_box->irradiance_texture);
        gfx.destroy_texture(&probe_box->visibility_texture);
    }


    inline static void 
    set_probes(gfx::vul::state_t& vk_gfx, probe_box_t* probe_box, arena_t* arena = 0, probe_t* probes=0) {
        const v3f step_size = v3f{probe_box->grid_size};
        const v3u probe_count = v3u{glm::floor(probe_box->aabb.size() / probe_box->grid_size)};
        probe_box->settings.dim = glm::max(v3u{1}, probe_count);
        probe_box->aabb.max = probe_box->aabb.min + v3f{probe_count} * step_size;
        const u32 total_probe_count = probe_count.x * probe_count.y * probe_count.z;
        if (arena) {
            tag_array(probe_box->probes, probe_t, arena, total_probe_count);
        } else {
            probe_box->probes = probes;
        }
        probe_box->probe_count = total_probe_count;
        probe_box->settings.aabb_min = probe_box->aabb.min;
        probe_box->settings.aabb_max = probe_box->aabb.max;
        probe_box->settings.grid_size = v3f{probe_box->grid_size};

        ztd_info(__FUNCTION__, "Creating Light Probe Box");
        ztd_info(__FUNCTION__, "-- Total Probe Count: {}", total_probe_count);
        ztd_info(__FUNCTION__, "-- Probe Dimensions: {} x {} x {}", probe_count.x, probe_count.y, probe_count.z);
        ztd_info(__FUNCTION__, "-- Probe AABB: {} - {}", probe_box->settings.aabb_min, probe_box->settings.aabb_max);

        u32 i = 0;
        range_u32(z, 0, probe_count.z) {
            range_u32(y, 0, probe_count.y) {
                range_u32(x, 0, probe_count.x) {
                    assert(v3u(x,y,z) == index_3d(probe_count, i));
                    assert(i == index_1d(probe_count, v3u{x,y,z}));
                    auto* probe = probe_box->probes + i++;
                    probe->position = probe_box->aabb.min + step_size * v3f{x,y,z};
                    // probe->id = i++;
                }
            }
        }
    }       
    inline static void 
    update_probe_positions(probe_box_t* probe_box) {
        const v3f step_size = v3f{probe_box->grid_size};
        const v3u probe_count = v3u{glm::floor(probe_box->aabb.size() / probe_box->grid_size)};
        probe_box->settings.dim = glm::max(v3u{1}, probe_count);
        probe_box->aabb.max = probe_box->aabb.min + v3f{probe_count} * step_size;
        const u32 total_probe_count = probe_count.x * probe_count.y * probe_count.z;
        probe_box->probe_count = total_probe_count;
        probe_box->settings.aabb_min = probe_box->aabb.min;
        probe_box->settings.aabb_max = probe_box->aabb.max;
        probe_box->settings.grid_size = v3f{probe_box->grid_size};


        ztd_info(__FUNCTION__, "Updating Light Probe Box");
        ztd_info(__FUNCTION__, "-- Total Probe Count: {}", total_probe_count);
        ztd_info(__FUNCTION__, "-- Probe Dimensions: {} x {} x {}", probe_count.x, probe_count.y, probe_count.z);
        ztd_info(__FUNCTION__, "-- Probe AABB: {} - {}", probe_box->settings.aabb_min, probe_box->settings.aabb_max);

        u32 i = 0;
        range_u32(z, 0, probe_count.z) {
            range_u32(y, 0, probe_count.y) {
                range_u32(x, 0, probe_count.x) {
                    assert(v3u(x,y,z) == index_3d(probe_count, i));
                    assert(i == index_1d(probe_count, v3u{x,y,z}));
                    auto* probe = probe_box->probes + i++;
                    probe->position = probe_box->aabb.min + step_size * v3f{x,y,z};
                    // probe->id = i++;
                    // probe->ray_count = 0;
                    // probe->backface_count = 0;
                }
            }
        }
    }       

    v3f probe_start_position(v3u probe_coord, v3f grid_size, v3f min_pos) {
	    return min_pos + grid_size * v3f(probe_coord);
    }
    v3f probe_position(probe_box_t* probe_box, probe_t* probe, u32 id) {
        return probe->position + probe_start_position(index_3d(probe_box->settings.dim, id), v3f{probe_box->grid_size}, probe_box->aabb.min);
    }
}



#endif