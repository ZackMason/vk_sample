#ifndef APP_PHYSICS_HPP
#define APP_PHYSICS_HPP


#include "uid.hpp"

#define PHYSICS_MAX_RIGIDBODY_COUNT (4096<<2)
#define PHYSICS_MAX_COLLIDER_COUNT 256
#define PHYSICS_MAX_CHARACTER_COUNT 512

namespace physics {

struct api_t;

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
struct collider_box_info_t {
    v3f size;
};

enum struct rigidbody_type {
    DYNAMIC, STATIC, KINEMATIC, CHARACTER, SIZE
};

struct rigidbody_t;

// Note(Zack): These are the frontend structs that the game code will interact with
DEFINE_TYPED_ID(collider_id);
struct collider_t {
    collider_shape_type type;
    collider_id         id{uid::invalid_id};
    void*               shape;

    rigidbody_t*        rigidbody{0};

    u32                 is_trigger{0};

    void set_trigger(bool x);
    void set_active(bool x);

    // Todo(Zack)
    // v3f                 offset;
    // glm::quat           orientation;
};

using rigidbody_on_collision_function = void(*)(rigidbody_t*, rigidbody_t*);
using rigidbody_set_active_function = void(*)(rigidbody_t*, bool);

struct rigidbody_flags {
    enum {
        ACTIVE = BIT(0),     // todo(Zack): this can be removed
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
    u64             layer{0};
    api_t*          api{0};

    f32 mass{1.0f};
    f32 linear_dampening{0.5f};
    f32 angular_dampening{0.5f};

    v3f             position{0.0f};
    quat            orientation{};

    v3f             velocity{0.0f};
    v3f             angular_velocity{0.0f};

    v3f             force{0.0f};
    v3f             torque{0.0f};

    m33             inertia{0.0f}, inverse_inertia{0.0f};

    std::array<collider_t, RIGIDBODY_MAX_COLLIDER_COUNT>      colliders;
    size_t          collider_count{0};

    const void*     user_data{0};
    const void*     api_data{0}; // stores the pxActor


    rigidbody_on_collision_function on_trigger{0};
    rigidbody_on_collision_function on_trigger_end{0};

    rigidbody_on_collision_function on_collision{0};
    rigidbody_on_collision_function on_collision_end{0};
    
    inline void add_force_ex(const v3f& force);
    inline void add_force_at_point_ex(const v3f& force, const v3f& point);

    // transform direction from body space to world space
    inline v3f transform_direction(const v3f& direction) const
    { return orientation * direction; }

    // transform direction from world space to body space
    inline v3f inverse_transform_direction(const v3f& direction) const
    { return glm::inverse(orientation) * direction; }

    // get velocity and angular velocity in body space
    // inline v3f get_point_velocity(const v3f& point) const
    // { return inverse_transform_direction(velocity) + glm::cross(angular_velocity, point); }

    // force and point vectors are in body space
    inline void add_force_at_point(const v3f& force_, const v3f& point)
    { add_force_at_point_ex(force_, point); }
    // { force += transform_direction(force_), torque += glm::cross(point, force_); }

    // force vector in body space
    // inline void add_relative_force(const v3f& force_)
    // { force += transform_direction(force_); }

    // force vector in body space
    // inline void add_relative_impulse(const v3f& force_, float dt)
    // { force += transform_direction(force_) * mass / dt; }

    // force vector in body space
    // inline void add_impulse(const v3f& force_, float dt)
    // { force += force_ * mass / dt; }

    // force vector in body space
    inline void add_force(const v3f& force_)
    { add_force_ex(force_); }
    // { force += force_; }

    inline void remove_collider(collider_t* collider);

    inline void set_transform(const m44& transform); 

    // Note(Zack): position is integrated by the api not here
    // Note(Zack): this should probably happen there instead of in game
    // Todo(Zack): allow custom gravity;
    void integrate(float dt, float gravity = 0.0f, v3f gravity_direction = axis::down) {
        v3f acceleration = force / mass;

        const bool is_on_ground = flags & rigidbody_flags::IS_ON_GROUND;

        if (!is_on_ground) { acceleration += gravity_direction * gravity; }
        velocity += acceleration * dt;
        // position += velocity * dt;

        angular_velocity += inverse_inertia *
            (torque - glm::cross(angular_velocity, inertia * angular_velocity)) * dt;
        // orientation += (orientation * glm::quat(0.0f, angular_velocity)) * (0.5f * dt);
        // orientation = glm::normalize(orientation);

        force = v3f{0.0f};
        torque = v3f{0.0f};

    }

