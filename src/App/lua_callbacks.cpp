#pragma once

#include "ztd_core.hpp"

#include "lua.h"
#include "luacode.h"
#include "lualib.h"

#include "App/Game/World/world.hpp"
#include "App/Game/GUI/debug_state.hpp"

static ztd::world_t* get_world(lua_State* L) {
    lua_getglobal(L, "world");

    auto* world = (ztd::world_t*)lua_touserdata(L, -1);
    assert(world);
    lua_pop(L, 1);

    return world;
}

template <typename T>
static T* check_ud(lua_State* L, const char* name, int idx = 1) {
    auto* ud = (T*)luaL_checkudata(L, idx, name);
    luaL_argcheck(L, ud != NULL, idx, fmt::format("'{}' expected", name).c_str());

    return ud;
}

static std::optional<std::string_view> get_sv(lua_State* L, int idx) {
    if (lua_isstring(L, idx)) {
        size_t slen;
        const char* str = lua_tolstring(L, idx, &slen);

        return std::string_view{str, slen};
    } else {
        CLOG("Error getting string, wrong type");
    }
    return std::nullopt;
}

static std::optional<void*> get_userdata(lua_State* L, int idx) {
    if (lua_isuserdata(L, idx)) {
        return lua_touserdata(L, idx);
    } else {
        CLOG("Error getting string, wrong type");
    }
    return std::nullopt;
}


void set_type(lua_State* L, const char* type_name) {
	lua_pushstring(L, type_name);

	lua_pushvalue(L, -1);
	lua_setfield(L, -3, "__type");

	lua_pushcclosure(L, [](lua_State* L) {
		lua_pushstring(L, lua_tostring(L, lua_upvalueindex(1)));
		return 1;
    }, "__tostring", 1);
	// lua_setfield(L, -2, "__tostring");
}

static int l_console_print(lua_State* L) {
    int nargs = lua_gettop(L);

    for (int i=1; i <= nargs; i++) {
        if (lua_isstring(L, i)) {
            /* Pop the next arg using lua_tostring(L, i) and do your print */
            size_t slen;
            const char* str = lua_tolstring(L, i, &slen);
            
            console_log(DEBUG_STATE.console, 
                fmt::format(("{}"), std::string_view{str, slen}), gfx::color::rgba::purple, 0, 0, 1);
        } else {
            /* Do something with non-strings if you like */
            lua_getglobal(L, "tostring");
            lua_pushvalue(L, i);
            if (lua_pcall(L, 1, 1, 0) != 0) {
                console_log(DEBUG_STATE.console, 
                    fmt::format(("Error running function 'tostring' : {}"), lua_tostring(L, 1)), gfx::color::rgba::red, 0, 0, 0);

                return -1;
            }

            console_log(DEBUG_STATE.console, 
                fmt::format(("{}"), luaL_checkstring(L, -1)), gfx::color::rgba::purple, 0, 0, 1);

        }
    }

    return 0;
}

static int l_alert(lua_State* L) {
    int nargs = lua_gettop(L);

    for (int i=1; i <= nargs; i++) {
        if (lua_isstring(L, i)) {
            /* Pop the next arg using lua_tostring(L, i) and do your print */
            size_t slen;
            const char* str = lua_tolstring(L, i, &slen);

            DEBUG_ALERT(str);
        } else {
            /* Do something with non-strings if you like */
            lua_getglobal(L, "tostring");
            lua_pushvalue(L, i);
            if (lua_pcall(L, 1, 1, 0) != 0) {
                DEBUG_ALERT(lua_tostring(L, 1));
                
                return -1;
            }

            DEBUG_ALERT(luaL_checkstring(L, -1));
        }
    }

    return 0;
}

    
static int l_destroy_entity(lua_State* L) {
    int nargs = lua_gettop(L);

    auto* e = check_ud<ztd::entity_t>(L, "Entity");

    if (nargs >= 2) {
        e->queue_free(luaL_checkinteger(L, 2));
    } else {
        e->queue_free();
    }
    
    return 0;
}

static int l_create_entity(lua_State* L) {
    int nargs = lua_gettop(L);
    lua_getglobal(L, "world");

    auto* world = get_world(L);
    auto* e = ztd::world_create_entity(world);
    
    lua_pushlightuserdata(L, e);
    luaL_getmetatable(L, "Entity");
    lua_setmetatable(L, -2);

    return 1;
}

static int l_tostring_entity(lua_State* L) {
    int nargs = lua_gettop(L);

    // auto* world = get_world(L);

    if (nargs >= 1) {
        auto* e = check_ud<ztd::entity_t>(L, "Entity");

        lua_pushstring(L, e->name.c_data);
        return 1;
    }

    return 0;
}

static int l_name_entity(lua_State* L) {
    int nargs = lua_gettop(L);

    auto* world = get_world(L);

    if (nargs >= 2) {
        auto* e = check_ud<ztd::entity_t>(L, "Entity");
        auto name = get_sv(L, 2);
        if (name) {
            e->name.own(&world->arena, *name);
            lua_pop(L, 2);
        } else {
            luaL_argcheck(L, 0, 2, "No name");
        }
    }

    return 0;
}

