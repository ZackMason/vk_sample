#ifndef RENDERING_PARTICLE_HPP
#define RENDERING_PARTICLE_HPP

#include "ztd_core.hpp"

DEFINE_TYPED_ID(particle_system_id);

struct particle_t {
    v3f position{0.0f};
    f32 life_time{};

    quat orientation{};
    
    v4f color{1.0f};
    
    f32 scale{1.0f};
    v3f velocity{};
    
    v3f angular_velocity{};
    u32 sprite_index{0};
};

enum struct particle_emitter_type {
    sphere, box,
};

struct flat_particle_system_t {
    particle_t particles[1024];
};

enum ParticleSettingsFlags : u32 {
    ParticleSettingsFlags_None = 0,
    ParticleSettingsFlags_WorldSpace    = 1 << 0,
    ParticleSettingsFlags_SampleOnSpawn = 1 << 1,
    ParticleSettingsFlags_AdditiveBlend = 1 << 2,
};

struct particle_system_settings_t {
    u64 VERSION{4};

    particle_t template_particle;

    u32 flags{0}; // sneak added @Version 4
    // u32 world_space{0}; // sneak @Version 4
    b32 is_world_space() const {
        return (flags & ParticleSettingsFlags_WorldSpace) > 0;
    }

    u32 max_count{1024};

    math::rect3d_t aabb{};

    v3f acceleration{axis::down * 9.81f};
    math::rect3d_t velocity_random{v3f{0.0f}, v3f{0.0f}};
    math::rect3d_t angular_velocity_random{v3f{0.0f}, v3f{0.0f}};
    u32 stream_rate{1}; // number of particles spawned before the system picks a new velocity, useful for streams

    f32 spawn_rate{0.0f};

    math::range_t life_random{0.0f, 0.0f};

    math::range_t scale_over_life_time{1.0f, 1.0f};

    particle_emitter_type emitter_type{particle_emitter_type::sphere};
    union {
        math::sphere_t sphere{v3f{0.0f}, 1.0f};
        math::rect3d_t box;
    };

    gfx::color::color_variant_t particle_color{};
};

template<>
void utl::memory_blob_t::serialize<particle_system_settings_t>(arena_t* arena, const particle_system_settings_t& settings) {
    serialize(arena, particle_system_settings_t{}.VERSION);
    serialize(arena, settings.template_particle);
    serialize(arena, settings.flags);
    serialize(arena, settings.max_count);
    serialize(arena, settings.aabb);
    serialize(arena, settings.acceleration);
    serialize(arena, settings.velocity_random);
    serialize(arena, settings.angular_velocity_random);
    serialize(arena, settings.stream_rate);
    serialize(arena, settings.spawn_rate);
    serialize(arena, settings.life_random);
    serialize(arena, settings.scale_over_life_time);
    serialize(arena, settings.emitter_type);
    serialize(arena, settings.box);
    serialize(arena, settings.particle_color._type);
    using namespace gfx::color;
    switch(settings.particle_color._type) {
        case color_variant_type::uniform:
            serialize(arena, settings.particle_color.uniform);
            break;
        case color_variant_type::range:
            serialize(arena, settings.particle_color.uniform);
            break;
        case color_variant_type::hex:
            serialize(arena, settings.particle_color.hex);
            break;
        case color_variant_type::gradient:
            serialize(arena, settings.particle_color.gradient);
            break;
    }
}