    // rigidbody_t& operator=(const rigidbody_t& o) {
    //     if (this == &o) {return *this;}

    //     position = o.position;
    //     orientation = o.orientation;

    //     velocity = o.velocity;
    //     angular_velocity = o.angular_velocity;

    //     mass = o.mass;

    //     linear_dampening = o.linear_dampening;
    //     angular_dampening = o.angular_dampening;

    //     force = o.force;
    //     torque = o.torque;

    //     inertia = o.inertia;
    //     inverse_inertia = o.inverse_inertia;

    //     flags = o.flags;
    //     layer = o.layer;

    //     return *this;
    // }
};

// Todo(Zack): Change to hit result
struct raycast_result_t {
    bool hit{false};
    v3f point{};
    v3f normal{};
    f32 distance{};
    const void* user_data{0};
};

struct api_t;

// Note(Zack): Functions for the api to
using create_scene_function = void(*)(api_t*, const void* filter_fn);
using destroy_scene_function = void(*)(api_t*);

using create_rigidbody_function = rigidbody_t*(*)(api_t*, void* entity, rigidbody_type, const v3f& position, const quat& rotation);
using create_collider_function = collider_t*(*)(api_t*, rigidbody_t*, collider_shape_type, void* collider_info);
using simulate_function = void(*)(api_t*, f32 dt);
using raycast_world_function = raycast_result_t(*)(const api_t*, v3f ro, v3f rd);

using collider_set_trigger_function = void(*)(collider_t*, bool);

#if GEN_INTERNAL
using get_debug_table_function = debug_table_t*(*)(void);
using get_debug_table_size_function = size_t(*)(void);
#endif

// updates a rigidbody pos and orientation
using update_rigidbody_function = void(*)(api_t*, rigidbody_t*);
using rigidbody_add_force_function = void(*)(rigidbody_t*, const v3f&);
using rigidbody_add_force_at_point_function = void(*)(rigidbody_t*, const v3f&, const v3f&);

struct export_dll api_t {
    backend_type type;
    void*        backend{nullptr};

    void* user_world{nullptr};

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

    // rigidbody_set_active_function rigidbody_set_active{0};
    collider_set_trigger_function collider_set_trigger{0};
    collider_set_trigger_function collider_set_active{0};

    rigidbody_add_force_function rigidbody_add_force{0};
    rigidbody_add_force_at_point_function rigidbody_add_force_at_point{0};

    // TODO(Zack): Add hashes
    std::array<rigidbody_t, PHYSICS_MAX_RIGIDBODY_COUNT> rigidbodies;
    size_t      rigidbody_count{0};

    std::array<collider_t, PHYSICS_MAX_COLLIDER_COUNT>  colliders;
    size_t      collider_count{0};

    std::array<rigidbody_t*, PHYSICS_MAX_CHARACTER_COUNT> characters;
    size_t       character_count{0};

    using layer_mask_t = std::array<std::array<u8, 64>, 64>;
    layer_mask_t layer_mask{};

    size_t      entity_transform_offset{0};

    api_t& operator=(const api_t& o) {
        if (this == &o) { return *this; }

        // if (arena && o.arena) {
        //     std::memcpy(arena->start, o.arena->start, o.arena->top);
        //     arena->top = o.arena->top;
        //     arena->size = o.arena->size;
        // }

        rigidbodies = o.rigidbodies;
        colliders = o.colliders;
        characters = o.characters;

        rigidbody_count = o.rigidbody_count;
        collider_count = o.collider_count;
        character_count = o.character_count;

        layer_mask = o.layer_mask;
        return *this;
    }

#if GEN_INTERNAL
    get_debug_table_function get_debug_table{0};
    get_debug_table_size_function get_debug_table_size{0};
#endif
};

using init_function = void(__cdecl *)(api_t* api, backend_type type, arena_t* arena);

void collider_t::set_trigger(bool x) {
    rigidbody->api->collider_set_trigger(this, x);
}

void collider_t::set_active(bool x) {
    rigidbody->api->collider_set_active(this, x);
}

void rigidbody_t::set_transform(const m44& transform) {
    position = v3f{transform[3]};
    orientation = glm::quat_cast(transform);

    this->api->set_rigidbody(this->api, this);
}

void rigidbody_t::add_force_ex(const v3f& f) {
    this->api->rigidbody_add_force(this, f);
}
void rigidbody_t::add_force_at_point_ex(const v3f& f, const v3f& p) {
    this->api->rigidbody_add_force_at_point(this, f, p);
}

};

#endif 