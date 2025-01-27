#pragma once

#include <iostream>
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include <erebus/system/logger2.hxx>
#include <erebus/system/type_id.hxx>

extern "C" 
{
#include <lua.h>
#include <lauxlib.h>
}

namespace Er
{

template <typename T>
TypeIndex userType()
{
    auto id = typeId<T>().index();
    return id;
}

} // namespace Er {}

namespace Er::Lua 
{

namespace detail 
{

struct GetUserdataParameterFromLuaTypeError 
{
    std::string metatable_name;
    int index;
};

} // namespace detail {}

namespace MetatableRegistry 
{

using TypeID = TypeIndex;


ER_SYSTEM_EXPORT void Create(lua_State* state);

ER_SYSTEM_EXPORT void PushNewMetatable(lua_State* state, TypeID type, const std::string& name);

ER_SYSTEM_EXPORT bool SetMetatable(lua_State* state, TypeID type);

ER_SYSTEM_EXPORT bool IsRegisteredType(lua_State* state, TypeID type);

ER_SYSTEM_EXPORT std::string GetTypeName(lua_State* state, TypeID type);

ER_SYSTEM_EXPORT std::string GetTypeName(lua_State* state, int index);

ER_SYSTEM_EXPORT bool IsType(lua_State* state, TypeID type, const int index);

ER_SYSTEM_EXPORT void CheckType(lua_State* state, TypeID type, const int index);

} // namespace MetatableRegistry {}

} // namespace Er::Lua {}
