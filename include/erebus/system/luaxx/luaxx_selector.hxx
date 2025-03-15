#pragma once

#include <erebus/system/luaxx/luaxx_exception_handler.hxx>
#include <erebus/system/luaxx/luaxx_function.hxx>
#include <erebus/system/luaxx/luaxx_lua_ref.hxx>
#include <erebus/system/luaxx/luaxx_references.hxx>
#include <erebus/system/luaxx/luaxx_registry.hxx>
#include <erebus/system/luaxx/luaxx_resource_handler.hxx>
#include <erebus/system/luaxx/luaxx_util.hxx>

#include <functional>
#include <string>
#include <tuple>
#include <vector>

namespace Er::Lua 
{

class State;

class ER_SYSTEM_EXPORT Selector 
{
    friend class State;

private:
    lua_State* _state;
    Registry* _registry;
    ExceptionHandler* _exception_handler;
    std::string _name;

    // Traverses the structure up to this element
    std::vector<LuaRef> _traversal;

    // Key of the value to act upon.
    LuaRef _key;

    std::vector<LuaRef> _functor_arguments;

    // Functor is activated when the () operator is invoked.
    mutable  MovingFlag _functor_active;

    Selector(lua_State* s, Registry& r, ExceptionHandler& eh, const std::string& name, std::vector<LuaRef> traversal, LuaRef key)
        : _state(s)
        , _registry(&r)
        , _exception_handler(&eh)
        , _name(name)
        , _traversal(traversal)
        , _key(key) 
    {}

    Selector(lua_State* s, Registry& r, ExceptionHandler& eh, const std::string& name)
        : _state(s)
        , _registry(&r)
        , _exception_handler(&eh)
        , _name(name)
        , _key(make_Ref(s, name)) 
    {}

    void _get(LuaRef r) const 
    {
        r.Push(_state);
        lua_gettable(_state, -2);
        lua_remove(_state, lua_absindex(_state, -2));
    }

    // Pushes this element to the stack
    void _get() const 
    {
        _get(_key);
    }

    // Sets this element from a function that pushes a value to the stack
    template<typename PushFunction>
    void _put(PushFunction fun) const 
    {
        _key.Push(_state);
        fun();
        lua_settable(_state, -3);
        lua_pop(_state, 1);
    }

    void _check_create_table() const;

    void _traverse() const;

    template <typename Fun>
    void _evaluate_store(Fun&& push) const 
    {
        ResetStackOnScopeExit save(_state);
        _traverse();
        _put(std::forward<Fun>(push));
    }

    void _evaluate_retrieve(int num_results) const;

    void _evaluate_function_call(int num_ret) const;

public:
    Selector(const Selector&) = default;
    Selector(Selector&&) = default;
    Selector&  operator=(const Selector&) = default;
    Selector&  operator=(Selector&&) = default;

    ~Selector() noexcept(false);

    // Allow automatic casting when used in comparisons
    bool operator==(Selector& other) = delete;

    template <typename... Args>
    const Selector operator()(Args&&... args) const 
    {
        Selector copy{*this};
        const auto state = _state; // gcc-5.1 doesn't support implicit member capturing
        const auto eh = _exception_handler;
        copy._functor_arguments = make_Refs(_state, std::forward<Args>(args)...);
        copy._functor_active = true;
        return copy;
    }

    template <typename L>
    void operator=(L lambda) const 
    {
        _evaluate_store([this, lambda]() 
        {
            _registry->Register(lambda);
        });
    }

    void operator=(bool b) const 
    {
        _evaluate_store([this, b]() 
        {
            detail::_push(_state, b);
        });
    }

    void operator=(int i) const 
    {
        _evaluate_store([this, i]() 
        {
            detail::_push(_state, i);
        });
    }

    void operator=(unsigned int i) const 
    {
        _evaluate_store([this, i]() 
        {
            detail::_push(_state, i);
        });
    }

    void operator=(lua_Number n) const 
    {
        _evaluate_store([this, n]() 
        {
            detail::_push(_state, n);
        });
    }

    void operator=(const std::string& s) const 
    {
        _evaluate_store([this, s]() 
        {
            detail::_push(_state, s);
        });
    }

    template <typename Ret, typename... Args>
    void operator=(std::function<Ret(Args...)> fun) 
    {
        _evaluate_store([this, fun]() 
        {
            _registry->Register(fun);
        });
    }

    template<typename T>
    void operator=(Reference<T> const&  ref) 
    {
        _evaluate_store([this,& ref]() 
        {
            detail::_push(_state, ref);
        });
    }

