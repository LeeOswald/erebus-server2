#pragma once

#include <erebus/system/binary.hxx>
#include <erebus/system/bool.hxx>

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
using StringVector = std::vector<std::string>;
using BinaryVector = std::vector<Binary>;


//
// Yes, we DO need that large alignment value
// struct Property carries a pointer to PropertyInfo instance
// and 5 lower bits of this (aligned) pointer are used for PropertyType
//
struct ER_SYSTEM_EXPORT alignas(32) PropertyInfo final
{
    PropertyType type = PropertyType::Empty;
    std::string name;
    std::string readableName;
    
    constexpr PropertyInfo() noexcept = default;

    constexpr PropertyInfo(PropertyType type, std::string_view name, std::string_view readableName)
        : type(type)
        , name(name)
        , readableName(readableName)
    {}
};


namespace Unspecified
{

extern ER_SYSTEM_EXPORT const PropertyInfo Empty;

} // namespace Unspecified {}

    
} // namespace Er {}
