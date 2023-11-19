#pragma once

#include "zyy_core.hpp"
#include "App/vk_state.hpp"
#include "App/Game/Rendering/lighting.hpp"

namespace rendering {

    using gpu_ptr_t = u64;

    struct gfx_entity_t {
        gpu_ptr_t vertex_start; 
        gpu_ptr_t index_start;
        gpu_ptr_t material;
        gpu_ptr_t transform;
        u32 instance_offset;
        u32 instance_count;
        u32 albedo;
    };

    // DEFINE_TYPED_ID(gfx_entity_id);
    using gfx_entity_id = u32;

    struct scene_t {
        gpu_ptr_t vertex_buffer;
        gpu_ptr_t index_buffer;
        gpu_ptr_t transform_buffer;
        gpu_ptr_t material_buffer;
        gpu_ptr_t instance_buffer;
        gpu_ptr_t indirect_buffer;
        gpu_ptr_t texture_cache;

        gpu_ptr_t entities;

        gpu_ptr_t environment;

        gpu_ptr_t point_lights;

        gpu_ptr_t ddgi;
    };

    #define MAX_SCENE_ENTITIES 1'000'000
    constexpr umm max_scene_vertex_count{10'000'000};
    constexpr umm max_scene_index_count{30'000'000};

    struct scene_context_t {
        gfx::vul::state_t& gfx;
        gfx::vul::uniform_buffer_t<scene_t>                                 scene_ubo;
        gfx::vul::vertex_buffer_t<gfx::vertex_t, max_scene_vertex_count>    vertices;
        gfx::vul::index_buffer_t<max_scene_index_count>                     indices;
        gfx::vul::storage_buffer_t<gfx_entity_t, MAX_SCENE_ENTITIES>        entities;
        gfx::vul::storage_buffer_t<m44, MAX_SCENE_ENTITIES*2>               transform_storage_buffer;
        gfx::vul::storage_buffer_t<m44, 2'000'000>                          instance_storage_buffer;
        gfx::vul::storage_buffer_t<v4f, 2'000'000>                          instance_color_storage_buffer;
        gfx::vul::storage_buffer_t<gfx::material_t, 100>                    material_storage_buffer;
        gfx::vul::storage_buffer_t<lighting::point_light_t, 512>            point_light_storage_buffer;
        gfx::vul::storage_buffer_t<gfx::indirect_indexed_draw_t, MAX_SCENE_ENTITIES> indexed_indirect_storage_buffer{};
    
        u32 pingpong{0};
        u32 entity_count{0};

        explicit scene_context_t(gfx::vul::state_t& gfx_) : gfx{gfx_}
        {
            gfx.create_vertex_buffer(&vertices);
            gfx.create_index_buffer(&indices);
            gfx.create_uniform_buffer(&scene_ubo);
            gfx.create_storage_buffer(&entities);
            gfx.create_storage_buffer(&transform_storage_buffer);
            gfx.create_storage_buffer(&material_storage_buffer);
            gfx.create_storage_buffer(&indexed_indirect_storage_buffer);
            gfx.create_storage_buffer(&instance_storage_buffer);
            gfx.create_storage_buffer(&instance_color_storage_buffer);

            utl::memzero(&entities.pool[0], entities.size);

            get_scene().vertex_buffer = gfx.get_buffer_device_address(vertices.buffer);
            get_scene().index_buffer = gfx.get_buffer_device_address(indices.buffer);
            get_scene().entities = gfx.get_buffer_device_address(entities.buffer);
            get_scene().transform_buffer = gfx.get_buffer_device_address(transform_storage_buffer.buffer);
            get_scene().material_buffer = gfx.get_buffer_device_address(material_storage_buffer.buffer);
            get_scene().indirect_buffer = gfx.get_buffer_device_address(indexed_indirect_storage_buffer.buffer);
        }

        ~scene_context_t() {
            gfx.destroy_data_buffer(vertices);
            gfx.destroy_data_buffer(indices);
            gfx.destroy_data_buffer(scene_ubo);
            gfx.destroy_data_buffer(entities);
            gfx.destroy_data_buffer(transform_storage_buffer);
            gfx.destroy_data_buffer(material_storage_buffer);
            gfx.destroy_data_buffer(indexed_indirect_storage_buffer);
            gfx.destroy_data_buffer(instance_storage_buffer);
            gfx.destroy_data_buffer(instance_color_storage_buffer);
        }

        void begin_frame() {
            pingpong = !pingpong;
        }

        scene_t& get_scene() {
            return *scene_ubo.data;
        }

        gfx_entity_t& get_entity(gfx_entity_id id) {
            return entities.pool[id];
        }

        gpu_ptr_t scene_address() {
            return gfx.get_buffer_device_address(scene_ubo.buffer);
        }

        gpu_ptr_t entity_address() {
            return gfx.get_buffer_device_address(entities.buffer);
        }

        gfx_entity_id register_entity() {
            // range_u64(i, 0, entity_count+1) {
            //     if (get_entity(i).vertex_start==0) {
            //         entity_count++;
            //         return i;
            //     }
            // }
            entities.pool.allocate(1);
            return entity_count++;
        }

        void release_entity(gfx_entity_id id) {
            get_entity(id).vertex_start = 0;
        }

        void set_entity_transform(gfx_entity_id id, const m44& transform) {
            transform_storage_buffer.pool[id*2+pingpong] = transform;
        }

    };

}
