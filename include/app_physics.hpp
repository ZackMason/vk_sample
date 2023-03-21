#ifndef APP_PHYSICS_HPP
#define APP_PHYSICS_HPP

#include "uid.hpp"

#define PHYSICS_MAX_RIGIDBODY_COUNT 4096
#define PHYSICS_MAX_COLLIDER_COUNT 256

namespace physics {

enum struct backend_type {
    PHYSX, BULLET, CUSTOM, SIZE
};

enum struct collider_shape_type {
    CONVEX, TRIMESH, SPHERE, CAPSULE, BOX, NONE, SIZE
};

struct collider_convex_info_t {
    std::byte*  mesh;
    size_t      size;
};
struct collider_trimesh_info_t {
    std::byte*  mesh;
    size_t      size;
};
struct collider_sphere_info_t {
    f32 radius;
};

enum struct rigidbody_type {
    DYNAMIC, STATIC, KINEMATIC, CHARACTER, SIZE
};

// Note(Zack): These are the frontend structs that the game code will interact with
DEFINE_TYPED_ID(collider_id);
struct collider_t {
    collider_shape_type type;
    collider_id         id{uid::invalid_id};
    void*               shape;

    v3f                 offset;
    glm::quat           orientation;
};

#define MAX_COLLIDER_COUNT 8
DEFINE_TYPED_ID(rigidbody_id)
struct rigidbody_t {
    rigidbody_type  type;
    rigidbody_id    id{uid::invalid_id};

    v3f             velocity{0.0f};
    v3f             angular_velocity{0.0f};

    collider_t*     colliders[MAX_COLLIDER_COUNT];
    size_t          collider_count{0};

    const void*     user_data{0};
};

struct raycast_result_t {
    v3f point;
    f32 distance;
    const void* user_data;
};

struct api_t;

using create_scene = void(*)(const api_t*);
using destroy_scene = void(*)(const api_t*);

using create_rigidbody_function = rigidbody_t*(*)(const api_t*, void* entity, rigidbody_type);
using create_collider_function = collider_t*(*)(const api_t*, const rigidbody_t*, collider_shape_type, void* collider_info);
using raycast_world_function = raycast_result_t(*)(const api_t*, v3f ro, v3f rd);
using simulate_function = void(*)(f32 dt);

struct api_t {
    backend_type type;
    void*        backend{nullptr};

    simulate_function simulate{0};

    create_rigidbody_function create_rigidbody{0};
    create_collider_function create_collider{0};
    raycast_world_function raycast_world{0};

    // TODO(Zack): Add hashes
    rigidbody_t rigidbodies[PHYSICS_MAX_RIGIDBODY_COUNT];
    size_t      rigidbody_count{0};

    collider_t  colliders[PHYSICS_MAX_COLLIDER_COUNT];
    size_t      collider_count{0};
};

using init_function = void(__cdecl *)(api_t* api, backend_type type, arena_t* arena);

};

#endif 