#include "App/Game/Items/lightning_effect.hpp"

void spawn_beam(
    zyy::world_t* w,
    zyy::entity_t* e1,
    zyy::entity_t* e2
) {
    auto p1 = e1->global_transform().origin;
    auto p2 = e2->global_transform().origin;
    auto p = (p1 + p2) * 0.5f;
    auto dist = glm::distance(p1, p2) * 0.50f;

    auto prefab = zyy::db::particles::beam;
    // prefab.emitter->max_count = (u32)(dist/(0.6f));

    auto* sparks = zyy::spawn(w, w->render_system(), prefab, p);
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

void lightning_on_hit_effect_impl2(
    zyy::world_t* w, 
    zyy::item::effect_t* effect,
    zyy::entity_t* entity,
    v3f p, v3f n,
    lightning_cache_t* cache
) {
    TIMED_FUNCTION;

    assert(w);
    assert(effect);
    assert(entity);

    arena_t* arena = &w->frame_arena.get();

    auto* b = utl::hash_trie_get_insert(&cache, entity, arena);

    if (*b) {
        // zyy_info(__FUNCTION__, "Already hit");
    } else {
        if (entity->stats.character.health.damage(1.0f)) {
            entity->queue_free();
        }

        // auto* sparks = zyy::spawn(w, w->render_system(), zyy::db::particles::sparks, p);
        // sparks->coroutine->start();
        // sparks->gfx.material_id = 7;

        auto temp_arena = *arena;

        auto* near = w->physics->sphere_overlap_world(
            w->physics,
            &temp_arena,
            entity->global_transform().origin,
            20.0f
        );

        *b = 1;

        if (near) {
            u32 actual_hit = 0;
            range_u64(i, 0, near->hit_count) {
                auto* rb = (physics::rigidbody_t*)near->hits[i].user_data;
                auto* e = (zyy::entity_t*)rb->user_data;

                if (e == entity || e->type != zyy::entity_type::bad) {
                    continue;
                }

                if (*utl::hash_trie_get_insert(&cache, e, arena) == 1) {
                    continue;
                }

                actual_hit++;

                spawn_beam(w, e, entity);

                lightning_on_hit_effect_impl2(
                    w, 
                    effect,
                    e,
                    e->global_transform().origin, axis::up,
                    cache
                );
            }
            // zyy_info(__FUNCTION__, "hit: {}", actual_hit);
        } else {
            // zyy_warn(__FUNCTION__, "No Lightning hit");
        }
    }
}

void lightning_on_hit_effect_impl(
    zyy::world_t* w, 
    zyy::item::effect_t* effect,
    zyy::entity_t* entity,
    v3f p, v3f nrm,
    lightning_cache_t* cache
) {
    TIMED_FUNCTION;
    zyy_warn(__FUNCTION__, "hit: {}", (void*)entity);

    assert(w);
    assert(effect);
    assert(entity);


    arena_t temp = w->arena;
    arena_t* arena = &temp; 
    // auto mark = arena_get_mark(arena);
    arena_t stack_arena = w->frame_arena.get();

    umm top=0;
    umm count=1;
    zyy::entity_t** stack = push_struct<zyy::entity_t*>(&stack_arena);
    stack[top++] = entity;

    *utl::hash_trie_get_insert(&cache, entity, arena) = 1;

    while(top) {
        auto* e = stack[--top];
        assert(e);

        if (e->stats.character.health.damage(1.0f)) {
            e->queue_free();
        }

        auto* near = w->physics->sphere_overlap_world(
            w->physics,
            arena,
            e->global_transform().origin,
            10.0f
        );

        if (near) {
            u32 actual_hit = 0;
            range_u64(i, 0, near->hit_count) {
                auto* rb = (physics::rigidbody_t*)near->hits[i].user_data;
                auto* n = (zyy::entity_t*)rb->user_data;

                if (n == e || n->type != zyy::entity_type::bad) {
                    continue;
                }

                auto* b = utl::hash_trie_get_insert(&cache, n, arena);

                if (*b) continue;
                *b = 1;             

                spawn_beam(w, e, n);

                actual_hit++;

                if (top + 1 > count) {
                    push_struct<zyy::entity_t*>(&stack_arena);
                    count++;
                }
                stack[top++] = n;

            }
            // zyy_info(__FUNCTION__, "hit: {} {}/{}", (void*)e, actual_hit, near->hit_count);
        }
    }
    // arena_set_mark(arena, mark);

}
