
struct Entity {
    uint64_t vertex_start;
    uint64_t index_start;
    uint64_t material;
    uint64_t transform;
    uint instance_offset;
    uint instance_count;
    uint albedo;
};

struct Camera {
    mat4 view;
    mat4 proj;
};


struct Scene {
    uint64_t vertex_buffer;
    uint64_t index_buffer;
    uint64_t transform_buffer;
    uint64_t material_buffer;

    uint64_t instance_buffer;
    uint64_t indirect_buffer;
    uint64_t texture_cache;

    uint64_t entities;

    uint64_t environment;

    uint64_t point_lights;

    uint64_t ddgi;
};