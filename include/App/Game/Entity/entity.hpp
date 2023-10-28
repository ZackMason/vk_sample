#pragma once

#include "zyy_core.hpp"

#include "uid.hpp"

#include "App/Game/Util/camera.hpp"
#include "App/Game/Weapons/base_weapon.hpp"
#include "App/Game/Items/base_item.hpp"
#include "App/Game/Entity/entity_concept.hpp"
#include "App/Game/Entity/brain.hpp"
#include "App/Game/Rendering/particle.hpp"
#include "App/Game/Rendering/lighting.hpp"


#include <variant>

namespace zyy {

struct world_t;
struct world_t;

// using entity_update_function = void(*)(entity_t*, world_t*);
// using entity_interact_function = void(*)(entity_t*, world_t*);

struct entity_t {
    entity_id   id{uid::invalid_id};
    entity_type type{entity_type::SIZE};
    u64         flags{0};
    u64         tag{0};
    string_t    name;

#if ZYY_INTERNAL
    DEBUG_entity_meta_info_t _DEBUG_meta{};
#endif

    world_t*    world{0};

    entity_t*   next{nullptr};
    entity_t*   parent{nullptr};
    entity_t*   next_id_hash{nullptr};

    entity_t* first_child{nullptr};
    entity_t* next_child{nullptr};

    brain_id    brain_id{uid::invalid_id};
    brain_t     brain{};

    math::transform_t   transform;
    // math::transform_t   _global_transform;

    math::rect3d_t        aabb;

    void dirty_transform() {
        flags |= EntityFlags_DirtyTransform;
        
        for (auto* child = first_child; child; node_next(child)) {
            child->dirty_transform();
        }
    }

    math::transform_t global_transform() const {
        if (parent) {
            return math::transform_t{parent->global_transform().to_matrix() * transform.to_matrix()};
        }
        return transform;
    }

    struct renderable_t {
        u64 gfx_id{0};
        u64 gfx_entity_count{0};
        u64 mesh_id{0};
        u32 albedo_id{0};
        u32 material_id{0};
        u32 animation_id{0};
        // u32 object_id{0};
        // u32 indirect_start{0};

        particle_system_id particle_system_id{uid::invalid_id};
        particle_system_t* particle_system{0};

        rendering::lighting::point_light_t* light{0};        

        u32 _instance_count{1};
        union
        {
            struct {
                m44* dynamic_instance_buffer;
                m44* buffer; // non moving buffer
            };
            m44* instance_buffer;
        };
        u32 instance_buffer_offset{0};

        enum RenderFlag {
            RenderFlag_Instance = 1,
            RenderFlag_DynamicInstance = 2,
        };

        u32 render_flags = 0;

        void swap() {
            if (dynamic_instance_buffer == buffer) {
                dynamic_instance_buffer = buffer + _instance_count;
            } else {
                dynamic_instance_buffer = buffer;
            }
        }

        void instance(utl::pool_t<m44>& pool, u32 count, bool dynamic = false) {
            render_flags |= dynamic ? RenderFlag_DynamicInstance : RenderFlag_Instance;
            render_flags &= !dynamic ? ~RenderFlag_DynamicInstance : ~RenderFlag_Instance;

            instance_buffer_offset = safe_truncate_u64(pool.count());

            if (dynamic) {
                dynamic_instance_buffer = pool.allocate(count * 2);
                std::fill(dynamic_instance_buffer, dynamic_instance_buffer+count*2+1, m44{1.0f});
                buffer = dynamic_instance_buffer;
            } else {
                buffer = instance_buffer = pool.allocate(count);
                std::fill(buffer, buffer+count+1, m44{1.0f});
            }
            _instance_count = count;
        }

        std::byte* instance_end() {
            if (render_flags & RenderFlag_DynamicInstance) {
                return (std::byte*)(buffer + _instance_count * 2);
            } else {
                return (std::byte*)(buffer + _instance_count);
            }
        }

        u32 instance_offset() {
            if (render_flags & RenderFlag_DynamicInstance) {
                auto ret = dynamic_instance_buffer == buffer ? 0 : _instance_count;
                swap();
                return ret + instance_buffer_offset;
            } else {
                return instance_buffer_offset;
            }
        }

        u32 instance_count() {
            return (particle_system ? particle_system->live_count : _instance_count); 
        }


        
    } gfx{};

    struct physics_t {
        u32 flags{0};
        physics::rigidbody_t* rigidbody;
        v3f impulse{0.0f};

        void queue_free() {
            flags |= PhysicsEntityFlags_Dying;
        }
    } physics{};

    struct stats_t {
        character_stats_t character{};
        wep::base_weapon_t weapon{};
        item::effect_t effect{};
    } stats{};

    entity_ref_t primary_weapon{0};
    entity_ref_t secondary_weapon{0};

    cam::first_person_controller_t camera_controller;

    std::optional<entity_coroutine_t> coroutine{};


    // virtual ~entity_t() = default;
    // constexpr entity_t() noexcept = default;

    constexpr bool is_renderable() const noexcept {
        return (gfx.mesh_id != -1) || gfx.particle_system != nullptr;
    }

    constexpr bool is_alive() const noexcept {
        return id != uid::invalid_id &&
            ((flags & (EntityFlags_Dying|EntityFlags_Dead)) == 0);
    }

    void queue_free(b32 and_children = 1) noexcept {
        assert(uid::is_valid(id));
        assert(((flags & EntityFlags_Dead) == 0) && "Should not have access to dead entities");
        // zyy_info(__FUNCTION__, "Killing entity: {} - {}", (void*)this, name.c_str() ? name.c_str() : "<null>");
        flags |= EntityFlags_Dying;
        
        if (and_children) {
            auto* child = first_child;
            for (;child; child = child->next_child) {
                child->queue_free();
            }
        }
    }

    void remove_child(entity_t* child) {
        auto child_transform = child->global_transform();
        entity_t* last=first_child;
        if (last==child) {
            first_child = first_child->next_child;
        } else {
            for (auto* c = first_child; c; c = c->next_child) {
                if (c == child) {
                    last->next_child = c->next_child;
                    break;
                }
                last = c;
            }
        }
        child->parent = nullptr;
        child->transform = child_transform;
    }

    void add_child(entity_t* child, bool maintain_world_pos = false) {
        assert(child->is_alive());
        assert(child->parent == nullptr && "reassignment is not supported yet"); 
        assert(child->next_child == nullptr && "reassignment is not supported yet");
        child->next_child = first_child;            
        first_child = child;

        child->parent = this;
        if (!maintain_world_pos) {
            child->transform = math::transform_t{};
        }
    }
};

static_assert(CEntity<entity_t>);

// inline void 
// entity_add_coroutine(entity_t* entity, entity_coroutine_t* coro) {
//     node_push(coro, entity->coroutine);
// }

inline void
entity_init(entity_t* entity, u64 mesh_id = -1) {
    // entity->flags |= EntityFlags_Spatial;
    // entity->flags |= mesh_id != -1 ? EntityFlags_Renderable : 0;
    entity->gfx.mesh_id = mesh_id;
    entity->flags = 0;
}

inline void 
player_init(
    entity_t* player, 
    cam::camera_t* camera, 
    u64 mesh_id) {

    player->type = entity_type::player;

    player->gfx.mesh_id = mesh_id;
    player->camera_controller.camera = camera;
}

u32
entity_child_count(
    entity_t* entity
) {
    u32 result = 0;
    for (auto* e = entity->first_child; e; e = e->next_child) {
        result++;
    }
    return result;
}

}; // namespace zyy