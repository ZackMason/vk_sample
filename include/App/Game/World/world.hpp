#pragma once

#include "ztd_core.hpp"

#include "App/game_state.hpp"
#include "App/Game/Entity/entity.hpp"
#include "App/Game/Entity/ztd_entity_prefab.hpp"
#include "App/Game/Rendering/render_system.hpp"


struct game_state_t;

struct world_generator_t;

namespace ztd {

    enum physics_layers : u32 {
        none = 0ui32,
        player = 1ui32,
        enemy = 2ui32,
        player_bullets = 4ui32,
        enemy_bullets = 8ui32,
        environment = (1ui32 << 31ui32),
        everything = 0xffff'ffffui32,        
    };

    constexpr u32 player_collision_group = ~physics_layers::player_bullets;
    constexpr u32 enemy_collision_group = ~physics_layers::enemy_bullets;
    constexpr u32 player_bullet_collision_group = ~physics_layers::player & ~physics_layers::player_bullets;
    constexpr u32 enemy_bullet_collision_group = ~physics_layers::enemy & ~physics_layers::enemy_bullets;

    struct prefab_loader_t {
        utl::hash_trie_t<std::string_view, ztd::prefab_t>* map = 0; 

        ztd::prefab_t load(arena_t* arena, std::string_view name) {
            b32 had = 0;
            auto* prefab = utl::hash_get(&map, name, arena, &had);
            
            if (had == 0) {
                *prefab = load_from_file(arena, name);
            }
            return *prefab;
        }
    };

    static constexpr u64 max_entities = 15000;

    struct entity_block_t {
        entity_block_t* next;
        entity_block_t* prev;

        entity_id   id;
        entity_t    entities[32];
        u32         free_mask{~0ui32};

        math::rect3d_t aabb;

        u32 first_free_index() {
            range_u32(i, 0, 32) {
                if ((1<<i) & free_mask) {
                    return i;
                }
            }
            return ~0ui32;
        }
    };

    enum struct event_type {
        invalid, hit, damage,
    };

    struct hit_event_t {
        v3f point{0.0f};
    };

    struct damage_event_t {
        v3f point{0.0f};
        f32 damage{0.0f};  
    };

    struct event_t {
        event_t*   next{0};

        entity_t*  entity{0};

        event_type type{event_type::invalid};
        
        union {
            hit_event_t hit_event{};
            damage_event_t damage_event;
        };
    };
    
    struct world_path_t {
        world_path_t* next{0};
        world_path_t* prev{0};

        u64 VERSION{0};

        u32 count{0};
        v3f* points{0};

        void add_point(arena_t* arena, v3f point) {
            tag_array(auto* new_points, v3f, arena, count+1);
            if (count > 0) [[unlikely]] {
                utl::copy(new_points, points, sizeof(v3f) * count);
            }
            new_points[count] = point;
            count++;
            points = new_points;
        }
    };

    // note(zack): everything I need to rebuild game state should be in this struct
    struct world_t {
        game_state_t* game_state;

        arena_t arena;
        arena_t particle_arena;
        frame_arena_t frame_arena;

        prefab_loader_t prefab_loader{};

        size_t          entity_count{0};
        size_t          entity_capacity{0};
        ztd::entity_id next_entity_id{1};
        ztd::entity_t  entities[max_entities]; // entities from [0,cap]
        ztd::entity_t* entity_id_hash[max_entities*2];
        ztd::entity_t* free_entities{0};

        event_t* events{0};

        // umm           brain_count{0};
        umm           brain_capacity{0};
        // brain_t  brains[4096];
        // brain_t* free_brain{0};

        ztd::entity_t* player;

        ztd::cam::camera_t camera;

        physics::api_t* physics{0};

        utl::rng::random_t<utl::rng::xor64_random_t> entropy;

        world_generator_t* world_generator{0};

        particle_cache_t* particle_cache{0};