    template<typename T>
    void operator=(Pointer<T> const&  ptr) 
    {
        _evaluate_store([this,& ptr]() 
        {
            detail::_push(_state, ptr);
        });
    }

    template <typename Ret, typename... Args>
    void operator=(Ret (*fun)(Args...)) 
    {
        _evaluate_store([this, fun]() 
        {
            _registry->Register(fun);
        });
    }

    void operator=(const char *s) const 
    {
        _evaluate_store([this, s]() 
        {
            detail::_push(_state, s);
        });
    }

    template <typename T, typename... Funs>
    void SetObj(T& t, Funs... funs) 
    {
        auto fun_tuple = std::make_tuple(std::forward<Funs>(funs)...);
        _evaluate_store([this,& t,& fun_tuple]() 
        {
            _registry->Register(t, fun_tuple);
        });
    }

    template <typename T, typename... Args, typename... Funs>
    void SetClass(Funs... funs) 
    {
        auto fun_tuple = std::make_tuple(std::forward<Funs>(funs)...);
        _evaluate_store([this,& fun_tuple]() 
        {
            typename detail::_indices_builder<sizeof...(Funs)>::type d;
            _registry->RegisterClass<T, Args...>(_name, fun_tuple, d);
        });
    }

    template <typename... Ret>
    std::tuple<Ret...> GetTuple() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(sizeof...(Ret));
        return detail::_get_n<Ret...>(_state);
    }

    template<
        typename T,
        typename = typename std::enable_if<
            !detail::is_primitive<typename std::decay<T>::type>::value
        >::type
    >
    operator T&() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<T&>{}, _state);
    }

    template <typename T>
    operator T*() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<T*>{}, _state);
    }

    template <typename T>
    operator Reference<T>() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<Reference<T>>{}, _state);
    }

    template <typename T>
    operator Pointer<T>() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<Pointer<T>>{}, _state);
    }

    operator bool() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<bool>{}, _state);
    }

    operator int() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<int>{}, _state);
    }

    operator unsigned int() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<unsigned int>{}, _state);
    }

    operator lua_Number() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<lua_Number>{}, _state);
    }

    operator std::string() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<std::string>{}, _state);
    }

    template <typename R, typename... Args>
    operator Er::Lua::function<R(Args...)>() 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        auto ret = detail::_pop(detail::_id<Er::Lua::function<R(Args...)>>{}, _state);
        ret._enable_exception_handler(_exception_handler);
        return ret;
    }

    // Chaining operators. If the selector is an rvalue, modify in
    // place. Otherwise, create a new Selector and return it.

    Selector&& operator[](const std::string& name) && 
    {
        _name += std::string(".") + name;
        _check_create_table();
        _traversal.push_back(_key);
        _key = make_Ref(_state, name);
        return std::move(*this);
    }

    Selector&& operator[](const char* name) && 
    {
        return std::move(*this)[std::string{name}];
    }

    Selector&& operator[](const int index) && 
    {
        _name += std::string(".") + std::to_string(index);
        _check_create_table();
        _traversal.push_back(_key);
        _key = make_Ref(_state, index);
        return std::move(*this);
    }

    Selector operator[](const std::string& name) const & 
    {
        auto n = _name + "." + name;
        _check_create_table();
        auto traversal = _traversal;
        traversal.push_back(_key);
        return Selector{_state, *_registry, *_exception_handler, n, traversal, make_Ref(_state, name)};
    }

    Selector operator[](const char* name) const & 
    {
        return (*this)[std::string{name}];
    }

    Selector operator[](const int index) const & 
    {
        auto name = _name + "." + std::to_string(index);
        _check_create_table();
        auto traversal = _traversal;
        traversal.push_back(_key);
        return Selector{_state, *_registry, *_exception_handler, name, traversal, make_Ref(_state, index)};
    }

    friend bool operator==(const Selector& , const char*);

    friend bool operator==(const char *, const Selector&);

    bool exists() 
    {
        ResetStackOnScopeExit save(_state);
        _traverse();
        _get();

        return !lua_isnil(_state, -1);
    }

private:
    std::string ToString() const 
    {
        ResetStackOnScopeExit save(_state);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<std::string>{}, _state);
    }
};


inline bool operator==(const Selector& s, const char* c) 
{
    return std::string{c} == s.ToString();
}

inline bool operator==(const char* c, const Selector& s) 
{
    return std::string{c} == s.ToString();
}

template <typename T>
inline bool operator==(const Selector& s, T&& t) 
{
    return T(s) == t;
}

template <typename T>
inline bool operator==(T&& t, const Selector& s) 
{
    return T(s) == t;
}

} // namespace Er::Lua {}
