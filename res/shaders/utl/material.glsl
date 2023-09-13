#include "lighting.glsl"

struct IndirectIndexedDraw {
    uint     index_count;
    uint     instance_count;
    uint     first_index;
    int      vertex_offset;
    uint     first_instance;
    uint     albedo_id;
    uint     normal_id;
    uint     object_id;
};

#define MATERIAL_LIT       1
#define MATERIAL_TRIPLANAR 2
#define MATERIAL_BILLBOARD 4

struct Material {
	vec4 albedo;

    float ao;
    float emission;
    float metallic;
    float roughness;

    uint flags;     // for material effects
    uint opt_flags; // for performance
    
    uint albedo_texture_id;
    uint normal_texture_id;

    float scale;
    uint padding[3];
};




struct LightProbeSettings {
    vec3 aabb_min;
    // float p0; 
    vec3 aabb_max;
    // float p1;
    uvec3 dim;
    vec3 grid_size;
    int sample_max;
    float hysteresis;
    float boost;
};

vec3 light_probe_aabb_size(LightProbeSettings settings) {
    return settings.aabb_max - settings.aabb_min;
}

vec3 light_probe_grid_size(LightProbeSettings settings) {
    return settings.grid_size;
}

vec3 light_probe_aabb_center(LightProbeSettings settings) {
    return settings.aabb_min + light_probe_aabb_size(settings) * 0.5;
}

vec3 light_probe_local_pos(LightProbeSettings settings, vec3 p) {
    return p - settings.aabb_min;
}

vec3 light_probe_local_pos_normalized(LightProbeSettings settings, vec3 p) {
    return light_probe_local_pos(settings, p) / light_probe_aabb_size(settings);
}

ivec3 light_probe_probe_index(LightProbeSettings settings, vec3 p) {
    vec3 grid = light_probe_grid_size(settings);
    return ivec3(floor(saturate(light_probe_local_pos_normalized(settings, p + grid*0.0)) * (vec3(settings.dim))));
}

struct SH9 {
    float h[9];
};

struct SH9Irradiance {
	vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
};

struct SH9Depth {
	vec2 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
};

struct SLOL9Irradiance {
	vec3 irradiance[9];
};

SH9Irradiance sh_identity() {
    SH9Irradiance r;
    r.l00 = vec3(0.0);
    r.l1m1 = vec3(0.0);
    r.l10 = vec3(0.0);
    r.l11 = vec3(0.0);
    r.l2m2 = vec3(0.0);
    r.l2m1 = vec3(0.0);
    r.l20 = vec3(0.0);
    r.l21 = vec3(0.0);
    r.l22 = vec3(0.0);
    return r;
}
SH9Depth sh_depth_identity() {
    SH9Depth r;
    r.l00 = vec2(0.0);
    r.l1m1 = vec2(0.0);
    r.l10 = vec2(0.0);
    r.l11 = vec2(0.0);
    r.l2m2 = vec2(0.0);
    r.l2m1 = vec2(0.0);
    r.l20 = vec2(0.0);
    r.l21 = vec2(0.0);
    r.l22 = vec2(0.0);
    return r;
}

struct LightProbe {
    // SLOL9Irradiance irradiance;
    // SH9Irradiance irradiance;
    // SH9Depth depth;
    vec3 p;
    // uint id;
    uint ray_back_count;
    // uint backface_count;
};

struct ProbeRayResult {
    vec3 radiance;
    float depth;
    vec3 direction;
};

struct ProbeRayPacked {
    uvec2 direction_depth; // packed normal, float
    uvec2 radiance; // rgbe, unused
};

