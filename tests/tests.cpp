#include "core.hpp"

#include <cassert>
#include <exception>
#include <execution>


#include "App/Game/Entity/entity.hpp"
#include "App/Game/Items/base_item.hpp"
#include "App/Game/Weapons/base_weapon.hpp"

#include "uid.hpp"

static size_t tests_run = 0;
static size_t tests_passed = 0;
static size_t asserts_passed = 0;

struct test_failed : std::exception {
    std::string message;
    test_failed(std::string&& text){ 
        message = std::move(text);
    }

    const char* what() const noexcept override {
        return message.c_str();
    }
};

constexpr auto throw_assert(const bool b, std::string&& text) -> auto {
    if (!b) throw test_failed(std::move(text));
    asserts_passed++;
}

constexpr auto throw_assert(const bool b) -> auto {
    throw_assert(b, "Assert Failed");
}

#define TEST_ASSERT( x ) throw_assert((x), "TEST FAILED: " #x);

template <typename Fn>
auto run_test(const char* name, const Fn& test) -> auto {
    ++tests_run;
    try {
        // utl::profile_t p{name};
        test();
        // const auto time_delta = p.end();
        ++tests_passed;
        
        fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold,
            "{} - Passed\n", name);
            // time_delta < 1000 ? time_delta : time_delta < 1000000 ? time_delta/1000 : time_delta / 1'000'000,
            // time_delta < 1000 ? "ns" : time_delta < 1000000 ? "us" : "ms");

    } catch (std::exception & e) {
        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold,
            "{} - Failed\n\n", name);
        fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold,
            "{}\n\n", e.what());
    }
}

struct item_t {
    u64 id;
};

struct entity_t {
    u64 id;
    v2f pos;
    u64 parent;
    // utl::offset_pointer_t<item_t> item;
};

struct scene_t {
    utl::offset_array_t<entity_t> entities;
};

template<>
void utl::memory_blob_t::serialize<scene_t>( arena_t* arena, const scene_t& s) {
    serialize(arena, s.entities);
}

// template<>
// void utl::memory_blob_t::deserialize<scene_t>(scene_t* s) {
//     deserialize(s->entities);
// }

template<>
void utl::memory_blob_t::serialize<entity_t>( arena_t* arena, const entity_t& e) {
    // gen_info("entity", "Saving entity: {}", (void*)&e);
    serialize(arena, e.id);
    serialize(arena, e.pos);
    // serialize(arena, e.item);
    serialize(arena, e.parent);
}

template<>
entity_t utl::memory_blob_t::deserialize<entity_t>() {
    // gen_info("entity", "Loading entity");
    entity_t e;
    e.id = deserialize<u64>();
    e.pos = deserialize<v2f>();
    e.parent = deserialize<u64>();
    // deserialize(&e.parent);
    return e;
}

struct transform {
    v3f origin;
    v3f rotation;
    v3f scale;
    math::aabb_t<v3f> aabb;
};


REFLECT_TYPE(math::aabb_t<v3f>) {
    REFLECT_TYPE_INFO(math::aabb_t<v3f>)
    }
    .REFLECT_PROP(math::aabb_t<v3f>, min)
    .REFLECT_PROP(math::aabb_t<v3f>, max);

};

REFLECT_TYPE(transform) {
    REFLECT_TYPE_INFO(transform)
    }
    .REFLECT_PROP(transform, aabb)
    .REFLECT_PROP(transform, origin)
    .REFLECT_PROP(transform, scale)
    .REFLECT_PROP(transform, rotation);
};


void print_properties(auto obj) {
    const auto type = reflect::type<decltype(obj)>::info;
    for(auto& p: std::span{type.properties, type.property_count}) {
        std::byte* data = new std::byte[p.size];
        defer {
            delete [] data;
        };
        
        if (p.type) {
            gen_info("refl", "{}: {} = {} - ({} offset, {} bytes)", p.name.data(), p.type->name, p.get_value(obj, data), p.offset, p.size);
            print_properties(*p.type);
        } else {
            gen_warn("refl", "Unregistered type");
        }
    }
}

