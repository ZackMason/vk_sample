#pragma once

#include "App/Game/Entity/zyy_entity_prefab.hpp"
#include "App/Game/Entity/entity.hpp"
#include "App/Game/GUI/debug_state.hpp"
#include "App/Game/Rendering/particle.hpp"
#include "App/Game/Items/lightning_effect.hpp"
#include "App/Game/Items/items_common.hpp"

void co_kill_in_ten(coroutine_t* co, frame_arena_t& frame_arena) {
    auto* e = (zyy::entity_t*)co->data;

    co_begin(co);

        co_wait(co, 10.0f);
        e->queue_free();

    co_end(co);
}

namespace zyy::db {

#define DB_ENTRY static constexpr zyy::prefab_t

namespace misc {

DB_ENTRY
sphere {
    .type = entity_type::environment,
    .type_name = "Sphere",
    .gfx = {
        .mesh_name = "res/models/sphere.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
teapot_particle {
    .type = entity_type::environment,
    .type_name = "Teapot Particle",
    .gfx = {
        .mesh_name = "res/models/particles/teapot_particle.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
teapot {
    .type = entity_type::environment,
    .type_name = "Teapot",
    .gfx = {
        .mesh_name = "res/models/utah-teapot.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .stats = character_stats_t {
        .health = {
            50
        },
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic,
    #if 1 // use convex
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::CONVEX,
            },
        },
    #else 
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
    #endif
    },
};

DB_ENTRY
door {
    .type = entity_type::environment,
    .type_name = "door",
    .gfx = {
        .mesh_name = "res/models/environment/door_02.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Kinematic,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::BOX,
                // .flags = 1,
                .box = {
                    .size = v3f{7.5f, 8.5f, 1.0f },
                },
            },
        },
    },
};

DB_ENTRY
torch_01 {
    .type = entity_type::environment,
    .type_name = "torch",
    .gfx = {
        .mesh_name = "res/models/environment/torch_01.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .children = {
        {
            .entity = {},
            .offset = v3f{0.0f, 1.5f, -0.45f}
        }
    },
};

DB_ENTRY
platform_1000 {
    .type = entity_type::environment,
    .type_name = "platform_1000",
    .gfx = {
        .mesh_name = "res/models/platform_1000x1000.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::BOX,
                .box = {
                    .size = v3f{1000.0f, 1.0f, 1000.0f},
                },
            },
        },
    },
};

void co_platform(coroutine_t* co, frame_arena_t& frame_arena) {
    // return;
    auto* e = (zyy::entity_t*)co->data;
    auto& y_pos = e->physics.rigidbody->position.y;

    const auto lerp = tween::generic<f32>(tween::in_out_elastic);
    auto* stack = co_stack(co, frame_arena);
    f32* start  = co_push_stack(co, stack, f32);
    f32* end    = co_push_stack(co, stack, f32);

    co_begin(co);

        *start = y_pos;
        *end = math::fcmp(*start, 20.0f) ? 0.0f : 20.0f;

        DEBUG_DIAGRAM(e->physics.rigidbody->position.y);
        DEBUG_DIAGRAM(v3f(e->physics.rigidbody->position.x, *end, e->physics.rigidbody->position.z));

        // Platform.audio.play_sound(assets::sounds::unlock.id);

        co_lerp(co, y_pos, *start, *end, 10.0f, lerp);

    co_end(co);
}

DB_ENTRY
platform_3x3 {
    .type = entity_type::environment,
    .type_name = "platform",
    .gfx = {
        .mesh_name = "res/models/platform_3x3.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .coroutine = co_platform,
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Kinematic | PhysicsEntityFlags_Trigger,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::BOX,
                .box = {
                    .size = v3f{3.0f, .2f, 3.0f},
                },
            },
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::BOX,
                .flags = 1,
                .box = {
                    // .size = v3f{5.5f, 32.0f, 5.5f},
                    .size = v3f{2.5f, 32.0f, 2.5f},
                },
            },
        },
    },
};

