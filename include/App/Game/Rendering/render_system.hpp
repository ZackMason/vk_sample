#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "core.hpp"

#include "App/vk_state.hpp"

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
            return meshes[id]->mesh;
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
        const material_node_t*      material{0};
        const gfx::mesh_list_t*     meshes{0};

        m44                         transform{1.0f};
    };

    // for double buffering
    struct frame_data_t {
        VkSemaphore present_semaphore, render_semaphore;
        VkFence render_fence;

        VkCommandPool command_pool;
        VkCommandBuffer main_command_buffer;
    };

    struct system_t {
        inline static constexpr size_t  frame_arena_size = megabytes(64);
        inline static constexpr u32     frame_overlap = 2;
        inline static constexpr size_t max_scene_vertex_count{10'000'000};
        inline static constexpr size_t max_scene_index_count{30'000'000};

        arena_t arena;
        arena_t frame_arena;

        utl::deque<render_job_t> render_jobs{};

        mesh_cache_t    mesh_cache{};
        utl::str_hash_t mesh_hash{};

        utl::deque<material_node_t> materials{};

        u64             frame_count{0};
        frame_data_t    frames[frame_overlap];

        m44 projection{1.0f};

        gfx::vul::vertex_buffer_t<gfx::vertex_t, max_scene_vertex_count> vertices;
        gfx::vul::index_buffer_t<max_scene_index_count> indices;

        gfx::vul::storage_buffer_t<m44, 10'000>    job_storage_buffer;


        frame_data_t& get_frame_data() {
            return frames[frame_count%frame_overlap];
        }
    };

    template <size_t Size = gigabytes(1)>
    inline system_t*
    init(
        gfx::vul::state_t& state,
        arena_t* arena
    ) noexcept {
        static_assert(Size > system_t::frame_arena_size);
        auto* rs = arena_alloc_ctor<system_t>(arena, 1,
            system_t {
                .arena = arena_sub_arena(arena, Size),
            }
        );
        utl::str_hash_create(rs->mesh_hash);
        rs->frame_arena = arena_sub_arena(&rs->arena, system_t::frame_arena_size);

        state.create_storage_buffer(&rs->job_storage_buffer);

        VkCommandPoolCreateInfo command_pool_info = gfx::vul::utl::command_pool_create_info();
        command_pool_info.queueFamilyIndex = state.graphics_index;
        command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        for (int i = 0; i < system_t::frame_overlap; i++) {
            VK_OK(vkCreateCommandPool(state.device, &command_pool_info, nullptr, &rs->frames[i].command_pool));

            //allocate the default command buffer that we will use for rendering
            auto cmdAllocInfo = gfx::vul::utl::command_buffer_allocate_info(rs->frames[i].command_pool, 1);

            VK_OK(vkAllocateCommandBuffers(state.device, &cmdAllocInfo, &rs->frames[i].main_command_buffer));
        }
        return rs;
    }

    inline void
    clear(system_t* rs) {
        rs->render_jobs.clear();
        rs->materials.clear();
        rs->mesh_cache.meshes.clear();
        utl::str_hash_create(rs->mesh_hash);

        arena_clear(&rs->arena);
    }

    inline void
    begin_frame(system_t* rs) {
        rs->render_jobs.clear();
        rs->job_storage_buffer.pool.clear();

        arena_clear(&rs->frame_arena);
    }

    inline void
    submit_job(
        system_t* rs,
        u64 mesh_id,
        material_node_t* material, // todo(zack): remove this
        m44 transform
    ) {
        auto* job = arena_alloc_ctor<render_job_t>(&rs->frame_arena);
        job->meshes = &rs->mesh_cache.get(mesh_id);
        job->material = material;
        job->transform = transform;

        *rs->job_storage_buffer.pool.allocate(1) = transform;

        rs->render_jobs.push_back(job);
    }

    inline void
    build_commands(
        system_t* rs,
        VkCommandBuffer command_buffer
    ) {
        const material_node_t* last_material{0};

        for (size_t i = 0; i < rs->render_jobs.size(); i++) {
            render_job_t* job = rs->render_jobs[i];

            if (job->material != last_material) {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, job->material->pipeline);
                last_material = job->material;
            }

            gfx::vul::object_push_constants_t push_constants;
            push_constants.material = *job->material;
            push_constants.model    = job->transform;

            vkCmdPushConstants(command_buffer, job->material->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_constants), &push_constants);

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

        begin_frame(rs);
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

    inline material_node_t* 
    create_material(
        system_t* rs,
        std::string_view name, 
        gfx::material_t&& p_material,
        VkPipeline pipeline,
        VkPipelineLayout pipeline_layout
    ) {
        material_node_t* material{0};
        for (size_t i = 0; i < rs->materials.size(); i++) {
            if (rs->materials[i]->name == name) {
                material = rs->materials[i];
            }
        }
        if (!material) {
            gen_info("rendering", "Creating material: {}", name);
            material = arena_alloc_ctor<material_node_t>(&rs->arena, 1, std::move(p_material));
            material->name.own(&rs->arena, name);
            material->pipeline = pipeline;
            material->pipeline_layout = pipeline_layout;
            rs->materials.push_back(material);
        }
        return material;
    }

    
};



#endif 