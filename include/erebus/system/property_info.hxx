#pragma once

#include <erebus/system/binary.hxx>
#include <erebus/system/bool.hxx>

#include <functional>
#include <vector>


namespace Er
{

enum class PropertyType : uint32_t
{
    // order is important
    Empty,
    Bool,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Double,
    String,
    Binary,
    Map,
    Max // should go last
};

static_assert(unsigned(PropertyType::Max) <= 0x10, "Align PropertyInfo so that we can fit enough PropertyTypes into it's lower address bits");


struct Property;

//
// Yes, we DO need that large alignment value
// struct Property carries a pointer to PropertyInfo instance
// and 4 lower bits of this (aligned) pointer are used for PropertyType
//
struct alignas(16) PropertyInfo 
{
    using Formatter = std::function<std::string(const Property&)>;

    constexpr void* self()
    {
        return this;
    }

    constexpr PropertyType type() const noexcept
    {
        return m_type;
    }

    constexpr const std::string& name() const noexcept 
    {
        return m_name;
    }

    constexpr const std::string& readableName() const noexcept
    {
        return m_readableName;
    }
    
    ~PropertyInfo()
    {
        unregisterProperty(this);
    }
    
    PropertyInfo(PropertyType type, std::string_view name, std::string_view readableName, Formatter&& formatter = Formatter{})
        : m_type(type)
        , m_name(name)
        , m_readableName(readableName)
        , m_formatter(std::move(formatter))
    {
    }

    std::string format(const Property& prop) const;

    static const PropertyInfo* lookup(const std::string& name) noexcept;

private:
    static void registerProperty(const PropertyInfo* info);
    static void unregisterProperty(const PropertyInfo* info) noexcept;

    PropertyType m_type;
    std::string m_name;
    std::string m_readableName;
    Formatter m_formatter;
};


namespace Unspecified
{

extern ER_SYSTEM_EXPORT const PropertyInfo Empty;
extern ER_SYSTEM_EXPORT const PropertyInfo Bool;
extern ER_SYSTEM_EXPORT const PropertyInfo Int32;
extern ER_SYSTEM_EXPORT const PropertyInfo UInt32;
extern ER_SYSTEM_EXPORT const PropertyInfo Int64;
extern ER_SYSTEM_EXPORT const PropertyInfo UInt64;
extern ER_SYSTEM_EXPORT const PropertyInfo Double;
extern ER_SYSTEM_EXPORT const PropertyInfo String;
extern ER_SYSTEM_EXPORT const PropertyInfo Binary;
extern ER_SYSTEM_EXPORT const PropertyInfo Map;


} // namespace Unspecified {}

    
} // namespace Er {}
