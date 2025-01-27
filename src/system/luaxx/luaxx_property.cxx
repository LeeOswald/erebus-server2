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
    explicit PropertyException(std::source_location location, const char* action, Er::PropId id, const char* expected, const char* actual)
        : LuaException(location, Er::format("Failed to {} property {:08x}: expected a \'{}\', got \'{}\'", action, id, expected, actual))
    {}
};

namespace
{

uintptr_t getPropertyId(const Er::Property& prop)
{
    return reinterpret_cast<uintptr_t>(prop.info());
}

uint32_t getPropertyType(const Er::Property& prop)
{
    return static_cast<uint32_t>(prop.type());
}

bool getPropertyBool(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Bool) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop.id, "Bool", Er::propertyTypeToString(prop.type()));
    return get<Bool>(prop.value) == True;
}

void setPropertyBool(Er::Property& prop, bool val)
{
    if (prop.type() != PropertyType::Bool) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop.id, "Bool", Er::propertyTypeToString(prop.type()));
    prop.value = val ? True : False;
}

int32_t getPropertyInt32(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Int32) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop.id, "Int32", Er::propertyTypeToString(prop.type()));
    return get<int32_t>(prop.value);
}

void setPropertyInt32(Er::Property& prop, int32_t val)
{
    if (prop.type() != PropertyType::Int32) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop.id, "Int32", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

uint32_t getPropertyUInt32(const Er::Property& prop)
{
    if (prop.type() != PropertyType::UInt32) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop.id, "UInt32", Er::propertyTypeToString(prop.type()));
    return get<uint32_t>(prop.value);
}

void setPropertyUInt32(Er::Property& prop, uint32_t val)
{
    if (prop.type() != PropertyType::UInt32) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop.id, "UInt32", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

Er::Lua::Int64Wrapper getPropertyInt64(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Int64) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop.id, "Int64", Er::propertyTypeToString(prop.type()));
    
    return Er::Lua::Int64Wrapper(get<int64_t>(prop.value));
}

void setPropertyInt64(Er::Property& prop, const Er::Lua::Int64Wrapper& val)
{
    if (prop.type() != PropertyType::Int64) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop.id, "Int64", Er::propertyTypeToString(prop.type()));
    prop.value = val.value;
}

Er::Lua::UInt64Wrapper getPropertyUInt64(const Er::Property& prop)
{
    if (prop.type() != PropertyType::UInt64) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop.id, "UInt64", Er::propertyTypeToString(prop.type()));

    return Er::Lua::UInt64Wrapper(get<uint64_t>(prop.value));
}

void setPropertyUInt64(Er::Property& prop, const Er::Lua::UInt64Wrapper& val)
{
    if (prop.type() != PropertyType::UInt64) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop.id, "UInt64", Er::propertyTypeToString(prop.type()));
    prop.value = val.value;
}

double getPropertyDouble(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Double) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop.id, "Double", Er::propertyTypeToString(prop.type()));
    return get<double>(prop.value);
}

void setPropertyDouble(Er::Property& prop, double val)
{
    if (prop.type() != PropertyType::Double) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop.id, "Double", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

const std::string& getPropertyString(const Er::Property& prop)
{
    if (prop.type() != PropertyType::String) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop.id, "String", Er::propertyTypeToString(prop.type()));
    return get<std::string>(prop.value);
}

void setPropertyString(Er::Property& prop, const std::string& val)
{
    if (prop.type() != PropertyType::String) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop.id, "String", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

const std::string& getPropertyBytes(const Er::Property& prop)
{
    if (prop.type() != PropertyType::Binary) [[unlikely]]
        throw PropertyException(std::source_location::current(), "get", prop.id, "Binary", Er::propertyTypeToString(prop.type()));
    return get<Binary>(prop.value).bytes();
}

void setPropertyBytes(Er::Property& prop, const std::string& val)
{
    if (prop.type() != PropertyType::Binary) [[unlikely]]
        throw PropertyException(std::source_location::current(), "set", prop.id, "Binary", Er::propertyTypeToString(prop.type()));
    prop.value = Er::Binary(val);
}


class PropertyInfoWrapper
{
public:
    explicit PropertyInfoWrapper(const std::string& domain, uint32_t id)
        : m_pi(Er::lookupProperty(domain, id))
    {
    }

    bool valid() const
    {
        return !!m_pi;
    }

    uint32_t type() const
    {
        return m_pi ? static_cast<uint32_t>(m_pi->type()) : static_cast<uint32_t>(Er::PropertyType::Empty);
    }

    uint32_t id() const
    {
        return m_pi ? m_pi->id() : Er::InvalidPropId;
    }

    std::string id_str() const
    {
        return m_pi ? m_pi->id_str() : std::string();
    }

    std::string name() const
    {
        return m_pi ? m_pi->name() : std::string();
    }

    std::string format(const Er::Property& prop) const
    {
        if (!m_pi)
            return std::string();

        return m_pi->to_string(prop);
    }

private:
    std::shared_ptr<Er::IPropertyInfo> m_pi;
};


} // namespace {}


EREBUS_EXPORT void registerPropertyTypes(State& state)
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
        s["getId"] = &getPropertyId;
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
        s["getBytes"] = &getPropertyBytes;
        s["setBytes"] = &setPropertyBytes;
    }

    {
        Selector s = state["Er"]["PropertyInfo"];
        s.SetClass<PropertyInfoWrapper, std::string, uint32_t>(
            "valid", &PropertyInfoWrapper::valid,
            "type", &PropertyInfoWrapper::type,
            "id", &PropertyInfoWrapper::id,
            "id_str", &PropertyInfoWrapper::id_str,
            "name", &PropertyInfoWrapper::name,
            "format", &PropertyInfoWrapper::format
        );

    }
}

} // namespace Er::Lua {}