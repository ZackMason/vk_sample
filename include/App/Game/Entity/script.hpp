#ifndef ENTITY_SCRIPT_HPP
#define ENTITY_SCRIPT_HPP

#include "core.hpp"
#include "uid.hpp"

namespace game {
    DEFINE_TYPED_ID(entity_id);
};

namespace game {
    namespace static_mesh {
        DEFINE_TYPED_ID(static_mesh_id);
        struct component_t final {
            constexpr explicit component_t(static_mesh_id id) : _id{id} {}
            constexpr explicit component_t() : _id{uid::invalid_id} {}
            constexpr static_mesh_id get_id() const noexcept { return _id; }
            constexpr bool is_valid() const noexcept { return uid::is_valid(_id); }

        private:
            static_mesh_id _id;
        };
    };
    namespace script {
        DEFINE_TYPED_ID(script_id);
        struct component_t final {
            constexpr explicit component_t(script_id id) : _id{id} {}
            constexpr explicit component_t() : _id{uid::invalid_id} {}
            constexpr script_id get_id() const noexcept { return _id; }
            constexpr bool is_valid() const noexcept { return uid::is_valid(_id); }

        private:
            script_id _id;
        };
    };
    namespace transform {
        DEFINE_TYPED_ID(transform_id);
        struct component_t final : public math::transform_t {
            constexpr explicit component_t(transform_id id) : _id{id} {}
            constexpr explicit component_t() : _id{uid::invalid_id} {}
            constexpr transform_id get_id() const noexcept { return _id; }
            constexpr bool is_valid() const noexcept { return uid::is_valid(_id); }

        private:
            transform_id _id{uid::invalid_id};
        };
    };
};

namespace game {
    struct ecs_world_t;
    
    struct scriptable_entity_t {

        constexpr explicit scriptable_entity_t(entity_id id) noexcept : _id{id} {}
        constexpr explicit scriptable_entity_t() noexcept : _id{uid::invalid_id} {}
        constexpr entity_id get_id() const { return _id; }
        constexpr [[nodiscard]] bool is_valid() const { return uid::is_valid(_id); }

        script::component_t& script(ecs_world_t* world) const;
        transform::component_t& transform(ecs_world_t* world) const;
        static_mesh::component_t& static_mesh(ecs_world_t* world) const;

    private:
        entity_id _id{uid::invalid_id};
    };
};

namespace game::script {
    struct entity_script_t;
};

namespace game {
    struct ecs_world_t {
        u64 capacity{0};

        scriptable_entity_t* entities{0};
        u64 entity_count{0};

        script::entity_script_t** scripts{0};
        u64 script_count{0};

        struct components_t {
            transform::component_t*         transforms{0};
            u64                             transform_count{0};
            script::component_t*            scripts{0};
            u64                             script_count{0};
            static_mesh::component_t*            static_meshes{0};
            u64                             static_mesh_count{0};
        } components;

        entity_id create_entity() {
            const u32 id = safe_truncate_u64(entity_count++);
            return (entities[id] = scriptable_entity_t{id}).get_id();
        }

        scriptable_entity_t& get_entity(entity_id id) {
            return entities[uid::index(id)];
        }
    };

    inline ecs_world_t*
    world_init(arena_t* arena, u64 capacity) {
        auto* w = arena_alloc_ctor<ecs_world_t>(arena, 1);

        w->capacity = 
        w->components.script_count =
        w->components.transform_count = 
        w->components.static_mesh_count = capacity;
        
        w->entities = arena_alloc_ctor<entity::scriptable_entity_t>(arena, w->capacity, uid::invalid_id);
        w->scripts = arena_alloc_ctor<script::entity_script_t*>(arena, w->capacity, nullptr);
        w->components.scripts = arena_alloc_ctor<script::component_t>(arena, w->capacity);
        w->components.transforms = arena_alloc_ctor<transform::component_t>(arena, w->capacity);
        w->components.static_meshes = arena_alloc_ctor<static_mesh::component_t>(arena, w->capacity);

        return w;
    }

    template <typename Comp>
    void get_component(ecs_world_t* w, entity_id id, Comp** comp) {
        const auto index = uid::index(id);
        if constexpr (std::is_same_v<Comp, transform::component_t>) {
            *comp = &w->components.transforms[index];
        }
        if constexpr (std::is_same_v<Comp, script::component_t>) {
            *comp = &w->components.scripts[index];
        }
    }

    static_mesh::component_t& scriptable_entity_t::static_mesh(ecs_world_t* world) const {
        const auto index = uid::index(this->get_id());

        // assert(world->components.script_count > index);

        return world->components.static_meshes[index];
    }

    script::component_t& scriptable_entity_t::script(ecs_world_t* world) const {
        const auto index = uid::index(this->get_id());

        // assert(world->components.script_count > index);

        return world->components.scripts[index];
    }

    transform::component_t& scriptable_entity_t::transform(ecs_world_t* world) const {
        const auto index = uid::index(this->get_id());

        // assert(world->components.transform_count > index);

        return world->components.transforms[index];
    }
};

namespace game::script {

    struct entity_script_t : public entity::scriptable_entity_t {

        virtual void begin_play() {}
        virtual void update(f32 dt) {}

        virtual ~entity_script_t() = default;

    // protected:
        constexpr explicit entity_script_t(entity::scriptable_entity_t entity)
            : entity::scriptable_entity_t{entity.get_id()} {}

    };


    namespace detail {
        using script_ptr = entity_script_t*;
        using script_creator = script_ptr(*)(arena_t*, game::scriptable_entity_t);

        template <typename ScriptClass>
        script_ptr create_script(arena_t* arena, game::scriptable_entity_t entity) {
            assert(entity.is_valid());
            return arena_alloc_ctor<ScriptClass>(arena, 1, entity);
        }
    };
};

namespace game {
    namespace static_mesh {
        struct init_info_t {
            u32 mesh_id;
            u32 mat_id;
        };

        component_t create(
            entity::ecs_world_t* world, 
            const init_info_t& init_info, 
            entity::scriptable_entity_t entity
        ) {
            const uid::id_type index = uid::index(entity.get_id());
            assert(index < world->capacity);
            assert(index < world->entity_count);
            
            return world->components.static_meshes[index] = component_t{init_info.mesh_id};
        }
    };

    namespace script {
        struct init_info_t {
            detail::script_creator init{0};
        };

        component_t create(
            entity::ecs_world_t* world, 
            const init_info_t& init_info, 
            entity::scriptable_entity_t entity,
            arena_t* arena
        ) {
            const uid::id_type index = uid::index(entity.get_id());
            assert(index < world->capacity);
            assert(index < world->entity_count);
            
            world->scripts[index] = init_info.init(arena, entity);
            return world->components.scripts[index] = component_t{index};
        }
    };

    namespace transform {
        struct init_info_t {
            v3f origin{0.0f};
            v3f rotation{0.0f};
            v3f scale{1.0f};
        };

        component_t create(
            entity::ecs_world_t* world, 
            const init_info_t& init_info, 
            entity::scriptable_entity_t entity
        ) {
            const uid::id_type index = uid::index(entity.get_id());
            assert(index < world->capacity);
            assert(index < world->entity_count);
            
            world->components.transforms[index] = component_t{index};
            world->components.transforms[index].origin = init_info.origin;
            world->components.transforms[index].set_rotation(init_info.rotation);
            world->components.transforms[index].set_scale(init_info.scale);

            return world->components.transforms[index];
        }
    };
};

#endif