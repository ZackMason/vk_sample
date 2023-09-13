vec3 light_probe_irradiance(vec3 p, vec3 dir, vec3 n, LightProbeSettings settings) {
    vec3 irradiance = vec3(0.0);

    float total_weight = 0.0;

    vec3 biased_world_pos = p + (n * 0.02 + dir * 0.62);
    // vec3 biased_world_pos =     p + (n * 0.02);
    uvec3 biased_probe_coord = light_probe_probe_index(settings, biased_world_pos);
    // uint biased_probe_id = index_1d(settings.dim, biased_probe_coord);
    // without offset
    vec3 biased_probe_true_pos = probe_position(biased_probe_coord, settings.grid_size, settings.aabb_min);

    vec3 grid_distance = (biased_world_pos - biased_probe_true_pos);
    vec3 alpha = saturate(grid_distance / settings.grid_size);
    
    // vec3 grid_size = light_probe_grid_size(settings);

    // for (uint i = 1; i < 8; i++) {
    //     grid_size = max(grid_size, abs(probes[i].p - probes[0].p));
    // }

    // vec3 cell_rcp = 1. / grid_size;

    // vec3 alpha = saturate((p - probes[0].p) * cell_rcp);

    for (uint i = 0; i < 8; i++) {
		uvec3 offset = uvec3(i, i>>1, i>>2) & uvec3(1);
        uvec3 adj_probe_coord = clamp(biased_probe_coord + offset, uvec3(0), uvec3(settings.dim-1));
        uint adj_probe_index = index_3d(settings.dim, adj_probe_coord);

        // ignore probes inside walls
        uint ray_count = probes[adj_probe_index].ray_back_count & 0xffff;
        uint backface_count = (probes[adj_probe_index].ray_back_count>>16) & 0xffff;
    
        if (ray_count / 2 < backface_count) { continue; }

        vec3 adj_probe_pos = probe_position(adj_probe_coord, settings.grid_size, settings.aabb_min) + probes[adj_probe_index].p;
		
        vec3 world_to_adj = normalize(adj_probe_pos - p);
        vec3 biased_to_adj = normalize(adj_probe_pos - biased_world_pos);
        float biased_to_adj_dist = length(adj_probe_pos - biased_world_pos);
        
        vec3 tri = max(vec3(0.001), mix(1.0 - alpha, alpha, offset));
        float weight = 1.0;

        float smooth_backface = 1.0;
        float wrap_shading = (dot(world_to_adj, n) + 1.0) * 0.5;
        float normal_shading = saturate(dot(world_to_adj, n));
        weight *= mix(normal_shading, sqr(wrap_shading) + 0.2, smooth_backface);

        uvec3 probe_coord = adj_probe_coord;

        vec2 depth_size_rcp = 1.0/textureSize(uProbeSampler[1], 0).xy;
        vec2 probe_depth = textureLod(uProbeSampler[1], probe_depth_uv(adj_probe_coord, -biased_to_adj, settings.dim, depth_size_rcp), 0).rg * 2.0;

        float variance = abs(sqr(probe_depth.x) - probe_depth.y);
        float cheb = 1.0;
        if (biased_to_adj_dist > probe_depth.x) {
            float v = biased_to_adj_dist - probe_depth.x;
            cheb = variance / (variance + sqr(v));
            cheb = max(cheb*cheb*cheb,0.0);
        }
        weight *= max(0.05, cheb);

        weight = max(0.00001, weight);
		
        vec3 probe_irradiance = texture(uProbeSampler[0], probe_color_uv(adj_probe_coord, n, settings.dim, 1.0/textureSize(uProbeSampler[0],0).xy)).rgb;
        // linear
        probe_irradiance = sqrt(saturate(probe_irradiance));

        const float crush = 0.2;
        if (weight < crush) {
            weight *= weight * weight * (1.0 / sqr(crush));
        }
        weight *= tri.x * tri.y * tri.z;

       
        irradiance += probe_irradiance * weight;
        total_weight += weight;
	}

    if (total_weight==0.0) { return vec3(0.0); }
    
    irradiance /= total_weight;

    irradiance.x = isnan(irradiance.x) ? 0.0f : irradiance.x;
    irradiance.y = isnan(irradiance.y) ? 0.0f : irradiance.y;
    irradiance.z = isnan(irradiance.z) ? 0.0f : irradiance.z;

    irradiance = sqr((irradiance));

    return irradiance * 2.0 * 3.1415;

}