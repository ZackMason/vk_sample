#ifndef RENDERING_PARTICLE_HPP
#define RENDERING_PARTICLE_HPP

#include "core.hpp"

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

struct particle_system_t {
    particle_t template_particle;

    particle_t* particles;

    v3f acceleration{axis::down * 0.5f * 9.81f};
    math::aabb_t<v3f> velocity_random{v3f{0.0f}, v3f{0.0f}};
    math::aabb_t<v3f> angular_velocity_random{v3f{0.0f}, v3f{0.0f}};

    f32 spawn_rate{1.0f};
    f32 spawn_timer{spawn_rate};

    math::aabb_t<f32> life_random{0.0f, 0.0f};

    math::aabb_t<f32> scale_over_life_time{1.0f, 1.0f};

    u32 max_count{};
    u32 live_count{};
    math::aabb_t<v3f> aabb{};

    utl::rng::random_t<utl::rng::xor64_random_t> rng;

    particle_emitter_type emitter_type{particle_emitter_type::sphere};
    union {
        math::sphere_t sphere{v3f{0.0f}, 1.0f};
        math::aabb_t<v3f> box;
    };
};

// user needs to check can spawn before calling
inline static void particle_system_spawn(
    particle_system_t* system
) {
    auto* particle = system->particles + system->live_count++;
    *particle = system->template_particle;
    particle->life_time += system->rng.range(system->life_random);
    particle->velocity += system->rng.aabb(system->velocity_random);
    particle->angular_velocity += system->rng.aabb(system->angular_velocity_random);
}

inline static void particle_system_update(
    particle_system_t* system,
    f32 dt
) {
    system->spawn_timer -= dt;

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
            .set_scale(v3f{particle->scale})
            .rotate_quat(particle->orientation)
            .to_matrix();
    }
}

inline static particle_system_t*
particle_system_create(
    arena_t* system_arena,
    u32 max_particle_count,
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
