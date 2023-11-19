#pragma once

namespace utl {
    
namespace rng {

//constexpr 
inline uint64_t fnv_hash_u64(uint64_t val) {
#if defined(GAME_USE_SIMD) && 0
    const u64 start = 14695981039346656037ull;
    __m256i hash = _mm256_set1_epi64x(start);
    const __m256i zero = _mm256_set1_epi64x(0);
    const __m256i val256 = _mm256_set1_epi64x(val);
    const __m256i prime = _mm256_set1_epi64x(1099511628211ull);
    const __m128i prime128 = _mm_set1_epi64x(1099511628211ull);
    __m256i v0 = _mm256_set_epi64x(0*8,1*8,2*8,3*8);
    __m256i v1 = _mm256_set_epi64x(4*8,5*8,6*8,7*8);

    v0 = _mm256_srlv_epi64(v0, val256);
    v1 = _mm256_srlv_epi64(v1, val256);

    v0 = _mm256_unpacklo_epi8(v0, zero);
    v1 = _mm256_unpacklo_epi8(v1, zero);

    hash = _mm256_xor_si256(hash, v0);
    hash = _mm256_mul_epu32(hash, prime);

    hash = _mm256_xor_si256(hash, v1);
    hash = _mm256_mul_epu32(hash, prime);

    __m128i lo = _mm256_extracti128_si256(hash, 0);
    __m128i hi = _mm256_extracti128_si256(hash, 1);

    lo = _mm_xor_si128(lo, hi);
    lo = _mm_mul_epu32(lo, prime128);
    lo = _mm_xor_si128(lo, _mm_srli_si128(lo, 8));
    lo = _mm_mul_epu32(lo, prime128);
    lo = _mm_xor_si128(lo, _mm_srli_si128(lo, 4));
    lo = _mm_mul_epu32(lo, prime128);
    lo = _mm_xor_si128(lo, _mm_srli_si128(lo, 2));
    lo = _mm_mul_epu32(lo, prime128);
    lo = _mm_xor_si128(lo, _mm_srli_si128(lo, 1));

    return _mm_cvtsi128_si64(lo);
    return start;
#else 
    u64 hash = 14695981039346656037ull;
    for (u64 i = 0; i < 8; i+=4) {
        const u64 v0 = ((val >> (8 * (i+0))) & 0xff);
        const u64 v1 = ((val >> (8 * (i+1))) & 0xff);
        const u64 v2 = ((val >> (8 * (i+2))) & 0xff);
        const u64 v3 = ((val >> (8 * (i+3))) & 0xff);
        hash = hash ^ v0;
        hash = hash * 1099511628211ull;
        hash = hash ^ v1;
        hash = hash * 1099511628211ull;
        hash = hash ^ v2;
        hash = hash * 1099511628211ull;
        hash = hash ^ v3;
        hash = hash * 1099511628211ull;
    }
    return hash;
#endif
}

struct xoshiro256_random_t {
    uint64_t state[4];
    constexpr static uint64_t max = std::numeric_limits<uint64_t>::max();

    void randomize() {
        constexpr u64 comp_time_entropy = sid(__TIME__) ^ sid(__DATE__);
        const u64 this_entropy = utl::rng::fnv_hash_u64((std::uintptr_t)this);
        const u64 pid_entropy = utl::rng::fnv_hash_u64(RAND_GETPID);
        
        state[3] = static_cast<uint64_t>(time(0)) ^ comp_time_entropy;
        state[2] = (static_cast<uint64_t>(time(0)) << 17) ^ this_entropy;
        state[1] = (static_cast<uint64_t>(time(0)) >> 21) ^ (static_cast<uint64_t>(~time(0)) << 13);
        state[0] = (static_cast<uint64_t>(~time(0)) << 23) ^ pid_entropy;
    };

    uint64_t rand() {
        uint64_t *s = state;
        uint64_t const result = rol64(s[1] * 5, 7) * 9;
        uint64_t const t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];

        s[2] ^= t;
        s[3] = rol64(s[3], 45);

        return result;
    }

    uint64_t rol64(uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }
};

struct xor64_random_t {
    uint64_t state;
    constexpr static uint64_t max = std::numeric_limits<uint64_t>::max();

    void randomize() {
        if constexpr (sizeof(time_t) >= 8) {
            constexpr u64 comp_time_entropy = sid(__TIME__) ^ sid(__DATE__);
            const u64 this_entropy = utl::rng::fnv_hash_u64((std::uintptr_t)this);
            // const u64 pid_entropy = utl::rng::fnv_hash_u64(RAND_GETPID);
        
            state = static_cast<uint64_t>(time(0)) + 7 * comp_time_entropy + 23 * this_entropy;
            state = utl::rng::fnv_hash_u64(state);
        }
        else {
            state = static_cast<uint64_t>(time(0) | (~time(0) << 32));
        }
    };

