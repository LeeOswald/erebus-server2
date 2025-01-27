#pragma once

#include <erebus/system/luaxx/luaxx_primitives.hxx>

#include <functional>


extern "C" 
{
#include <lua.h>
#include <lauxlib.h>
}

namespace Er::Lua 
{

struct stored_exception 
{
    std::string what;
    std::exception_ptr exception;
};


ER_SYSTEM_EXPORT void store_current_exception(lua_State* l, char const* what);

ER_SYSTEM_EXPORT stored_exception* test_stored_exception(lua_State* l);

ER_SYSTEM_EXPORT bool push_stored_exceptions_what(lua_State* l);

ER_SYSTEM_EXPORT std::exception_ptr extract_stored_exception(lua_State* l);


class ExceptionHandler 
{
public:
    using function = std::function<void(int,std::string,std::exception_ptr)>;

    ExceptionHandler() = default;

    explicit ExceptionHandler(function&& handler) 
        : _handler(handler) 
    {}

    void Handle(int luaStatusCode, std::string message, std::exception_ptr exception = nullptr) 
    {
        if (_handler) 
        {
            _handler(luaStatusCode, std::move(message), std::move(exception));
        }
    }

    void Handle_top_of_stack(int luaStatusCode, lua_State* L) 
    {
        stored_exception* stored = test_stored_exception(L);
        if (stored) 
        {
            Handle(luaStatusCode, stored->what, stored->exception);
        } 
        else 
        {
            Handle(luaStatusCode, detail::_get(detail::_id<std::string>(), L, -1));
        }
    }

private:
    function _handler;
};

} // namespace Er::Lua {}
