#pragma once

struct acceleration_structure_t {
    VkAccelerationStructureKHR  handle;
    u64                         device_address;
    gfx::vul::gpu_buffer_t      buffer;

    gfx::vul::gpu_buffer_t      vertex_buffer;
    gfx::vul::gpu_buffer_t      index_buffer;
    gfx::vul::gpu_buffer_t      transform_buffer;
};

struct rt_cache_t {
    struct rt_data_t {
        i32      texture_id;   
        uint64_t vertex_ptr;   
        uint64_t index_ptr;    
        uint64_t material_id;  
        uint64_t entity_id;
    };

    // rt_data_t* rt_data;

    explicit rt_cache_t(gfx::vul::state_t& gfx) {
        // gfx.create_data_buffer(sizeof(rt_data_t) * array_count(blas), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &mesh_data_buffer);
        // void* gpu_ptr = 0;
        // gfx.map_data_buffer(&mesh_data_buffer, gpu_ptr);
        // rt_data = (rt_data_t*)gpu_ptr;
    }
    
    void init(gfx::vul::state_t& gfx, VkDescriptorSetLayout& descriptor_set_layout) {
        build_pipeline(gfx, descriptor_set_layout);
        build_shader_table(gfx);
    }

    acceleration_structure_t blas[512<<2];
    umm blas_count{0};

    VkRayTracingShaderGroupCreateInfoKHR   shader_groups[32];
    umm                                    shader_group_count{0};

    VkPipelineShaderStageCreateInfo        shader_stages[32];
    umm                                    shader_stage_count{0};

    VkPipeline pipeline{};
    VkPipelineLayout pipeline_layout{};
    
    gfx::vul::gpu_buffer_t raygen_shader_binding_table;
    gfx::vul::gpu_buffer_t miss_shader_binding_table;
    gfx::vul::gpu_buffer_t hit_shader_binding_table;

    // gfx::vul::gpu_buffer_t mesh_data_buffer;

    struct push_constants_t {
        m44 random_rotation{1.0f};
        u64 scene;
        u32 frame{0};
        u32 super_sample{0};
        // u32 sc
    };
    push_constants_t constants;
   
    void add_miss_shader(gfx::vul::state_t& gfx, std::string_view name) {
        shader_stages[shader_stage_count++] = gfx.load_shader(name, VK_SHADER_STAGE_MISS_BIT_KHR);
		VkRayTracingShaderGroupCreateInfoKHR raygen_group_ci{};
		raygen_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		raygen_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		raygen_group_ci.generalShader      = static_cast<u32>(shader_stage_count) - 1;
		raygen_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups[shader_group_count++] = (raygen_group_ci);
    }

