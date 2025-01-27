#pragma once

#include <erebus/system/exception.hxx>

namespace Er::Lua 
{

class ER_SYSTEM_EXPORT LuaException 
    : public Er::Exception
{
public:
    LuaException() = default;

    explicit LuaException(std::source_location location, auto&& message)
        : Er::Exception(location, std::forward<decltype(message)>(message))
    {}
};

class ER_SYSTEM_EXPORT TypeError
    : public LuaException 
{
public:
    explicit TypeError(std::source_location location, std::string expected)
      : LuaException(location, std::move(expected) + " expected, got no object")
    {}

    explicit TypeError(std::source_location location, std::string expected, std::string const& actual)
      : LuaException(std::move(location), std::move(expected) + " expected, got " + actual)
    {}
};

class ER_SYSTEM_EXPORT CopyUnregisteredType
    : public LuaException 
{
public:
    using TypeID = std::reference_wrapper<const std::type_info>;
    
    explicit CopyUnregisteredType(std::source_location location, TypeID type)
        : LuaException(std::move(location), "Tried to copy an object of an unregistered type")
        , _type(type) 
    {}

    TypeID type() const
    {
        return _type;
    }

private:
    TypeID _type;
};

} // namespace Er::Lua {}
