#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "zyy_core.hpp"

#include "App/vk_state.hpp"

#include "App/Game/Util/loading.hpp"

#include "lighting.hpp"
#include "assets.hpp"
#include "descriptor_allocator.hpp"

struct RenderingStats {
    bool show{false};
    u64 triangle_count{0};
    u64 vertex_count{0};
    u64 shader_count{0};
    u64 texture_count{0};

    void reset() {
        triangle_count = 0;
        vertex_count = 0;
        shader_count = 0;
        texture_count = 0;
    }
};

REFLECT_TYPE(RenderingStats) {REFLECT_TYPE_INFO(RenderingStats)}
    .REFLECT_PROP(RenderingStats, show)
    .REFLECT_PROP(RenderingStats, triangle_count)
    .REFLECT_PROP(RenderingStats, vertex_count)
    .REFLECT_PROP(RenderingStats, shader_count)
    .REFLECT_PROP(RenderingStats, texture_count);
};


namespace rendering {
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

        gfx::mesh_list_t& get(u64 id) {
            assert(id < meshes.size() && "Mesh is not loaded");
            return meshes[id]->mesh;
        }
    };
    
    struct texture_cache_t {
        static inline constexpr u64 invalid = std::numeric_limits<u64>::max();
        struct link_t {
            std::string_view name;
            u64 hash{0};
            gfx::vul::texture_2d_t texture;        

            // hash will probably never be 1, should probably check this
            void kill() {
                hash = 0x1;
            }
            bool is_dead() const {
                return hash==0||hash==0x1;
            }
        };

        // @hash
        link_t textures[4096]{};

        u64 insert(std::string_view name, gfx::vul::texture_2d_t texture) {
            u64 hash = sid(name);
            u64 index = hash % array_count(textures);
            assert(textures[index].is_dead());
            textures[index].name = name;
            textures[index].hash = hash;
            textures[index].texture = texture;
            return index;
        }

        void reload_all(
            arena_t arena,
            gfx::vul::state_t vk_gfx
        ) {
            range_u64(i, 0, array_count(textures)) {
                if (textures[i].hash) {
                    textures[i].hash = 0;

#if !ZYY_INTERNAL // for debug builds, we dont want to destroy resources because of hot reloading
                    // Todo(Zack): destroy texture
#endif
                    load(
                        arena, 
                        vk_gfx,
                        textures[i].name.data()
                    );
                }
            }
        }

        u64 load(
            arena_t arena,
            gfx::vul::state_t vk_gfx,
            const char* filename
        ) {
            gfx::vul::texture_2d_t texture{};
            if (utl::has_extension(filename, "png")) {
                assert(0); // need to remove extension
                vk_gfx.load_texture_sampler(&texture, filename, &arena);
            } else {
                vk_gfx.load_texture_sampler(&texture, fmt_sv("res/textures/{}", filename), &arena);
            }
            return add(
                texture, 
                filename
            );
        }

        // returns a bitmask of which textures were loaded
        u32 load_material(
            arena_t arena,
            const gfx::vul::state_t& vk_gfx,
            gfx::material_info_t& material_info,
            u64 ids[2]
        ) {
            u32 count = 0;
            if (material_info.albedo[0]) { ids[0] = load(arena, vk_gfx, material_info.albedo); count |= 0x1; }
            if (material_info.normal[0]) { ids[1] = load(arena, vk_gfx, material_info.normal); count |= 0x2; }
            return count;
        }

        u64 add(
            const gfx::vul::texture_2d_t& texture,
            std::string_view name            
        ) {
            u64 hash = sid(name);
            assert(hash!=1);
            u64 id = hash % array_count(textures);
            // probe
            while(textures[id].is_dead() == false && textures[id].hash != hash) {
                zyy_warn(__FUNCTION__, "probing for texture, {} collided with {}", name, textures[id].name);
                id = (id+1) % array_count(textures);
            }

            zyy_info(__FUNCTION__, "Adding texture: {}", name);
            textures[id].hash = hash;
            textures[id].name = name;
            textures[id].texture = texture;
            
            return id;
        }

private:
        const link_t& get_(std::string_view name) const {
            u64 hash = sid(name);
            assert(hash!=1);
            u64 id = hash % array_count(textures);
            u64 start = id;
            // probe
            while(textures[id].hash && textures[id].hash != hash) {
                zyy_warn(__FUNCTION__, "probing for texture");
                id = (id+1) % array_count(textures);
                if (id == start) {
                    zyy_warn(__FUNCTION__, "hash map need to be bigger, this should never happen");
                    assert(0);
                    return textures[0];
                }
            }
            return textures[id];
        }
        link_t& get_(std::string_view name) {
            u64 hash = sid(name);
            assert(hash!=1);
            u64 id = hash % array_count(textures);
            u64 start = id;
            // probe
            while(textures[id].hash && textures[id].hash != hash) {
                zyy_warn(__FUNCTION__, "probing for texture");
                id = (id+1) % array_count(textures);
                if (id == start) {
                    zyy_warn(__FUNCTION__, "hash map need to be bigger, this should never happen");
                    assert(0);
                    return textures[0];
                }
            }
            return textures[id];
        }

public:
        inline static constexpr u64 npos = invalid;

        u64 first(u64 i = 0) const {
            range_u64(j, i, array_count(textures)) {
                if (textures[j].hash != 0) {
                    return j;
                }
            }
            return npos;
        }

        u64 next(u64 i) const {
            return first(i+1);
        }

        u64 end() const {
            return npos;
        }

        gfx::vul::texture_2d_t* operator[](u64 id) {
            return &textures[id].texture;
        }
        const gfx::vul::texture_2d_t* operator[](u64 id) const {
            return &textures[id].texture;
        }
        const gfx::vul::texture_2d_t* operator[](std::string_view name) const {
            return get(name);
        }
        gfx::vul::texture_2d_t* operator[](std::string_view name) {
            return get(name);
        }

        const gfx::vul::texture_2d_t* get(std::string_view name) const {
            return &get_(name).texture;
        }
        gfx::vul::texture_2d_t* get(std::string_view name) {
            return &get_(name).texture;
        }
        u64 get_id(std::string_view name) {
            return get_(name).hash % array_count(textures);
        }

        const gfx::vul::texture_2d_t* get(u64 id) const {
            assert(textures[id].hash);
            assert(textures[id].texture.image != VK_NULL_HANDLE);
            return &textures[id].texture;
        }
    };

    struct shader_cache_t {
        struct link_t {
            std::string_view name;
            u64 hash{0};
            VkShaderEXT shader{VK_NULL_HANDLE};
            VkDescriptorSetLayout* descriptor_layout{0};
            VkShaderStageFlagBits stage{};
            VkShaderStageFlagBits next_stage{};
            u32 descriptor_count{0};
            u32 push_constant_size{0};
            SpvReflectShaderModule reflection;

            // hash will probably never be 1, should probably check this
            void kill() {
                hash = 0x1;
            }
            bool is_dead() const {
                return !hash||hash == 0x1;
            }
        };

        inline void
        destroy(const gfx::vul::state_t& vk_gfx) {
            range_u64(i, 0, array_count(shaders)) {
                if (shaders[i].hash) {
                    shaders[i].hash = 0;
                    vk_gfx.ext.vkDestroyShaderEXT(vk_gfx.device, shaders[i].shader, nullptr);
                }
            }
        }

        // @hash
        link_t shaders[64<<2]{};

        void reload_all(
            arena_t arena,
            gfx::vul::state_t vk_gfx
        ) {
            range_u64(i, 0, array_count(shaders)) {
                if (shaders[i].hash) {
                    shaders[i].hash = 0;
#if !ZYY_INTERNAL // for debug builds, we dont want to destroy resources because of hot reloading
                    vk_gfx.ext.vkDestroyShaderEXT(vk_gfx.device, shaders[i].shader, nullptr);
#endif
                    load(
                        arena, 
                        vk_gfx,
                        shaders[i].name.data(),
                        shaders[i].stage,
                        shaders[i].next_stage,
                        shaders[i].descriptor_layout,
                        shaders[i].descriptor_count,
                        shaders[i].push_constant_size
                    );
                }
            }
        }

        u64 load(
            arena_t arena,
            const gfx::vul::state_t& vk_gfx,
            const char* filename,
            VkShaderStageFlagBits stage,
            VkShaderStageFlagBits next_stage,
            VkDescriptorSetLayout* descriptor_set_layouts,
            u32 descriptor_count,
            u32 push_constant_size
        ) {
            VkShaderEXT shader[1];
            SpvReflectShaderModule reflect[1];
            create_shader_objects_from_files(
                arena,
                vk_gfx,
                descriptor_set_layouts,
                descriptor_count,
                push_constant_size,
                &stage, &next_stage,
                &filename,
                1,
                shader /* out */,
                reflect /* out */
            );
            return add(
                shader[0], 
                filename, 
                stage, next_stage,
                descriptor_set_layouts, 
                descriptor_count, 
                push_constant_size, 
                reflect[0]
            );
        }

        u64 load(
            arena_t arena,
            const gfx::vul::state_t& vk_gfx,
            const gfx::shader_description_t& shader_description,
            VkDescriptorSetLayout* descriptor_set_layouts,
            u32 descriptor_count
        ) {
            return load(
                arena, vk_gfx, 
                shader_description.filename.data(), 
                (VkShaderStageFlagBits)shader_description.stage, 
                (VkShaderStageFlagBits)shader_description.next_stages, 
                descriptor_set_layouts, 
                descriptor_count,
                shader_description.push_constant_size
            );
        }

        u64 add(
            VkShaderEXT shader, 
            std::string_view name, 
            VkShaderStageFlagBits stage,
            VkShaderStageFlagBits next_stage,
            VkDescriptorSetLayout* descriptor_layout, 
            u32 descriptor_count,
            u32 push_constant_size,
            std::optional<SpvReflectShaderModule> reflect = std::nullopt
        ) {
            u64 hash = sid(name);
            assert(hash!=1);
            u64 id = hash % array_count(shaders);
            // probe
            while(shaders[id].hash != 0 && shaders[id].hash != hash) {
                zyy_warn(__FUNCTION__, "probing for shader, {} collided with {}", name, shaders[id].name);
                id = (id+1) % array_count(shaders);
            }

            zyy_info(__FUNCTION__, "Adding shader: {}", name);
            shaders[id].hash = hash;
            shaders[id].name = name;
            shaders[id].stage = stage;
            shaders[id].next_stage = next_stage;
            shaders[id].shader = shader;
            shaders[id].descriptor_layout = descriptor_layout;
            shaders[id].descriptor_count = descriptor_count;
            shaders[id].push_constant_size = push_constant_size;
            if (reflect) {
                shaders[id].reflection = *reflect;
            }
            return id;
        }

private:
        const link_t& get_(std::string_view name) const {
            u64 hash = sid(name);
            assert(hash!=1);
            u64 id = hash % array_count(shaders);
            u64 start = id;
            // probe
            while(!shaders[id].hash && shaders[id].hash != hash) {
                zyy_warn(__FUNCTION__, "probing for shader ({}), hit {}", name, shaders[id].name);
                id = (id+1) % array_count(shaders);
                if (id == start) {
                    zyy_warn(__FUNCTION__, "hash map need to be bigger, this should never happen");
                    assert(0);
                    return shaders[0];
                }
            }
            return shaders[id];
        }

public:
        const VkShaderEXT* operator[](std::string_view name) const {
            return get(name);
        }

        const VkShaderEXT* get(std::string_view name) const {
            return &get_(name).shader;
        }

        SpvReflectShaderModule reflect(std::string_view name) const {
            return get_(name).reflection;
        }

        VkShaderEXT get(u64 id) const {
            assert(shaders[id].hash);
            assert(shaders[id].shader != VK_NULL_HANDLE);
            return shaders[id].shader;
        }
    };

