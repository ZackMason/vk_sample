struct Material {
	vec4 albedo;

    float ao;
    float emission;
    float metallic;
    float roughness;

    uint flags;     // for material effects
    uint opt_flags; // for performance
    uint padding[2 + 4];
};

layout(std430, set = 2, binding = 0) readonly buffer MaterialBuffer {
	Material materials[];
} uMaterialBuffer;

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
    float saturation;
    float contrast;
    uint light_count;
    vec4 more_padding;
    
    PointLight point_lights[512];
};

layout(std430, set = 3, binding = 0) readonly buffer EnvironmentBuffer {
	Environment uEnvironment;
};
