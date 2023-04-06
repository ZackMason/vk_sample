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



    struct ray_scene_t {
        struct model_t {
            gfx::vertex_t*  vertices{0};
            size_t          n_vertices{0};

            u32*            indices{0};
            size_t          n_indices{0};

            math::aabb_t<v3f> aabb{};
        };

        
        struct node_t {
            node_t* next{};
            model_t* model{};
            math::transform_t transform{};
        };

        model_t models[128];
        size_t  n_models;
        node_t* nodes;
        math::ray_t camera;

        u32* pixels{0};
        u64 n_pixels{0};
        u64 width{0};
        u64 height{0};

        u32 sample_count{50};
        utl::rng::random_t<utl::rng::xor64_random_t> entropy{0xf233898dae};
    };

    inline static ray_scene_t*
    init_ray_scene(arena_t* arena, u64 width, u64 height) {
        auto* scene = arena_alloc<ray_scene_t>(arena);

        scene->width = width;
        scene->height = height;

        scene->pixels = arena_alloc_ctor<u32>(arena, scene->n_pixels = width * height);
        std::memset(scene->pixels, 0x00, scene->n_pixels);
        
        return scene;
    }

    inline static ray_scene_t::model_t*
    add_model(
        ray_scene_t* scene,
        gfx::vertex_t*  vertices,
        size_t          n_vertices,
        u32*            indices,
        size_t          n_indices
    ) {
        auto* model = scene->models + scene->n_models++;

        model->vertices = vertices;
        model->n_vertices = n_vertices;
        model->indices = indices;
        model->n_indices = n_indices;

        model->aabb = {};

        range_u64(i, 0, n_vertices) {
            model->aabb.expand(vertices[i].pos);
        }

        return model;
    }

    inline static void
    add_node(
        ray_scene_t* scene, 
        arena_t* arena, 
        ray_scene_t::model_t* model, 
        const math::transform_t& transform
    ) {
        auto* node = arena_alloc<ray_scene_t::node_t>(arena);
        node->model = model;
        node->transform = transform;

        node_push(node, scene->nodes);
    }

    inline static void 
    raytrace(ray_scene_t* scene) noexcept {
        TIMED_FUNCTION;
        
        auto& [
            models,
            n_models,
            nodes,
            camera,
            pixels,
            n_pixels,
            width, height,
            sample_count,
            entropy
        ] = *scene;

        const f32 focal_length = 100.0f;
        const v3f hor{f32(width)/2.0f, 0.0f, 0.0f};
        const v3f ver{0.0f, f32(height)/2.0f, 0.0f};
        const v3f left_corner = camera.origin - hor/2.0f - ver/2.0f - v3f{0.0f, 0.0f, focal_length};
        const v3f L = glm::normalize(v3f{1.0f,2.0f,3.0f});
        range_u64(x, 0, width) {
            range_u64(y, 0, height) {
                const u64 pixel_index = x + y * width;
                f32 distance = 1000000000.0f;
                math::ray_t ray = camera;
                const f32 u = (f32(x)/f32(width-1));
                const f32 v = 1.0f-(f32(y)/f32(height-1));

                camera.direction = left_corner + u*hor + v*ver - camera.origin;
                camera.direction = glm::normalize(camera.direction);

                node_for(auto, nodes, node) {
                    const auto aabb = node->transform.xform_aabb(node->model->aabb);
                    if (auto hit = math::intersect(ray, aabb); hit.intersect) {
                        TIMED_BLOCK(TraceModel);
                        auto* model = node->model;
                        auto* tris = model->indices;
                        auto* vert = model->vertices;
                        range_u64(i, 0, model->n_indices/3) {
                            math::triangle_t tri;
                            tri.p[0] = vert[tris[i*3+0]].pos;
                            tri.p[1] = vert[tris[i*3+1]].pos;
                            tri.p[2] = vert[tris[i*3+2]].pos;
                            if (math::intersect(ray, tri, distance)) {
                                pixels[pixel_index] = gfx::color::to_color32(
                                    glm::abs(v3f{glm::dot(L,tri.normal())})
                                );
                            }
                        }
                    } else {
                        pixels[pixel_index] = gfx::color::rgba::black;
                    }
                }
            }
        }
    }

};

#endif