template<>
particle_system_settings_t 
utl::memory_blob_t::deserialize<particle_system_settings_t>(arena_t* arena) {
    particle_system_settings_t settings{};

    #define DESER(x) settings.x = deserialize<decltype(settings.x)>()
    
        DESER(VERSION);

        ztd_info(__FUNCTION__, "Loading particle version: {}", settings.VERSION);

        constexpr u64 v3_particle_size = 76;// sizeof(particle_t);

        if (settings.VERSION == 0) {
            constexpr u64 v0_size = 232;
            read_offset -= sizeof(u64);
            utl::copy(&settings, data+read_offset, v0_size);
            advance(v0_size);
            assert(!"I dont think this should run");
        } else {
            if (settings.VERSION <= 3) {
                utl::copy(&settings.template_particle, data+read_offset, v3_particle_size);
                advance(v3_particle_size);
                settings.template_particle.color.a = 1.0f;
            } else {
                DESER(template_particle);
            }
            DESER(flags);
            DESER(max_count);
            DESER(aabb);
            DESER(acceleration);
            DESER(velocity_random);
            DESER(angular_velocity_random);
            DESER(stream_rate);
            DESER(spawn_rate);
            DESER(life_random);
            DESER(scale_over_life_time);
            DESER(emitter_type);
            DESER(box);
            if (settings.VERSION > 1) {
                // DESER(particle_color._type);
                gfx::color::color_variant_type color_type = deserialize<gfx::color::color_variant_type>();
            
                settings.particle_color.set_type(color_type);
                using namespace gfx::color;
                switch(settings.particle_color._type) {
                    case color_variant_type::uniform:
                        DESER(particle_color.uniform);
                        break;
                    case color_variant_type::range:
                        DESER(particle_color.uniform);
                        break;
                    case color_variant_type::hex:
                        DESER(particle_color.hex);
                        break;
                    case color_variant_type::gradient:
                        // DESER(particle_color.gradient);
                        if (settings.VERSION == 2) {
                            assert(!"Loaded degen gradient");
                            advance(sizeof(gfx::color::gradient_t));
                        } else {
                            settings.particle_color.gradient = deserialize<gfx::color::gradient_t>(arena);
                        }
                        break;
                }
            }
        }

    #undef DESER

    return settings;
}


struct particle_system_t : public particle_system_settings_t {
    // particle_t* particles;
    buffer<particle_t> particles;
    utl::rng::random_t<utl::rng::xor64_random_t> rng;
    u32 live_count{0};
    u32 instance_offset{0};
    f32 spawn_timer{spawn_rate};

    u32 _stream_count{0};
    v3f _current_velocity{};
    v3f _current_position{};

