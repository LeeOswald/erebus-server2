#pragma once

#include <erebus/luaxx/luaxx_selector.hxx>

#include <tuple>


namespace Er::Lua 
{

template <typename... T>
class Tuple 
{
private:
    std::tuple<T&...> _tuple;

public:
    Tuple(T&... args) 
        : _tuple(args...) 
    {}

    void operator=(const Er::Lua::Selector& s) 
    {
        _tuple = s.GetTuple<typename std::remove_reference<T>::type...>();
    }
};

template <typename... T>
Tuple<T&...> tie(T&... args) 
{
    return Tuple<T&...>(args...);
}


} // namespace Er::Lua {}
