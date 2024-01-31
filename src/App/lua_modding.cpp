#pragma once

#include "ztd_core.hpp"

#include "lua.h"
#include "luacode.h"
#include "lualib.h"

namespace ztd::luau {

    struct bytecode_t {
        char*       bytecode{0};
        size_t      length{0};
    };

    struct user_data_t {
        // change to pointer?
        utl::allocator_t allocator{};
        const void* user_data = 0;
    };
    
    void* lua_alloc(void* ud, void* ptr, size_t os, size_t nsize) {
        auto* user_data = (user_data_t*)ud;
        auto* allocator = (utl::allocator_t*)&user_data->allocator;
        if (nsize == 0) {
            // free(ptr);
            if (ptr)
                allocator->free(ptr);
            return 0;
        } else {
            // auto* p = realloc(ptr, nsize);
            auto* p = (void*)allocator->allocate(nsize);
            assert(p);

            utl::copy(p, ptr, os);
            if (ptr) allocator->free(ptr);
            
            // if (log_memory) {
                // std::print("[{}]: {} - {}{}\n", 
                //     __FUNCTION__, 
                //     reflect::ptr(p),
                //     math::pretty_bytes(nsize), 
                //     math::pretty_bytes_postfix(nsize)
                // );
            // }
            return p;
        }
    }

    template<size_t N = 0>
    struct function_registrar_t {
        using next_ref = function_registrar_t<N+1>;

        constexpr next_ref copy() const {
            next_ref t{};
            t.count = count;
            range_u64(i, 0, N+1) {
                t.fns[i] = fns[i];
            }
            return t;
        }
        constexpr next_ref add(luaL_Reg f) const {
            next_ref t = copy();
            t.fns[t.count++] = f;
            return t;
        }
        constexpr next_ref end() const {
            return add({0,0});
        }

        u32 count = 0;
        std::array<luaL_Reg, N+1> fns = {};
    };

    struct vm_t {
        lua_State* L = 0;
        user_data_t user_data = {};

        void init(void* world) {
            user_data.user_data = world;
            L = lua_newstate(lua_alloc, &user_data);
            assert(L);

            luaL_openlibs(L);

            lua_pushlightuserdata(L, world);
            lua_setglobal(L, "world");
        }

        // remember to null terminate?
        void register_functions(const auto&& funcs) {
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            luaL_register(L, 0, funcs.data());
            lua_pop(L, 1);
        }

        void register_type(const auto&& funcs, const char* name) {
            luaL_newmetatable(L, name);
            luaL_register(L, name, funcs.data());
            lua_pop(L, 1);
        }

        void push_float(f32 x) {
            lua_pushnumber(L, x);
        }

        void push_fn(const char* fn) {
            lua_getglobal(L, fn);
        }
        void push_tablefn(const char* table, const char* field) {
            lua_getglobal(L, table);
            lua_getfield(L, -1, field);
            lua_pushvalue(L, -2);
        }

        void callf(int args) {
            lua_pcall(L, args, 0, 0);
            // lua_pop(L, args + 1);
        }

        void register_better_type(const auto&& create, const auto&& funcs, const char* name) {
            luaL_newmetatable(L, name);

            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);  /* pushes the metatable */
            lua_settable(L, -3);  /* metatable.__index = metatable */
            
            luaL_register(L, NULL, funcs.data());
            
            luaL_register(L, name, create.data());
            
            lua_pop(L, 1);
        }
        void register_better_type_dtor(const auto&& create, const auto&& funcs, auto dtor, const char* name) {
            luaL_newmetatable(L, name);

            // lua_pop(L, 2);
            lua_pushstring(L, "__gc");
            lua_pushcfunction(L, dtor, "dtor");
            lua_settable(L, -3);

            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);  /* pushes the metatable */
            lua_settable(L, -3);  /* metatable.__index = metatable */
            
            
            luaL_register(L, NULL, funcs.data());
            
            luaL_register(L, name, create.data());
            
            lua_pop(L, 1);
        }

        void
        dofile(std::string_view filename) {
            arena_t arena = {};
            defer {
                arena_clear(&arena);
            };
            // std::print("Opening file: {}\n", filename);
            auto file = utl::read_text_file(&arena, filename);
            dostr(file.sv());
        }

        std::optional<std::string> makelibrary(bytecode_t bytecode, std::string_view name) {
            auto bc = dobytecode(bytecode, 0);
            if (bc) { // there was an error
                return bc;
            } else {
                lua_setglobal(L, name.data());
                return bc;
            }
        }

        std::optional<std::string> dobytecode(bytecode_t bytecode, b32 call = 1) {
            int r;
            {
                TIMED_BLOCK(Luau_Load);
                r = luau_load(L, "ztd", bytecode.bytecode, bytecode.length, 0);
            }
            if (r != LUA_OK) {
                size_t len;
                const char* msg = lua_tolstring(L, -1, &len);

                std::string error(msg, len);
                lua_pop(L, 1);
                ztd_error(__FUNCTION__, "Error: {}\n", error);
                free(bytecode.bytecode);
                return error;
            } else {
                assert(r == 0);
                if (call) {
                    r = lua_pcall(L, 0, 0, 0);
                    if (r) {
                        size_t len;
                        const char* msg = lua_tolstring(L, -1, &len);

                        std::string error(msg, len);
                        lua_pop(L, 1);
                        ztd_error(__FUNCTION__, "Error: {}\n", error);
                        return error;
                    }
                }
            }
            return std::nullopt;
        }

        bytecode_t loadfile(std::string_view filename) {
            arena_t arena = {};
            defer {
                arena_clear(&arena);
            };
            // std::print("Opening file: {}\n", filename);
            auto file = utl::read_text_file(&arena, filename);
            auto code = file.sv();
            
            bytecode_t bytecode;
            
            bytecode.bytecode = luau_compile(code.data(), code.size(), 0, &bytecode.length);
            assert(bytecode.bytecode);

            return (bytecode);
        }

        std::optional<std::string> dostr(std::string_view code) {
            bytecode_t bytecode;
            
            bytecode.bytecode = luau_compile(code.data(), code.size(), 0, &bytecode.length);
            assert(bytecode.bytecode);

            return dobytecode(bytecode);
        }
    };

} // namespace ztd::lua