static int l_transform_entity(lua_State* L) {
    int nargs = lua_gettop(L);

    if (nargs >= 2) {
        auto* e = check_ud<ztd::entity_t>(L, "Entity");
        auto* t = check_ud<math::transform_t>(L, "Transform",2);
        if (e&&t) {
            e->transform = *t;
        }
    }

    return 0;
}

static int l_load_prefab(lua_State* L) {
    int nargs = lua_gettop(L);

    lua_getglobal(L, "world");

    auto* world = get_world(L);

    if (nargs >= 1) {
        auto name = get_sv(L, 1);
        if (name) {
            lua_pop(L, 1);
            auto* prefab = (ztd::prefab_t*)lua_newuserdata(L, sizeof(ztd::prefab_t));
            // set_type(L, "prefab_t");
            *prefab = world->prefab_loader.load(&world->arena, *name);
            return 1;
        } else {
            console_log(DEBUG_STATE.console, 
                "no name", gfx::color::rgba::red, 0, 0, 0);
        }
    }
    return 0;
}

static int l_spawn_prefab(lua_State* L) {
    int nargs = lua_gettop(L);

    lua_getglobal(L, "world");

    auto* world = get_world(L);

    if (nargs >= 1) {
        auto prefab = get_userdata(L, 1);
        lua_pop(L, 1);
        if (prefab) {
            // lua_Debug ar;
            // lua_pushvalue(L, 1);
            // lua_getinfo(L, 1, ">nSl", &ar);
            // int line = ar.currentline;
            // auto* e = ztd::tag_spawn_(world, *(ztd::prefab_t*)*prefab, "lua_prefab",  ar.short_src, ar.what, line, v3f{0.0f});

            auto* e = ztd::tag_spawn(world, *(ztd::prefab_t*)*prefab);
        
            lua_pushlightuserdata(L, e);
            luaL_getmetatable(L, "Entity");
            lua_setmetatable(L, -2);

            return 1;
        } else {
            console_log(DEBUG_STATE.console, 
                "No prefab", gfx::color::rgba::red, 0, 0, 0);
        }
    }
    return 0;
}

static int l_play_sound(lua_State* L) {
    int nargs = lua_gettop(L);

    // lua_getglobal(L, "world");

    // auto* world = get_world(L);

    // if (nargs >= 1) {
    //     if (lua_isnumber(L, -1)) {
    //         auto x = lua_tonumber(L, -1);
    //         world->emit_event((sound_event::ENUM)x);
    //     } else {
    //         console_log(DEBUG_STATE.console, 
    //             "No Args", gfx::color::rgba::red, 0, 0, 0);
    //     }
    // }
    // return 0;
}

static int l_create_transform(lua_State* L) {
    int nargs = lua_gettop(L);

    auto* t = (math::transform_t*)lua_newuserdata(L, sizeof(math::transform_t));
    new (t) math::transform_t;
    luaL_getmetatable(L, "Transform");
    lua_setmetatable(L, -2);

    return 1;
}

static int l_transform_tostring(lua_State* L) {
    auto* ud = check_ud<math::transform_t>(L, "Transform");
    
    auto str = fmt::format("{}", ud->origin);

    lua_pushstring(L, str.c_str());

    return 1;
}

static int l_transform_set_origin(lua_State* L) {
    auto* l = check_ud<math::transform_t>(L, "Transform");
    
    auto x = (f32)luaL_checknumber(L, 2);
    l->origin.x = x;
    auto y = (f32)luaL_checknumber(L, 3);
    l->origin.y = y;
    auto z = (f32)luaL_checknumber(L, 4);
    l->origin.z = z;
    
    return 0;
}

static int l_transform_xform(lua_State* L) {
    auto* l = check_ud<math::transform_t>(L, "Transform");
    auto* r = check_ud<math::transform_t>(L, "Transform", -2);
    
    return 1;
}

static void load_engine_functions(ztd::luau::vm_t& L) {

    L.register_functions(ztd::luau::function_registrar_t<>{}
        .add({"print", l_console_print})
        .add({"alert", l_alert})
        .add({"load_prefab", l_load_prefab})
        .add({"spawn_prefab", l_spawn_prefab})
        .end().fns);
    L.register_better_type(
        ztd::luau::function_registrar_t<>{}
            .add({"new", l_create_entity})
            .end().fns,
        ztd::luau::function_registrar_t<>{}
            .add({"set_name", l_name_entity})
            .add({"queue_free", l_destroy_entity})
            .add({"__tostring", l_tostring_entity})
            .add({"set_transform", l_transform_entity})
            .end().fns
    , "Entity");

    L.register_better_type(
        ztd::luau::function_registrar_t<>{}
            .add({"new", l_create_transform})
            .end().fns,
        ztd::luau::function_registrar_t<>{}
            .add({"__tostring", l_transform_tostring})
            .add({"set_origin", l_transform_set_origin})
            .end().fns
    , "Transform");

    L.dofile("res/lua/waveworld.lua");
}
