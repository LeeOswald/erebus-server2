#pragma once

#include <erebus/system/luaxx/luaxx_exception_handler.hxx>

#include <iostream>
#include <utility>

extern "C" 
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace Er::Lua 
{

ER_SYSTEM_EXPORT std::ostream& operator<<(std::ostream& os, lua_State* l);

inline void _print() 
{
    std::cout << std::endl;
}

template <typename T, typename... Ts>
inline void _print(T arg, Ts... args) 
{
    std::cout << arg << ", ";
    _print(args...);
}

ER_SYSTEM_EXPORT bool check(lua_State* L, int code);

ER_SYSTEM_EXPORT int Traceback(lua_State* L);

ER_SYSTEM_EXPORT int ErrorHandler(lua_State* L);

ER_SYSTEM_EXPORT int SetErrorHandler(lua_State* L);


template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) 
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}


} // namespace Er::Lua {}
