#ifndef SKELETON_HPP
#define SKELETON_HPP

#include "zyy_core.hpp"

namespace gfx {

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

template <typename T>
inline T
anim_pool_get_time_value(anim_pool_t<T>& pool, u64 count, f32 time) {
    if (count == 0) {
        return T{};
    } else if (count == 1) {
        return pool[0].value;
    }

    if (time <= 0.0f) {
        return pool[0].value;
    } else if (time >= pool[count-1].time) {
        return pool[count-1].value;
    }

    for (size_t i = 0; i < count - 1; i++) {
        if (pool[i].time <= time && time <= pool[i+1].time) {
            const f32 d = (pool[i+1].time - pool[i].time);
            const f32 p = time - pool[i].time;
            if constexpr (std::is_same_v<T, glm::quat>) {
                return glm::slerp(pool[i].value, pool[i+1].value, p/d);
            } else {
                return glm::mix(
                    pool[i].value,
                    pool[i+1].value,
                    p/d
                );
            }
        }
    }

    return T{};
}

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

    m44 update(f32 time) {
        m44 translation = glm::translate(m44{1.0f}, anim_pool_get_time_value(positions, position_count, time));
        m44 rotation = glm::toMat4(anim_pool_get_time_value(rotations, rotation_count, time));
        m44 scale = glm::scale(m44{1.0f}, anim_pool_get_time_value(scales, scale_count, time));
        return transform = translation * rotation * scale;
    }

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
    std::array<node_t, skeleton_t::max_bones_()> nodes{};
};

struct animator_t {
    f32 time{0.0f};
    animation_t* animation{nullptr};
    std::array<m44, skeleton_t::max_bones_()> matrices;

    void update(float dt) {
        if (animation) {
            time += dt * animation->ticks_per_second;
            time = fmod(time, animation->duration-1.0f);

            for (auto& node : animation->nodes) {
                const auto& bone_transform = node.bone ? node.bone->update(time) : node.transform;
                const auto& parent_transform = (node.parent >= 0) ?
                    animation->nodes[node.parent].transform : m44(1.0f);
                node.transform = parent_transform * bone_transform;
                if(node.bone) {
                    const auto index = node.bone->id;
                    assert(index < matrices.size() && "Too many bones");
                    matrices[index] =  node.transform * node.offset;
                }
            }
        }
    }

    void play_animation(animation_t* p_animation) {
		animation = p_animation;
        std::fill(matrices.begin(), matrices.end(), m44(1.0f));
		time = 0.0f;
	}
};

template <typename T>
inline void
anim_pool_add_time_value(anim_pool_t<T>& pool, size_t* pool_size, f32 time, T val) {
    auto* time_value = pool[(*pool_size)++];
    *time_value = keyframe<T>{
        .time = time,
        .value = val
    };
}


}; // namespace anim





};

#endif