#include "core.hpp"

#include <cassert>
#include <exception>
#include <execution>


static size_t tests_run = 0;
static size_t tests_passed = 0;

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
}

constexpr auto throw_assert(const bool b) -> auto {
    throw_assert(b, "Assert Failed");
}

#define TEST_ASSERT( x ) throw_assert(x, "TEST FAILED: " #x);

template <typename Fn>
auto run_test(const char* name, const Fn& test) -> auto {
    ++tests_run;
    try {
        // profile_t p{name};
        test();
        ++tests_passed;
        fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold,
            "{} - Passed\n", name);//, p.end());
    } catch (std::exception & e) {
        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold,
            "{} - Failed\n\n", name);
        fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold,
            "{}\n\n", e.what());
    }
}

// template<>
// void utl::memory_blob_t::serialize<v2f>( arena_t* arena, v2f& v) {
//     serialize(arena, v.x);
//     serialize(arena, v.y);
// }

int main(int argc, char** argv) {
    run_test("rng", [] {
        using namespace utl::rng;

        random_t<xor32_random_t> xor32;
        random_t<xor64_random_t> xor64;
        random_t<xoshiro256_random_t> xor256;
        xor32.randomize();
        xor64.randomize();
        xor256.randomize();

        constexpr size_t rng_test_count = 100'000;

        for (size_t i = 0; i < rng_test_count; i++) {
            const auto rf32 = xor32.randf();
            const auto rn32 = xor32.randn();
            const auto rf64 = xor64.randf();
            const auto rn64 = xor64.randn();
            const auto rf256 = xor256.randf();
            const auto rn256 = xor256.randn();

            TEST_ASSERT(rf32 <= 1.0f && rf32 >= 0.0f);
            TEST_ASSERT(rn32 <= 1.0f && rn32 >= -1.0f);
            TEST_ASSERT(rf64 <= 1.0f && rf64 >= 0.0f);
            TEST_ASSERT(rn64 <= 1.0f && rn64 >= -1.0f);
            TEST_ASSERT(rf256 <= 1.0f && rf256 >= 0.0f);
            TEST_ASSERT(rn256 <= 1.0f && rn256 >= -1.0f);
        }
    });

    run_test("offset data structures", [] {
        u32 test = 0x69;
        utl::offset_pointer_t<u32> ptr = &test;

        TEST_ASSERT(*ptr == test);
    });

    run_test("string_t", [] {
        string_t s1;
        s1.view("hello world");

        TEST_ASSERT(s1 == "hello world");
    });

    run_test("math", [] {
        for (f32 x = -360.0f; x < 360.0f; x += 1.5f) {
            for (f32 y = -90.0f; y < 90.0f; y += 1.5f) {
                TEST_ASSERT(glm::length(math::get_spherical(x,y)) <= 1.01f);
            }
        }
    });

    run_test("serialize", [] {
        constexpr size_t arena_size = megabytes(32);
        arena_t arena = arena_create(new u8[arena_size], arena_size);

        const u64 u_data = 4321;
        const f64 d_data = 3.14159;
        const v2f v_data = v2f{3.0, 4.0};
        utl::offset_pointer_t<u64> op_data = &u_data;

        u64 real_a_data[4] = {0,1,2,3};
        utl::offset_array_t<u64> a_data{real_a_data, 4};
        

        TEST_ASSERT(*op_data.get() == u_data);


        utl::memory_blob_t blob{&arena};
        blob.serialize(&arena, u_data);
        blob.serialize(&arena, d_data);
        blob.serialize(&arena, v_data);
        blob.serialize(&arena, op_data);
        blob.serialize(&arena, a_data);

        u64 read_u_data;
        f64 read_d_data;
        v2f read_v_data;
        utl::offset_pointer_t<u64> read_op_data;
        utl::offset_pointer_t<u64> read_a_data;

        blob.reset();
        
        TEST_ASSERT((read_u_data = blob.deserialize<u64>()) == u_data);
        TEST_ASSERT((read_d_data = blob.deserialize<f64>()) == d_data);
        TEST_ASSERT((read_v_data = blob.deserialize<v2f>()) == v_data);
        blob.deserialize_ptr<u64>(&read_op_data);
        TEST_ASSERT(*op_data.get() == u_data);
        blob.deserialize_ptr<u64>(&read_a_data);
        for (size_t i = 0; i < 4; i++) {
            // TEST_ASSERT(read_a_data.get()[i] == real_a_data[i]);
        }
    });


    const auto fmt_color = tests_passed == 0 ? fg(fmt::color::crimson) : 
                            tests_passed == tests_run ? fg(fmt::color::green) : 
                                                        fg(fmt::color::yellow);
    fmt::print(fmt_color | fmt::emphasis::bold,
        "===================================================\n");
    fmt::print(fmt_color | fmt::emphasis::bold,
        "{} / {} Tests Passed\n", tests_passed, tests_run);

    return safe_truncate_u64(tests_run - tests_passed);
}


