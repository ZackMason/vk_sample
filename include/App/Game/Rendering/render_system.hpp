#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "core.hpp"

#include "App/vk_state.hpp"

#include "lighting.hpp"


namespace game::rendering {
    struct mesh_cache_t {
        struct link_t : node_t<link_t> {
            gfx::mesh_list_t mesh;
        };

        utl::deque<link_t> meshes;

        u64 add(arena_t* arena, const gfx::mesh_list_t& r) {
            link_t* link = arena_alloc_ctor<link_t>(arena, 1);
            link->mesh = r;
            meshes.push_back(link);
            return meshes.size() - 1;
        }

        const gfx::mesh_list_t& get(u64 id) const {
            assert(id < meshes.size() && "Mesh is not loaded");
            return meshes[id]->mesh;
        }
    };
    
    struct shader_cache_t {
        struct link_t : node_t<link_t> {
            std::string_view name;
            u64 hash{0};
            VkShaderEXT shader{VK_NULL_HANDLE};
        };

        link_t shaders[512]{};

        u64 add(VkShaderEXT shader, std::string_view name) {
            u64 hash = sid(name);
            u64 id = hash % array_count(shaders);
            // probe
            while(shaders[id].hash != 0 && shaders[id].hash != hash) {
                id = (id+1) % array_count(shaders);
            }
            shaders[id].hash = hash;
            shaders[id].name = name;
            shaders[id].shader = shader;
            return id;
        }

        VkShaderEXT get(std::string_view name) const {
            u64 hash = sid(name);
            u64 id = hash % array_count(shaders);
            u64 start = id;
            // probe
            while(shaders[id].hash != id) {
                id = (id+1) % array_count(shaders);
                if (id == start) return VK_NULL_HANDLE;
            }
            return shaders[id].shader;
        }

        VkShaderEXT get(u64 id) const {
            assert(shaders[id].hash);
            assert(shaders[id].shader != VK_NULL_HANDLE);
            return shaders[id].shader;
        }
    };

    struct material_node_t : public gfx::material_t, node_t<material_node_t> {
        using gfx::material_t::material_t;

        material_node_t(gfx::material_t&& mat) : gfx::material_t{std::move(mat)} {}
        string_t name{};
        VkPipeline pipeline;
        VkPipelineLayout pipeline_layout;
    };

    struct render_job_t : public node_t<render_job_t> {
        u32                         material{0};
        const gfx::mesh_list_t*     meshes{0};

        m44                         transform{1.0f};
    };

    struct directional_light_t {
        v4f direction{};
        v4f color{};
    };

    struct point_light_t {
        v4f pos; // position, w unused
        v4f col; // color
        v4f rad; // radiance, w unused
        v4f pad;
    };

    struct environment_t {
        directional_light_t sun{};

        v4f ambient_color{};

        v4f fog_color{};
        f32 fog_density{};

        // color correction
        f32 saturation{};
        f32 contrast{};
        u32 light_count{};
        v4f more_padding{};
        
        point_light_t point_lights[512];
    };


    // for double buffering
    struct frame_data_t {
        VkSemaphore present_semaphore, render_semaphore;
        VkFence render_fence;

        VkCommandPool command_pool;
        VkCommandBuffer main_command_buffer;
    };

    struct render_data_t {
        m44 model;
        u32 mat_id;
        u32 padding[3+4*3];
    };

    struct system_t {
        inline static constexpr size_t  frame_arena_size = megabytes(64);
        inline static constexpr u32     frame_overlap = 2;
        inline static constexpr size_t max_scene_vertex_count{10'000'000};
        inline static constexpr size_t max_scene_index_count{30'000'000};

        system_t(arena_t _arena) : arena{_arena} {}

        arena_t arena;
        arena_t frame_arena;

        gfx::vul::state_t* vk_gfx{0};

        std::mutex ticket;

        // utl::deque<render_job_t> render_jobs{};
        render_job_t* render_jobs{};
        size_t render_job_count{};

        mesh_cache_t    mesh_cache{};
        utl::str_hash_t mesh_hash{};

        utl::deque<material_node_t> materials{};