        struct effects_buffer_t {
            m44* blood_splats{0};
            rendering::instance_extra_data_t* blood_colors{0};
            u32 blood_entity{0};
            umm blood_splat_count{0};
            umm blood_splat_max{0};
        } effects;

        f32 time() const {
            return game_state->time;
        }

        std::array<gfx::render_group_t, 16> render_groups = {};

        rendering::system_t* 
        render_system() {
            return game_state->render_system;
        }
        // world_t() = default;
    };

    event_t* new_event(world_t* world, entity_t* entity, event_type type) {
        auto* arena = &world->frame_arena.get();

        tag_struct(auto* event, event_t, arena);

        event->type = type;
        event->entity = entity;

        node_push(event, world->events);

        return event;
    }

    static void 
    add_entity_to_id_hash(world_t* world, entity_t* entity) {
        // todo(zack): better hash,
        const u64 id_hash = utl::rng::fnv_hash_u64(entity->id);
        const u64 id_bucket = id_hash & (array_count(world->entity_id_hash)-1);

        if (world->entity_id_hash[id_bucket]) {
            entity->next_id_hash = world->entity_id_hash[id_bucket];
        }
        world->entity_id_hash[id_bucket] = entity;
    }

    // static brain_t*
    // world_find_brain(world_t* world, brain_id id) {
    //     range_u64(i, 0, world->brain_capacity) {
    //         if (world->brains[i].id == id) {
    //             return world->brains + i;
    //         }
    //     }
    //     return nullptr;
    // }


    static entity_t*
    world_create_entity(world_t* world) {
        TIMED_FUNCTION;
        assert(world);
        assert(world->entity_capacity < array_count(world->entities));
        entity_t* e{0};

        if (world->free_entities) {
            node_pop(e, world->free_entities);
            world->entity_count++;
            assert(e);
            new (e) entity_t;
            e->id = world->next_entity_id++;
            add_entity_to_id_hash(world, e);
        } else {
            e = world->entities + world->entity_capacity++;
            world->entity_count++;
            assert(e);
            new (e) entity_t;
            e->id = world->next_entity_id++;
            add_entity_to_id_hash(world, e);
        }

        e->world = world;
        // e->gfx.particle_system = 0;
        return e;
    }

    static brain_id
    world_new_brain(world_t* world, brain_type type) {
        return uid::new_generation(world->brain_capacity); 
    }

    static void
    world_new_brain(world_t* world, ztd::entity_t* entity, brain_type type) {
        assert(entity);
        assert(world);

        entity->brain.type = type;
        brain_init(&world->arena, entity, &entity->brain);
        auto brain_id = entity->brain.id = entity->brain_id = world_new_brain(world, type);
        ztd_info(__FUNCTION__, "Brain {} activated", brain_id);
    }
        
