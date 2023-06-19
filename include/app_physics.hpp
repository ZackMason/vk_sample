#ifndef APP_PHYSICS_HPP
#define APP_PHYSICS_HPP

#include "uid.hpp"

#define PHYSICS_MAX_RIGIDBODY_COUNT 4096
#define PHYSICS_MAX_COLLIDER_COUNT 256
#define PHYSICS_MAX_CHARACTER_COUNT 512

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

    // v3f                 offset;
    // glm::quat           orientation;
};

struct rigidbody_t;
using rigidbody_on_collision_function = void(*)(rigidbody_t*, rigidbody_t*);

struct rigidbody_flags {
    enum {
        SKIP_SYNC = BIT(0),
        IS_ON_GROUND = BIT(1), // note(zack): these are only set for character bodies
        IS_ON_WALL = BIT(2),
    };
};

#define RIGIDBODY_MAX_COLLIDER_COUNT 8
DEFINE_TYPED_ID(rigidbody_id)
struct rigidbody_t {
    rigidbody_type  type;
    rigidbody_id    id{uid::invalid_id};
    u64             flags{0};

    f32 mass{1.0f};
    f32 linear_dampening{0.5f};
    f32 angular_dampening{0.5f};

    v3f             position{0.0f};
    glm::quat       orientation{};

    v3f             velocity{0.0f};
    v3f             angular_velocity{0.0f};

    v3f             force{0.0f};
    v3f             torque{0.0f};

    m33             inertia{0.0f}, inverse_inertia{0.0f};

    collider_t      colliders[RIGIDBODY_MAX_COLLIDER_COUNT];
    size_t          collider_count{0};

    const void*     user_data{0};
    const void*     api_data{0};

    rigidbody_on_collision_function on_collision{0};
    rigidbody_on_collision_function on_collision_end{0};

    // transform direction from body space to world space
    inline glm::vec3 transform_direction(const glm::vec3& direction) const
    { return orientation * direction; }

    // transform direction from world space to body space
    inline glm::vec3 inverse_transform_direction(const glm::vec3& direction) const
    { return glm::inverse(orientation) * direction; }

    // get velocity and angular velocity in body space
    inline glm::vec3 get_point_velocity(const glm::vec3& point) const
    { return inverse_transform_direction(velocity) + glm::cross(angular_velocity, point); }

    // force and point vectors are in body space
    inline void add_force_at_point(const glm::vec3& force_, const glm::vec3& point)
    { force += transform_direction(force_), torque += glm::cross(point, force_); }

    // force vector in body space
    inline void add_relative_force(const glm::vec3& force_)
    { force += transform_direction(force_); }

    // integrate using the euler method
    void integrate(float dt, bool apply_gravity = false) {
        // integrate position
        v3f acceleration = force / mass;

        const bool is_on_ground = flags & rigidbody_flags::IS_ON_GROUND;

        if (apply_gravity && !is_on_ground) { acceleration.y -= 9.81f * 0.15f; }
        velocity += acceleration * dt;
        // position += velocity * dt;
        
        // integrate orientation
        angular_velocity += inverse_inertia *
            (torque - glm::cross(angular_velocity, inertia * angular_velocity)) * dt;
        // orientation += (orientation * glm::quat(0.0f, angular_velocity)) * (0.5f * dt);
        // orientation = glm::normalize(orientation);

        // reset accumulators
        force = v3f{0.0f};
        torque = v3f{0.0f};
    }
};

struct raycast_result_t {
    bool hit{false};
    v3f point;
    v3f normal;
    f32 distance;
    const void* user_data{0};
};

struct api_t;

using create_scene_function = void(*)(api_t*, const void* filter_fn);
using destroy_scene_function = void(*)(api_t*);

using create_rigidbody_function = rigidbody_t*(*)(api_t*, void* entity, rigidbody_type);
using create_collider_function = collider_t*(*)(api_t*, rigidbody_t*, collider_shape_type, void* collider_info);
using simulate_function = void(*)(api_t*, f32 dt);
using raycast_world_function = raycast_result_t(*)(const api_t*, v3f ro, v3f rd);

// updates a rigidbody pos and orientation
using update_rigidbody_function = void(*)(api_t*, rigidbody_t*);

struct export_dll api_t {
    backend_type type;
    void*        backend{nullptr};

    arena_t*        arena;

    simulate_function           simulate{0};
    update_rigidbody_function   set_rigidbody{0};
    update_rigidbody_function   sync_rigidbody{0};

    update_rigidbody_function   add_rigidbody{0};
    update_rigidbody_function   remove_rigidbody{0};

    create_rigidbody_function   create_rigidbody{0};
    create_collider_function    create_collider{0};
    raycast_world_function      raycast_world{0};
    create_scene_function       create_scene{0};
    destroy_scene_function      destroy_scene{0};

    // TODO(Zack): Add hashes
    rigidbody_t rigidbodies[PHYSICS_MAX_RIGIDBODY_COUNT];
    size_t      rigidbody_count{0};

    collider_t  colliders[PHYSICS_MAX_COLLIDER_COUNT];
    size_t      collider_count{0};

    rigidbody_t* characters[PHYSICS_MAX_CHARACTER_COUNT];
    size_t       character_count{0};
};

using init_function = void(__cdecl *)(api_t* api, backend_type type, arena_t* arena);

};

#endif 