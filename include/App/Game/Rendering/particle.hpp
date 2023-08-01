#ifndef RENDERING_PARTICLE_HPP
#define RENDERING_PARTICLE_HPP

#include "zyy_core.hpp"

DEFINE_TYPED_ID(particle_system_id);

struct particle_t {
    v3f position{0.0f};
    f32 life_time{};
    quat orientation{};
    v4f color{};
    f32 scale{1.0f};
    v3f velocity{};
    v3f angular_velocity{};
};

enum struct particle_emitter_type {
    sphere, box,
};

struct flat_particle_system_t {
    particle_t particles[1024];
};

struct particle_system_t {
    particle_t template_particle;

    particle_t* particles;
    u32 max_count{1024};
    u32 live_count{};
    u32 instance_offset{0};

    math::aabb_t<v3f> aabb{};

    v3f acceleration{axis::down * 0.5f * 9.81f};
    math::aabb_t<v3f> velocity_random{v3f{0.0f}, v3f{0.0f}};
    math::aabb_t<v3f> angular_velocity_random{v3f{0.0f}, v3f{0.0f}};
    u32 stream_rate{1}; // number of particles spawned before the system picks a new velocity, useful for streams

    f32 spawn_rate{1.0f};
    f32 spawn_timer{spawn_rate};

    math::aabb_t<f32> life_random{0.0f, 0.0f};

    math::aabb_t<f32> scale_over_life_time{1.0f, 1.0f};

    utl::rng::random_t<utl::rng::xor64_random_t> rng;

    particle_emitter_type emitter_type{particle_emitter_type::sphere};
    union {
        math::sphere_t sphere{v3f{0.0f}, 1.0f};
        math::aabb_t<v3f> box;
    };

    u32 _stream_count{0};
    v3f _current_velocity{};
    v3f _current_position{};
};

struct particle_system_info_t {
    particle_system_id  id{uid::invalid_id};
    u32                 frames_since_touch{0};
};

static constexpr u64 MAX_PARTICLE_SYSTEM_COUNT = 128;
struct particle_cache_t {
    particle_system_info_t  particle_system_infos[MAX_PARTICLE_SYSTEM_COUNT];
    flat_particle_system_t  particle_systems[MAX_PARTICLE_SYSTEM_COUNT];

    m44*                    instance_buffer{0};
};

inline static particle_cache_t*
particle_cache_create(
    arena_t* arena
) {
    auto* cache = arena_alloc<particle_cache_t>(arena);
    
    return cache;
}

inline static flat_particle_system_t*
init_particle_system(
    particle_cache_t* cache,
    particle_system_id id,
    flat_particle_system_t* system
) {
    u32 index = (u32)(system - cache->particle_systems);

    cache->particle_system_infos[index].id = id;
    cache->particle_system_infos[index].frames_since_touch = 0;

    return system;
}

inline static void
touch_particle_system(
    particle_cache_t* cache,
    flat_particle_system_t* system
) {
    u32 index = (u32)(system - cache->particle_systems);
    cache->particle_system_infos[index].frames_since_touch = 0;
}

inline static void
update_particle_cache(
    particle_cache_t* cache
) {
    range_u64(i, 0, array_count(cache->particle_system_infos)) {
        auto* info = cache->particle_system_infos + i;
        info->frames_since_touch++;
    }
}

inline static flat_particle_system_t*
get_or_create_particle_system_from_cache(
    particle_cache_t* cache,
    particle_system_id id,
    particle_system_t* system,
    b32 create_if_no_found
) {
    flat_particle_system_t* replace = 0;
    u32 min_frame = std::numeric_limits<u32>::max();

    range_u64(i, 0, array_count(cache->particle_system_infos)) {
        auto* info = cache->particle_system_infos + i;
        if (info->id == id) {
            return cache->particle_systems + i;
        }
        if (info->frames_since_touch < min_frame) {
            min_frame = info->frames_since_touch;
            replace = cache->particle_systems + i;
        }
    }

    if (create_if_no_found) {
        replace = init_particle_system(cache, id, replace);
        u32 index = (u32)(replace - cache->particle_systems);
        system->instance_offset = index * 1024;
    }

    return replace;
}

// user needs to check can spawn before calling
inline static void particle_system_spawn(
    particle_system_t* system
) {
    auto* particle = system->particles + system->live_count++;
    *particle = system->template_particle;
    
    particle->life_time += system->rng.range(system->life_random);

    if (++system->_stream_count == system->stream_rate) {
        system->_stream_count = 0;
        system->_current_velocity = system->rng.aabb(system->velocity_random);
        switch (system->emitter_type) {
        case particle_emitter_type::box:
            system->_current_position = system->rng.aabb(system->box);
            break;
        case particle_emitter_type::sphere:
            system->_current_position = system->sphere.origin + system->rng.randnv<v3f>() * system->sphere.radius;
            break;
        case_invalid_default;
        }
    }
    particle->position += system->_current_position; 
    particle->velocity += system->_current_velocity;
    particle->angular_velocity += system->rng.aabb(system->angular_velocity_random);
}

inline static void particle_system_update(
    particle_system_t* system,
    f32 dt
) {
    system->spawn_timer -= dt;
    system->aabb = {};

    while (system->spawn_timer <= 0.0f) {
        system->spawn_timer += system->spawn_rate;
        if (system->live_count < system->max_count) {
            particle_system_spawn(system);
        }
    }

    range_u64(i, 0, system->live_count) {
        restart_particle_update:
        auto* particle = system->particles + i;
        particle->life_time -= dt;

        // Note(Zack): kill particle and restart loop
        if (particle->life_time <= 0.0f) {
            std::swap(*particle, system->particles[system->live_count-1]);
            if (0 != --system->live_count) {
                goto restart_particle_update;
            }
        }

        const f32 life_alpha = particle->life_time / (system->template_particle.life_time + system->life_random.max);

        particle->velocity += system->acceleration * dt;
        particle->position += particle->velocity * dt;
        particle->orientation += (particle->orientation * glm::quat(0.0f, particle->angular_velocity)) * (0.5f * dt);
        particle->orientation = glm::normalize(particle->orientation);
        particle->scale = system->template_particle.scale * system->scale_over_life_time.sample(1.0f-life_alpha);

        if (particle->position.y < 0.0f) {
            particle->velocity.y = 7.0f;
        }
        system->aabb.expand(particle->position);
    }
}

inline static void 
particle_system_build_matrices(
    particle_system_t* system, 
    m44* matrix, u32 matrix_count
) {
    assert(system->live_count <= matrix_count);

    range_u32(i, 0, system->live_count) {
        auto* particle = system->particles + i;
        matrix[i] = math::transform_t{}
            .translate(particle->position)
            .rotate_quat(particle->orientation)
            .set_scale(v3f{particle->scale})
            .to_matrix();
    }
}

inline static particle_system_t*
particle_system_create(
    arena_t* system_arena,
    const u32 max_particle_count = 1024,
    arena_t* particle_arena = 0,
    u64 seed = 0
) {
    auto* system = arena_alloc<particle_system_t>(system_arena);
    system->particles = arena_alloc<particle_t>(particle_arena?particle_arena:system_arena, max_particle_count);

    system->max_count = max_particle_count;
    system->live_count = 0;

    if (seed) system->rng.seed(seed);

    return system;
}

#endif