#include "raytracing.hpp"

    struct material_node_t : public gfx::material_t, node_t<material_node_t> {
        using gfx::material_t::material_t;

        material_node_t(
            gfx::material_t&& mat,
            const VkShaderEXT** shaders_,
            u32 shader_count_
        ) : gfx::material_t{std::move(mat)}, shader_count{shader_count_}
        {
            std::memcpy(shaders, shaders_, sizeof(const VkShaderEXT*) * shader_count_);
        }

        string_t name{};
        VkPipeline pipeline;
        VkPipelineLayout pipeline_layout;
        const VkShaderEXT* shaders[10];
        u32 shader_count=0;
    };

    struct render_job_t : public node_t<render_job_t> {
        u32                         material{0};
        const gfx::mesh_list_t*     meshes{0};

        m44                         transform{1.0f};
        u32                         instance_count{1};
        u32                         blas_id{0};
    };

    struct render_pass_t {
        VkImage     image;
        VkImageView image_view;

        u32 width{}, height{};

        render_pass_t* sinks[32]{0};
        render_pass_t* sources[32]{0};
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

        v4f ambient_color{0.9569f, 0.8941f, 0.5804f, 1.0f};

        v4f fog_color{};
        f32 fog_density{};

        // color correction
        f32 ambient_strength{1.0f};
        f32 contrast{};
        u32 light_count{};
        v4f more_padding{};
        
        point_light_t point_lights[512];
    };

    struct pp_material_t {
        std::array<f32, 16> data{};
    };

    struct pp_pass_t {
        VkPipelineLayout pipeline_layout{};
        VkDescriptorSet texture_descriptor{};
        VkDescriptorSet parameters_descriptor{};
        VkDescriptorSetLayout descriptor_layouts[2];
        gfx::vul::storage_buffer_t<pp_material_t, 1> parameter_buffer;
        u32 descriptor_count{2};
        VkDevice device;
        pp_material_t parameters{};

        void build_layout(gfx::vul::state_t& vk_state) {
            utl::memzero(parameters.data.data(), sizeof(f32)*16);
            device = vk_state.device;
            pipeline_layout = gfx::vul::create_pipeline_layout(vk_state.device, descriptor_layouts, 1, sizeof(f32) * 16);
        }

        void build_buffer_sets(gfx::vul::descriptor_builder_t& builder) {
            VkDescriptorBufferInfo buffer_info[1];
            u32 i = 0;
            buffer_info[i].buffer = parameter_buffer.buffer;
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;
            
            using namespace gfx::vul;
            builder
                .bind_buffer(0, buffer_info + 0, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(parameters_descriptor, descriptor_layouts[1]);
            
        }

        void bind_images(gfx::vul::descriptor_builder_t& builder, gfx::vul::texture_2d_t* texture) {
            using namespace gfx::vul;
            VkDescriptorImageInfo vdii[1];
                vdii[0].imageLayout = texture->image_layout;
                vdii[0].imageView = texture->image_view;
                vdii[0].sampler = texture->sampler;

            builder
                .bind_image(0, vdii, array_count(vdii), (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Sampler, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build(texture_descriptor, descriptor_layouts[0]);
        }
    };

    struct mesh_pass_t {
        VkPipelineLayout pipeline_layout{};
        VkDescriptorSet sporadic_descriptors{};
        VkDescriptorSet object_descriptors{};
        VkDescriptorSet material_descriptor{};
        VkDescriptorSet enviornment_descriptor{};
        VkDescriptorSet texture_descriptor{};
        VkDescriptorSet light_probe_descriptor{};
        VkDescriptorSetLayout descriptor_layouts[6];
        u32 descriptor_count{6};
        VkDevice device;

        void build_layout(VkDevice device_) {
            device = device_;
            pipeline_layout = gfx::vul::create_pipeline_layout(device, descriptor_layouts, descriptor_count, sizeof(m44) * 2);
        }

        void build_buffer_sets(gfx::vul::descriptor_builder_t& builder, VkBuffer* buffers) {
            VkDescriptorBufferInfo buffer_info[8];
            u32 i = 0;
            buffer_info[i].buffer = buffers[i];
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;//sizeof(gfx::vul::sporadic_buffer_t);

            buffer_info[i].buffer = buffers[i];
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;//(sizeof(m44) + sizeof(u32) * 4 * 4) * 10'000; // hardcoded
            
            buffer_info[i].buffer = buffers[i];
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;// (sizeof(m44)) * 2'000'000; // hardcoded

            buffer_info[i].buffer = buffers[i];
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;//(sizeof(gfx::indirect_indexed_draw_t)) * 10'000; // hardcoded
    
            buffer_info[i].buffer = buffers[i];
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;//sizeof(gfx::material_t) * 100; // hardcoded

            buffer_info[i].buffer = buffers[i];
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;//sizeof(rendering::environment_t);

            buffer_info[i].buffer = buffers[i];
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;

            buffer_info[i].buffer = buffers[i];
            buffer_info[i].offset = 0; 
            buffer_info[i++].range = VK_WHOLE_SIZE;

            using namespace gfx::vul;
            builder
                .bind_buffer(0, buffer_info + 0, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Uniform, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(sporadic_descriptors, descriptor_layouts[0]);
            builder
                .bind_buffer(0, buffer_info + 1, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .bind_buffer(1, buffer_info + 2, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .bind_buffer(2, buffer_info + 3, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(object_descriptors, descriptor_layouts[1]);
            builder
                .bind_buffer(0, buffer_info + 4, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(material_descriptor, descriptor_layouts[2]);
            builder
                .bind_buffer(0, buffer_info + 5, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(enviornment_descriptor, descriptor_layouts[3]);
            builder
                .bind_buffer(0, buffer_info + 6, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .bind_buffer(1, buffer_info + 7, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(light_probe_descriptor, descriptor_layouts[5]);
        }

        void bind_images(
            gfx::vul::descriptor_builder_t& builder, 
            texture_cache_t& texture_cache,
            gfx::vul::texture_2d_t* irradiance_texture,
            gfx::vul::texture_2d_t* visibility_texture
        ) {
            using namespace gfx::vul;
            VkDescriptorImageInfo vdii[4096];
            VkDescriptorImageInfo pvdii[2];

            auto* null_texture = texture_cache["null"];
            range_u64(i, 0, array_count(texture_cache.textures)) {
                vdii[i].imageLayout = null_texture->image_layout;
                vdii[i].imageView = null_texture->image_view;
                vdii[i].sampler = null_texture->sampler;
            }

            u64 w{0};
            for(u64 i = texture_cache.first(); 
                i != texture_cache.end(); 
                i = texture_cache.next(i)
            ) {
                vdii[i].imageLayout = texture_cache[i]->image_layout;
                vdii[i].imageView = texture_cache[i]->image_view;
                vdii[i].sampler = texture_cache[i]->sampler;
            }

            pvdii[0].imageLayout = irradiance_texture->image_layout;
            pvdii[0].imageView = irradiance_texture->image_view;
            pvdii[0].sampler = irradiance_texture->sampler;

            pvdii[1].imageLayout = visibility_texture->image_layout;
            pvdii[1].imageView = visibility_texture->image_view;
            pvdii[1].sampler = visibility_texture->sampler;
    
            builder
                .bind_image(0, vdii, array_count(vdii), (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Sampler, VK_SHADER_STAGE_FRAGMENT_BIT)
                .bind_image(1, pvdii, array_count(pvdii), (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Sampler, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build(texture_descriptor, descriptor_layouts[4]);
        }
    };

    struct anim_pass_t {
        VkPipelineLayout pipeline_layout{};
        VkDescriptorSet sporadic_descriptors{};
        VkDescriptorSet object_descriptors{};
        VkDescriptorSet material_descriptor{};
        VkDescriptorSet enviornment_descriptor{};
        VkDescriptorSet texture_descriptor{};
        VkDescriptorSet animation_descriptor{};
        VkDescriptorSetLayout descriptor_layouts[6];
        u32 descriptor_count{6};
        VkDevice device;

        void build_layout(VkDevice device_) {
            device = device_;
            pipeline_layout = gfx::vul::create_pipeline_layout(device, descriptor_layouts, 6, sizeof(m44) + sizeof(v4f));
        }

        void build_buffer_sets(gfx::vul::descriptor_builder_t& builder, VkBuffer* buffers) {
            VkDescriptorBufferInfo buffer_info[5];
            buffer_info[0].buffer = buffers[0];
            buffer_info[0].offset = 0; 
            buffer_info[0].range = VK_WHOLE_SIZE;// sizeof(gfx::vul::sporadic_buffer_t);

            buffer_info[1].buffer = buffers[1];
            buffer_info[1].offset = 0; 
            buffer_info[1].range = VK_WHOLE_SIZE;// (sizeof(m44) + sizeof(u32) * 4 * 4) * 10'000; // hardcoded
            
            buffer_info[2].buffer = buffers[2];
            buffer_info[2].offset = 0; 
            buffer_info[2].range = VK_WHOLE_SIZE;// sizeof(gfx::material_t) * 100; // hardcoded

            buffer_info[3].buffer = buffers[3];
            buffer_info[3].offset = 0; 
            buffer_info[3].range = VK_WHOLE_SIZE;// sizeof(rendering::environment_t);

            buffer_info[4].buffer = buffers[4];
            buffer_info[4].offset = 0; 
            buffer_info[4].range = VK_WHOLE_SIZE;// sizeof(m44) * 256;

            using namespace gfx::vul;
            builder
                .bind_buffer(0, buffer_info + 0, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Uniform, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(sporadic_descriptors, descriptor_layouts[0]);
            builder
                .bind_buffer(0, buffer_info + 1, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(object_descriptors, descriptor_layouts[1]);
            builder
                .bind_buffer(0, buffer_info + 2, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(material_descriptor, descriptor_layouts[2]);
            builder
                .bind_buffer(0, buffer_info + 3, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT)
                .build(enviornment_descriptor, descriptor_layouts[3]);
            builder
                .bind_buffer(0, buffer_info + 4, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Storage, VK_SHADER_STAGE_VERTEX_BIT)
                .build(animation_descriptor, descriptor_layouts[5]);
        }

        void bind_images(gfx::vul::descriptor_builder_t& builder, gfx::vul::texture_2d_t* texture) {
            using namespace gfx::vul;
            VkDescriptorImageInfo vdii[1];
            vdii[0].imageLayout = texture->image_layout;
            vdii[0].imageView = texture->image_view;
            vdii[0].sampler = texture->sampler;
            builder
                .bind_image(4, vdii, 1, (VkDescriptorType)descriptor_create_info_t::DescriptorFlag_Sampler, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build(texture_descriptor, descriptor_layouts[4]);
        }
    };

    // for double buffering
    struct frame_data_t {
        // these arent being used
        VkSemaphore present_semaphore, render_semaphore;
        VkFence render_fence;

        VkCommandPool command_pool;
        VkCommandBuffer main_command_buffer;

        pp_pass_t postprocess_pass{};
        mesh_pass_t mesh_pass{};
        anim_pass_t anim_pass{};
        rt_compute_pass_t rt_compute_pass{};

        gfx::vul::descriptor_allocator_t* dynamic_descriptor_allocator{nullptr};

        gfx::vul::storage_buffer_t<gfx::indirect_indexed_draw_t, 1'000'000> indexed_indirect_storage_buffer{};
    };

    struct render_data_t {
        m44 model;
        v4f bounds;
        u32 mat_id;
        u32 padding[3+4*2];
    };

    struct frame_image_t {
        gfx::vul::texture_2d_t texture;

        u32 width() const {
            return (u32)texture.size[0];
        }

        u32 height() const {
            return (u32)texture.size[1];
        }
    };


    struct padded_vertex_t {
        v3f p;
        float p0;
        v3f n;
        float p1;
        v3f c;
        float p2;
        v2f t;
        float p3;
        float p4;
    };
    
    struct system_t {
        inline static constexpr umm  frame_arena_size = megabytes(64);
        inline static constexpr u32     frame_overlap = 3;
        inline static constexpr umm max_scene_vertex_count{10'000'000};
        inline static constexpr umm max_scene_index_count{30'000'000};
        inline static constexpr umm max_scene_skinned_vertex_count{1'000'000};
        inline static constexpr umm max_scene_skinned_index_count{3'000'000};

        system_t(arena_t _arena) : arena{_arena} {}

        arena_t arena;
        arena_t frame_arena;

        gfx::vul::state_t* vk_gfx{0};
        utl::res::pack_file_t* resource_file{0};

        std::mutex ticket{};

        // utl::deque<render_job_t> render_jobs{};
        render_job_t* render_jobs[frame_overlap]{};
        size_t render_job_count{};
        u32 total_instance_count{0};

        mesh_cache_t    mesh_cache{};
        utl::str_hash_t mesh_hash{};

        shader_cache_t  shader_cache{};
        texture_cache_t texture_cache{};

        utl::deque<material_node_t> materials{};

        u64            frame_count{0};
        frame_data_t   frames[frame_overlap];

        gfx::vul::descriptor_allocator_t* permanent_descriptor_allocator{nullptr};
        gfx::vul::descriptor_layout_cache_t* descriptor_layout_cache{nullptr};

        VkRenderPass        render_passes[8];
        u32                 render_pass_count{1};
        VkFramebuffer*      framebuffers{0};
        u32                 framebuffer_count{0};
        gfx::vul::framebuffer_t* render_targets{0};
        u32                     render_target_count{0};

        gfx::vul::vertex_buffer_t<gfx::vertex_t, max_scene_vertex_count> vertices;
        gfx::vul::index_buffer_t<max_scene_index_count> indices;

        gfx::vul::vertex_buffer_t<gfx::skinned_vertex_t, max_scene_skinned_vertex_count> skinned_vertices;
        gfx::vul::index_buffer_t<max_scene_skinned_index_count> skinned_indices;

        gfx::vul::storage_buffer_t<render_data_t, 1'000'000>   job_storage_buffers[frame_overlap];
        gfx::vul::storage_buffer_t<render_data_t, 1'000'000>&  job_storage_buffer() {
            return job_storage_buffers[frame_count%frame_overlap];
        }

        gfx::vul::storage_buffer_t<gfx::material_t, 100>    material_storage_buffer;
        gfx::vul::storage_buffer_t<environment_t, 1>        environment_storage_buffer;
        gfx::vul::storage_buffer_t<m44, 256>                animation_storage_buffer;
        gfx::vul::storage_buffer_t<m44, 2'000'000>          instance_storage_buffer;

        m44 vp{1.0f};
        m44 projection{1.0f};
        m44 view{1.0f};
        v3f camera_pos{0.0f};
        u32 width{}, height{};

        pp_material_t postprocess_params{};

        lighting::probe_buffer_t<10000> probe_storage_buffer;
        gfx::vul::storage_buffer_t<lighting::probe_settings_t, 1> light_probe_settings_buffer;
        // lighting::probe_box_t light_probes{.aabb={v3f{-200.0f, 1.0f, -100.0f}, v3f{200.0f, 50.0f, 100.0f}}};
        // lighting::probe_box_t light_probes{.aabb={v3f{-15.0f, 1, -30.0f}, v3f{25.0f, 25.0f, 20.0f}}};
        lighting::probe_box_t light_probes{.aabb={v3f{-20.0f, 1.0f, -10.0f}, v3f{20.0f, 24.0f, 15.0f}}};

        frame_image_t frame_images[8]{};

        RenderingStats stats{};

        rt_cache_t* rt_cache{0};

        v4f viewport() const {
            return v4f{0,0,(f32)width, (f32)height};
        }

        void set_view(const m44& view_, u32 w, u32 h) {
            view = view_;
            vp = projection * view;
            width = w;
            height = h;
        }

        size_t get_frame() {
            return frame_count%frame_overlap;
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

    static inline void
    cleanup(
        system_t* rs
    ) {
        utl::profile_t p{__FUNCTION__};
        rs->shader_cache.destroy(*rs->vk_gfx);
        rs->descriptor_layout_cache->cleanup();
        range_u64(i, 0, rs->framebuffer_count) {
            vkDestroyFramebuffer(rs->vk_gfx->device, rs->framebuffers[i], 0);
        }
        range_u64(i, 0, system_t::frame_overlap) {
            vkDestroyCommandPool(rs->vk_gfx->device, rs->frames[i].command_pool, 0);
            // vkDestroySemaphore(rs->vk_gfx->device, rs->frames[i].present_semaphore, 0);
            // vkDestroySemaphore(rs->vk_gfx->device, rs->frames[i].render_semaphore, 0);
            
            rs->frames[i].dynamic_descriptor_allocator->cleanup();
            rs->vk_gfx->destroy_data_buffer(rs->frames[i].indexed_indirect_storage_buffer);
        }

        rs->vk_gfx->destroy_data_buffer(rs->vertices);
        rs->vk_gfx->destroy_data_buffer(rs->indices);
        rs->vk_gfx->destroy_data_buffer(rs->skinned_vertices);
        rs->vk_gfx->destroy_data_buffer(rs->skinned_indices);
        rs->vk_gfx->destroy_data_buffer(rs->job_storage_buffers[0]);
        rs->vk_gfx->destroy_data_buffer(rs->job_storage_buffers[1]);
        rs->vk_gfx->destroy_data_buffer(rs->material_storage_buffer);
        rs->vk_gfx->destroy_data_buffer(rs->instance_storage_buffer);
        rs->vk_gfx->destroy_data_buffer(rs->animation_storage_buffer);
        rs->vk_gfx->destroy_data_buffer(rs->environment_storage_buffer);
        rs->permanent_descriptor_allocator->cleanup();
    }

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
        rs->render_jobs[0] = (render_job_t*)arena_alloc(&rs->arena, 10000);
        rs->render_jobs[1] = (render_job_t*)arena_alloc(&rs->arena, 10000);
        rs->render_jobs[2] = (render_job_t*)arena_alloc(&rs->arena, 10000);

        rs->permanent_descriptor_allocator = arena_alloc_ctor<gfx::vul::descriptor_allocator_t>(&rs->arena, 1, state.device); 
        range_u64(i, 0, array_count(rs->frames)) {
            rs->frames[i].dynamic_descriptor_allocator = arena_alloc_ctor<gfx::vul::descriptor_allocator_t>(&rs->arena, 1, state.device);
        }
        rs->descriptor_layout_cache = arena_alloc_ctor<gfx::vul::descriptor_layout_cache_t>(&rs->arena, 1, state.device); 

        rs->texture_cache.add(state.null_texture, "null");

        rs->postprocess_params.data[0] = 2.0f; // tonemap
        rs->postprocess_params.data[1] = 1.0f; // exposure
        rs->postprocess_params.data[2] = 1.0f; //contrast
        rs->postprocess_params.data[3] = 1.0f; // gamma

        const i32 w = state.swap_chain_extent.width;
        const i32 h = state.swap_chain_extent.height;
        {
            const f32 aspect = (f32)w / (f32)h;

            rs->projection = glm::perspective(45.0f, aspect, 0.3f, 1000.0f);
            rs->projection[1][1] *= -1.0f;
        }

        state.create_storage_buffer(&rs->probe_storage_buffer);
        state.create_storage_buffer(&rs->light_probe_settings_buffer);
        lighting::set_probes(*rs->vk_gfx, &rs->light_probes, 0, &rs->probe_storage_buffer.pool[0]);
        lighting::init_textures(*rs->vk_gfx, &rs->light_probes);
        rs->light_probe_settings_buffer.pool[0] = rs->light_probes.settings;
        

        create_render_pass(rs, state.device, state.swap_chain_image_format);
        create_framebuffers(
            rs, &rs->arena, state.device, 
            state.swap_chain_image_views.data(), state.swap_chain_image_views.size(),
            state.depth_stencil_texture.image_view, state.swap_chain_extent.width, state.swap_chain_extent.height
        );

        state.create_texture(&rs->frame_images[0].texture, w, h, 4, 0, 0, sizeof(float)*2);
        state.create_texture(&rs->frame_images[1].texture, w, h, 4, 0, 0, sizeof(float)*2);
        state.create_texture(&rs->frame_images[2].texture, w, h, 4, 0, 0, sizeof(float)*2);
        state.create_texture(&rs->frame_images[3].texture, w, h, 4, 0, 0, sizeof(float)*2);

        rs->frame_images[0].texture.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        rs->frame_images[1].texture.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        rs->frame_images[6].texture.format = VK_FORMAT_R8G8B8A8_UNORM;
        // rs->frame_images[0].texture.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        state.load_texture_sampler(&rs->frame_images[0].texture);
        state.load_texture_sampler(&rs->frame_images[1].texture);
        state.load_texture_sampler(&rs->frame_images[2].texture);
        state.load_texture_sampler(&rs->frame_images[3].texture);

        state.create_depth_stencil_image(&rs->frame_images[4].texture, w, h);
        state.create_depth_stencil_image(&rs->frame_images[5].texture, w, h);

        {
            arena_t ta = rs->arena; // temp arena
            u32* pixels = arena_alloc<u32>(&ta, w * h);
            range_u64(x, 0, w) {
                range_u64(y, 0, h) {
                    pixels[y*w+x] = (u32)utl::rng::random_s::rand();
                }
            }
            state.create_texture(&rs->frame_images[6].texture, w, h, 4, 0, (u8*)pixels);
            state.load_texture_sampler(&rs->frame_images[6].texture, true);
        }

        state.create_storage_buffer(&rs->job_storage_buffers[0]);
        state.create_storage_buffer(&rs->job_storage_buffers[1]);
        state.create_storage_buffer(&rs->job_storage_buffers[2]);
        state.create_storage_buffer(&rs->material_storage_buffer);
        state.create_storage_buffer(&rs->environment_storage_buffer);
        state.create_storage_buffer(&rs->animation_storage_buffer);
        state.create_storage_buffer(&rs->instance_storage_buffer);

       
        rs->environment_storage_buffer.pool[0].fog_color = v4f{0.5f,0.6f,0.7f,0.0f};
        rs->environment_storage_buffer.pool[0].sun.direction = v4f{glm::normalize(v3f{1.f,2.f,3.f}),0.0f};

        rs->rt_cache = arena_alloc_ctor<rt_cache_t>(&rs->arena, 1, state);

        range_u64(i, 0, array_count(rs->frames)) {
            state.create_storage_buffer(&rs->frames[i].indexed_indirect_storage_buffer, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
            
            VkBuffer buffers[]{
                state.sporadic_uniform_buffer.buffer,
                rs->job_storage_buffer().buffer,
                rs->instance_storage_buffer.buffer,
                rs->frames[i].indexed_indirect_storage_buffer.buffer,
                rs->material_storage_buffer.buffer,
                rs->environment_storage_buffer.buffer,
                rs->probe_storage_buffer.buffer,
                rs->light_probe_settings_buffer.buffer,
            };
            VkBuffer rt_buffers[]{
                state.sporadic_uniform_buffer.buffer,
                rs->job_storage_buffer().buffer,
                rs->instance_storage_buffer.buffer,
                rs->frames[i].indexed_indirect_storage_buffer.buffer,
                rs->material_storage_buffer.buffer,
                rs->environment_storage_buffer.buffer,
                rs->vertices.buffer,
                rs->indices.buffer,
            };
            {
                auto builder = gfx::vul::descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->frames[i].dynamic_descriptor_allocator);
                rs->frames[i].rt_compute_pass.build_buffer_sets(builder, rt_buffers);
                rs->frames[i].rt_compute_pass.bind_images(builder, rs->texture_cache, &rs->frame_images[6].texture);
                rs->frames[i].rt_compute_pass.build_layout(state.device);
                rs->frames[i].rt_compute_pass.build_descriptors(
                    state,
                    rs->texture_cache,
                    &rs->rt_cache->mesh_data_buffer,
                    &rs->probe_storage_buffer,
                    &rs->light_probe_settings_buffer,
                    gfx::vul::descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->frames[i].dynamic_descriptor_allocator),
                    // &rs->frame_images[0].texture
                    &rs->light_probes.irradiance_texture,
                    &rs->light_probes.visibility_texture
                );
            }
            {
                auto builder = gfx::vul::descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->frames[i].dynamic_descriptor_allocator);
                rs->frames[i].mesh_pass.build_buffer_sets(builder, buffers);
                rs->frames[i].mesh_pass.bind_images(builder, rs->texture_cache, &rs->light_probes.irradiance_texture, &rs->light_probes.visibility_texture);
                rs->frames[i].mesh_pass.build_layout(state.device);
            }
            {
                auto builder = gfx::vul::descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->frames[i].dynamic_descriptor_allocator);
                state.create_storage_buffer(&rs->frames[i].postprocess_pass.parameter_buffer);
                // rs->frames[i].postprocess_pass.build_buffer_sets(builder);
                rs->frames[i].postprocess_pass.bind_images(builder, rs->texture_cache["null"]);
                rs->frames[i].postprocess_pass.build_layout(state);
            }
        }

        range_u64(i, 0, array_count(rs->frames)) {
            VkBuffer buffers[]{
                state.sporadic_uniform_buffer.buffer,
                rs->job_storage_buffer().buffer,
                rs->material_storage_buffer.buffer,
                rs->environment_storage_buffer.buffer,
                rs->animation_storage_buffer.buffer
            };
            auto builder = gfx::vul::descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->frames[i].dynamic_descriptor_allocator);
            rs->frames[i].anim_pass.build_buffer_sets(builder, buffers);

            // rs->frames[i].anim_pass.bind_images(builder, rs->texture_cache);
            rs->frames[i].anim_pass.bind_images(builder, &state.null_texture);
            rs->frames[i].anim_pass.build_layout(state.device);
        }

        VkCommandPoolCreateInfo command_pool_info = gfx::vul::utl::command_pool_create_info();
        command_pool_info.queueFamilyIndex = state.graphics_index;
        command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        for (int i = 0; i < system_t::frame_overlap; i++) {
            VK_OK(vkCreateCommandPool(state.device, &command_pool_info, nullptr, &rs->frames[i].command_pool));

            //allocate the default command buffer that we will use for rendering
            auto cmdAllocInfo = gfx::vul::utl::command_buffer_allocate_info(rs->frames[i].command_pool, 1);

            VK_OK(vkAllocateCommandBuffers(state.device, &cmdAllocInfo, &rs->frames[i].main_command_buffer));
        }

        state.create_vertex_buffer(&rs->vertices);
        state.create_index_buffer(&rs->indices);
        state.create_vertex_buffer(&rs->skinned_vertices);
        state.create_index_buffer(&rs->skinned_indices);
    
       // lighting::set_probes(*rs->vk_gfx, &rs->light_probes, &rs->arena);

        rs->rt_cache->init(state, rs->frames[0].rt_compute_pass.descriptor_set_layouts[0]);

        return rs;
    }

    inline void
    begin_frame(system_t* rs) {
        rs->stats.reset();
        rs->render_job_count = 0;
        rs->frame_count++;
        rs->get_frame_data().dynamic_descriptor_allocator->reset_pools();        
        rs->job_storage_buffer().pool.clear();
        rs->get_frame_data().indexed_indirect_storage_buffer.pool.clear();

        rs->get_frame_data().rt_compute_pass.instance_count = 0;

        arena_clear(&rs->frame_arena);
    }

    void
    memory_barriers(system_t* rs, VkCommandBuffer command_buffer) {
        rs->get_frame_data().indexed_indirect_storage_buffer.insert_memory_barrier(command_buffer);
        rs->vertices.insert_memory_barrier(command_buffer);
        rs->indices.insert_memory_barrier(command_buffer);
        rs->material_storage_buffer.insert_memory_barrier(command_buffer);
        rs->environment_storage_buffer.insert_memory_barrier(command_buffer);
        rs->instance_storage_buffer.insert_memory_barrier(command_buffer);
        rs->animation_storage_buffer.insert_memory_barrier(command_buffer);
        rs->job_storage_buffer().insert_memory_barrier(command_buffer);
    }

    inline void
    submit_job(
        system_t* rs,
        u64 mesh_id,
        u32 mat_id, // todo(zack): remove this
        m44 transform,
        v4f bounds,
        u32 instance_count = 1,
        u32 instance_offset = 0
    ) {
        TIMED_FUNCTION;

        auto* job = rs->render_jobs[rs->frame_count%rs->frame_overlap] + rs->render_job_count++;
        auto* instance_buffer = &rs->instance_storage_buffer.pool[0];
        job->meshes = &rs->mesh_cache.get(mesh_id);
        job->material = mat_id;
        job->instance_count = instance_count;

        for (size_t i = 0; i < job->meshes->count; i++) {
            for (size_t j = 0; j < instance_count && j < 1'000; j++) { // @hardcoded limit for instancing
                rs->get_frame_data().rt_compute_pass.add_to_tlas(
                    *rs->vk_gfx,
                    *rs->rt_cache,
                    job->meshes->meshes[i].blas,
                    instance_count == 1 ? transform : transform * instance_buffer[instance_offset + j]
                );
            }


            gfx::indirect_indexed_draw_t* draw_cmd = rs->get_frame_data().indexed_indirect_storage_buffer.pool.allocate(1);
            draw_cmd->index_count = job->meshes->meshes[i].index_count;
            draw_cmd->instance_count = std::max(instance_count, 1ui32);
            draw_cmd->first_index = job->meshes->meshes[i].index_start;
            draw_cmd->vertex_offset = job->meshes->meshes[i].vertex_start;
            draw_cmd->first_instance = 0;
            draw_cmd->object_id = (u32)rs->render_job_count - 1;
            draw_cmd->albedo_id = u32(job->meshes->meshes[i].material.albedo_id) % array_count(rs->texture_cache.textures);
            draw_cmd->normal_id = u32(job->meshes->meshes[i].material.normal_id) % array_count(rs->texture_cache.textures);
        }
        
        const auto* mat = rs->materials[mat_id];
        
        auto* gpu_job = rs->job_storage_buffer().pool.allocate(1);
        gpu_job->model = transform;
        gpu_job->mat_id = mat_id;
        gpu_job->bounds = bounds;
        // gpu_job->padding[0] = mat->padding[0];
        // gpu_job->padding[1] = mat->padding[1];
        gpu_job->padding[2] = instance_offset; // instance offset

        rs->total_instance_count += instance_count != 1 ? instance_count + instance_offset : 0;
    }

    inline void 
    draw_mesh(
        system_t* rs, 
        VkCommandBuffer command_buffer,
        const gfx::mesh_list_t* meshes
    ) {
        range_u64(j, 0, meshes->count) {
            if (meshes->meshes[j].index_count) {
                rs->stats.triangle_count += meshes->meshes[j].index_count/3;
                vkCmdDrawIndexed(command_buffer,
                    meshes->meshes[j].index_count,
                    1, 
                    meshes->meshes[j].index_start,
                    meshes->meshes[j].vertex_start,
                    0           
                );
            } else {
                rs->stats.triangle_count += meshes->meshes[j].vertex_count/3;
                vkCmdDraw(command_buffer,
                    meshes->meshes[j].vertex_count,
                    meshes->meshes[j].instance_count,
                    meshes->meshes[j].vertex_start,
                    0
                );
            }
        }
    }

    inline void
    draw_mesh_indirect(
        system_t* rs, 
        VkCommandBuffer command_buffer,
        const gfx::mesh_list_t* meshes, u64 i
    ) {
        range_u64(j, 0, meshes->count) {
            if (meshes->meshes[j].index_count) {
                rs->stats.triangle_count += meshes->meshes[j].index_count/3;
                vkCmdDrawIndexed(command_buffer,
                    meshes->meshes[j].index_count,
                    1, 
                    meshes->meshes[j].index_start,
                    meshes->meshes[j].vertex_start,
                    safe_truncate_u64(i)
                );
            } else {
                rs->stats.triangle_count += meshes->meshes[j].vertex_count/3;
                vkCmdDraw(command_buffer,
                    meshes->meshes[j].vertex_count,
                    meshes->meshes[j].instance_count,
                    meshes->meshes[j].vertex_start,
                    safe_truncate_u64(i)
                );
            }
        }
    }

    inline void build_shader_commands(
        system_t* rs,
        VkCommandBuffer command_buffer
    ) {
        TIMED_FUNCTION;
        auto& ext = rs->vk_gfx->ext;
        auto& khr = rs->vk_gfx->khr;
        
        VkPipelineLayout last_layout = VK_NULL_HANDLE;
        const VkShaderEXT* last_shader = VK_NULL_HANDLE;

        if (rs->render_job_count==0) return;
        render_job_t* job = rs->render_jobs[rs->frame_count%rs->frame_overlap];

        const auto* material = rs->materials[job->material];

        struct pc_t {
            m44 v;
            m44 p;
        } constants;
        constants.v = rs->view;
        constants.p = rs->projection;
        
        vkCmdPushConstants(command_buffer, material->pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(constants), &constants
        );

        if (material->pipeline_layout != last_layout) {
            using namespace gfx::vul;
            
            mesh_pass_t& mesh_pass = rs->get_frame_data().mesh_pass;

            if (mesh_pass.object_descriptors == VK_NULL_HANDLE) 
            {
                VkBuffer buffers[]{
                    rs->vk_gfx->sporadic_uniform_buffer.buffer,
                    rs->job_storage_buffer().buffer,
                    rs->instance_storage_buffer.buffer,
                    rs->get_frame_data().indexed_indirect_storage_buffer.buffer,                    
                    rs->material_storage_buffer.buffer,
                    rs->environment_storage_buffer.buffer,
                    rs->probe_storage_buffer.buffer,
                    rs->light_probe_settings_buffer.buffer,
                };
                auto builder = descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->get_frame_data().dynamic_descriptor_allocator);
                mesh_pass.build_buffer_sets(builder, buffers);
                // mesh_pass.bind_images(builder, rs->texture_cache);
                mesh_pass.bind_images(builder, rs->texture_cache, &rs->light_probes.irradiance_texture, &rs->light_probes.visibility_texture);
            }

            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 0, 1, &mesh_pass.sporadic_descriptors, 0, nullptr);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 1, 1, &mesh_pass.object_descriptors, 0, nullptr);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 2, 1, &mesh_pass.material_descriptor, 0, nullptr);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 3, 1, &mesh_pass.enviornment_descriptor, 0, nullptr);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 4, 1, &mesh_pass.texture_descriptor, 0, nullptr);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 5, 1, &mesh_pass.light_probe_descriptor, 0, nullptr);
        }
        last_layout = material->pipeline_layout;

        if (last_shader != material->shaders[1]) {
            VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
            VkShaderEXT shaders[10];
            range_u64(j, 0, material->shader_count) {
                shaders[j] = material->shaders[j] ? *material->shaders[j] : VK_NULL_HANDLE;
            }
            ext.vkCmdBindShadersEXT(command_buffer, material->shader_count, stages, shaders);
            rs->stats.shader_count++;
        }
        last_shader = material->shaders[1];


        auto draw_count = rs->get_frame_data().indexed_indirect_storage_buffer.pool.count();
        vkCmdDrawIndexedIndirect(command_buffer, rs->get_frame_data().indexed_indirect_storage_buffer.buffer, 0, (u32)draw_count, sizeof(gfx::indirect_indexed_draw_t));
    }

    inline void build_shader_commands2(
        system_t* rs,
        VkCommandBuffer command_buffer
    ) {
        TIMED_FUNCTION;
        auto& ext = rs->vk_gfx->ext;
        auto& khr = rs->vk_gfx->khr;
        
        VkPipelineLayout last_layout = VK_NULL_HANDLE;
        const VkShaderEXT* last_shader = VK_NULL_HANDLE;

        for (size_t i = 0; i < rs->render_job_count; i++) {
            render_job_t* job = rs->render_jobs[rs->frame_count%rs->frame_overlap] + i;

            const auto* material = rs->materials[job->material];

            if (material->pipeline_layout != last_layout) {
                using namespace gfx::vul;
                
                mesh_pass_t& mesh_pass = rs->get_frame_data().mesh_pass;

                if (mesh_pass.object_descriptors == VK_NULL_HANDLE) {
                    auto builder = descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->get_frame_data().dynamic_descriptor_allocator);
                    VkBuffer buffers[]{
                        rs->vk_gfx->sporadic_uniform_buffer.buffer,
                        rs->job_storage_buffer().buffer,
                        rs->instance_storage_buffer.buffer,
                        rs->get_frame_data().indexed_indirect_storage_buffer.buffer,
                        rs->material_storage_buffer.buffer,
                        rs->environment_storage_buffer.buffer
                    };
                    mesh_pass.build_buffer_sets(builder, buffers);
                    // mesh_pass.bind_images(builder, rs->texture_cache);
                    mesh_pass.bind_images(builder, rs->texture_cache, &rs->light_probes.irradiance_texture, &rs->light_probes.visibility_texture);
                }

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 0, 1, &mesh_pass.sporadic_descriptors, 0, nullptr);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 1, 1, &mesh_pass.object_descriptors, 0, nullptr);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 2, 1, &mesh_pass.material_descriptor, 0, nullptr);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 3, 1, &mesh_pass.enviornment_descriptor, 0, nullptr);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pass.pipeline_layout, 4, 1, &mesh_pass.texture_descriptor, 0, nullptr);
            }
            last_layout = material->pipeline_layout;

            if (last_shader != material->shaders[1]) {
                VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
                VkShaderEXT shaders[10];
                range_u64(j, 0, material->shader_count) {
                    shaders[j] = material->shaders[j] ? *material->shaders[j] : VK_NULL_HANDLE;
                }
                ext.vkCmdBindShadersEXT(command_buffer, material->shader_count, stages, shaders);
                rs->stats.shader_count++;
            }
            last_shader = material->shaders[1];

            auto* meshes = job->meshes;
            struct pc_t {
                m44 vp;
                v4f cp;
                u32 albedo;
                u32 normal;
            } constants;
            constants.vp = rs->vp;
            constants.cp = v4f{rs->camera_pos, 0.0f};

            range_u64(j, 0, meshes->count) {
                constants.albedo = safe_truncate_u64(job->meshes->meshes[j].material.albedo_id != std::numeric_limits<u64>::max() ? job->meshes->meshes[j].material.albedo_id : 0);
                constants.normal = safe_truncate_u64(job->meshes->meshes[j].material.normal_id != std::numeric_limits<u64>::max() ? job->meshes->meshes[j].material.normal_id : 0);
                vkCmdPushConstants(command_buffer, material->pipeline_layout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                    0, sizeof(constants), &constants
                );
                
                if (meshes->meshes[j].index_count) {
                    rs->stats.triangle_count += meshes->meshes[j].index_count/3;
                    vkCmdDrawIndexed(command_buffer,
                        meshes->meshes[j].index_count,
                        job->instance_count, 
                        meshes->meshes[j].index_start,
                        meshes->meshes[j].vertex_start,
                        safe_truncate_u64(i)
                    );
                } else {
                    rs->stats.triangle_count += meshes->meshes[j].vertex_count/3;
                    vkCmdDraw(command_buffer,
                        meshes->meshes[j].vertex_count,
                        job->instance_count,
                        meshes->meshes[j].vertex_start,
                        safe_truncate_u64(i)
                    );
                }
            }
        }
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
            render_job_t* job = rs->render_jobs[rs->frame_count%rs->frame_overlap] + i;
            
            const material_node_t* mat = rs->materials[job->material];

            if (!last_material || (mat->pipeline != last_material->pipeline)) {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat->pipeline);

                assert(mat->pipeline_layout != VK_NULL_HANDLE);

                last_material = mat;
            }

            struct pc_t {
                m44 vp;
                v4f cp;
                u32 albedo;
                u32 normal;
            } constants;
            constants.vp = rs->vp;
            constants.cp = v4f{rs->camera_pos, 0.0f};
                            
            // gfx::vul::object_push_constants_t push_constants;
            // push_constants.material = *job->material;
            // push_constants.model    = job->transform;

            // vkCmdPushConstants(command_buffer, job->material->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_constants), &push_constants);

            loop_iota_u64(j, job->meshes->count) {
                if (job->meshes->meshes[j].index_count) {
                    constants.albedo = safe_truncate_u64(job->meshes->meshes[j].material.albedo_id != std::numeric_limits<u64>::max() ? job->meshes->meshes[j].material.albedo_id : 0);
                    constants.albedo = safe_truncate_u64(job->meshes->meshes[j].material.normal_id != std::numeric_limits<u64>::max() ? job->meshes->meshes[j].material.normal_id : 0);
                    vkCmdPushConstants(command_buffer, mat->pipeline_layout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                        0, sizeof(constants), &constants
                    );
                    vkCmdDrawIndexed(command_buffer,
                        job->meshes->meshes[j].index_count,
                        1, 
                        job->meshes->meshes[j].index_start,
                        job->meshes->meshes[j].vertex_start,
                        safe_truncate_u64(i)
                    );
                } else {
                    constants.albedo = safe_truncate_u64(job->meshes->meshes[j].material.albedo_id != std::numeric_limits<u64>::max() ? job->meshes->meshes[j].material.albedo_id : 0);
                    constants.albedo = safe_truncate_u64(job->meshes->meshes[j].material.normal_id != std::numeric_limits<u64>::max() ? job->meshes->meshes[j].material.normal_id : 0);
                    vkCmdPushConstants(command_buffer, mat->pipeline_layout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                        0, sizeof(constants), &constants
                    );
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
        if (name.empty()) return std::numeric_limits<u64>::max();
        auto id = utl::str_hash_find(rs->mesh_hash, name);
        if (id == utl::invalid_hash) {
            std::byte* file_data = utl::res::pack_file_get_file_by_name(rs->resource_file, name);
                        
            auto loaded_mesh = load_bin_mesh_data(
                &rs->arena,
                file_data, 
                &rs->vertices.pool,
                &rs->indices.pool
            );

            range_u64(m, 0, loaded_mesh.count) {
                u64 ids[2];
                u32 mask = rs->texture_cache.load_material(rs->arena, *rs->vk_gfx, loaded_mesh.meshes[m].material, ids);
                loaded_mesh.meshes[m].material.albedo_id = (mask&0x1) ? ids[0] : std::numeric_limits<u64>::max();
                loaded_mesh.meshes[m].material.normal_id = (mask&0x2) ? ids[1] : std::numeric_limits<u64>::max();
            }

            rs->rt_cache->build_blas(*rs->vk_gfx, loaded_mesh, rs->vertices, rs->indices);

            return add_mesh(rs, name, loaded_mesh);
        }
        return id;
    }

    inline u64
    safe_get_mesh_id(
        system_t* rs,
        std::string_view name
    ) {
        std::lock_guard lock{rs->ticket};
        u64 id = get_mesh_id(rs, name);
        return id;
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
        VkPipelineLayout pipeline_layout,
        const VkShaderEXT** shaders,
        u32 shader_count
    ) {
        material_node_t* material{0};
        for (u32 i = 0; i < rs->materials.size(); i++) {
            if (rs->materials[i]->name == name) {
                material = rs->materials[i];
                return i;
            }
        }
        if (!material) {
            zyy_info("rendering", "Creating material: {}", name);
            material = arena_alloc_ctor<material_node_t>(&rs->arena, 1, std::move(p_material), shaders, shader_count);
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
            zyy_error("vulkan", "failed to create render pass!");
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
                zyy_error("vulkan", "failed to create framebuffer!");
                std::terminate();
            }
        }
    }

    inline void
    draw_skybox(
        system_t* rs, 
        VkCommandBuffer command_buffer,
        std::string_view name,
        VkPipelineLayout layout,
        v3f view, v4f light_direction
    ) {
        TIMED_FUNCTION;
        auto& ext = rs->vk_gfx->ext;
        auto& khr = rs->vk_gfx->khr;

        const auto& sky_mesh_id = get_mesh_id(rs, "res/models/sphere.obj");
        const auto& sky_mesh = get_mesh(rs, "res/models/sphere.obj");

        auto viewport = gfx::vul::utl::viewport((f32)rs->width, (f32)rs->height, 0.0f, 1.0f);
        auto scissor = gfx::vul::utl::rect2D(rs->width, rs->height, 0, 0);

        ext.vkCmdSetViewportWithCountEXT(command_buffer, 1, &viewport);
        ext.vkCmdSetScissorWithCountEXT(command_buffer, 1, &scissor);
        ext.vkCmdSetCullModeEXT(command_buffer, VK_CULL_MODE_NONE);
        ext.vkCmdSetPrimitiveTopologyEXT(command_buffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        struct sky_push_constant_t {
            m44 vp;
            v4f directional_light;
        } sky_constants;

        sky_constants.vp = rs->projection * glm::lookAt(v3f{0.0f}, view, axis::up);
        sky_constants.directional_light = light_direction;

        vkCmdPushConstants(command_buffer, layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(sky_push_constant_t), &sky_constants
        );

        {
            local_persist VkVertexInputBindingDescription2EXT vertexInputBinding{};
            vertexInputBinding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
            vertexInputBinding.binding = 0;
            vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            vertexInputBinding.stride = sizeof(gfx::vertex_t);
            vertexInputBinding.divisor = 1;

            local_persist VkVertexInputAttributeDescription2EXT vertexAttributes[] = {
                { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::vertex_t, pos) },
                { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::vertex_t, nrm) },
                { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(gfx::vertex_t, col) },
                { VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(gfx::vertex_t, tex) }
            };

            ext.vkCmdSetVertexInputEXT(command_buffer, 1, &vertexInputBinding, array_count(vertexAttributes), vertexAttributes);
        }

        VkShaderEXT shaders[] = {
            *rs->shader_cache[assets::shaders::skybox_vert.filename],
            *rs->shader_cache[name]
        };
        
        VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
        ext.vkCmdBindShadersEXT(command_buffer, 2, stages, shaders);

        draw_mesh(rs, command_buffer, &sky_mesh);

    };

    inline void
    draw_postprocess(
        system_t* rs, 
        VkCommandBuffer command_buffer,
        std::string_view shader_name,
        gfx::vul::texture_2d_t* texture,
        pp_material_t parameters
    ) {
        TIMED_FUNCTION;
        auto& ext = rs->vk_gfx->ext;
        auto& khr = rs->vk_gfx->khr;

        auto viewport = gfx::vul::utl::viewport((f32)rs->width, (f32)rs->height, 0.0f, 1.0f);
        auto scissor = gfx::vul::utl::rect2D(rs->width, rs->height, 0, 0);

        ext.vkCmdSetViewportWithCountEXT(command_buffer, 1, &viewport);
        ext.vkCmdSetScissorWithCountEXT(command_buffer, 1, &scissor);
        ext.vkCmdSetCullModeEXT(command_buffer, VK_CULL_MODE_NONE);
        ext.vkCmdSetPrimitiveTopologyEXT(command_buffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        // todo bind texture
        auto& pp_pass = rs->get_frame_data().postprocess_pass;
        auto builder = gfx::vul::descriptor_builder_t::begin(rs->descriptor_layout_cache, rs->get_frame_data().dynamic_descriptor_allocator);
        // pp_pass.build_buffer_sets(builder);
        pp_pass.bind_images(builder, texture);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pp_pass.pipeline_layout, 0, 1, &pp_pass.texture_descriptor, 0, nullptr);
        // vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pp_pass.pipeline_layout, 1, 1, &pp_pass.parameters_descriptor, 0, nullptr);

        vkCmdPushConstants(command_buffer, pp_pass.pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(pp_material_t), &parameters
        );

        VkShaderEXT shaders[] = {
            *rs->shader_cache[assets::shaders::screen_vert.filename],
            *rs->shader_cache[shader_name]
        };
        
        VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
        ext.vkCmdBindShadersEXT(command_buffer, 2, stages, shaders);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
    };

    void
    update_probe_aabb(system_t* rs, const math::aabb_t<v3f>& aabb) {
        rs->light_probes.aabb = aabb;
        lighting::update_probe_positions(&rs->light_probes);
        rs->light_probe_settings_buffer.pool[0] = rs->light_probes.settings;
        lighting::destroy_textures(*rs->vk_gfx, &rs->light_probes);
        lighting::init_textures(*rs->vk_gfx, &rs->light_probes);
    }

    #include "raytrace_pass.hpp"

};



#endif 