    static entity_t*
    spawn(
        world_t* world,
        rendering::system_t* rs,
        const ztd::prefab_t& def,
        const v3f& pos = {},
        const m33& basis = m33{1.0f}
    ) {
        using namespace std::string_view_literals;
        TIMED_FUNCTION;
        utl::res::pack_file_t* resource_file = world->game_state->resource_file;
        auto* mod_loader = &world->game_state->modding.loader;

        entity_t* entity = world_create_entity(world);

        entity->coroutine = std::nullopt;

        assert(entity);
        assert(uid::is_valid(entity->id));
        assert(entity->parent == nullptr);
        assert(entity->first_child == nullptr);
        assert(entity->next_child == nullptr);

        entity_init(entity, rendering::get_mesh_id(rs, def.gfx.mesh_name.view()));

        entity->flags = def.flags;

        entity->gfx.material_id = def.gfx.material_id;
        entity->gfx.gfx_id  = rendering::register_entity(rs);
        entity->gfx.gfx_entity_count = 1;

        if (def.gfx.albedo_texture != ""sv) {
            auto aid = (u32)rs->texture_cache.get_id(def.gfx.albedo_texture.view());
            if (aid == 0) {
                tag_array(auto* texture, char, &rs->arena, def.gfx.albedo_texture.size()+1);
                utl::copy(texture, def.gfx.albedo_texture.buffer, def.gfx.albedo_texture.size());
                texture[def.gfx.albedo_texture.size()] = '\0';
                aid = (u32)rs->texture_cache.load(&rs->arena, *rs->vk_gfx, texture);
            }
            entity->gfx.albedo_id = aid;
        }

        if (def.gfx.mesh_name != ""sv) {
            auto& mesh_list = rendering::get_mesh(rs, def.gfx.mesh_name.view());
            entity->gfx.gfx_entity_count = (u32)mesh_list.count;

            rendering::initialize_entity(rs, entity->gfx.gfx_id, mesh_list.meshes[0].vertex_start, mesh_list.meshes[0].index_start);
            rendering::set_entity_albedo(rs, entity->gfx.gfx_id, u32(mesh_list.meshes[0].material.albedo_id));
            for(u32 i = 1; i < mesh_list.count; i++) {
                auto& mesh = mesh_list.meshes[i];
                rendering::register_entity(rs);
                rendering::initialize_entity(rs, entity->gfx.gfx_id + i, mesh.vertex_start, mesh.index_start);
                rendering::set_entity_albedo(rs, entity->gfx.gfx_id + i, u32(mesh.material.albedo_id));
            }
            entity->aabb = rendering::get_mesh_aabb(rs, def.gfx.mesh_name.view());
        }
        if (def.type_name.empty() == false) {
            entity->name.own(&world->arena, def.type_name.view());
        }
        entity->transform.origin = pos;
        entity->transform.basis = basis;

        entity->type = def.type;
        entity->physics.flags = def.physics ? def.physics->flags : 0;

        if (def.coroutine) {
            entity->coroutine.emplace(
                ztd::entity_coroutine_t{
                    .coroutine={world->game_state->time, (void*)entity}, 
                    .func=def.coroutine.get(mod_loader)
                }
            );
            // Maybe start coroutines automatically? some are just stored and called by triggers
            // have enum to describe behavior?
            // entity->coroutine->start();
        }

        if (def.emitter) {
            entity->gfx.particle_system = particle_system_create(&world->particle_arena, def.emitter->max_count);
            entity->gfx.instance(
                world->render_system()->scene_context->instance_storage_buffer.pool,
                world->render_system()->scene_context->instance_color_storage_buffer.pool, def.emitter->max_count, 1);

            range_u32(gi, 0, entity->gfx.gfx_entity_count) {
                rendering::set_entity_instance_data(rs, entity->gfx.gfx_id + gi, entity->gfx.instance_offset(0), def.emitter->max_count);
            }

            particle_system_settings_t& settings = *entity->gfx.particle_system;
            settings = *def.emitter;
            entity->gfx.particle_system->_stream_count = settings.stream_rate;
        }

        switch(entity->type) {
            case entity_type::weapon:
                entity->flags |= EntityFlags_Pickupable;
                break;
            case entity_type::player: {
                player_init(
                    entity,
                    &world->camera, 
                    def.gfx.mesh_name.view() != ""sv ? rendering::get_mesh_id(rs, def.gfx.mesh_name.view()) : 0
                );
                world->player = entity;
            } break;
        }

        if (def.on_hit_effect.name[0]) {
            tag_struct(entity->stats.effect, item::effect_t, &world->arena);
            entity->stats.effect->on_hit_effect = def.on_hit_effect.get(mod_loader);
        }

        // temporary
        auto inventory_size = def.inventory_size;
        if (def.brain_type == brain_type::player) {
            inventory_size = 16;
        }

        if (inventory_size > 0) {
            entity->inventory.init(&world->arena, inventory_size);
        }

        if (def.stats) {
            (character_stats_t&)entity->stats.character = *def.stats;
            entity->stats.character.health.current = 
                entity->stats.character.health.max;
        } else if (def.weapon) {
            entity->stats.weapon = *def.weapon;
            if (def.spawn_bullet.name[0]) {
                entity->stats.weapon.bullet_fn = def.spawn_bullet.get(mod_loader);
            }
        }

        if (def.physics) {
            physics::rigidbody_t* rb{0};
            if (def.physics->flags & PhysicsEntityFlags_Character) {
                rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::CHARACTER, entity->transform.origin, entity->transform.get_orientation());
            } else if (def.physics->flags & (PhysicsEntityFlags_Static | PhysicsEntityFlags_Trigger)) {
                rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::STATIC, entity->transform.origin, entity->transform.get_orientation());
            } else if (def.physics->flags & PhysicsEntityFlags_Kinematic) {
                rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::KINEMATIC, entity->transform.origin, entity->transform.get_orientation());
            } else if (def.physics->flags & PhysicsEntityFlags_Dynamic) {
                rb = world->physics->create_rigidbody(world->physics, entity, physics::rigidbody_type::DYNAMIC, entity->transform.origin, entity->transform.get_orientation());
            }
            if (!rb) {
                ztd_error(__FUNCTION__, "Failed to spawn rigidbody!");
            }
            assert(rb);
            entity->physics.rigidbody = rb;
            entity->physics.rigidbody->set_layer(physics_layers::everything);

            rb->on_collision = def.physics->on_collision.get(mod_loader);
            rb->on_collision_end = def.physics->on_collision_end.get(mod_loader);
            rb->on_trigger = def.physics->on_trigger.get(mod_loader);
            rb->on_trigger_end = def.physics->on_trigger_end.get(mod_loader);
            
            rb->position = pos;
            rb->orientation = entity->global_transform().get_orientation();
            std::string collider_name;
            for (size_t i = 0; i < array_count(def.physics->shapes); i++) {
                if (def.physics->shapes[i] == std::nullopt) continue;

                const auto& shape = def.physics->shapes[i]->shape;

                physics::collider_t* collider{nullptr};

                switch(shape){
                    case physics::collider_shape_type::CONVEX: {
                        collider_name = fmt_str("{}.convex.physx", def.gfx.mesh_name.data());
                        physics::collider_convex_info_t ci;
                        ci.mesh = utl::res::pack_file_get_file_by_name(resource_file, collider_name);
                        if (ci.mesh == nullptr) {
                            ztd_warn(__FUNCTION__, "Error loading convex physics mesh: {}", collider_name);
                        } else {
                            ci.size = safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name));
                            collider = world->physics->create_collider(
                                world->physics,
                                rb, shape, &ci
                            );
                        }
                    }   break;
                    case physics::collider_shape_type::TRIMESH: {
                        collider_name = fmt_str("{}.trimesh.physx", def.gfx.mesh_name.data());
                        physics::collider_trimesh_info_t ci;
                        ci.mesh = utl::res::pack_file_get_file_by_name(resource_file, collider_name);
                        if (ci.mesh == nullptr) {
                            ztd_warn(__FUNCTION__, "Error loading trimesh physics mesh: {}", collider_name);
                        } else {
                            ci.size = safe_truncate_u64(utl::res::pack_file_get_file_size(resource_file, collider_name));
                            collider = world->physics->create_collider(
                                world->physics,
                                rb, shape, &ci
                            );
                        }
                    }   break;
                    case physics::collider_shape_type::SPHERE: {
                        physics::collider_sphere_info_t ci;
                        ci.radius = def.physics->shapes[i]->sphere.radius;
                        ci.origin = def.physics->shapes[i]->sphere.origin;
                        entity->aabb.expand(ci.origin + v3f{ci.radius});
                        entity->aabb.expand(ci.origin + v3f{-ci.radius});
                        collider = world->physics->create_collider(
                            world->physics,
                            rb, shape, &ci
                        );
                    }   break;
                    case physics::collider_shape_type::BOX: {
                        physics::collider_box_info_t ci;
                        ci.size = def.physics->shapes[i]->box.size;
                        ci.origin = v3f{0.0f};
                        ci.rot = math::quat_identity();
                        entity->aabb.expand(def.physics->shapes[i]->box.size*0.5f);
                        entity->aabb.expand(def.physics->shapes[i]->box.size*-0.5f);
                        collider = world->physics->create_collider(
                            world->physics,
                            rb, shape, &ci
                        );
                    }   break;
                }

                if (collider && (def.physics->shapes[i]->flags & 1)) {
                    collider->set_trigger(true);
                }
            }
            world->physics->set_rigidbody(0, rb);
        }

        // TODO(Zack): Move layer settings to entity editor, for now
        // player will be player level
        // and all other will be enemy
        if (def.brain_type != brain_type::invalid) {
            world_new_brain(world, entity, def.brain_type);

            if (entity->physics.rigidbody == nullptr) {
                DEBUG_ALERT(fmt_sv("AI Entity: {} has no rigidbody", def.type_name.view()));
            } else {
                if (def.brain_type == brain_type::player) {
                    entity->physics.rigidbody->set_layer(physics_layers::player);
                    entity->physics.rigidbody->set_group(player_collision_group);
                } else { 
                    entity->physics.rigidbody->set_layer(physics_layers::enemy);
                    entity->physics.rigidbody->set_group(enemy_collision_group);
                }
            }
        }

        range_u64(i, 0, def.children.size()) {
            if (def.children[i].entity) {
                auto child = spawn(world, rs, *def.children[i].entity);
                entity->add_child(child);
                child->transform.origin = def.children[i].offset;
            }
        }

        return entity;
    }

    static entity_t*
    tag_spawn_(
        world_t* world,
        const ztd::prefab_t& def,
        const char* prefab_name,
        const char* file_name,
        const char* function,
        u64 line_number,
        const v3f& pos = {},
        const m33& basis = m33{1.0f}
    ) {
        auto* e = spawn(world, world->render_system(), def, pos, basis);
        e->_DEBUG_meta = ztd::DEBUG_entity_meta_info_t {
            .prefab_name = prefab_name,
            .file_name = file_name,
            .function = function,
            .line_number = line_number,
            .game_time = world->time(),
        };
        return e;
    }

}; // namespace ztd

