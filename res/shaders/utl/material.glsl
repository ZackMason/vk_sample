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


struct DirectionalLight {
    vec4 direction;
    vec4 color;
};

struct PointLight {
    vec4 pos; // position, w unused
    vec4 col; // color
    vec4 rad; // radiance, w unused
    vec4 pad;
};

struct Environment {
    DirectionalLight sun;

    vec4 ambient_color;

    vec4 fog_color;
    float fog_density;

    // color correction
    float ambient_strength;
    float contrast;
    uint light_count;
    vec4 more_padding;
    
    PointLight point_lights[512];
};

struct LightProbeSettings {
    vec3 aabb_min;
    // float p0; 
    vec3 aabb_max;
    // float p1;
    uvec3 dim;
    int sample_max;
};

vec3 light_probe_aabb_size(LightProbeSettings settings) {
    return settings.aabb_max - settings.aabb_min;
}

vec3 light_probe_grid_size(LightProbeSettings settings) {
    return light_probe_aabb_size(settings) / vec3(settings.dim);
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
    return ivec3(floor(saturate(light_probe_local_pos_normalized(settings, p + grid*0.)) * (vec3(settings.dim))));
}

struct SH9 {
    float h[9];
};

struct SH9Irradiance {
	vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
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

struct LightProbe {
    SLOL9Irradiance irradiance;
    // SH9Irradiance irradiance;
    vec3 p;
    uint id;
    uint samples;
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

// [0] and [2] are min and max
vec3 light_probe_irradiance(vec3 p, LightProbe probes[8]) {
    vec3 irradiance = vec3(0.0);
    float sigma = 1.0;
    
    float b0 = exp(-pow(distance(p, probes[0].p), 2.0) / (2.0*sigma*sigma));
    float b1 = exp(-pow(distance(p, probes[1].p), 2.0) / (2.0*sigma*sigma));
    float b2 = exp(-pow(distance(p, probes[2].p), 2.0) / (2.0*sigma*sigma));
    float b3 = exp(-pow(distance(p, probes[3].p), 2.0) / (2.0*sigma*sigma));
    float b4 = exp(-pow(distance(p, probes[4].p), 2.0) / (2.0*sigma*sigma));
    float b5 = exp(-pow(distance(p, probes[5].p), 2.0) / (2.0*sigma*sigma));
    float b6 = exp(-pow(distance(p, probes[6].p), 2.0) / (2.0*sigma*sigma));
    float b7 = exp(-pow(distance(p, probes[7].p), 2.0) / (2.0*sigma*sigma));
    // float b0 = distance(p, probes[0].p);
    // float b1 = distance(p, probes[1].p);
    // float b2 = distance(p, probes[2].p);
    // float b3 = distance(p, probes[3].p);
    // float b4 = distance(p, probes[4].p);
    // float b5 = distance(p, probes[5].p);
    // float b6 = distance(p, probes[6].p);
    // float b7 = distance(p, probes[7].p);

    float tb = b0 + b1 + b2 + b3 + b4 + b5 + b6 + b7;

    vec3 n0 = normalize(probes[0].p - p);
    vec3 n1 = normalize(probes[1].p - p);
    vec3 n2 = normalize(probes[2].p - p);
    vec3 n3 = normalize(probes[3].p - p);
    vec3 n4 = normalize(probes[4].p - p);
    vec3 n5 = normalize(probes[5].p - p);
    vec3 n6 = normalize(probes[6].p - p);
    vec3 n7 = normalize(probes[7].p - p);
    
    // irradiance += sh9_to_irradiance(n0, probes[0].irradiance) * ((b0 / tb));
    // irradiance += sh9_to_irradiance(n1, probes[1].irradiance) * ((b1 / tb));
    // irradiance += sh9_to_irradiance(n2, probes[2].irradiance) * ((b2 / tb));
    // irradiance += sh9_to_irradiance(n3, probes[3].irradiance) * ((b3 / tb));
    // irradiance += sh9_to_irradiance(n4, probes[4].irradiance) * ((b4 / tb));
    // irradiance += sh9_to_irradiance(n5, probes[5].irradiance) * ((b5 / tb));
    // irradiance += sh9_to_irradiance(n6, probes[6].irradiance) * ((b6 / tb));
    // irradiance += sh9_to_irradiance(n7, probes[7].irradiance) * ((b7 / tb));

    return irradiance;
}

uint index_3d(uvec3 dim, uvec3 i) {
    return i.x + i.y * dim.x + i.z * dim.x * dim.y;
}

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
              vec3(0.0, 0.0, 0.0),
    normalize(vec3(0.0, 0.0, 1.0)*2.-1.),
    normalize(vec3(0.0, 1.0, 0.0)*2.-1.),
    normalize(vec3(0.0, 1.0, 1.0)*2.-1.),
    normalize(vec3(1.0, 0.0, 0.0)*2.-1.),
    normalize(vec3(1.0, 0.0, 1.0)*2.-1.),
    normalize(vec3(1.0, 1.0, 0.0)*2.-1.),
    normalize(vec3(1.0, 0.0, 1.0)*2.-1.),
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
        float weight = saturate(dot(-N, PROBE_DIRS[i]));
        irradiance = mix(irradiance, encoding.irradiance[i], weight);
    }