    void add_gen_shader(gfx::vul::state_t& gfx, std::string_view name) {
        shader_stages[shader_stage_count++] = gfx.load_shader(name, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
		VkRayTracingShaderGroupCreateInfoKHR raygen_group_ci{};
		raygen_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		raygen_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		raygen_group_ci.generalShader      = static_cast<u32>(shader_stage_count) - 1;
		raygen_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups[shader_group_count++] = (raygen_group_ci);
    }

    void add_closest_hit_shader(gfx::vul::state_t& gfx, std::string_view name) {
        shader_stages[shader_stage_count++] = gfx.load_shader(name, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        shader_stages[shader_stage_count++] = gfx.load_shader("res/shaders/bin/anyhit.rahit.spv", VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
		VkRayTracingShaderGroupCreateInfoKHR raygen_group_ci{};
		raygen_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		raygen_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		raygen_group_ci.generalShader      = VK_SHADER_UNUSED_KHR;
		raygen_group_ci.closestHitShader   = static_cast<u32>(shader_stage_count) - 2;
		raygen_group_ci.anyHitShader       = static_cast<u32>(shader_stage_count) - 1;
		raygen_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
		shader_groups[shader_group_count++] = (raygen_group_ci);
    }

    // void add_any_hit_shader(gfx::vul::state_t& gfx, std::string_view name) {
    //     shader_stages[shader_stage_count++] = gfx.load_shader(name, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	// 	VkRayTracingShaderGroupCreateInfoKHR raygen_group_ci{};
	// 	raygen_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	// 	raygen_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	// 	raygen_group_ci.generalShader      = VK_SHADER_UNUSED_KHR;
	// 	raygen_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
	// 	raygen_group_ci.anyHitShader       = static_cast<u32>(shader_stage_count) - 1;
	// 	raygen_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
	// 	shader_groups[shader_group_count++] = (raygen_group_ci);
    // }

    void build_pipeline(gfx::vul::state_t& gfx, VkDescriptorSetLayout& descriptor_set_layout) {
        // VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
        // pipeline_layout_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // pipeline_layout_create_info.setLayoutCount = 1;
        // pipeline_layout_create_info.pSetLayouts    = &descriptor_set_layout;
        // pipeline_layout_create_info.

        pipeline_layout = gfx::vul::create_pipeline_layout(gfx.device, &descriptor_set_layout, 1, sizeof(constants), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

        // VK_OK(vkCreatePipelineLayout(gfx.device, &pipeline_layout_create_info, nullptr, &pipeline_layout));

        /*
            Setup ray tracing shader groups
            Each shader group points at the corresponding shader in the pipeline
        */
        
        // add_gen_shader(gfx, "res/shaders/bin/raygen.rgen.spv");
        add_gen_shader(gfx, "res/shaders/bin/probe_raygen.rgen.spv");
        add_miss_shader(gfx, "res/shaders/bin/miss.rmiss.spv");
        add_miss_shader(gfx, "res/shaders/bin/shadow.rmiss.spv");
        add_closest_hit_shader(gfx, "res/shaders/bin/closesthit.rchit.spv");

        /*
            Create the ray tracing pipeline
        */
        VkRayTracingPipelineCreateInfoKHR raytracing_pipeline_create_info{};
        raytracing_pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        raytracing_pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stage_count);
        raytracing_pipeline_create_info.pStages                      = shader_stages;
        raytracing_pipeline_create_info.groupCount                   = static_cast<uint32_t>(shader_group_count);
        raytracing_pipeline_create_info.pGroups                      = shader_groups;
        raytracing_pipeline_create_info.maxPipelineRayRecursionDepth = 8;
        raytracing_pipeline_create_info.layout                       = pipeline_layout;
        VK_OK(gfx.khr.vkCreateRayTracingPipelinesKHR(gfx.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracing_pipeline_create_info, nullptr, &pipeline));
    }

    void build_shader_table(gfx::vul::state_t& gfx) {
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties{};
        ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	    VkPhysicalDeviceProperties2 device_properties{};
	    device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	    device_properties.pNext = &ray_tracing_pipeline_properties;
	    vkGetPhysicalDeviceProperties2(gfx.gpu_device, &device_properties);

        u32 handle_size = ray_tracing_pipeline_properties.shaderGroupHandleSize;
        u32 handle_size_aligned = align_2n(handle_size, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);
        u32 handle_alignment = ray_tracing_pipeline_properties.shaderGroupHandleAlignment;
        u32 group_count = safe_truncate_u64(shader_group_count);
        u32 sbt_size = group_count * handle_size_aligned;
        VkBufferUsageFlags sbt_buffer_usage_flags =  VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        gfx.create_data_buffer(handle_size, sbt_buffer_usage_flags, &raygen_shader_binding_table);
        gfx.create_data_buffer(handle_size_aligned*2, sbt_buffer_usage_flags, &miss_shader_binding_table);
        gfx.create_data_buffer(handle_size, sbt_buffer_usage_flags, &hit_shader_binding_table);

        std::vector<uint8_t> shader_handle_storage(sbt_size);
	    VK_OK(gfx.khr.vkGetRayTracingShaderGroupHandlesKHR(gfx.device, pipeline, 0, group_count, sbt_size, shader_handle_storage.data()));

        gfx.fill_data_buffer(&raygen_shader_binding_table, shader_handle_storage.data(), handle_size);
        gfx.fill_data_buffer(&miss_shader_binding_table, shader_handle_storage.data() + handle_size_aligned, handle_size_aligned * 2);
        gfx.fill_data_buffer(&hit_shader_binding_table, shader_handle_storage.data() + handle_size_aligned * 3, handle_size * 1);
    }

    template <umm VSize, umm ISize>
    void build_blas(
        gfx::vul::state_t& gfx, 
        gfx::mesh_list_t& mesh, 
        gfx::vul::vertex_buffer_t<gfx::vertex_t, VSize>& vertex_buffer,
        gfx::vul::index_buffer_t<ISize>& index_buffer
        // gfx::vul::gpu_buffer_t* entity_buffer,
        // size_t entity_size
    ) {
        TIMED_FUNCTION;
        // zyy_warn(__FUNCTION__, "Building BLAS");
        const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        range_u64(i, 0, mesh.count) {
            auto& m = mesh.meshes[i];
            if (m.blas!=0) continue;

            zyy_warn(__FUNCTION__, "Building BLAS Mesh: vs {}, vc {}, is {}, ic {}", m.vertex_start, m.vertex_count, m.index_start, m.index_count);
            auto b = blas_count++;
            auto& c_blas = blas[b];
            assert(blas_count < array_count(blas));

            auto vertex_buffer_size = sizeof(gfx::vertex_t) * m.vertex_count;
            auto index_buffer_size = sizeof(u32) * m.index_count;


            VkDeviceOrHostAddressConstKHR vertex_buffer_device_address{};
            VkDeviceOrHostAddressConstKHR index_buffer_device_address{};

            // rt_data[b].texture_id = u32(m.material.albedo_id);
            // rt_data[b].texture_id = u32(m.material.albedo_id);
            // rt_data[b].vertex_ptr = 
            vertex_buffer_device_address.deviceAddress = gfx.get_buffer_device_address(vertex_buffer.buffer) + sizeof(gfx::vertex_t) * m.vertex_start;
            // rt_data[b].index_ptr = 
            index_buffer_device_address.deviceAddress = gfx.get_buffer_device_address(index_buffer.buffer) + sizeof(u32) * m.index_start;

            VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
            acceleration_structure_geometry.sType                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            acceleration_structure_geometry.geometryType                     = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            acceleration_structure_geometry.flags                            = 0;
            acceleration_structure_geometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            acceleration_structure_geometry.geometry.triangles.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
            acceleration_structure_geometry.geometry.triangles.vertexData    = vertex_buffer_device_address;
            acceleration_structure_geometry.geometry.triangles.maxVertex     = m.vertex_count;
            acceleration_structure_geometry.geometry.triangles.vertexStride  = sizeof(gfx::vertex_t);
            acceleration_structure_geometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
            acceleration_structure_geometry.geometry.triangles.indexData     = index_buffer_device_address;

            acceleration_structure_geometry.geometry.triangles.transformData = {};

            const uint32_t primitive_count = m.index_count/3;

            VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
            acceleration_structure_build_geometry_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            acceleration_structure_build_geometry_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            acceleration_structure_build_geometry_info.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            acceleration_structure_build_geometry_info.geometryCount = 1;
            acceleration_structure_build_geometry_info.pGeometries   = &acceleration_structure_geometry;


            VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
            acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
            gfx.khr.vkGetAccelerationStructureBuildSizesKHR(
                gfx.device,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &acceleration_structure_build_geometry_info,
                &primitive_count,
                &acceleration_structure_build_sizes_info);

            gfx.create_data_buffer(
                acceleration_structure_build_sizes_info.accelerationStructureSize,
	            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, &c_blas.buffer);

            VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
            acceleration_structure_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            acceleration_structure_create_info.buffer = c_blas.buffer.buffer;
            acceleration_structure_create_info.size   = acceleration_structure_build_sizes_info.accelerationStructureSize;
            acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            
            VK_OK(gfx.khr.vkCreateAccelerationStructureKHR(gfx.device, &acceleration_structure_create_info, nullptr, &c_blas.handle));

            auto scratch_buffer = gfx.create_scratch_buffer(acceleration_structure_build_sizes_info.buildScratchSize);

            VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
            acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            acceleration_build_geometry_info.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            acceleration_build_geometry_info.dstAccelerationStructure  = c_blas.handle;
            acceleration_build_geometry_info.geometryCount             = 1;
            acceleration_build_geometry_info.pGeometries               = &acceleration_structure_geometry;
            acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.device_address;

            VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
            acceleration_structure_build_range_info.primitiveCount                                           = primitive_count;
            acceleration_structure_build_range_info.primitiveOffset                                          = 0;
            acceleration_structure_build_range_info.firstVertex                                              = 0;
            acceleration_structure_build_range_info.transformOffset                                          = 0;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR *> acceleration_build_structure_range_infos = {&acceleration_structure_build_range_info};

            {
                // Build the acceleration structure on the device via a one-time command buffer submission
                // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
                gfx::vul::quick_cmd_raii_t c{&gfx};
                VkCommandBuffer command_buffer = c.c();

                gfx.khr.vkCmdBuildAccelerationStructuresKHR(
                    command_buffer,
                    1,
                    &acceleration_build_geometry_info,
                    acceleration_build_structure_range_infos.data());
            }

            gfx.destroy_scratch_buffer(scratch_buffer);

            // Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
            VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
            acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            acceleration_device_address_info.accelerationStructure = c_blas.handle;
            c_blas.device_address     = gfx.khr.vkGetAccelerationStructureDeviceAddressKHR(gfx.device, &acceleration_device_address_info);

            m.blas = safe_truncate_u64(b);
        }
        // zyy_warn(__FUNCTION__, "Finished BLAS build");
    }
};

struct rt_compute_pass_t {
    VkPipelineLayout pipeline_layout[2]{};
    
    VkDevice device;

    acceleration_structure_t tlas;
    VkAccelerationStructureInstanceKHR tlas_instances[100'000];
    VkAccelerationStructureBuildGeometryInfoKHR tlas_geometry[100'000];
    
    u32 instance_count{0};

    gfx::vul::gpu_buffer_t instance_buffer;
    gfx::vul::gpu_buffer_t object_data_buffer;


    VkDescriptorSet descriptor_sets[3]{0,0,0};
    VkDescriptorSetLayout descriptor_set_layouts[3]{0,0,0};

    void build_descriptors(
        gfx::vul::state_t& gfx, 
        texture_cache_t& texture_cache,
        gfx::vul::gpu_buffer_t* rt_mesh_data,
        gfx::vul::gpu_buffer_t* probe_data,
        gfx::vul::gpu_buffer_t* probe_settings,
        gfx::vul::gpu_buffer_t* probe_rays,
        gfx::vul::gpu_buffer_t* environment,
        gfx::vul::gpu_buffer_t* point_lights,
        gfx::vul::descriptor_builder_t&& builder, 
        gfx::vul::texture_2d_t* irradiance_texture,
        gfx::vul::texture_2d_t* visibility_texture,
        gfx::vul::texture_2d_t* filter_texture
    ) {
        VkDescriptorBufferInfo buffer_info[9];
        u32 b = 0;
        buffer_info[b].buffer = tlas.buffer.buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = rt_mesh_data->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = probe_data->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = probe_settings->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = probe_rays->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = environment->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = point_lights->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
        descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptor_acceleration_structure_info.accelerationStructureCount = 1;
        descriptor_acceleration_structure_info.pAccelerationStructures    = &tlas.handle;

        VkDescriptorImageInfo vdii[4096];
        VkDescriptorImageInfo ovdii[3];

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

        ovdii[0].imageLayout = irradiance_texture->image_layout;
        ovdii[0].imageView = irradiance_texture->image_view;
        ovdii[0].sampler = irradiance_texture->sampler;

        ovdii[1].imageLayout = visibility_texture->image_layout;
        ovdii[1].imageView = visibility_texture->image_view;
        ovdii[1].sampler = visibility_texture->sampler;

        ovdii[2].imageLayout = filter_texture->image_layout;
        ovdii[2].imageView = filter_texture->image_view;
        ovdii[2].sampler = filter_texture->sampler;


        builder
            .bind_buffer(0, buffer_info + 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, &descriptor_acceleration_structure_info)
            .bind_image(1, ovdii, array_count(ovdii), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .bind_image(2, vdii, array_count(vdii), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
            .bind_buffer(3, buffer_info + 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .bind_buffer(4, buffer_info + 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
            .bind_buffer(5, buffer_info + 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .bind_buffer(6, buffer_info + 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .bind_buffer(7, buffer_info + 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            .bind_buffer(8, buffer_info + 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .build(descriptor_sets[0], descriptor_set_layouts[0]);
    }

    void push_descriptors(
        gfx::vul::state_t& gfx,
        VkCommandBuffer command_buffer,
        VkPipelineLayout pipeline_layout_, 
        texture_cache_t& texture_cache,
        gfx::vul::gpu_buffer_t* rt_mesh_data,
        gfx::vul::gpu_buffer_t* probe_data,
        gfx::vul::gpu_buffer_t* probe_settings,
        gfx::vul::gpu_buffer_t* probe_rays,
        gfx::vul::gpu_buffer_t* environment,
        gfx::vul::gpu_buffer_t* point_lights,
        gfx::vul::descriptor_builder_t&& builder, 
        gfx::vul::texture_2d_t* irradiance_texture,
        gfx::vul::texture_2d_t* visibility_texture
    ) {
        VkDescriptorBufferInfo buffer_info[9];
        u32 b = 0;
        buffer_info[b].buffer = tlas.buffer.buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = rt_mesh_data->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = probe_data->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = probe_settings->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = probe_rays->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = environment->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        buffer_info[b].buffer = point_lights->buffer;
        buffer_info[b].offset = 0; 
        buffer_info[b++].range = VK_WHOLE_SIZE;

        VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
        descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptor_acceleration_structure_info.accelerationStructureCount = 1;
        descriptor_acceleration_structure_info.pAccelerationStructures    = &tlas.handle;

        VkDescriptorImageInfo vdii[4096];
        VkDescriptorImageInfo ovdii[2];

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

        ovdii[0].imageLayout = irradiance_texture->image_layout;
        ovdii[0].imageView = irradiance_texture->image_view;
        ovdii[0].sampler = irradiance_texture->sampler;

        ovdii[1].imageLayout = visibility_texture->image_layout;
        ovdii[1].imageView = visibility_texture->image_view;
        ovdii[1].sampler = visibility_texture->sampler;


        builder
            .bind_buffer(0, buffer_info + 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, &descriptor_acceleration_structure_info)
            .bind_image(1, ovdii, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .bind_image(2, vdii, array_count(vdii), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
            // .bind_buffer(3, buffer_info + 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            .bind_buffer(4, buffer_info + 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
            .bind_buffer(5, buffer_info + 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .bind_buffer(6, buffer_info + 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .bind_buffer(7, buffer_info + 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            .bind_buffer(8, buffer_info + 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .bind_buffer(9, buffer_info + 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .build_push(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_layout_, descriptor_sets[0], gfx.khr.vkCmdPushDescriptorSetKHR);
    }
    

    void add_to_tlas(
        gfx::vul::state_t& gfx,
        rt_cache_t& cache,
        u64 gfx_id,
        u64 blas_id,
        m44 t,
        umm transform_count = 1
    ) {
        TIMED_FUNCTION;
        VkTransformMatrixKHR transform_matrix{};
        t = glm::transpose(t);
        utl::copy(&transform_matrix, &t, sizeof(transform_matrix));
        auto i = instance_count++;
        assert(instance_count < array_count(tlas_instances));
        if (instance_count > array_count(tlas_instances)) {
            zyy_error(__FUNCTION__, "TLAS OVERFLOW"); return;
        }
        VkAccelerationStructureInstanceKHR& acceleration_structure_instance    = tlas_instances[i];
        acceleration_structure_instance.transform                              = transform_matrix;
        acceleration_structure_instance.instanceCustomIndex                    = gfx_id;
        acceleration_structure_instance.mask                                   = 0xFF;
        acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
        acceleration_structure_instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        acceleration_structure_instance.accelerationStructureReference         = cache.blas[blas_id].device_address;
    }

    void build_tlas(
        gfx::vul::state_t& gfx,
        rt_cache_t& cache,
        VkCommandBuffer command_buffer,
        bool update = false
    ) {
        TIMED_FUNCTION;

        if (update == false) { // create
            gfx.create_data_buffer(sizeof(VkAccelerationStructureInstanceKHR) * array_count(tlas_instances), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, &instance_buffer);
        }
        gfx.fill_data_buffer(&instance_buffer, tlas_instances);

        VkDeviceOrHostAddressConstKHR instance_data_device_address{};
	    instance_data_device_address.deviceAddress = gfx.get_buffer_device_address(instance_buffer.buffer);

        VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
        acceleration_structure_geometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        acceleration_structure_geometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        acceleration_structure_geometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
        acceleration_structure_geometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
        acceleration_structure_geometry.geometry.instances.data            = instance_data_device_address;

        VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
        acceleration_structure_build_geometry_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        acceleration_structure_build_geometry_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        acceleration_structure_build_geometry_info.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        acceleration_structure_build_geometry_info.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        acceleration_structure_build_geometry_info.geometryCount = 1;
        acceleration_structure_build_geometry_info.pGeometries   = &acceleration_structure_geometry;

        const uint32_t primitive_count = array_count(tlas_instances); 
        // const uint32_t primitive_count = instance_count;

        VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
        acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        gfx.khr.vkGetAccelerationStructureBuildSizesKHR(
            gfx.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &acceleration_structure_build_geometry_info,
            &primitive_count,
            &acceleration_structure_build_sizes_info);


        // if (update && tlas.buffer.buffer) {
        //     gfx.destroy_data_buffer(tlas.buffer);
        // }

        if (update == false) {
            TIMED_BLOCK(build_tlas_CreateBuffer);

            gfx.create_data_buffer(acceleration_structure_build_sizes_info.accelerationStructureSize,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, &tlas.buffer);
        
            VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
            acceleration_structure_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            acceleration_structure_create_info.buffer = tlas.buffer.buffer;
            acceleration_structure_create_info.size   = acceleration_structure_build_sizes_info.accelerationStructureSize;
            acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            gfx.khr.vkCreateAccelerationStructureKHR(gfx.device, &acceleration_structure_create_info, nullptr, &tlas.handle);

            VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
            acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            acceleration_device_address_info.accelerationStructure = tlas.handle;
            tlas.device_address        = gfx.khr.vkGetAccelerationStructureDeviceAddressKHR(gfx.device, &acceleration_device_address_info);
        }

        local_persist auto scratch_buffer = gfx.create_scratch_buffer(acceleration_structure_build_sizes_info.buildScratchSize);
        
        VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
        acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        acceleration_build_geometry_info.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        acceleration_build_geometry_info.srcAccelerationStructure  = update ? tlas.handle : VK_NULL_HANDLE;
        acceleration_build_geometry_info.dstAccelerationStructure  = tlas.handle;
        acceleration_build_geometry_info.geometryCount             = 1;
        acceleration_build_geometry_info.pGeometries               = &acceleration_structure_geometry;
        acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.device_address;

        VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
        acceleration_structure_build_range_info.primitiveCount                                           = instance_count;
        acceleration_structure_build_range_info.primitiveOffset                                          = 0;
        acceleration_structure_build_range_info.firstVertex                                              = 0;
        acceleration_structure_build_range_info.transformOffset                                          = 0;
        VkAccelerationStructureBuildRangeInfoKHR * acceleration_build_structure_range_infos[] = {&acceleration_structure_build_range_info};

        {
            // this will wait if rendering is slow and using single command, and make it look like this is the slow part
            TIMED_BLOCK(build_tlas_BuildAccelerationStructure);
            // gfx::vul::quick_cmd_raii_t c{&gfx};
            // VkCommandBuffer command_buffer = c.c();
            gfx.khr.vkCmdBuildAccelerationStructuresKHR(
                command_buffer,
                1,
                &acceleration_build_geometry_info,
                acceleration_build_structure_range_infos);
        }
        // gfx.destroy_scratch_buffer(scratch_buffer);
    }


    void build_layout(VkDevice device_) {
        device = device_;
        pipeline_layout[0] = gfx::vul::create_pipeline_layout(device, &descriptor_set_layouts[1], 1, sizeof(u32), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline_layout[1] = gfx::vul::create_pipeline_layout(device, &descriptor_set_layouts[2], 1, sizeof(u32), VK_SHADER_STAGE_COMPUTE_BIT);

    }

    void build_integrate_pass(
        gfx::vul::descriptor_builder_t& builder, 
        VkBuffer* buffers,
        gfx::vul::texture_2d_t* irradiance_texture,
        gfx::vul::texture_2d_t* visibility_texture,
        bool depth = false
    ) {
        VkDescriptorBufferInfo buffer_info[8];
        u32 i = 0;
        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;

        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;

        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;
        
        buffer_info[i].buffer = buffers[i];
        buffer_info[i].offset = 0; 
        buffer_info[i++].range = VK_WHOLE_SIZE;
        
        VkDescriptorImageInfo vdii[2];
        
        vdii[0].imageLayout = irradiance_texture->image_layout;
        vdii[0].imageView = irradiance_texture->image_view;
        vdii[0].sampler = irradiance_texture->sampler;

        vdii[1].imageLayout = visibility_texture->image_layout;
        vdii[1].imageView = visibility_texture->image_view;
        vdii[1].sampler = visibility_texture->sampler;

        builder
            .bind_buffer(0, buffer_info + 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_image(1, vdii, array_count(vdii), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_buffer(2, buffer_info + 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_buffer(3, buffer_info + 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_buffer(4, buffer_info + 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build(descriptor_sets[1 + depth], descriptor_set_layouts[1 + depth]);
    }
};
