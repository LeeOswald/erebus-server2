#pragma once

#include <erebus/system/luaxx/luaxx_state.hxx>

namespace Er
{

class ER_SYSTEM_EXPORT LuaState final
    : public Er::Lua::State
{
public:
    ~LuaState();
    explicit LuaState(Er::Log2::ILogger* log);

private:
    static int _print(lua_State* L);
    int print();
};


} // namespace Er {}