#include "App/Game/Entity/brain_behavior.hpp"

namespace ztd {

    static void 
    world_free(world_t*& world) {
        arena_clear(&world->particle_arena);
        
        world->render_system()->scene_context->instance_storage_buffer.pool.clear();
        world->render_system()->scene_context->entities.pool.clear();
        world->render_system()->scene_context->entity_count = 0;
        
        arena_clear(&world->arena);
        world = nullptr;
    }

    void world_init_effects(world_t* world) {
        auto* rs = world->render_system();
        rs->scene_context->instance_storage_buffer.pool.clear();
        rs->scene_context->instance_color_storage_buffer.pool.clear();
        world->effects.blood_splats = rs->scene_context->instance_storage_buffer.pool.allocate(world->effects.blood_splat_max = 1024*2);
        world->effects.blood_colors = rs->scene_context->instance_color_storage_buffer.pool.allocate(world->effects.blood_splat_max);
        std::fill(world->effects.blood_colors, world->effects.blood_colors + world->effects.blood_splat_max + 1, rendering::instance_extra_data_t{});
        auto blood_id = world->effects.blood_entity = rendering::register_entity(rs);
        auto blood_mid = rendering::get_mesh_id(rs, "res/models/misc/bloodsplat_02.gltf");
        auto& mesh = rendering::get_mesh(rs, blood_mid);
        rendering::initialize_entity(rs, blood_id, mesh.meshes->vertex_start, mesh.meshes->index_start);
        rendering::set_entity_material(rs, blood_id, 9);
        rendering::set_entity_albedo(rs, blood_id, safe_truncate_u64(mesh.meshes->material.albedo_id));
    }
    
