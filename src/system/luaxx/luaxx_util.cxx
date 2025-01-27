#include <erebus/system/erebus.hxx>
#include <erebus/system/luaxx/luaxx_util.hxx>

namespace Er::Lua
{

    ER_SYSTEM_EXPORT std::ostream& operator<<(std::ostream& os, lua_State* l)
{
    int top = lua_gettop(l);
    for (int i = 1; i <= top; ++i)
    {
        int t = lua_type(l, i);
        switch (t)
        {
        case LUA_TSTRING:
            os << lua_tostring(l, i);
            break;
        case LUA_TBOOLEAN:
            os << (lua_toboolean(l, i) ? "true" : "false");
            break;
        case LUA_TNUMBER:
            os << lua_tonumber(l, i);
            break;
        default:
            os << lua_typename(l, t);
            break;
        }
        os << " ";
    }

    return os;
}

    ER_SYSTEM_EXPORT bool check(lua_State* L, int code)
{
#if LUA_VERSION_NUM >= 502
    if (code == LUA_OK)
    {
#else
    if (code == 0)
    {
#endif
        return true;
    }
    else
    {
        std::cout << lua_tostring(L, -1) << std::endl;
        return false;
    }
}

    ER_SYSTEM_EXPORT int Traceback(lua_State * L)
{
    // Make nil and values not convertible to string human readable.
    const char* msg = "<not set>";
    if (!lua_isnil(L, -1))
    {
        msg = lua_tostring(L, -1);
        if (!msg)
            msg = "<error object>";
    }
    lua_pushstring(L, msg);

    // call debug.traceback
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, -3);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);

    return 1;
}

    ER_SYSTEM_EXPORT int ErrorHandler(lua_State * L)
{
    if (test_stored_exception(L) != nullptr)
    {
        return 1;
    }

    return Traceback(L);
}

    ER_SYSTEM_EXPORT int SetErrorHandler(lua_State * L)
{
    lua_pushcfunction(L, &ErrorHandler);
    return lua_gettop(L);
}

} // namespace Er::Lua {}