    uint64_t rand(){
        auto x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return state = x;
    }
};

struct xor32_random_t {
    uint32_t state;
    constexpr static uint32_t max = std::numeric_limits<uint32_t>::max();

    void randomize() {
        constexpr u64 comp_time_entropy = sid(__TIME__) ^ sid(__DATE__);
        const u64 this_entropy = utl::rng::fnv_hash_u64((std::uintptr_t)this);
        const u64 pid_entropy = utl::rng::fnv_hash_u64(RAND_GETPID);
    
        state = static_cast<uint32_t>(time(0)) ^ (u32)(comp_time_entropy ^ this_entropy ^ pid_entropy);
        state = (u32)utl::rng::fnv_hash_u64(state);
    };

    uint32_t rand(){
        auto x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state = x;
    }
};

struct pcg_random_t {
    uint64_t state;
    constexpr static uint32_t max = std::numeric_limits<uint32_t>::max();
    constexpr static u64 mult = 6364136223846793005ui64;
    constexpr static u64 incr = 1442695040888963407ui64;

    u32 rotr32(u32 x, u32 r) {
        return x>>r|x<<(-(i32)r&31);
    }

    void randomize() {
        constexpr u64 comp_time_entropy = sid(__TIME__) ^ sid(__DATE__);
        const u64 this_entropy = utl::rng::fnv_hash_u64((std::uintptr_t)this);
        const u64 pid_entropy = utl::rng::fnv_hash_u64(RAND_GETPID);
    
        state = static_cast<uint32_t>(time(0)) ^ (u32)(comp_time_entropy ^ this_entropy ^ pid_entropy);
        state = (u32)utl::rng::fnv_hash_u64(state);
    };

    uint32_t rand() {
        u64 x = state;
        u32 count = (u32)(x>>59);
        state = x * mult + incr;
        x^=x>>18;
        return rotr32((u32)(x>>27), count);
    }
};

template <typename Generator>
struct random_t {
    Generator rng;

    random_t(const decltype(Generator::state)& x = {}) noexcept {
        seed(x);
    }

    void randomize() {
        rng.randomize();
    }

    void seed(const decltype(Generator::state)& x) {
        if constexpr (std::is_integral_v<decltype(Generator::state)>) {
            if (x == 0) {
                randomize();
            } else {
                rng.state = x;
            }
        } else {
            randomize();
        }
    }

    auto norm_dist() {
        const auto theta = math::constants::tau32 * randf();
        const auto rho = glm::sqrt(-2.0f * glm::log(randf()));
        return rho * glm::cos(theta);
    }

    auto rand() {
        return rng.rand();
    }

    float randf() {
        return float(rng.rand()) / float(Generator::max);
    }

    float randn() {
        return randf() * 2.0f - 1.0f;
    }

    template <typename T = v3f>
    T randv() {
        auto v = T{1.0f};
        for (decltype(T::length()) i = 0; i < T::length(); i++) {
            v[i] = randf();
        }
        return v;
    }

    template <typename T>
    T randnv() {
        auto v = T{1.0f};
        for (decltype(T::length()) i = 0; i < T::length(); i++) {
            v[i] = randn();
        }
        return v;
    }

    template <typename T>
    auto choice(const T& indexable) -> const typename T::value_type& {
        return indexable[rng.rand()%indexable.size()];
    }

    f32 range(const math::range_t& range) {
        return this->randf() * range.size() + range.min;
    }

    v3f aabb(const math::rect3d_t& box) {
        return v3f{
            (this->randf() * (box.max.x - box.min.x)) + box.min.x,
            (this->randf() * (box.max.y - box.min.y)) + box.min.y,
            (this->randf() * (box.max.z - box.min.z)) + box.min.z
        };
    }

};

struct random_s {
    inline static random_t<pcg_random_t> state{0};

    template <typename T>
    static auto choice(const T& indexable) -> const typename T::value_type& {
        return state.choice(indexable);
    }

    static void randomize() {
        state.randomize();
    }
    //random float
    static v3f randv() {
        return {randf(), randf(), randf()};
    }
    static v3f randnv() {
        return {randn(), randn(), randn()};
    }
    static float randf() {
        return state.randf();
    }
    static u32 rand() {
        return state.rand();
    }
    // random normal
    static float randn() {
        return state.randn();
    }
    template <typename AABB>
    static auto aabb(const AABB& box) -> decltype(box.min) {
        return state.aabb(box);
    }
};

} // namespace rng

}