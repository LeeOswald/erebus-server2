#include <erebus/system/luaxx/luaxx_base_fun.hxx>
#include <erebus/system/luaxx/luaxx_metatable_registry.hxx>


namespace Er::Lua 
{
    
namespace detail 
{

int ER_SYSTEM_EXPORT _lua_dispatcher(lua_State* l)
{
    BaseFun* fun = (BaseFun*)lua_touserdata(l, lua_upvalueindex(1));
    _lua_check_get raiseParameterConversionError = nullptr;
    const char* wrong_meta_table = nullptr;
    int erroneousParameterIndex = 0;
    try 
    {
        return fun->Apply(l);
    } 
    catch (GetParameterFromLuaTypeError& e) 
    {
        raiseParameterConversionError = e.checked_get;
        erroneousParameterIndex = e.index;
    } 
    catch (GetUserdataParameterFromLuaTypeError& e) 
    {
        wrong_meta_table = lua_pushlstring(l, e.metatable_name.c_str(), e.metatable_name.length());
        erroneousParameterIndex = e.index;
    } 
    catch (std::exception& e) 
    {
        lua_pushstring(l, e.what());
        Traceback(l);
        store_current_exception(l, lua_tostring(l, -1));
    } 
    catch (...) 
    {
        lua_pushliteral(l, "<Unknown exception>");
        Traceback(l);
        store_current_exception(l, lua_tostring(l, -1));
    }

    if (raiseParameterConversionError) 
    {
        raiseParameterConversionError(l, erroneousParameterIndex);
    }
    else if (wrong_meta_table) 
    {
        luaL_checkudata(l, erroneousParameterIndex, wrong_meta_table);
    }

    return lua_error(l);
}

} // namespace detail {}

} // namespace Er::Lua {}