        u64             frame_count{0};
        frame_data_t    frames[frame_overlap];

        VkRenderPass        render_passes[8];
        u32                 render_pass_count{1};
        VkFramebuffer*      framebuffers{0};
        u32                 framebuffer_count{0};
        gfx::vul::framebuffer_t* render_targets{0};
        u32                     render_target_count{0};

        gfx::vul::vertex_buffer_t<gfx::vertex_t, max_scene_vertex_count> vertices;
        gfx::vul::index_buffer_t<max_scene_index_count> indices;

        gfx::vul::storage_buffer_t<render_data_t, 10'000>   job_storage_buffer;
        gfx::vul::storage_buffer_t<gfx::material_t, 100>    material_storage_buffer;
        gfx::vul::storage_buffer_t<environment_t, 1>    environment_storage_buffer;

        m44 vp{1.0f};
        m44 projection{1.0f};
        v3f camera_pos{0.0f};

        lighting::probe_box_t light_probes{v3f{-30.0f, -6.0f, -30.0f}, v3f{40.0f}};

        void set_view(const m44& view) {
            vp = projection * view;
        }

        frame_data_t& get_frame_data() {
            return frames[frame_count%frame_overlap];
        }
    };
    
    inline void 
    create_render_pass(
        system_t* rs,
        VkDevice device, 
        VkFormat format
    );
    inline void 
    create_framebuffers(
        system_t* rs,
        arena_t* arena,
        VkDevice device, 
        VkImageView* swap_chain_image_views,
        size_t swap_chain_count,
        VkImageView depth_image_view,
        u32 w, u32 h
    );

    template <size_t Size = gigabytes(1)>
    inline system_t*
    init(
        gfx::vul::state_t& state,
        arena_t* arena
    ) noexcept {
        static_assert(Size > system_t::frame_arena_size);
        auto* rs = arena_alloc_ctor<system_t>(arena, 1,
            arena_sub_arena(arena, Size)
        );
        rs->vk_gfx = &state;
        utl::str_hash_create(rs->mesh_hash);
        rs->frame_arena = arena_sub_arena(&rs->arena, system_t::frame_arena_size);
        rs->render_jobs = (render_job_t*)arena_alloc(&rs->frame_arena, 10000);

        {
            const f32 w = (f32)state.swap_chain_extent.width;
            const f32 h = (f32)state.swap_chain_extent.height;
            const f32 aspect = w / h;

            rs->projection = glm::perspective(45.0f, aspect, 0.3f, 1000.0f);
            rs->projection[1][1] *= -1.0f;
        }

        create_render_pass(rs, state.device, state.swap_chain_image_format);
        create_framebuffers(
            rs, &rs->arena, state.device, 
            state.swap_chain_image_views.data(), state.swap_chain_image_views.size(),
            state.depth_stencil_texture.image_view, state.swap_chain_extent.width, state.swap_chain_extent.height
        );

        state.create_storage_buffer(&rs->job_storage_buffer);
        state.create_storage_buffer(&rs->material_storage_buffer);
        state.create_storage_buffer(&rs->environment_storage_buffer);

        rs->environment_storage_buffer.pool[0].fog_color = v4f{0.5f,0.6f,0.7f,0.0f};

        VkCommandPoolCreateInfo command_pool_info = gfx::vul::utl::command_pool_create_info();
        command_pool_info.queueFamilyIndex = state.graphics_index;
        command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        for (int i = 0; i < system_t::frame_overlap; i++) {
            VK_OK(vkCreateCommandPool(state.device, &command_pool_info, nullptr, &rs->frames[i].command_pool));

            //allocate the default command buffer that we will use for rendering
            auto cmdAllocInfo = gfx::vul::utl::command_buffer_allocate_info(rs->frames[i].command_pool, 1);

            VK_OK(vkAllocateCommandBuffers(state.device, &cmdAllocInfo, &rs->frames[i].main_command_buffer));
        }

        lighting::set_probes(*rs->vk_gfx, &rs->light_probes, &rs->arena);

        return rs;
    }

