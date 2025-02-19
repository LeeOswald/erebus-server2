#include <erebus/system/exception.hxx>
#include <erebus/system/property.hxx>

#include <erebus/system/luaxx/luaxx_int64.hxx>
#include <erebus/system/luaxx/luaxx_property.hxx>
#include <erebus/system/luaxx/luaxx_state.hxx>

namespace Er::Lua
{

class ER_SYSTEM_EXPORT PropertyException
    : public LuaException
{
public:
    PropertyException(std::source_location location, std::string_view action, const Property& prop,std::string_view expected, std::string_view actual)
        : LuaException(location, Er::format("Failed to {} property {}: expected a \'{}\', got \'{}\'", action, prop.name(), expected, actual))
    {}
};

namespace
{

uint32_t getPropertyType(const Er::Property& prop)
{
    return static_cast<uint32_t>(prop.type());
}

bool getPropertyBool(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Bool) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop, "Bool", Er::propertyTypeToString(prop.type()));
    return prop.getBool() == True;
}

void setPropertyBool(Er::Property& prop, Bool val)
{
    if (prop.type() != PropertyType::Bool) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop, "Bool", Er::propertyTypeToString(prop.type()));
    prop = Property(val, *prop.info());
}

std::int32_t getPropertyInt32(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Int32) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop, "Int32", Er::propertyTypeToString(prop.type()));
    return prop.getInt32();
}

void setPropertyInt32(Er::Property& prop, int32_t val)
{
    if (prop.type() != PropertyType::Int32) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop, "Int32", Er::propertyTypeToString(prop.type()));
    prop = Property(val, *prop.info());
}

uint32_t getPropertyUInt32(const Er::Property& prop)
{
    if (prop.type() != PropertyType::UInt32) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop, "UInt32", Er::propertyTypeToString(prop.type()));
    return prop.getUInt32();
}

void setPropertyUInt32(Er::Property& prop, uint32_t val)
{
    if (prop.type() != PropertyType::UInt32) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop, "UInt32", Er::propertyTypeToString(prop.type()));
    prop = Property(val, *prop.info());
}

Er::Lua::Int64Wrapper getPropertyInt64(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Int64) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop, "Int64", Er::propertyTypeToString(prop.type()));
    
    return Er::Lua::Int64Wrapper(prop.getInt64());
}

void setPropertyInt64(Er::Property& prop, const Er::Lua::Int64Wrapper& val)
{
    if (prop.type() != PropertyType::Int64) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop, "Int64", Er::propertyTypeToString(prop.type()));
    prop = Property(val.value, *prop.info());
}

Er::Lua::UInt64Wrapper getPropertyUInt64(const Er::Property& prop)
{
    if (prop.type() != PropertyType::UInt64) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop, "IUInt64", Er::propertyTypeToString(prop.type()));

    return Er::Lua::UInt64Wrapper(prop.getUInt64());
}

void setPropertyUInt64(Er::Property& prop, const Er::Lua::UInt64Wrapper& val)
{
    if (prop.type() != PropertyType::UInt64) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop, "UInt64", Er::propertyTypeToString(prop.type()));
    prop = Property(val.value, *prop.info());
}

double getPropertyDouble(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Double) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop, "Double", Er::propertyTypeToString(prop.type()));
    return prop.getDouble();
}

void setPropertyDouble(Er::Property& prop, double val)
{
    if (prop.type() != PropertyType::Double) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop, "Double", Er::propertyTypeToString(prop.type()));
    prop = Property(val, *prop.info());
}

const std::string& getPropertyString(const Er::Property& prop)
{
    if (prop.type() != PropertyType::String) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop, "String", Er::propertyTypeToString(prop.type()));
    return prop.getString();
}

void setPropertyString(Er::Property& prop, const std::string& val)
{
    if (prop.type() != PropertyType::String) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop, "String", Er::propertyTypeToString(prop.type()));
    prop = Property(val, *prop.info());
}

const std::string& getPropertyBinary(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Binary) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop, "Binary", Er::propertyTypeToString(prop.type()));
    return prop.getBinary().bytes();
}

void setPropertyBinary(Er::Property& prop, const std::string& val)
{
    if (prop.type() != PropertyType::Binary) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop, "Binary", Er::propertyTypeToString(prop.type()));
    prop = Property(Binary(val), *prop.info());
}

const std::string& getPropertyName(const Er::Property& prop)
{
    static const std::string unknown("</?/?/?>");
    return prop.info() ? prop.info()->name() : unknown;
}

const std::string& getPropertyReadableName(const Er::Property& prop)
{
    static const std::string unknown("</?/?/?>");
    return prop.info() ? prop.info()->readableName() : unknown;
}

std::string formatProperty(const Er::Property& prop)
{
    return prop.info() ? prop.info()->format(prop) : prop.str();
}

} // namespace {}


ER_SYSTEM_EXPORT void registerPropertyTypes(State& state)
{
    {
        Er::Lua::Selector s = state["Er"]["PropertyType"];
        s["Empty"] = static_cast<uint32_t>(Er::PropertyType::Empty);
        s["Bool"] = static_cast<uint32_t>(Er::PropertyType::Bool);
        s["Int32"] = static_cast<uint32_t>(Er::PropertyType::Int32);
        s["UInt32"] = static_cast<uint32_t>(Er::PropertyType::UInt32);
        s["Int64"] = static_cast<uint32_t>(Er::PropertyType::Int64);
        s["UInt64"] = static_cast<uint32_t>(Er::PropertyType::UInt64);
        s["Double"] = static_cast<uint32_t>(Er::PropertyType::Double);
        s["String"] = static_cast<uint32_t>(Er::PropertyType::String);
        s["Binary"] = static_cast<uint32_t>(Er::PropertyType::Binary);
    }

    {
        Er::Lua::Selector s = state["Er"]["Property"];
        s["getType"] = &getPropertyType;
        s["getBool"] = &getPropertyBool;
        s["setBool"] = &setPropertyBool;
        s["getInt32"] = &getPropertyInt32;
        s["setInt32"] = &setPropertyInt32;
        s["getUInt32"] = &getPropertyUInt32;
        s["setUInt32"] = &setPropertyUInt32;
        s["getInt64"] = &getPropertyInt64;
        s["setInt64"] = &setPropertyInt64;
        s["getUInt64"] = &getPropertyUInt64;
        s["setUInt64"] = &setPropertyUInt64;
        s["getDouble"] = &getPropertyDouble;
        s["setDouble"] = &setPropertyDouble;
        s["getString"] = &getPropertyString;
        s["setString"] = &setPropertyString;
        s["getBinary"] = &getPropertyBinary;
        s["setBinary"] = &setPropertyBinary;
        s["getName"] = &getPropertyName;
        s["getReadableName"] = &getPropertyReadableName;
        s["format"] = &formatProperty;

    }
}

} // namespace Er::Lua {}