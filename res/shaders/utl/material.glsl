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