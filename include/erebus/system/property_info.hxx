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
    Bools,
    Int32s,
    UInt32s,
    Int64s,
    UInt64s,
    Doubles,
    Strings,
    Binaries,
    Max // should go last
};

static_assert(unsigned(PropertyType::Max) <= 0x20, "Align PropertyInfo so that we can fit enough PropertyTypes into it's lower address bits");


using BoolVector = std::vector<Bool>;
using Int32Vector = std::vector<std::int32_t>;
using UInt32Vector = std::vector<std::uint32_t>;
using Int64Vector = std::vector<std::int64_t>;
using UInt64Vector = std::vector<std::uint64_t>;
using DoubleVector = std::vector<double>;
using StringsVector = std::vector<std::string>;
using BinariesVector = std::vector<Binary>;


struct Property;

//
// Yes, we DO need that large alignment value
// struct Property carries a pointer to PropertyInfo instance
// and 5 lower bits of this (aligned) pointer are used for PropertyType
//
struct alignas(32) PropertyInfo 
{
    using Formatter = std::function<std::string(const Property&)>;

    std::uint32_t id;
    PropertyType type;
    std::string name;
    std::string readableName;
    
    ~PropertyInfo()
    {
        unregisterProperty(this);
    }
    
    PropertyInfo(PropertyType type, std::string_view name, std::string_view readableName, Formatter&& formatter = Formatter{})
        : id(registerProperty(this))
        , type(type)
        , name(name)
        , readableName(readableName)
        , m_formatter(std::move(formatter))
    {
    }

    std::string format(const Property& prop) const;

    static const PropertyInfo* lookup(const std::string& name) noexcept;

private:
    static std::uint32_t registerProperty(const PropertyInfo* info);
    static void unregisterProperty(const PropertyInfo* info) noexcept;

    Formatter m_formatter;
};


namespace Unspecified
{

extern ER_SYSTEM_EXPORT const PropertyInfo Empty;

} // namespace Unspecified {}

    
} // namespace Er {}