    static world_t*
    world_init(game_state_t* game_state, physics::api_t* phys_api) {
        TIMED_FUNCTION;
        
        arena_t arena = arena_create(megabytes(32));
        tag_struct(auto* world, world_t, &arena);
        world->arena = arena;
        // world_t* world = bootstrap_arena(world_t, megabytes(32));
        world->game_state = game_state;
        world->physics = phys_api;
        phys_api->user_world = world;
        
        // world->particle_arena = arena_create(megabytes(8));
        world->particle_arena = arena_create(kilobytes(8));
        
        // frame arenas cant be dynamic, because their blocks will be freed
        constexpr umm frame_arena_size = megabytes(8);
        world->frame_arena.arena[0] = arena_sub_arena(&world->arena, frame_arena_size);
        world->frame_arena.arena[1] = arena_sub_arena(&world->arena, frame_arena_size);

        world_init_effects(world);

        return world;
    }


    void world_place_bloodsplat(
        world_t* world, const m44& transform
    ) {
        
        world->effects.blood_splats[world->effects.blood_splat_count++ % world->effects.blood_splat_max] = transform;
    }

    void world_render_bloodsplat(
        world_t* world
    ) {
        if (world->effects.blood_splat_count) {
            // no gfx id, kinda scuffed (might be a bug)
            rendering::submit_job(
                world->render_system(), 
                rendering::get_mesh_id(world->render_system(), "res/models/misc/bloodsplat_02.gltf"),
                9, // todo make material per mesh
                m44{1.0f},
                world->effects.blood_entity, 1,
                (u32)glm::min(world->effects.blood_splat_count, world->effects.blood_splat_max),
                0
            );
        } 
    }

