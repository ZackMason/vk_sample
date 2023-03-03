#ifndef MAP_GEN_HPP
#define MAP_GEN_HPP

#include "core.hpp"


namespace prcgen::map {

    struct sdf_t {
        sdf_t* next{0};
        virtual f32 distance(v3f pos) const noexcept = 0;
    };

    struct sphere_sdf_t {
        v3f origin;
        f32 radius;

        f32 distance(v3f pos) const noexcept {
            return glm::distance(pos, origin) - radius;
        }
    };

    struct box_sdf_t {
        v3f origin;
        v3f size;

        f32 distance(v3f pos) const noexcept  {
            const v3f q = glm::abs(pos - origin) - size;
            return glm::length(glm::max(q,v3f{0.0f})) + glm::min(glm::max(q.x,glm::max(q.y,q.z)),0.0f);
        }
    };


    struct room_volume_t {
        u8 volume[64][64][64];
    };
    // g.v[0] = f32(vol.volume[x][y][z]) / f32(0xff);
    // g.v[1] = f32(vol.volume[x][y][z+1]) / f32(0xff);
    // g.v[2] = f32(vol.volume[x+1][y][z+1]) / f32(0xff);
    // g.v[3] = f32(vol.volume[x+1][y][z]) / f32(0xff);
    // g.v[4] = f32(vol.volume[x][y+1][z]) / f32(0xff);
    // g.v[5] = f32(vol.volume[x][y+1][z+1]) / f32(0xff);
    // g.v[6] = f32(vol.volume[x+1][y+1][z+1]) / f32(0xff);
    // g.v[7] = f32(vol.volume[x+1][y+1][z]) / f32(0xff);

    inline void
    room_volume_init(room_volume_t* room) {
        std::memset(room->volume, 0xff, sizeof(u8)*64*64*64);

        const size_t wall = 12;

        range_u64(x, wall, 64 - wall) {
            range_u64(y, wall, 64 - wall) {
                range_u64(z, wall, 64 - wall) {
                    room->volume[x][y][z] = 0;
                    room->volume[x][y][z] = glm::distance(v3f{32.0f}, v3f{x,y,z}) < 13.0f ? 0 : 0xff;
                }
            }
        }
    }

    struct map_t {
        f32 room_size = 64.0f;
        f32 corridor_size = 16.0f;

        i32 rooms[16][16];

        map_t() {
            std::memset(rooms, -1, sizeof(i32)*16*16);
        }
    };

    inline void
    init(map_t* map) {

    }
    
    inline void
    add_room(map_t* map, u32 x, u32 y) {
        map->rooms[x][y] = 1;
    }



};

#endif