    inline void
    clear(system_t* rs) {
        // rs->render_jobs.clear();
        rs->render_job_count = 0;
        rs->materials.clear();
        rs->mesh_cache.meshes.clear();
        utl::str_hash_create(rs->mesh_hash);

        arena_clear(&rs->arena);
    }

    inline void
    begin_frame(system_t* rs) {
        rs->render_job_count = 0;
        // rs->render_jobs.clear();
        rs->job_storage_buffer.pool.clear();

        arena_clear(&rs->frame_arena);
    }

    inline void
    submit_job(
        system_t* rs,
        u64 mesh_id,
        u32 mat_id, // todo(zack): remove this
        m44 transform
    ) {
        TIMED_FUNCTION;
        // auto* job = arena_alloc_ctor<render_job_t>(&rs->frame_arena);
        auto* job = rs->render_jobs + rs->render_job_count++;
        job->meshes = &rs->mesh_cache.get(mesh_id);
        job->material = mat_id;
        job->transform = transform;

        auto* gpu_job = rs->job_storage_buffer.pool.allocate(1);
        gpu_job->model = transform;
        gpu_job->mat_id = mat_id;

        // rs->render_jobs.push_back(job);
    }

    inline void
    build_commands(
        system_t* rs,
        VkCommandBuffer command_buffer
    ) {
        TIMED_FUNCTION;

        // std::lock_guard lock{rs->ticket};
        const material_node_t* last_material{0};

        for (size_t i = 0; i < rs->render_job_count; i++) {
            render_job_t* job = rs->render_jobs + i;
            
            const material_node_t* mat = rs->materials[job->material];

            if (!last_material || (mat->pipeline != last_material->pipeline)) {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline);

                assert(mat->pipeline_layout != VK_NULL_HANDLE);

                struct pc_t {
                    m44 vp;
                    v4f cp;
                } constants;
                constants.vp = rs->vp;
                constants.cp = v4f{rs->camera_pos, 0.0f};
                                
                vkCmdPushConstants(command_buffer, mat->pipeline_layout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                    0, sizeof(constants), &constants
                );
                last_material = mat;
            }

            // gfx::vul::object_push_constants_t push_constants;
            // push_constants.material = *job->material;
            // push_constants.model    = job->transform;

            // vkCmdPushConstants(command_buffer, job->material->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_constants), &push_constants);

            loop_iota_u64(j, job->meshes->count) {
                if (job->meshes->meshes[j].index_count) {
                    vkCmdDrawIndexed(command_buffer,
                        job->meshes->meshes[j].index_count,
                        1, 
                        job->meshes->meshes[j].index_start,
                        job->meshes->meshes[j].vertex_start,
                        safe_truncate_u64(i)
                    );
                } else {
                    vkCmdDraw(command_buffer,
                        job->meshes->meshes[j].vertex_count,
                        job->meshes->meshes[j].instance_count,
                        job->meshes->meshes[j].vertex_start,
                        safe_truncate_u64(i)
                        // job->meshes->meshes[j].instance_start
                    );
                }
            }
        }

