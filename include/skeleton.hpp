#ifndef SKELETON_HPP
#define SKELETON_HPP

#include "core.hpp"

namespace gfx {

struct material_info_t {
    using stack_string_t = char[256];
    stack_string_t textures[8]{};
    stack_string_t properties[32]{};
    u8 transparency{0};
};

namespace anim {

using bone_id_t = i32;

struct skeleton_bone_t {
    u64 name_hash{"invalid"_sid};
    bone_id_t parent{-1};
    m44 offset{1.0f};
};

struct skeleton_t {
    static constexpr u64 max_bones_() { return 256; }
    skeleton_bone_t bones[256];
    u64          bone_count{0};

    constexpr skeleton_t& add_bone(skeleton_bone_t&& bone) {
        assert(bone_count < max_bones());
        bones[bone_count++] = std::move(bone);
        return *this;
    }

    constexpr [[nodiscard]] bone_id_t 
    find_bone_id(string_id_t bone_hash) const {
        for (i32 i = 0; i < bone_count; i++) {
            if (bones[i].name_hash == bone_hash) return i;
        }
        return -1;
    }
    
    constexpr [[nodiscard]] const skeleton_bone_t& 
    find_bone(string_id_t bone_name) const {
        return bones[find_bone_id(bone_name)];
    }

    // void load(const std::string& path);

    constexpr u64 max_bones() const noexcept { return max_bones_(); }
};

struct skeletal_mesh_asset_t {
    gfx::skinned_vertex_t* vertices{0};
    u64 vertex_count{0};
    gfx::material_info_t* material_info{0};
};

struct skeletal_model_asset_t {
    skeleton_t skeleton{};
    skeletal_mesh_asset_t* meshes{0};
    u64 mesh_count{0};
};

template <typename T>
struct keyframe {
    f32 time{0.0f};
    T value{};
};

template <typename T>
using anim_pool_t = keyframe<T>[512];

struct bone_timeline_t {
    u64 position_count{0};
    u64 rotation_count{0};
    u64 scale_count{0};
    anim_pool_t<v3f>        positions;
    anim_pool_t<glm::quat>  rotations;
    anim_pool_t<v3f>        scales;

    m44 transform{};
    char name[1024]{};
    bone_id_t id{};

    // bone_timeline_t(std::string_view pname, bone_id_t pID, const aiNodeAnim* channel);

    size_t get_index(auto& pool, size_t pool_count, f32 time) const {
        for (size_t i = 0; i < pool_count; i++) {
            if (time < pool[i].time) {
                return i;
            }
        }
        assert(0);
        return std::numeric_limits<size_t>::max();
    }
};

struct animation_t {
    struct node_t {
        std::optional<bone_timeline_t> bone;
        bone_id_t parent{-1};
        m44 transform{1.0f};
        m44 offset{1.0f};
    };

    char name[256]{};
    f32 duration{0.0f};
    i32 ticks_per_second{24};

    u64 node_count{0};
    std::array<node_t, skeleton_t::max_bones_()> nodes;
};

struct animator_t {
    utl::pool_t<m44> matrices;

};

template <typename T>
inline void
anim_pool_add_time_value(anim_pool_t<T>* pool, size_t* pool_size, f32 time, T val) {
    auto* time_value = pool[0][(*pool_size)++];
    *time_value = keyframe<T>{
        .time = time,
        .value = val
    };
}

template <typename T>
inline T
anim_pool_get_time_value(anim_pool_t<T>* pool, f32 time) {
    if (pool->count == 0) {
        return T{0.0f};
    }

    if (time <= 0.0f) {
        return (*pool)[0].value;
    } else if (time >= (*pool)[pool->count-1].time) {
        return (*pool)[pool->count-1].value;
    }

    for (size_t i = 0; i < pool->count - 1; i++) {
        if (pool->data()[i].time <= time && time <= pool->data()[i+1].time) {
            const auto d = (pool->data()[i+1].time - pool->data()[i].time);
            const auto p = time - pool->data()[i].time;
            if constexpr (std::is_same_v<T, glm::quat>) {
                return glm::slerp(pool[0][i], pool[0][i+1], p/d);
            } else {
                return glm::mix(
                    pool->data()[i].value,
                    pool->data()[i+1].value,
                    p/d
                );
            }
        }
    }

    return T{};
}

}; // namespace anim





};

#endif