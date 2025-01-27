#pragma once

#include <erebus/system/luaxx/luaxx_exception_handler.hxx>
#include <erebus/system/luaxx/luaxx_lua_ref.hxx>
#include <erebus/system/luaxx/luaxx_primitives.hxx>
#include <erebus/system/luaxx/luaxx_references.hxx>
#include <erebus/system/luaxx/luaxx_resource_handler.hxx>
#include <erebus/system/luaxx/luaxx_util.hxx>

#include <tuple>


namespace Er::Lua 
{

class Selector;

namespace detail 
{

struct function_base 
{
    LuaRef _ref;
    lua_State* _state;
    ExceptionHandler* _exception_handler;

    function_base(int ref, lua_State* state)
        : _ref(state, ref)
        , _state(state)
        , _exception_handler(nullptr) 
    {}

    void _enable_exception_handler(ExceptionHandler* exception_handler) 
    {
        _exception_handler = exception_handler;
    }

    void protected_call(int const num_args, int const num_ret, int const handler_index) 
    {
        const auto status = lua_pcall(_state, num_args, num_ret, handler_index);

        if (status != LUA_OK && _exception_handler) 
        {
            _exception_handler->Handle_top_of_stack(status, _state);
        }
    }

    void Push(lua_State* state) const 
    {
        _ref.Push(state);
    }
};

} // namespace detail {}

/*
 * Similar to an std::function but refers to a lua function
 */
template <class>
class function 
{
};


template <typename R, typename... Args>
class function<R(Args...)> 
    : detail::function_base 
{
    friend class Selector;

public:
    using function_base::function_base;

    R operator()(Args... args) 
    {
        ResetStackOnScopeExit save(_state);

        int handler_index = SetErrorHandler(_state);
        _ref.Push(_state);
        detail::_push_n(_state, std::forward<Args>(args)...);
        constexpr int num_args = sizeof...(Args);

        protected_call(num_args, 1, handler_index);

        return detail::_get(detail::_id<R>{}, _state, -1);
    }

    using function_base::Push;
};

template <typename... Args>
class function<void(Args...)> 
    : detail::function_base
{
    friend class Selector;

public:
    using function_base::function_base;

    void operator()(Args... args) 
    {
        ResetStackOnScopeExit save(_state);

        int handler_index = SetErrorHandler(_state);
        _ref.Push(_state);
        detail::_push_n(_state, std::forward<Args>(args)...);
        constexpr int num_args = sizeof...(Args);

        protected_call(num_args, 1, handler_index);
    }

    using function_base::Push;
};

// Specialization for multireturn types
template <typename... R, typename... Args>
class function<std::tuple<R...>(Args...)> 
    : detail::function_base
{
    friend class Selector;

public:
    using function_base::function_base;

    std::tuple<R...> operator()(Args... args) 
    {
        ResetStackOnScopeExit save(_state);

        int handler_index = SetErrorHandler(_state);
        _ref.Push(_state);
        detail::_push_n(_state, std::forward<Args>(args)...);
        constexpr int num_args = sizeof...(Args);
        constexpr int num_ret = sizeof...(R);

        protected_call(num_args, num_ret, handler_index);

        lua_remove(_state, handler_index);
        return detail::_get_n<R...>(_state);
    }

    using function_base::Push;
};

namespace detail 
{

template <typename T>
struct is_primitive<Er::Lua::function<T>> 
{
    static constexpr bool value = true;
};

template <typename R, typename...Args>
inline Er::Lua::function<R(Args...)> _check_get(_id<Er::Lua::function<R(Args...)>>, lua_State* l, const int index) 
{
    lua_pushvalue(l, index);
    return Er::Lua::function<R(Args...)>{ luaL_ref(l, LUA_REGISTRYINDEX), l };
}

template <typename R, typename... Args>
inline Er::Lua::function<R(Args...)> _get(_id<Er::Lua::function<R(Args...)>> id, lua_State* l, const int index) 
{
    return _check_get(id, l, index);
}

template <typename R, typename... Args>
inline void _push(lua_State* l, Er::Lua::function<R(Args...)> fun) 
{
    fun.Push(l);
}

} // namespace detail {}

} // namespace Er::Lua {}