DB_ENTRY
bullet_hole {
    .type = entity_type::environment,
    .type_name = "bullet_hole",
    .gfx = {
        .mesh_name = "res/models/bullet_hole.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
    .coroutine = co_kill_in_ten,
};


DB_ENTRY
bullet_01 {
    .type = entity_type::bad,
    .type_name = "bullet_01",
    .gfx = {
        .mesh_name = "res/models/bullet_01.gltf",
    },
    .coroutine = co_kill_in_ten,
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 0.135f,
                },
            },
        },
    },
};

DB_ENTRY
plasma_bullet {
    .type = entity_type::bad,
    .type_name = "plasma_bullet",
    .gfx = {
        .mesh_name = "res/models/particles/particle_03.gltf",
    },
    .coroutine = co_kill_in_ten,
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 0.135f,
                },
            },
        },
    },
};


DB_ENTRY
rocket_bullet {
    .type = entity_type::bad,
    .type_name = "rocket_bullet",
    .gfx = {
        .mesh_name = "res/models/guns/rocket_01.gltf",
    },
    .coroutine = co_kill_in_ten,
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 0.135f,
                },
            },
        },
    },
};


}; //namespace misc

namespace particle {

DB_ENTRY
circle {
    .type = entity_type::environment,
    .type_name = "circle_particle",
    .gfx = {
        .mesh_name = "res/models/particles/particle_01.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
plasma {
    .type = entity_type::environment,
    .type_name = "circle_particle",
    .gfx = {
        .mesh_name = "res/models/particles/particle_02.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
orb {
    .type = entity_type::environment,
    .type_name = "orb_particle",
    .gfx = {
        .mesh_name = "res/models/particles/particle_03.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
    .coroutine = co_kill_in_ten,
    .emitter = particle_system_settings_t {
        .template_particle = particle_t {
            .position = v3f(0.0),
            .life_time = 0.50f,
            .color = gfx::color::v4::purple,
            .scale = 1.0f,
            .velocity = axis::up * 5.0f,
        },
        .world_space = 1,
        .max_count = 64,
        .velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f)),
        .angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f)),
        .stream_rate = 1,
        .spawn_rate = 0.005f,
        .scale_over_life_time = math::range_t(0.50f, 0.0f),
        .emitter_type = particle_emitter_type::box,
        .box = math::rect3d_t(v3f(-0.10f), v3f(0.10f)),
    },
};

} // namespace particle

namespace environmental {

DB_ENTRY
rock_01 {
    .type = entity_type::environment,
    .type_name = "rock",
    .gfx = {
        .mesh_name = "res/models/rocks/rock_01.obj",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
bloodsplat_01 {
    .type = entity_type::environment,
    .type_name = "bloodsplat",
    .gfx = {
        .mesh_name = "res/models/misc/bloodsplat_01.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

// ps->gfx.particle_system->spawn_rate = 0.005f;
// ps->gfx.particle_system->scale_over_life_time = math::range_t(3.0f, 2.50f);
// ps->gfx.particle_system->velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f));
// ps->gfx.particle_system->angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f));
// ps->gfx.particle_system->emitter_type = particle_emitter_type::box;
// ps->gfx.particle_system->box = math::rect3d_t(v3f(-0.10f), v3f(0.10f));
// ps->gfx.particle_system->stream_rate = 
// ps->gfx.particle_system->_stream_count = 1;


DB_ENTRY
blood_01 {
    .type = entity_type::environment,
    .type_name = "blood",
    .gfx = {
        .mesh_name = "res/models/misc/blood_01.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
    .emitter = particle_system_settings_t {
        .template_particle = particle_t {
            .position = v3f(0.0),
            .life_time = 10.0f,
            .color = gfx::color::v4::purple,
            .scale = 1.0f,
            .velocity = axis::up * 5.0f,
        },
        .max_count = 64,
        .velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f)),
        .angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f)),
        .stream_rate = 1,
        .spawn_rate = 0.005f,
        .scale_over_life_time = math::range_t(3.0f, 2.50f),
        .emitter_type = particle_emitter_type::box,
        .box = math::rect3d_t(v3f(-0.10f), v3f(0.10f)),
    },
};

DB_ENTRY
grass_01 {
    .type = entity_type::environment,
    .type_name = "grass",
    .gfx = {
        .mesh_name = "res/models/grass.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
grass_02 {
    .type = entity_type::environment,
    .type_name = "grass",
    .gfx = {
        .mesh_name = "res/models/environment/grass_01.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
tree_01 {
    .type = entity_type::environment,
    .type_name = "tree",
    .gfx = {
        .mesh_name = "res/models/environment/tree_01.gltf",
        .material = gfx::material_t::plastic(gfx::color::v4::light_gray),
    },
};

DB_ENTRY
tree_group {
    .children = {
        {
            .entity = &tree_01,
            .offset = v3f{0.0f}
        },
        {
            .entity = &tree_01,
            .offset = v3f{2.0f,0.0f, 0.5f}
        },
        {
            .entity = &tree_01,
            .offset = v3f{1.0f,0.0f, -0.25f}
        },
    },
};

};

namespace rooms {

DB_ENTRY
city_test_01 {
    .type = entity_type::environment,
    .type_name = "city_test_01",
    .gfx = {
        .mesh_name = "res/models/city_test_01.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
temple_01 {
    .type = entity_type::environment,
    .type_name = "temple_01",
    .gfx = {
        .mesh_name = "res/models/rooms/temple_01.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
parkcore_01 {
    .type = entity_type::environment,
    .type_name = "parkcore_01",
    .gfx = {
        .mesh_name = "res/models/rooms/parkcore_01.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};
DB_ENTRY
parkcore_02 {
    .type = entity_type::environment,
    .type_name = "parkcore_02",
    .gfx = {
        .mesh_name = "res/models/rooms/parkcore_02.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
room_03 {
    .type = entity_type::environment,
    .type_name = "parkcore_02",
    .gfx = {
        .mesh_name = "res/models/rooms/room_03.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
tower_01 {
    .type = entity_type::environment,
    .type_name = "tower_01",
    .gfx = {
        .mesh_name = "res/models/rooms/tower_01.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
sponza {
    .type = entity_type::environment,
    .type_name = "Sponza",
    .gfx = {
        .mesh_name = "res/models/Sponza.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
            // zyy::prefab_t::physics_t::shape_t{
            //     .shape = physics::collider_shape_type::SPHERE,
            //     .flags = 1,
            //     .sphere = {
            //         .radius = 5.0f,
            //     },
            // },
        },
    },
};

DB_ENTRY
room_01 {
    .type = entity_type::environment,
    .type_name = "room_01",
    .gfx = {
        // .mesh_name = "res/models/rooms/room_01.obj",
        .mesh_name = "res/models/rooms/rock_room_test.fbx",
        // .mesh_name = "res/models/Sponza.gltf",
        // .mesh_name = "res/models/rooms/rock_room_test.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
room_0 {
    .type = entity_type::environment,
    .type_name = "room_0",
    .gfx = {
        .mesh_name = "res/models/rooms/room_0.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

DB_ENTRY
map_01 {
    .type = entity_type::environment,
    .type_name = "map_01",
    .gfx = {
        .mesh_name = "res/models/map_01.obj",
        .material = gfx::material_t::metal(gfx::color::v4::light_gray),
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{.shape = physics::collider_shape_type::TRIMESH,},
        },
    },
};

}; //namespace rooms

namespace items {

DB_ENTRY
lightning_powerup {
    .type = entity_type::item,
    .type_name = "lightning_powerup",
    .gfx = {
        .mesh_name = "res/models/utah-teapot.obj",
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .on_trigger = zyy::item::pickup_item,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .flags = 1,
                .sphere = {
                    .radius = 3.0f,
                },
            },
        },
    },
    .effect = zyy::item::effect_t {
        .on_hit_effect = lightning_on_hit_effect,
    }

};

}; //namespace items

namespace weapons {

DB_ENTRY
shotgun {
    .type = entity_type::weapon,
    .type_name = "shotgun",
    .gfx = {
        .mesh_name = "res/models/guns/shotgun_01.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_shotgun(),
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .flags = 1,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
    },
};

DB_ENTRY
knife_bullet {
    .type = entity_type::weapon,
    .type_name = "knife_bullet",
    .gfx = {
        .mesh_name = "res/models/guns/knife_01.gltf",
    },
    .coroutine = co_kill_in_ten,
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 0.25f,
                },
            },
        },
    },
};

DB_ENTRY
knife {
    .type = entity_type::weapon,
    .type_name = "knives",
    .gfx = {
        .mesh_name = "res/models/guns/knife_01.gltf",
    },
    .weapon = wep::create_knife(),
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .flags = 1,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
    },
};

DB_ENTRY
rpg {
    .type = entity_type::weapon,
    .type_name = "rpg",
    .gfx = {
        .mesh_name = "res/models/guns/rpg_01.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_rpg(),
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .flags = 1,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
    },
};

DB_ENTRY
smg {
    .type = entity_type::weapon,
    .type_name = "smg",
    .gfx = {
        .mesh_name = "res/models/guns/smg_01.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_rifle(),
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .flags = 1,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
    },
};

DB_ENTRY
rifle {
    .type = entity_type::weapon,
    .type_name = "rifle",
    .gfx = {
        .mesh_name = "res/models/guns/rifle_01.gltf",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_rifle(),
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Static,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .flags = 1,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
    },
};

DB_ENTRY
pistol {
    .type = entity_type::weapon,
    .type_name = "pistol",
    .gfx = {
        .mesh_name = "pistol",
        .material = gfx::material_t::metal(gfx::color::v4::dark_gray),
    },
    .weapon = wep::create_pistol(),
};



}; // namespace weapons

namespace particles {

DB_ENTRY
fire {
    .type = entity_type::environment,
    .type_name = "fire",
    .gfx = {
        .mesh_name = "res/models/particles/particle_02.gltf",
    },
    .emitter = particle_system_settings_t {
        .template_particle = particle_t {
            .position = v3f(0.0),
            .life_time = 1.50f,
            .color = gfx::color::v4::purple,
            .scale = 1.0f,
            .velocity = axis::up * 0.10f,
        },
        .max_count = 64,
        .acceleration = v3f{axis::up*9.81f*0.1f},
        .velocity_random = math::rect3d_t(planes::xz*-.0050f, v3f(.0050f)),
        .angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f)),
        .stream_rate = 1,
        .spawn_rate = 0.02f,
        .scale_over_life_time = math::range_t(0.06f, 0.01f),
        .emitter_type = particle_emitter_type::box,
        .box = math::rect3d_t(v3f(-.10f), v3f(.10f)),
    },
};

DB_ENTRY
sparks {
    .type = entity_type::environment,
    .type_name = "sparks",
    .gfx = {
        .mesh_name = "res/models/particles/particle_02.gltf",
    },
    .coroutine = co_kill_in_ten,
    .emitter = particle_system_settings_t {
        .template_particle = particle_t {
            .position = v3f(0.0),
            .life_time = 10.50f,
            .color = v4f{0.4f, 0.3f, 0.9f, 1.0f},
            .scale = 1.0f,
            .velocity = axis::up * 0.80f,
        },
        .max_count = 32,
        .acceleration = v3f{axis::up*9.81f*-0.1f},
        .velocity_random = math::rect3d_t(planes::xz*-5.50f, v3f(5.50f)),
        // .angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f)),
        .stream_rate = 1,
        .spawn_rate = 0.002f,
        .scale_over_life_time = math::range_t(0.2f, 0.05f),
        .emitter_type = particle_emitter_type::box,
        .box = math::rect3d_t(v3f(-.10f), v3f(.10f)),
    },
};

DB_ENTRY
beam {
    .type = entity_type::environment,
    .type_name = "beam",
    .gfx = {
        .mesh_name = "res/models/particles/particle_02.gltf",
    },
    .coroutine = co_kill_in_ten,
    .emitter = particle_system_settings_t {
        .template_particle = particle_t {
            .position = v3f(0.0),
            .life_time = 10.50f,
            .color = v4f{0.4f, 0.3f, 0.9f, 1.0f},
            .scale = 1.0f,
            .velocity = axis::up * 0.0f,
        },
        .max_count = 32,
        .acceleration = v3f{0.0f},
        .velocity_random = math::rect3d_t(planes::xz*-.50f, v3f(.50f)),
        // .angular_velocity_random = math::rect3d_t(v3f(-4.0f), v3f(4.0f)),
        .stream_rate = 1,
        .spawn_rate = 0.002f,
        .scale_over_life_time = math::range_t(0.1f, 0.025f),
        .emitter_type = particle_emitter_type::box,
        .box = math::rect3d_t(v3f(-.10f), v3f(.10f)),
    },
};


}; // namespace particles

namespace bads {

DB_ENTRY
skull {
    .type = entity_type::bad,
    .type_name = "skull",
    .gfx = {
        .mesh_name = "res/models/entities/skull.gltf",
    },    
    .stats = character_stats_t {
        .health = {
            6.0f
        },
        .movement = {
            .move_speed = 90.0f,
        },
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Dynamic,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
    },
    .brain_type = brain_type::flyer,
};


DB_ENTRY
person {
    .type = entity_type::bad,
    .type_name = "person",
    .gfx = {
        .mesh_name = "res/models/capsule.obj",
    },
    .stats = character_stats_t {
        .health = {
            15
        },
        .movement = {
            .move_speed = 1.3f,
        },
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Character,
        .shapes = { 
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::CAPSULE,
                .capsule = {
                    .radius = 1.0f,
                    .height = 3.0f,
                },
            },
        },
    },
    .brain_type = brain_type::person,
};


}; // namespace bads

// Character Class Definitions

namespace characters {

DB_ENTRY
soldier {
    .type = entity_type::player,
    .type_name = "soldier",
    .gfx = {
        .mesh_name = "res/models/capsule.obj",
    },
    .stats = character_stats_t {
        .health = {
            120
        },
        .movement = {
            .move_speed = 1.0f,
        },
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Character | PhysicsEntityFlags_Dynamic,
        .shapes = {
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::SPHERE,
                .sphere = {
                    .radius = 1.0f,
                },
            },
        },
        // .shape = physics::collider_shape_type::CAPSULE,
        // .shape_def = {
        //     .capsule = {
        //         .radius = 1.0f,
        //         .height = 3.0f,
        //     },
        // },
    },
    .brain_type = brain_type::player,
};

DB_ENTRY
assassin {
    .type = entity_type::player,
    .type_name = "assassin",
    .gfx = {
        .mesh_name = "res/models/capsule.obj",
    },
    .stats = character_stats_t {
        .health = {
            80
        },
        .movement = {
            .move_speed = 1.3f,
        },
    },
    .physics = zyy::prefab_t::physics_t {
        .flags = PhysicsEntityFlags_Character,
        .shapes = { 
            zyy::prefab_t::physics_t::shape_t{
                .shape = physics::collider_shape_type::CAPSULE,
                .capsule = {
                    .radius = 1.0f,
                    .height = 3.0f,
                },
            },
        },
    },
    .brain_type = brain_type::player,
};

}; // namespace characters

using namespace std::string_view_literals;

template <size_t N>
using entity_lut_t = std::array<std::pair<std::string_view, const zyy::prefab_t*>, N>;

constexpr static entity_lut_t<2> gs_database{{
    {"pistol", &weapons::pistol},
    {"shotgun", &weapons::shotgun},
}};

template <size_t N>
constexpr const zyy::prefab_t*
query(
    std::string_view name,
    const entity_lut_t<N>& database
) noexcept {
    for (const auto& [key, val]: database) {
        if (key == name) {
            return val;
        }
    }
    return nullptr;
}

}; // namespace db
