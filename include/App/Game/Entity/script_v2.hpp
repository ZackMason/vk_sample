#pragma once

#include "core.hpp"

namespace game {

struct world_t;

DEFINE_TYPED_ID(script_id);

template <typename T>
concept CScript = requires(T script, world_t* w, f32 dt) {
    { T::script_name() } -> std::same_as<std::string_view>;
    { script.begin_play(w) } -> std::same_as<void>;
    { script.end_play(w) } -> std::same_as<void>;
    { script.update(w, dt) } -> std::same_as<void>;


    requires std::is_same_v<T*, decltype(script.next)>;
    requires std::is_same_v<script_id, decltype(script.id)>;
};

template <typename T>
concept CSerializedScript = requires(T script, utl::memory_blob_t& blob) {
    requires CScript<T>;

    { script.save(blob) } -> std::same_as<void>;
    { script.load(blob) } -> std::same_as<void>;
};

template <typename T, typename... Ts>
concept same_as_any = ( ... or std::same_as<T, Ts>);

template <CScript... TScript>
struct script_manager_t {
    inline static constexpr
        std::array<u64, sizeof...(TScript)> 
        type_ids{TScript::type_id()...};
    
    script_manager_t(world_t* world_) 
        : 
        world{world_}
    {
    }

    template <typename ScriptType>
    struct script_meta_t {
        ScriptType* scripts{nullptr};
        script_id   next_id{0x1};
    };

    std::tuple<script_meta_t<TScript>...> table;
    world_t* world{nullptr};
    
    // runtime: O(1), comptime: O(N)
    template <same_as_any<TScript...> T>
    static constexpr u64 get_type_index() {
        for(u64 i{0}; const auto id: type_ids) {
            if (id == T::type_id()) {
                return i;
            }
            i++;
        }
        // this is not reachable
        return 0xFFFF'FFFF'FFFF'FFFF;
    }
    
    // O(1)
    template <same_as_any<TScript...> T>
    script_id add_script(T* script) {
        assert(script->id == uid::invalid_id);
        node_push(script, std::get<script_meta_t<T>>(table).scripts);
        return script->id = std::get<script_meta_t<T>>(table).next_id++;
    }

    // O(N)
    template <same_as_any<TScript...> T>
    T* get_script(u64 index, script_id id) {
        constexpr u64 type_index = get_type_index<T>();
        node_for(auto, std::get<type_index>(table).scripts, script) {
            if (script->id == id) {
                return script;
            }
        }

        return nullptr;
    }

    // O(N)
    template <same_as_any<TScript...> T>
    void remove_script(T* s) {
        assert(s);
        remove_script<T>(s->id);
    }
    
    // O(N)
    template <same_as_any<TScript...> T>
    void remove_script(script_id id) {
        assert(id != uid::invalid_id);
        auto*& scripts = std::get<script_meta_t<T>>(table).scripts;
        if (scripts) {
            if (scripts->id == id) {
                scripts->id = uid::invalid_id;
                scripts = scripts->next;
                return;
            } 
            node_for(auto, scripts, script) {
                if (script->next && script->next->id == id) {
                    script->id = uid::invalid_id;
                    script->next = script->next->next;
                    return;
                }
            }
        }
    }

    void update_script(CScript auto* script, f32 dt) {
        if (script) {
            script->update(world, dt);
            update_script(script->next, dt);
        }
    }

    void begin_script(CScript auto* script) {
        if (script) {
            script->begin_play(world);
            begin_script(script->next);
        }
    }
    void end_script(CScript auto* script) {
        if (script) {
            script->end_play(world);
            end_script(script->next);
        }
    }

    size_t script_count(CScript auto* script) {
        return script?1+script_count(script->next):0;        
    }

    void update(f32 dt) {
        std::apply([&](auto&&... arg){ ((update_script(arg.scripts, dt)), ...); }, table);
    }

    void begin_play() {
        std::apply([&](auto&&... arg){ ((begin_script(arg.scripts)), ...); }, table);
    }
    void end_play() {
        std::apply([&](auto&&... arg){ ((end_script(arg.scripts)), ...); }, table);
    }
};

#define USCRIPT(Name) \
    Name* next{0};\
    script_id id{uid::invalid_id};\
    static constexpr std::string_view script_name() { return #Name; }\
    static constexpr u64 type_id() { return sid(script_name()); }

struct player_script_t {
    USCRIPT(player_script_t)

    int score{0};

    void save(utl::memory_blob_t& blob) {
        blob.serialize(0, score);
    }

    void load(utl::memory_blob_t& blob) {
        score = blob.deserialize<int>();
    }

    void end_play(world_t* world) {}
    void begin_play(world_t* world) {
        gen_info("PlayerScript", "Hello World: {} - {}", script_name(), id);
    }

    void update(world_t* world, f32 dt) {
        score++;
    }
};

};