    particle_system_t*
    clone(arena_t* arena, arena_t* particle_arena = 0);
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
    tag_struct(auto* cache, particle_cache_t, arena);
    
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
inline static void 
particle_system_spawn(
    particle_system_t* system,
    const m44& transform
) {
    const b32 world_space = system->is_world_space();
    // const m44 world_inv = glm::inverse(transform);

    auto* particle = system->particles.data + system->live_count++;
    *particle = system->template_particle;
    
    particle->life_time -= system->rng.range(system->life_random);

    if (++system->_stream_count >= system->stream_rate) {
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

    if (world_space) {
        particle->position = transform * v4f{particle->position, 1.0f};
        particle->velocity = transform * v4f{particle->velocity, 0.0f};
        particle->angular_velocity = transform * v4f{particle->angular_velocity, 0.0f};
    }
}

inline static void 
particle_system_kill_particle(
    particle_system_t* system,
    u64 i
) {
    if (system->live_count) {
        std::swap(system->particles.data[i], system->particles.data[system->live_count-1]);
        system->live_count--;
    }
}

inline static void 
particle_system_update(
    particle_system_t* system,
    const m44& transform,
    f32 dt
) {
    system->spawn_timer -= dt;
    system->aabb = {};

    while (system->spawn_timer <= 0.0f) {
        system->spawn_timer += system->spawn_rate;

        if (system->live_count < system->max_count) {
            particle_system_spawn(system, transform);
        } else {
            break;
        }

        // special case
        // to stop infinite loop when spawn_rate == 0.0f
        if (system->spawn_rate <= 0.0f && system->stream_rate > 1) {
            system->spawn_timer = 0.0f;
            break; 
        }
    }

    range_u64(i, 0, system->live_count) {
        restart_particle_update:
        auto* particle = system->particles.data + i;
        particle->life_time -= dt;

        // Note(Zack): kill particle and restart loop
        if (particle->life_time <= 0.0f) {
            if (0 != --system->live_count) {
                std::swap(*particle, system->particles.data[system->live_count]);
                goto restart_particle_update;
            }
        }

        const f32 life_alpha = particle->life_time / (system->template_particle.life_time);

        particle->velocity += system->acceleration * dt;
        particle->position += particle->orientation * (particle->velocity * dt);
        particle->orientation += (particle->orientation * glm::quat(0.0f, particle->angular_velocity)) * (0.5f * dt);
        particle->orientation = glm::normalize(particle->orientation);
        particle->scale = system->template_particle.scale * system->scale_over_life_time.sample(1.0f-life_alpha);
        // particle->color = v4f{1.0f};
        particle->color = system->particle_color.sample(1.0f-life_alpha);

        // if (particle->position.y < 0.0f) {
        //     particle->velocity.y = 7.0f;
        // }
        system->aabb.expand(particle->position);
    }
}

inline static void 
particle_system_build_colors(
    particle_system_t* system, 
    rendering::instance_extra_data_t* data, u32 color_count
) {
    assert(system->live_count <= color_count);

    range_u32(i, 0, system->live_count) {
        auto* particle = system->particles.data + i;
        data[i].color = particle->color;
    }
}

inline static void 
particle_system_build_matrices(
    particle_system_t* system, 
    const m44& transform,
    m44* matrix, u32 matrix_count
) {
    assert(system->live_count <= matrix_count);

    const b32 world_space = system->is_world_space();

    range_u32(i, 0, system->live_count) {
        auto* particle = system->particles.data + i;
        matrix[i] = math::transform_t{}
            .translate(particle->position)
            .rotate_quat(particle->orientation)
            .scale(v3f{particle->scale})
            .to_matrix();

        if (!world_space) {
            matrix[i] = transform * matrix[i];
        }
    }

}

inline static void
particle_system_sort_view(
    particle_system_t* system,
    const m44& transform,
    v3f camera_pos, v3f camera_forward
) {
    const b32 world_space = system->is_world_space();

    std::span<particle_t> view{system->particles.data, system->live_count};

    std::sort(view.begin(), view.end(), [&](auto& p0, auto& p1) {
        v3f pp0 = p0.position;
        v3f pp1 = p1.position;
        if (!world_space) {
            pp0 = transform * v4f{pp0, 1.0f};
            pp1 = transform * v4f{pp1, 1.0f};
        }
        auto d0 = pp0 - camera_pos;
        auto d1 = pp1 - camera_pos;
        return glm::dot(d0, camera_forward) > glm::dot(d1, camera_forward);
        return glm::dot(d0, camera_forward) < glm::dot(d1, camera_forward);
    });
}

inline static particle_system_t*
particle_system_create(
    arena_t* system_arena,
    const u32 max_particle_count = 1024,
    arena_t* particle_arena = 0,
    u64 seed = 0
) {
    tag_struct(auto* system, particle_system_t, system_arena);
    tag_array(system->particles.data, particle_t, particle_arena?particle_arena:system_arena, max_particle_count);

    system->particles.count =
        system->max_count = max_particle_count;
    system->live_count = 0;
    system->spawn_timer = 0.0f;

    range_u64(i, 0, max_particle_count) {
        system->particles.data[i].life_time = 0.0f;
    }

    if (seed) system->rng.seed(seed);

    return system;
}

particle_system_t* 
particle_system_t::clone(
    arena_t* arena,
    arena_t* particle_arena
) {
    auto* ps = particle_system_create(
        arena,
        max_count,
        particle_arena
    );

    particle_system_settings_t& settings = *ps;
    settings = particle_system_settings_t{*this};

    return ps;
}

#endif