    // static world_t* 
    // clone(world_t* src, arena_t* arena) noexcept {
    //     assert(src);

    //     world_t* new_world = world_init(arena, src->game_state, src->physics);

    //     new_world->entity_count = src->entity_count;
    //     new_world->entity_capacity = src->entity_capacity;
    //     new_world->next_entity_id = src->next_entity_id;

    //     new_world->camera = src->camera;
        
    //     new_world->entropy = src->entropy;

    //     // range_u64(i, 0, src->entity_capacity) {
    //     //     new_world->entities[i] = src->entities[i];
    //     // }

    //     return new_world;
    // }

    

    static void 
    remove_entity_from_id_hash(world_t* world, entity_t* entity) {
        const u64 id_hash = utl::rng::fnv_hash_u64(entity->id);
        const u64 id_bucket = id_hash & (array_count(world->entity_id_hash)-1);

        if (world->entity_id_hash[id_bucket]) {
            entity_t* last = nullptr;
            auto* c = world->entity_id_hash[id_bucket];
            while(c && c->id != entity->id) {
                last = c;
                c = c->next_id_hash;
            }
            if (last && c) {
                last->next_id_hash = c->next_id_hash;
            } else if (!last && c) {
                world->entity_id_hash[id_bucket] = c->next_id_hash;
            }
            c->next_id_hash = 0;
        } else {
            assert(0);
        }
    }

    static entity_t*
    find_entity_by_name(world_t* world, std::string_view name) {
        TIMED_FUNCTION;
        range_u64(i, 0, world->entity_capacity) {
            if (world->entities[i].is_alive()) {
                if (world->entities[i].name.sv() == name) {
                    return world->entities + i;
                }
            }
        }
         
        return nullptr;
    }