#define RUN_TEST(x) \
    run_test(x, [&] {\
        utl::profile_t p{x};

int main(int argc, char** argv) {
    using namespace std::string_view_literals;

    utl::profile_t p{"Total Run Time"};
    RUN_TEST("control")
        
    });

    RUN_TEST("uid")
        uid::id_type id = 0;
        TEST_ASSERT(uid::is_valid(id));
        TEST_ASSERT(uid::index(id) == 0);
        TEST_ASSERT(uid::new_generation(id) != id);
        
        TEST_ASSERT(uid::index(1) == 1);
        TEST_ASSERT(uid::index(uid::new_generation(1)) == 1);
    });
        

    // RUN_TEST("ecs")
    //     constexpr size_t arena_size = megabytes(32);
    //     arena_t arena = arena_create(new u8[arena_size], arena_size);
        
    //     using namespace game;

    //     entity::ecs_world_t* w = entity::world_init(&arena, 24);
    //     auto e = w->get_entity(w->create_entity());
    //     TEST_ASSERT(e.is_valid());

    //     TEST_ASSERT(e.transform(w).is_valid() == false);

    //     transform::component_t* tc;
    //     get_component(w, e.get_id(), &tc); // another way to get components
    //     TEST_ASSERT(tc && tc->is_valid() == false);

    //     transform::init_info_t t;
    //     script::init_info_t s;
    //     s.init = script::detail::create_script<test_script_t>;

    //     transform::create(w, t, e);
    //     script::create(w, s, e, &arena);

    //     TEST_ASSERT(e.transform(w).is_valid() == true);
    //     TEST_ASSERT(e.script(w).is_valid() == true);
        
    //     TEST_ASSERT(tc->is_valid() == true);
    //     const auto s_id = uid::index(e.script(w).get_id());
    //     auto* script = w->scripts[s_id];
    //     script->begin_play();

    //     delete [] arena.start;
    // });

    RUN_TEST("rng")
        using namespace utl::rng;

        random_t<xor32_random_t> xor32;
        random_t<xor64_random_t> xor64;
        random_t<xoshiro256_random_t> xor256;
        // xor32.randomize();
        // xor64.randomize();
        // xor256.randomize();

        constexpr size_t rng_test_count = 100'000;

        math::statistic_t stats[3];
        math::begin_statistic(stats[0]);
        math::begin_statistic(stats[1]);
        math::begin_statistic(stats[2]);

        for (size_t i = 0; i < rng_test_count; i++) {
            const auto rf32 = xor32.randf();
            const auto rn32 = xor32.randn();
            const auto rf64 = xor64.randf();
            const auto rn64 = xor64.randn();
            const auto rf256 = xor256.randf();
            const auto rn256 = xor256.randn();

            math::update_statistic(stats[0], (f64)rf32);
            math::update_statistic(stats[1], (f64)rf64);
            math::update_statistic(stats[2], (f64)rf256);

            TEST_ASSERT(rf32 <= 1.0f && rf32 >= 0.0f);
            TEST_ASSERT(rn32 <= 1.0f && rn32 >= -1.0f);
            TEST_ASSERT(rf64 <= 1.0f && rf64 >= 0.0f);
            TEST_ASSERT(rn64 <= 1.0f && rn64 >= -1.0f);
            TEST_ASSERT(rf256 <= 1.0f && rf256 >= 0.0f);
            TEST_ASSERT(rn256 <= 1.0f && rn256 >= -1.0f);
        }

        math::end_statistic(stats[0]);
        math::end_statistic(stats[1]);
        math::end_statistic(stats[2]);
        TEST_ASSERT(std::fabs(stats[0].average - 0.5) < 1e-2);
        TEST_ASSERT(std::fabs(stats[1].average - 0.5) < 1e-2);
        TEST_ASSERT(std::fabs(stats[2].average - 0.5) < 1e-2);

    });

    
    RUN_TEST("offset data")
        u32 test = 0x69;
        utl::offset_pointer_t<u32> ptr = &test;

        TEST_ASSERT(*ptr == test);

        utl::offset_pointer_t<u32> copied_ptr = ptr;
        TEST_ASSERT(*copied_ptr == test);

        utl::offset_pointer_t<u32> moved_ptr = std::move(ptr);
        TEST_ASSERT(*moved_ptr == test);

        std::byte* mapped_ptr = new std::byte[sizeof(utl::offset_pointer_t<u32>) + sizeof(u32)];
        
        *((u32*)(&(((utl::offset_pointer_t<u32>*)mapped_ptr)[1]))) = 99;
        utl::offset_pointer_t<u32>& mapped_ptr_ref = *(utl::offset_pointer_t<u32>*)mapped_ptr;
        mapped_ptr_ref.offset = 8;
        TEST_ASSERT(*mapped_ptr_ref == 99);
    });

    RUN_TEST("str")
        string_t s1;
        s1.view("hello world");

        TEST_ASSERT(s1 == "hello world");
        TEST_ASSERT(s1 == s1);
    });

    RUN_TEST("math")
        math::transform_t t;
        t.origin = v3f{1.0f, 0.0f, 0.0f};
        v3f o{-1.0f, 0.0f, 0.0f};
        TEST_ASSERT(t.inv_xform(v3f{0.0f}) == o);

        for (f32 x = -360.0f; x < 360.0f; x += 1.5f) {
            for (f32 y = -90.0f; y < 90.0f; y += 1.5f) {
                TEST_ASSERT(glm::length(math::get_spherical(x,y)) <= 1.01f);
            }
        }
    });

    RUN_TEST("ray")
        math::aabb_t<v3f> box;
        box.expand(v3f{-1.0f});
        box.expand(v3f{1.0f});
        math::ray_t ray;
        ray.direction = v3f{0,0,1};
        ray.origin = v3f{0,0,-5};
        
        TEST_ASSERT(box.contains(box));
        TEST_ASSERT(math::intersect(ray, box).intersect);
        TEST_ASSERT(math::intersect(ray, box).distance == 4.0f);
    });

    RUN_TEST("dynarray")
        utl::dynarray_t<std::string> array;

        range_u64(i, 0, 10) {
            array.push_back(fmt_str("helllllllllllllloooooooo: {}", i));
        }

        TEST_ASSERT(array.size() == 10);

        array.resize(15, "foo");
        TEST_ASSERT(array.size() == 15);

        array.remove(0);
        TEST_ASSERT(array.size() == 14);
        TEST_ASSERT(array[11] == "foo");

    });

    struct foo_t {
        float hp{100.0f};
        transform t{};
    };

    RUN_TEST("reflection")
        v3f test{10.0f};
        constexpr auto vec3_type = reflect::type<v3f>::info;

        static_assert(reflect::type<v2f>{}==false);
        static_assert(vec3_type.name == "v3f");
        static_assert(vec3_type.size == sizeof(v3f));

        // for(auto& property : std::span{vec3_type.properties, vec3_type.property_count}) {
        //     puts(property.name.data());
        // }
            
        constexpr auto x_prop = vec3_type.get_property("x");
        static_assert(x_prop.size == sizeof(f32));

        x_prop.set_value(test, 0.0f);
        TEST_ASSERT(test.x == 0.0f);
    });

    RUN_TEST("reflection2")
        static_assert(reflect::type<f32>::info.name == "f32");
        static_assert(reflect::type<glm::vec3>::info.name == "v3f");
        static_assert(reflect::type<glm::vec3>::info == reflect::type<v3f>::info);
        // print_properties(v3f{1,2,3});
        // print_properties(transform{});
    });

    RUN_TEST("serialize")
        constexpr size_t arena_size = megabytes(32);
        arena_t arena = arena_create(new u8[arena_size], arena_size);

        const u64 u_data = 4321;
        const f64 d_data = 3.14159;
        const v2f v_data = v2f{3.0, 4.0};
        utl::offset_pointer_t<u64> uop_data = &u_data;
        utl::offset_pointer_t<f64> dop_data = &d_data;
        std::string_view s = "Hello";
        string_t st;
        st.view(s.data());

        u64 real_a_data[4] = {0,1,2,3};
        utl::offset_array_t<u64> a_data{real_a_data, 4};

        TEST_ASSERT(*uop_data.get() == u_data);
        TEST_ASSERT(*dop_data.get() == d_data);

        utl::memory_blob_t blob{&arena};
        blob.serialize(&arena, u_data);
        blob.serialize(&arena, d_data);
        blob.serialize(&arena, v_data);
        blob.serialize(&arena, uop_data);
        blob.serialize(&arena, dop_data);
        blob.serialize(&arena, a_data);
        blob.serialize(&arena, std::string_view{st.c_data});

        u64 read_u_data;
        f64 read_d_data;
        v2f read_v_data;
        utl::offset_pointer_t<u64> read_uop_data;
        utl::offset_pointer_t<f64> read_dop_data;
        utl::offset_array_t<u64> read_a_data;

        TEST_ASSERT((read_u_data = blob.deserialize<u64>()) == u_data);
        TEST_ASSERT((read_d_data = blob.deserialize<f64>()) == d_data);
        TEST_ASSERT((read_v_data = blob.deserialize<v2f>()) == v_data);
        
        blob.deserialize(&read_uop_data);
        TEST_ASSERT(*read_uop_data.get() == u_data);

        blob.deserialize(&read_dop_data);
        TEST_ASSERT(*read_dop_data.get() == d_data);
        
        blob.deserialize(&read_a_data);
        for (size_t i = 0; i < 4; i++) {
            TEST_ASSERT(read_a_data[i] == real_a_data[i]);
        }

        TEST_ASSERT(blob.deserialize<std::string>() == "Hello");
        delete [] arena.start;
    });

    RUN_TEST("serialize scene")
        constexpr size_t arena_size = megabytes(32);
        arena_t arena = arena_create(new u8[arena_size], arena_size);
        arena_t scene_arena = arena_create(new u8[arena_size], arena_size);
        defer {
            delete [] arena.start;
            delete [] scene_arena.start;
        };

        scene_t scene;

        auto* data = arena_alloc_ctor<entity_t>(&scene_arena, 3);
        auto* items = arena_alloc_ctor<item_t>(&scene_arena, 2);
        scene.entities = utl::offset_array_t{data, 3};

        data[0].id = 1;
        data[1].id = 2;
        data[2].id = 3;

        data[0].parent =~0ui64;
        data[1].parent = 1;
        data[2].parent = 2;

        items[0].id = 4;
        items[1].id = 5;
        // data[0].item = items + 0;
        // data[1].item = items + 1;

        utl::memory_blob_t blob{&arena};
        blob.serialize(&arena, scene);

        scene_t loaded_scene;

        blob.deserialize(&loaded_scene.entities);

        TEST_ASSERT(loaded_scene.entities.count == 3);
        TEST_ASSERT(loaded_scene.entities[0].id == 1);
        TEST_ASSERT(loaded_scene.entities[1].id == 2);
        TEST_ASSERT(loaded_scene.entities[2].id == 3);

        TEST_ASSERT(loaded_scene.entities[0].parent == ~0ui64);
        TEST_ASSERT(loaded_scene.entities[1].parent == 1);
        TEST_ASSERT(loaded_scene.entities[2].parent == 2);
        
        // TEST_ASSERT(loaded_scene.entities[0].item.get()->id == 4);
        // TEST_ASSERT(loaded_scene.entities[1].item.get()->id == 5);
        // TEST_ASSERT(loaded_scene.entities[2].item.is_null() == true);
    });

    RUN_TEST("pack file")
        constexpr size_t arena_size = megabytes(64);
        arena_t arena = arena_create(new u8[arena_size], arena_size);
        defer {
            delete [] arena.start;
        };

        utl::res::pack_file_t* file = utl::res::load_pack_file(
            &arena, &arena, "./res/res.pack"
        );

        TEST_ASSERT(file != nullptr);
        TEST_ASSERT(file->file_count > 0);
    });

    constexpr u64 test_size = 10000000;
    RUN_TEST("vector control")
        std::vector<u64> vec(test_size);

        // TEST_ASSERT(vec.size() == 0);
        // TEST_ASSERT(vec.empty());


        for (size_t i = 0; i < test_size; i++) {
            // vec.push_back(i);
            vec[i] = i;
        }

        TEST_ASSERT(vec.size() == test_size);
        TEST_ASSERT(vec.front() == 0);
        TEST_ASSERT(vec.back() == test_size-1);
    });

    RUN_TEST("c array")
        u64* vec = (u64*)malloc(sizeof(u64)*test_size);

        for (size_t i = 0; i < test_size; i++) {
            vec[i]=i;
        }

        TEST_ASSERT(vec[0] == 0);
        TEST_ASSERT(vec[test_size-1] == test_size-1);

        free(vec);
    });

    RUN_TEST("dynarray perf")
        utl::dynarray_t<u64> vec(test_size);

        // TEST_ASSERT(vec.size() == 0);
        // TEST_ASSERT(vec.is_empty());

        // constexpr u64 test_size = 100000;

        for (size_t i = 0; i < test_size; i++) {
            // vec.push_back(i);
            vec[i]=i;
        }

        TEST_ASSERT(vec.size() == test_size);
        TEST_ASSERT(vec.front() == 0);
        TEST_ASSERT(vec.back() == test_size-1);
    });

    RUN_TEST("deque")
        constexpr size_t arena_size = megabytes(640);
        arena_t arena = arena_create(new u8[arena_size], arena_size);
        defer {
            delete [] arena.start;
        };

        struct ent : public node_t<ent> {
            u64 id;

            operator u64() {
                return id;
            }

            ent(u64 _id) : id{_id} {

            }
        };

        utl::deque<ent> deque;
        TEST_ASSERT(deque.size() == 0);
        TEST_ASSERT(deque.is_empty());

        // constexpr u64 test_size = 100000;

        for (size_t i = 0; i < test_size; i++) {
            deque.push_back(arena_alloc_ctor<ent>(&arena, 1, i));
        }

        TEST_ASSERT(deque.size() == test_size);
        TEST_ASSERT(deque.front());
        TEST_ASSERT(deque.front()->id == 0);
        TEST_ASSERT(deque.back());
        TEST_ASSERT(deque.back()->id == test_size-1);

    });

    RUN_TEST("entity db")
        constexpr size_t arena_size = megabytes(1);
        arena_t arena = arena_create(new u8[arena_size], arena_size);
        defer {
            delete [] arena.start;
        };
        
        u32 child_count = 0;
        for (size_t i = 0; i < array_count(game::db::characters::soldier.children); i++) {
            child_count += !!game::db::characters::soldier.children[i].entity;
        }
        TEST_ASSERT(child_count == 2);
        
        const auto& p0 = game::db::characters::soldier;
        const auto& p1 = game::db::rooms::map_01;
        TEST_ASSERT(p0.stats->health.max == 120);
        TEST_ASSERT(p0.stats->health.max == p0.stats->health.current);
        
        TEST_ASSERT(!p0.emitter);

        utl::memory_blob_t blob{&arena};
        blob.serialize(&arena, p0);
        blob.serialize(&arena, p1);

        auto r0 = blob.deserialize<game::db::prefab_t>();
        auto r1 = blob.deserialize<game::db::prefab_t>();
        
        TEST_ASSERT(r0.stats->health.max == 120);
        TEST_ASSERT(r0.type_name == "soldier"sv);
        TEST_ASSERT(!r1.stats);
    });

    RUN_TEST("weapons")
        static constexpr size_t arena_size = kilobytes(4); // should use little memory
        arena_t arena = arena_create(new std::byte[arena_size], arena_size);
        defer {
            delete [] arena.start;
        };

        using namespace game;

        wep::base_weapon_t rifle = wep::create_rifle();
        wep::base_weapon_t shotgun = wep::create_shotgun();

        item::augment_weapon::function_type items[] = {
            item::augment_weapon::double_damage,
            item::augment_weapon::hair_trigger
        };

        item::effect_t fire_effect{
            .type = item::effect_type::FIRE,
            .rate = 1.0f,
            .strength = 1.0f
        };
        
        item::effect_t ice_effect{
            .type = item::effect_type::ICE,
            .rate = 1.0f,
            .strength = 1.0f,
            .next = &fire_effect
        };

        rifle.effects = shotgun.effects = &fire_effect;

        for (size_t i = 0; i < array_count(items); i++) {
            rifle = items[i](rifle);
            shotgun = items[i](shotgun);
        }

        u64 rifle_bullet_count{0};
        wep::bullet_t* rifle_bullets = rifle.fire(&arena, 5.0f, &rifle_bullet_count);

        u64 shotgun_bullet_count{0};
        wep::bullet_t* shotgun_bullets = shotgun.fire(&arena, 5.0f, &shotgun_bullet_count);

        TEST_ASSERT(rifle.fire_rate == 0.5f);
        TEST_ASSERT(rifle.stats.damage == 2.0f);

        TEST_ASSERT(rifle_bullet_count == 9);
        TEST_ASSERT(shotgun_bullet_count == 72);

        for (size_t i = 0; i < rifle_bullet_count; i++) {
            TEST_ASSERT(rifle.stats.damage == rifle_bullets[i].damage);
        }
        for (size_t i = 0; i < shotgun_bullet_count; i++) {
            TEST_ASSERT(shotgun.stats.damage == shotgun_bullets[i].damage);
        }
    });

    RUN_TEST("script hashing")
        using namespace game;
        using namespace utl::rng;
        
        static constexpr size_t arena_size = kilobytes(4);
        arena_t arena = arena_create(new std::byte[arena_size], arena_size);
        defer {
            delete [] arena.start;
        };

        struct player_t {
            USCRIPT(player_t);
            u32 score{0};
            void end_play(world_t* world) {}
            void begin_play(world_t* world) {
                gen_info(__FUNCTION__, "type: {} - id: {}", script_name(), id);
            }

            void update(world_t* world, f32 dt) {
                score++;
            }
        };

        struct enemy_t {
            USCRIPT(enemy_t);
            u64 value{0};
            f32 strength{1.0f};
            void end_play(world_t* world) {}
            void begin_play(world_t* world) {
                gen_info(__FUNCTION__, "type: {} - id: {}", script_name(), id);
            }

            void update(world_t* world, f32 dt) {
            }
        };
        static_assert(CScript<player_t>);
        static_assert(CScript<enemy_t>);

        script_manager_t<player_t, enemy_t> scripts{0};

        struct script_hash_t {
            script_hash_t*  next{nullptr};
            u64             type{~0ui64};
            script_id       id{uid::invalid_id};
        };

        player_t players[4];
        enemy_t enemies[128];
        static script_hash_t* hash[1 << 5];


        range_u64(i, 0, array_count(players)) {
            scripts.add_script(players + i);

            const u64 id_hash = fnv_hash_u64(players[i].id);
            const u64 hash_bucket = id_hash & (array_count(hash) - 1);

            script_hash_t script_hash{};
            script_hash.id = players[i].id;
            script_hash.type = scripts.get_type_index<player_t>();

            if (script_hash_t* last_hash = hash[hash_bucket]) {
                auto* node = arena_alloc<script_hash_t>(&arena);
                *node = script_hash;
                node->next = last_hash;
                hash[hash_bucket] = node;
            } else {
                hash[hash_bucket] = arena_alloc<script_hash_t>(&arena);
                *hash[hash_bucket] = script_hash;
            }
        }
        range_u64(i, 0, array_count(enemies)) {
            scripts.add_script(enemies + i);

            const u64 id_hash = fnv_hash_u64(enemies[i].id);
            const u64 hash_bucket = id_hash & (array_count(hash) - 1);

            script_hash_t script_hash{};
            script_hash.id = enemies[i].id;
            script_hash.type = scripts.get_type_index<enemy_t>();

            if (script_hash_t* last_hash = hash[hash_bucket]) {
                auto* node = arena_alloc<script_hash_t>(&arena);
                *node = script_hash;
                node->next = last_hash;
                hash[hash_bucket] = node;
            } else {
                hash[hash_bucket] = arena_alloc<script_hash_t>(&arena);
                *hash[hash_bucket] = script_hash;
            }
        }
    });
    
    
    const auto fmt_color = tests_passed == 0 ? fg(fmt::color::crimson) : 
                            tests_passed == tests_run ? fg(fmt::color::green) : 
                                                        fg(fmt::color::yellow);
    fmt::print(fmt_color | fmt::emphasis::bold,
        "===================================================\n");
    fmt::print(fmt_color | fmt::emphasis::bold,
        "{} / {} Tests Passed - {} Cases\n", tests_passed, tests_run, asserts_passed);

    return safe_truncate_u64(tests_run - tests_passed);
}


