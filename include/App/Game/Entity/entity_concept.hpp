#ifndef GAME_ENTITY_CONCEPT_HPP
#define GAME_ENTITY_CONCEPT_HPP

#include "uid.hpp"

namespace game {

template <typename EntityType>
concept CEntity = requires(EntityType entity) {
    requires std::derived_from<math::transform_t, decltype(entity.transform)>;
    requires std::is_same_v<uid::id_type, decltype(entity.id)>;
    // requires std::is_same_v<uid::id_type, decltype(entity.parent_id)>;

    requires std::is_same_v<math::aabb_t<v3f>, decltype(entity.aabb)>;
    // requires std::is_trivially_constructible_v<EntityType>;
};

#define GENERATE_BASE_ENTITY_BODY(Name) \
    Name* next{nullptr};\
    void* next_in_hash{nullptr};\
    entity_id id{uid::invalid_id};\
    math::transform_t transform;\
    math::aabb_t<v3f> aabb;

}


#endif