    static entity_t*
    find_entity_by_id(world_t* world, entity_id id) {
        // TIMED_FUNCTION;
        const u64 id_hash = utl::rng::fnv_hash_u64(id);
        const u64 id_bucket = id_hash & (array_count(world->entity_id_hash)-1);

        if (world->entity_id_hash[id_bucket]) {
            auto* e = world->entity_id_hash[id_bucket];
            while(e && e->id != id) {
                e = e->next_id_hash;
            }
            return e;
        } 
        return nullptr;
    }

    static void
    world_update(world_t* world, f32 dt) {
        world->frame_arena.active += 1;
        arena_clear(&world->frame_arena.get());
        world->events = 0;
        std::fill(world->render_groups.begin(), world->render_groups.end(), gfx::render_group_t{});

        for (size_t i{0}; i < world->entity_capacity; i++) {
            auto* e = world->entities + i;
            if (e->is_alive() == false) {
                continue;
            }
        
            if (e->type == entity_type::player) {
                auto forward = e->camera_controller.forward();
                world->game_state->sfx->set_listener(e->global_transform().origin, forward);
            }
            if (e->attached_sound) {
                if (e->attached_sound->isValid() == false) {
                    e->attached_sound = nullptr;
                } else {
                    world->game_state->sfx->set_instance_position(e->attached_sound, e->global_transform().origin);
                }
            }
        }
    }

    static void
    world_update_brain(world_t* world, entity_t* entity, f32 dt) {
        auto* brain = &entity->brain;
        assert(brain && brain->id != uid::invalid_id && "Not a valid brain");
        
        switch(brain->type) {
            case brain_type::player: player_behavior(world, entity, brain, dt); break;
            case brain_type::flyer:  skull_behavior(world, entity, brain, dt); break;
            case brain_type::person: person_behavior(world, entity, brain, dt); break;
            case_invalid_default;
        }
    }

    static void
    world_update_kinematic_physics(world_t* world) {
        TIMED_FUNCTION;
        // const auto* input = &world->game_state->game_memory->input;
        for (size_t i{0}; i < world->entity_capacity; i++) {
            auto* e = world->entities + i;
            if (e->is_alive() == false) {
                continue;
            }
            if (e->physics.flags & ztd::PhysicsEntityFlags_Dying) {
                e->physics.flags = 0;
                world->physics->remove_rigidbody(world->physics, e->physics.rigidbody);
                e->physics.rigidbody = 0;
                continue;
            }
            if (e->physics.rigidbody && e->physics.flags & ztd::PhysicsEntityFlags_Kinematic) {
                world->physics->set_rigidbody(0, e->physics.rigidbody);
                e->transform.origin = e->physics.rigidbody->position;
                e->transform.basis = glm::toMat3(e->physics.rigidbody->orientation);
            }
        }
    }

    static void
    world_destroy_entity(world_t* world, entity_t*& e) {
        TIMED_FUNCTION;
        assert(world->entity_count > 0);
        assert(e->id != uid::invalid_id);

        e->flags = EntityFlags_Dead;
        remove_entity_from_id_hash(world, e);

        if (e->parent) {
            e->parent->remove_child(e);
        }
        for (auto* child = e->first_child; child; child = child->next_child) {
            if ((child->flags & EntityFlags_Dying) == 0) { // this entity gets to live
                e->remove_child(child);
            }
        }

        if (e->physics.rigidbody) {
            world->physics->remove_rigidbody(world->physics, e->physics.rigidbody);
            e->physics.rigidbody = 0;
        }

        e->gfx = {};
        new (e) ztd::entity_t;

        e->id = uid::invalid_id;
        node_push(e, world->free_entities);

        world->entity_count--;
        e = nullptr;
    }

    static void
    world_kill_free_queue(world_t* world) {
        for (size_t i{0}; i < world->entity_capacity; i++) {
            auto* e = world->entities + i;
            if (e->flags & EntityFlags_Dying) {
                world_destroy_entity(world, e);
            }
        }
    }