    return irradiance;
}

vec3 stupid_light_probe_irradiance(vec3 p, vec3 n, LightProbe probes[8], LightProbeSettings settings) {
    vec3 irradiance = vec3(0.0);
    
    vec3 alpha = saturate((p - probes[0].p) / light_probe_grid_size(settings));

    vec3 cell_rcp = 1. / light_probe_grid_size(settings);

    float sigma = 1.;
    float b0 = exp(-pow(distance(p, probes[0].p), 2.0) / (2.0*sigma*sigma));
    float b1 = exp(-pow(distance(p, probes[1].p), 2.0) / (2.0*sigma*sigma));
    float b2 = exp(-pow(distance(p, probes[2].p), 2.0) / (2.0*sigma*sigma));
    float b3 = exp(-pow(distance(p, probes[3].p), 2.0) / (2.0*sigma*sigma));
    float b4 = exp(-pow(distance(p, probes[4].p), 2.0) / (2.0*sigma*sigma));
    float b5 = exp(-pow(distance(p, probes[5].p), 2.0) / (2.0*sigma*sigma));
    float b6 = exp(-pow(distance(p, probes[6].p), 2.0) / (2.0*sigma*sigma));
    float b7 = exp(-pow(distance(p, probes[7].p), 2.0) / (2.0*sigma*sigma));
    // float b0 = saturate(1.-length((p - probes[0].p) * cell_rcp));
    // float b1 = saturate(1.-length((p - probes[1].p) * cell_rcp));
    // float b2 = saturate(1.-length((p - probes[2].p) * cell_rcp));
    // float b3 = saturate(1.-length((p - probes[3].p) * cell_rcp));
    // float b4 = saturate(1.-length((p - probes[4].p) * cell_rcp));
    // float b5 = saturate(1.-length((p - probes[5].p) * cell_rcp));
    // float b6 = saturate(1.-length((p - probes[6].p) * cell_rcp));
    // float b7 = saturate(1.-length((p - probes[7].p) * cell_rcp));

    float tb = max(0.01, b0 + b1 + b2 + b3 + b4 + b5 + b6 + b7);
    // tb = 1.;

    vec3 n0 = normalize(probes[0].p - p);
    vec3 n1 = normalize(probes[1].p - p);
    vec3 n2 = normalize(probes[2].p - p);
    vec3 n3 = normalize(probes[3].p - p);
    vec3 n4 = normalize(probes[4].p - p);
    vec3 n5 = normalize(probes[5].p - p);
    vec3 n6 = normalize(probes[6].p - p);
    vec3 n7 = normalize(probes[7].p - p);

    float bf0 = saturate((dot(n0, n) + 1.) * 0.5);
    float bf1 = saturate((dot(n1, n) + 1.) * 0.5);
    float bf2 = saturate((dot(n2, n) + 1.) * 0.5);
    float bf3 = saturate((dot(n3, n) + 1.) * 0.5);
    float bf4 = saturate((dot(n4, n) + 1.) * 0.5);
    float bf5 = saturate((dot(n5, n) + 1.) * 0.5);
    float bf6 = saturate((dot(n6, n) + 1.) * 0.5);
    float bf7 = saturate((dot(n7, n) + 1.) * 0.5);
    
    irradiance += stupid_irradiance_decoding(probes[0].irradiance, n0) * ((b0 / tb));// * bf0;
    irradiance += stupid_irradiance_decoding(probes[1].irradiance, n1) * ((b1 / tb));// * bf1;
    irradiance += stupid_irradiance_decoding(probes[2].irradiance, n2) * ((b2 / tb));// * bf2;
    irradiance += stupid_irradiance_decoding(probes[3].irradiance, n3) * ((b3 / tb));// * bf3;
    irradiance += stupid_irradiance_decoding(probes[4].irradiance, n4) * ((b4 / tb));// * bf4;
    irradiance += stupid_irradiance_decoding(probes[5].irradiance, n5) * ((b5 / tb));// * bf5;
    irradiance += stupid_irradiance_decoding(probes[6].irradiance, n6) * ((b6 / tb));// * bf6;
    irradiance += stupid_irradiance_decoding(probes[7].irradiance, n7) * ((b7 / tb));// * bf7;

    return irradiance;
}