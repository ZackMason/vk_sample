#include "App/Game/Items/lightning_effect.hpp"

void spawn_beam(
    ztd::world_t* w,
    ztd::entity_t* e1,
    ztd::entity_t* e2
) {
    auto p1 = e1->global_transform().origin;
    auto p2 = e2->global_transform().origin;
    auto p = (p1 + p2) * 0.5f;
    auto dist = glm::distance(p1, p2) * 0.50f;

    // auto prefab = ztd::db::particle::orb;
    auto prefab = ztd::db::particles::beam;
    prefab.emitter->template_particle.life_time = 10.50f;
    prefab.emitter->template_particle.velocity = v3f{0.0f};
    // prefab.emitter->max_count = (u32)(dist/(0.6f));

    auto* sparks = ztd::tag_spawn(w, prefab, p);
    sparks->coroutine->start();
    sparks->gfx.material_id = 7;

    auto* ps = sparks->gfx.particle_system;
    assert(ps);

    ps->box.min.z = -dist;
    ps->box.max.z = dist;
    sparks->transform.look_at(p1);

    DEBUG_DIAGRAM(p1);
    DEBUG_DIAGRAM(p2);
    DEBUG_DIAGRAM(p);
}


export_fn(void) lightning_on_hit_effect(
    ztd::world_t* w, 
    ztd::item::effect_t* effect,
    ztd::entity_t* self_entity,
    ztd::entity_t* entity,
    v3f p, v3f nrm
) {
    TIMED_FUNCTION;
    ztd_warn(__FUNCTION__, "hit: {}", (void*)entity);

    assert(w);
    assert(effect);
    assert(entity);

    lightning_cache_t* cache = 0;

    // arena_t temp = arena_create(kilobytes(1));
    auto memory = begin_temporary_memory(&w->arena);
    arena_t* arena = memory.arena; 
    // auto mark = arena_get_mark(arena);
    arena_t stack_arena = w->frame_arena.get();

    umm top=0;
    umm count=1;
    ztd::entity_t** stack = push_struct<ztd::entity_t*>(&stack_arena);
    stack[top++] = entity;

    *utl::hash_get(&cache, entity, arena) = 1;

    while(top) {
        auto* e = stack[--top];
        assert(e);

        if (e->stats.character.health.damage(1.0f)) {
            e->queue_free();
        }

        auto* near = w->physics->sphere_overlap_world(
            arena,
            e->global_transform().origin,
            10.0f
        );

        if (near) {
            u32 actual_hit = 0;
            range_u64(i, 0, near->hit_count) {
                auto* rb = (physics::rigidbody_t*)near->hits[i].user_data;
                auto* n = (ztd::entity_t*)rb->user_data;

                if (n == e || n->type != ztd::entity_type::bad) {
                    continue;
                }

                auto* b = utl::hash_get(&cache, n, arena);

                if (*b) continue;
                *b = 1;             

                spawn_beam(w, e, n);

                actual_hit++;

                if (top + 1 > count) {
                    push_struct<ztd::entity_t*>(&stack_arena);
                    count++;
                }
                stack[top++] = n;

            }
            // ztd_info(__FUNCTION__, "hit: {} {}/{}", (void*)e, actual_hit, near->hit_count);
        }
    }

    end_temporary_memory(memory);

    // arena_clear(&temp);
    // arena_set_mark(arena, mark);

}