SH9 coefficients(in vec3 N) {
    SH9 result;
 
    // Band 0
    result.h[0] = 0.282095f;
 
    // Band 1
    result.h[1] = 0.488603f * N.y;
    result.h[2] = 0.488603f * N.z;
    result.h[3] = 0.488603f * N.x;
 
    // Band 2
    result.h[4] = 1.092548f * N.x * N.y;
    result.h[5] = 1.092548f * N.y * N.z;
    result.h[6] = 0.315392f * (3.0f * N.z * N.z - 1.0f);
    result.h[7] = 1.092548f * N.x * N.z;
    result.h[8] = 0.546274f * (N.x * N.x - N.y * N.y);
 
    return result;
}

SH9Depth depth_to_sh9(in vec3 N, in float D)
{
    SH9 SHCoefficients = coefficients(N);
    SH9Depth result;

    vec2 depth = vec2(D, D*D);

    {
        result.l00  = depth * SHCoefficients.h[0];
        result.l1m1 = depth * SHCoefficients.h[1];
        result.l10  = depth * SHCoefficients.h[2];
        result.l11  = depth * SHCoefficients.h[3];
        result.l2m2 = depth * SHCoefficients.h[4];
        result.l2m1 = depth * SHCoefficients.h[5];
        result.l20  = depth * SHCoefficients.h[6];
        result.l21  = depth * SHCoefficients.h[7];
        result.l22  = depth * SHCoefficients.h[8];
    }
    return result;
}

SH9Irradiance irradiance_to_sh9(in vec3 N, in vec3 L)
{
    SH9 SHCoefficients = coefficients(N);
    SH9Irradiance result;
    // for (uint i = 0; i < 9; ++i)
    {
        result.l00  = L * SHCoefficients.h[0];
        result.l1m1 = L * SHCoefficients.h[1];
        result.l10  = L * SHCoefficients.h[2];
        result.l11  = L * SHCoefficients.h[3];
        result.l2m2 = L * SHCoefficients.h[4];
        result.l2m1 = L * SHCoefficients.h[5];
        result.l20  = L * SHCoefficients.h[6];
        result.l21  = L * SHCoefficients.h[7];
        result.l22  = L * SHCoefficients.h[8];
    }
    return result;
}

void irradiance_to_sh9(in vec3 N, in vec3 L, inout SH9Irradiance result)
{
    SH9 SHCoefficients = coefficients(N);
    // SH9Irradiance result;
    // for (uint i = 0; i < 9; ++i)
    {
        result.l00  += L * SHCoefficients.h[0];
        result.l1m1 += L * SHCoefficients.h[1];
        result.l10  += L * SHCoefficients.h[2];
        result.l11  += L * SHCoefficients.h[3];
        result.l2m2 += L * SHCoefficients.h[4];
        result.l2m1 += L * SHCoefficients.h[5];
        result.l20  += L * SHCoefficients.h[6];
        result.l21  += L * SHCoefficients.h[7];
        result.l22  += L * SHCoefficients.h[8];
    }
    // return result;
}

SH9Depth accumulate_depth(SH9Depth current_depth, SH9Depth new_depth, float weight)
{
    SH9Depth result;
    result.l00 = current_depth.l00 + new_depth.l00 * weight;
    result.l1m1 = current_depth.l1m1 + new_depth.l1m1 * weight;
    result.l10 = current_depth.l10 + new_depth.l10 * weight;
    result.l11 = current_depth.l11 + new_depth.l11 * weight;
    result.l2m2 = current_depth.l2m2 + new_depth.l2m2 * weight;
    result.l2m1 = current_depth.l2m1 + new_depth.l2m1 * weight;
    result.l20 = current_depth.l20 + new_depth.l20 * weight;
    result.l21 = current_depth.l21 + new_depth.l21 * weight;
    result.l22 = current_depth.l22 + new_depth.l22 * weight;
    return result;
}