    static void
    world_destroy_all(world_t* w) {
        range_u64(i, 0, w->entity_capacity) {
            auto* e = w->entities + i;
            if (e->is_alive()) {
                e->queue_free();
            }
        }
        world_kill_free_queue(w);
        w->entity_count = 0;
        w->entity_capacity = 0;
    }

    static void 
    entity_set_mesh_texture(world_t* world, entity_t* entity, std::string_view name) {
        auto& mesh=world->render_system()->mesh_cache.get(entity->gfx.mesh_id);
        auto tex=world->render_system()->texture_cache.get_id(name);
        
        range_u64(i, 0, mesh.count) {
            mesh.meshes[i].material.albedo_id = tex;
        }
    }
}

#if ZTD_INTERNAL
    #define tag_spawn(world, prefab, ...) \
        tag_spawn_((world), (prefab), #prefab, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else 
    // #define tag_spawn(world, ...) 
        // spawn((world), (world)->render_system(), __VA_ARGS__)
#endif

// World Loading Stuff

struct world_save_file_header_t {
    u64 prefab_count{0};
    u64 VERSION{1};

    u32 light_probe_count{0};
    v3f light_probe_min{};
    v3f light_probe_max{};
    f32 light_probe_grid_size{1.6f};
};

template<>
world_save_file_header_t
utl::memory_blob_t::deserialize<world_save_file_header_t>() {
    world_save_file_header_t result = {};

    result.prefab_count = deserialize<u64>();
    result.VERSION = deserialize<u64>();

    if (result.VERSION <= world_save_file_header_t{}.VERSION) {
        if (result.VERSION >= 1) {
            result.light_probe_count = deserialize<u32>();
            result.light_probe_min = deserialize<v3f>();
            result.light_probe_max = deserialize<v3f>();
            result.light_probe_grid_size = deserialize<f32>();
        }
    }

    return result;
}

void load_world_file(ztd::world_t* world, const char* name) {
    arena_t* arena = &world->arena;

    auto [bytes, file_size] = utl::read_bin_file(arena, name);
    utl::memory_blob_t blob{(std::byte*)bytes};

    auto header = blob.deserialize<world_save_file_header_t>();
    
    ztd_info(__FUNCTION__, "Loading world '{}' - save file version: {}", name, header.VERSION);
    range_u64(i, 0, header.prefab_count) {
        auto prefab = blob.deserialize<ztd::prefab_t>(arena);
        auto transform = blob.deserialize<math::transform_t>();

        ztd::tag_spawn(world, prefab, transform.origin, transform.basis);
    }

    if (header.light_probe_count > 0) {
        math::rect3d_t probe_aabb{
            .min = header.light_probe_min,
            .max = header.light_probe_max,
        };

        world->render_system()->light_probes.grid_size = header.light_probe_grid_size;
        rendering::update_probe_aabb(world->render_system(), probe_aabb);
    }
}

void load_world_file(ztd::world_t* world, const char* name, auto&& callback) {
    arena_t* arena = &world->arena;

    auto [bytes, file_size] = utl::read_bin_file(arena, name);

    utl::memory_blob_t blob{(std::byte*)bytes};

    auto header = blob.deserialize<world_save_file_header_t>();
    
    ztd_info(__FUNCTION__, "Loading world '{}' - save file version: {}", name, header.VERSION);
    range_u64(i, 0, header.prefab_count) {
        auto prefab = blob.deserialize<ztd::prefab_t>(arena);
        auto transform = blob.deserialize<math::transform_t>();

        callback(world, prefab, transform.origin, transform.basis);
    }

    if (header.light_probe_count > 0) {
        math::rect3d_t probe_aabb{
            .min = header.light_probe_min,
            .max = header.light_probe_max,
        };
        
        world->render_system()->light_probes.grid_size = header.light_probe_grid_size;
        rendering::update_probe_aabb(world->render_system(), probe_aabb);
    }
}
