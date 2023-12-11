#pragma once

#include "entity.hpp"
#include "zyy_entity_prefab.hpp"
#include "App/Game/Rendering/render_system.hpp"
#include "App/Game/World/world.hpp"


namespace zyy {

    static prefab_t 
    entity_to_prefab(
        world_t* world,
        entity_t* entity
    ) {
        prefab_t result = {};

        auto* rs = world->render_system();

        if (entity->gfx.mesh_id) {
            const auto& mesh = rendering::get_mesh(rs, entity->gfx.mesh_id);
            
            std::memcpy(result.gfx.mesh_name.buffer, mesh.name.data(), mesh.name.size());
        }

        return result;
    }

    static entity_t*
    clone_entity_with_prefab(
        world_t* world,
        entity_t* entity,
        world_t* clone_world = 0
    ) {
        prefab_t prefab = entity_to_prefab(world, entity);

        return tag_spawn(clone_world?clone_world:world, prefab, entity->global_transform().origin);
    }
}