        // begin_frame(rs);
    }

    inline u64
    add_mesh(
        system_t* rs,
        std::string_view name,
        const gfx::mesh_list_t& mesh
    ) {
        const auto mesh_id = rs->mesh_cache.add(&rs->arena, mesh);
        utl::str_hash_add(rs->mesh_hash, name, mesh_id);

        return mesh_id;
    }

    inline u64
    get_mesh_id(
        system_t* rs,
        std::string_view name
    ) {
        return utl::str_hash_find(rs->mesh_hash, name);
    }

    inline const gfx::mesh_list_t&
    get_mesh(
        system_t* rs,
        std::string_view name
    ) {
        return rs->mesh_cache.get(utl::str_hash_find(rs->mesh_hash, name));
    }

    inline math::aabb_t<v3f>
    get_mesh_aabb(
        system_t* rs,
        std::string_view name
    ) {
        return get_mesh(rs, name).aabb;
    }


    inline u32
    create_material(
        system_t* rs,
        std::string_view name, 
        gfx::material_t&& p_material,
        VkPipeline pipeline,
        VkPipelineLayout pipeline_layout
    ) {
        material_node_t* material{0};
        for (u32 i = 0; i < rs->materials.size(); i++) {
            if (rs->materials[i]->name == name) {
                material = rs->materials[i];
                return i;
            }
        }
        if (!material) {
            gen_info("rendering", "Creating material: {}", name);
            material = arena_alloc_ctor<material_node_t>(&rs->arena, 1, std::move(p_material));
            material->name.own(&rs->arena, name);
            material->pipeline = pipeline;
            material->pipeline_layout = pipeline_layout;
            rs->materials.push_back(material);

            *rs->material_storage_buffer.pool.allocate(1) = *material;
        }
        return safe_truncate_u64(rs->materials.size() - 1);
        // return material;
    }
    
    inline void 
    create_render_pass(
        system_t* rs,
        VkDevice device, 
        VkFormat format
    ) {
        VkAttachmentDescription vad[2];
        vad[0] = gfx::vul::utl::attachment_description(
            format,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );

        vad[1] = gfx::vul::utl::attachment_description(
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );

        VkAttachmentReference color_attachment_refs[1];
        VkAttachmentReference depth_attachment_refs[1];

        color_attachment_refs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment_refs[0].attachment = 0;

        depth_attachment_refs[0].attachment = 1;
        depth_attachment_refs[0].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = color_attachment_refs;

        subpass.pDepthStencilAttachment = depth_attachment_refs;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = vad;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        
        // idk if this is correct
        VkSubpassDependency dependencies[2];
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = dependencies;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &rs->render_passes[0]) != VK_SUCCESS) {
            gen_error("vulkan", "failed to create render pass!");
            std::terminate();
        }
    }

    inline void 
    create_framebuffers(
        system_t* rs,
        arena_t* arena,
        VkDevice device, 
        VkImageView* swap_chain_image_views,
        size_t swap_chain_count,
        VkImageView depth_image_view,
        u32 w, u32 h
    ) {
        rs->render_targets = arena_alloc_ctor<gfx::vul::framebuffer_t>(arena, 1);
        rs->render_target_count = 1;
        rs->render_targets->width = w;
        rs->render_targets->height = h;
        {
            rs->vk_gfx->create_framebuffer(rs->render_targets);
            {
                gfx::vul::framebuffer_attachment_create_info_t fbaci{};
                fbaci.width = w; fbaci.height = h;
                fbaci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                // fbaci.format = VK_FORMAT_B32G32R32A32_SFLOAT;
                // fbaci.format = VK_FORMAT_B8G8R8A8_SRGB;
                fbaci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                rs->vk_gfx->add_framebuffer_attachment(arena, rs->render_targets, fbaci);
            }
            {
                gfx::vul::framebuffer_attachment_create_info_t fbaci{};
                fbaci.width = w; fbaci.height = h;
                fbaci.format = VK_FORMAT_D24_UNORM_S8_UINT;
                fbaci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                rs->vk_gfx->add_framebuffer_attachment(arena, rs->render_targets, fbaci);
            }

            rs->vk_gfx->create_framebuffer_sampler(rs->render_targets, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
            rs->vk_gfx->framebuffer_create_renderpass(rs->render_targets);
        }


        rs->framebuffers = arena_alloc_ctor<VkFramebuffer>(arena, swap_chain_count);
        rs->framebuffer_count = safe_truncate_u64(swap_chain_count);
        for (size_t i = 0; i < swap_chain_count; i++) {
            VkImageView attachments[] = {
                swap_chain_image_views[i],
                depth_image_view
            };

            VkFramebufferCreateInfo vfci{};
            vfci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            vfci.renderPass = rs->render_passes[0];
            vfci.attachmentCount = array_count(attachments);
            vfci.pAttachments = attachments;
            vfci.width = w;
            vfci.height = h;
            vfci.layers = 1;

            if (vkCreateFramebuffer(device, &vfci, nullptr, &rs->framebuffers[i]) != VK_SUCCESS) {
                gen_error("vulkan", "failed to create framebuffer!");
                std::terminate();
            }
        }
    }
};



#endif 