SH9Irradiance accumulate_irradiance(SH9Irradiance existingIrradiance, SH9Irradiance additionalIrradiance, float weight)
{
    SH9Irradiance result;
    result.l00 = existingIrradiance.l00 + additionalIrradiance.l00 * weight;
    result.l1m1 = existingIrradiance.l1m1 + additionalIrradiance.l1m1 * weight;
    result.l10 = existingIrradiance.l10 + additionalIrradiance.l10 * weight;
    result.l11 = existingIrradiance.l11 + additionalIrradiance.l11 * weight;
    result.l2m2 = existingIrradiance.l2m2 + additionalIrradiance.l2m2 * weight;
    result.l2m1 = existingIrradiance.l2m1 + additionalIrradiance.l2m1 * weight;
    result.l20 = existingIrradiance.l20 + additionalIrradiance.l20 * weight;
    result.l21 = existingIrradiance.l21 + additionalIrradiance.l21 * weight;
    result.l22 = existingIrradiance.l22 + additionalIrradiance.l22 * weight;
    return result;
}

SH9Irradiance mix_irradiance(SH9Irradiance existingIrradiance, SH9Irradiance additionalIrradiance, float count)
{
    SH9Irradiance result;
    result.l00 = mix(existingIrradiance.l00, additionalIrradiance.l00, 1./(count+1.));
    result.l1m1 = mix(existingIrradiance.l1m1, additionalIrradiance.l1m1, 1./(count+1.));
    result.l10 = mix(existingIrradiance.l10, additionalIrradiance.l10, 1./(count+1.));
    result.l11 = mix(existingIrradiance.l11, additionalIrradiance.l11, 1./(count+1.));
    result.l2m2 = mix(existingIrradiance.l2m2, additionalIrradiance.l2m2, 1./(count+1.));
    result.l2m1 = mix(existingIrradiance.l2m1, additionalIrradiance.l2m1, 1./(count+1.));
    result.l20 = mix(existingIrradiance.l20, additionalIrradiance.l20, 1./(count+1.));
    result.l21 = mix(existingIrradiance.l21, additionalIrradiance.l21, 1./(count+1.));
    result.l22 = mix(existingIrradiance.l22, additionalIrradiance.l22, 1./(count+1.));
    return result;
}

// Equation 13 Ramamoorthi
vec2 sh9_to_depth(vec3 N, SH9Depth depth)
{
    const float A0 = 3.141593;
    const float A1 = 2.094395;
    const float A2 = 0.785398;
    const float A3 = 0.0;
    const float A4 = -0.130900;
    const float A5 = 0.0;
    const float A6 = 0.0490;
 
    return depth.l00 * 0.282095f * A0
        + depth.l1m1 * 0.488603f * N.y * A1
        + depth.l10 * 0.488603f * N.z * A1
        + depth.l11 * 0.488603f * N.x * A1
        + depth.l2m2 * 1.092548f * N.x * N.y * A2
        + depth.l2m1 * 1.092548f * N.y * N.z * A2
        + depth.l20 * 0.315392f * (3.0f * N.z * N.z - 1.0f) * A2
        + depth.l21 * 1.092548f * N.x * N.z * A2
        + depth.l22 * 0.546274f * (N.x * N.x - N.y * N.y) * A2;
}

vec3 sh9_to_irradiance(vec3 N, SH9Irradiance color)
{
    const float A0 = 3.141593;
    const float A1 = 2.094395;
    const float A2 = 0.785398;
    const float A3 = 0.0;
    const float A4 = -0.130900;
    const float A5 = 0.0;
    const float A6 = 0.0490;
 
    vec3 irradiance = color.l00 * 0.282095f * A0
        + color.l1m1 * 0.488603f * N.y * A1
        + color.l10 * 0.488603f * N.z * A1
        + color.l11 * 0.488603f * N.x * A1
        + color.l2m2 * 1.092548f * N.x * N.y * A2
        + color.l2m1 * 1.092548f * N.y * N.z * A2
        + color.l20 * 0.315392f * (3.0f * N.z * N.z - 1.0f) * A2
        + color.l21 * 1.092548f * N.x * N.z * A2
        + color.l22 * 0.546274f * (N.x * N.x - N.y * N.y) * A2;
    return irradiance;
}


// #define PROBE_USE_SH_COLOR
#define PROBE_USE_SAMPLER_COLOR
#define PROBE_USE_SAMPLER_DEPTH


// [0] is min and max



mat4 billboard_matrix(mat4 m, bool cylindical) {
    
    m[0][0] = length(m[0].xyz);
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;

    if (!cylindical) {
        m[1][1] = length(m[1].xyz);
        m[1][0] = 0.0f;
        m[1][2] = 0.0f;
    }

    m[2][2] = length(m[2].xyz);
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    return m;
}

const vec3 PROBE_DIRS[9] = {
             (vec3(0.0, 0.0, 1.0)),
    normalize(vec3(0.0, 0.0, 0.0)*2.-1.),
    normalize(vec3(0.0, 0.0, 1.0)*2.-1.),
    normalize(vec3(0.0, 1.0, 0.0)*2.-1.),
    normalize(vec3(0.0, 1.0, 1.0)*2.-1.),
    normalize(vec3(1.0, 0.0, 0.0)*2.-1.),
    normalize(vec3(1.0, 0.0, 1.0)*2.-1.),
    normalize(vec3(1.0, 1.0, 0.0)*2.-1.),
    normalize(vec3(1.0, 1.0, 1.0)*2.-1.)
};


void stupid_radiance_encoding(inout SLOL9Irradiance encoding, in vec3 N, in vec3 R, in float count) {
    float a = 1./(count+1.);
    encoding.irradiance[0] = mix(encoding.irradiance[0], R, a);
    for (uint i = 1; i < 9; i++) {
        float weight = a * saturate(dot(N, PROBE_DIRS[i]));
        encoding.irradiance[i] = mix(encoding.irradiance[i], R, weight);
    }
}

vec3 stupid_irradiance_decoding(in SLOL9Irradiance encoding, in vec3 N) {
    vec3 irradiance = encoding.irradiance[0] / 9.0;

    for (uint i = 1; i < 9; i++) {
        float weight = saturate(dot(N, PROBE_DIRS[i]));
        irradiance = mix(irradiance, encoding.irradiance[i], weight);
    }

    return irradiance;
}

vec3 stupid_light_probe_irradiance(vec3 p, vec3 n, LightProbe probes[8], LightProbeSettings settings) {
    vec3 irradiance = vec3(0.0);
    float total_weight = 0.0;
    
    vec3 cell_rcp = 1. / light_probe_grid_size(settings);
    vec3 alpha = saturate((p - probes[0].p) * cell_rcp);


    for (uint i = 0; i < 8; i++) {
		uvec3 offset = uvec3(i, i>>1, i>>2) & 1;
		
        vec3 r = p - probes[i].p + n * 0.001;
        vec3 d = normalize(-r); 

        vec3 tri = mix(1.0 - alpha, alpha, offset);
        float weight = 1.0;

        {
            vec3 bf = normalize(probes[i].p - p);
            const float smooth_backface = 1.0;
            weight *= mix(saturate(dot(d, n)), max(0.001, pow((dot(bf, n) + 1.0)*0.5,2.0))+0.02, smooth_backface);
        }

        weight = max(0.00001, weight);

        // vec3 probe_irradiance = stupid_irradiance_decoding(probes[i].irradiance, n);

        const float crush = 0.2;
        if (weight < crush) {
            weight *= weight * weight * (1.0 / sqrt(crush));
        }
        weight *= tri.x * tri.y * tri.z;

        // linear
        // probe_irradiance = sqrt(probe_irradiance);

        // irradiance += probe_irradiance * weight;
        total_weight += weight;
	}

    if (total_weight > 0.0) {
        // return sqrt(irradiance / total_weight);
        // return irradiance;
        return irradiance / total_weight;
    }

    return vec3(0.0);
}