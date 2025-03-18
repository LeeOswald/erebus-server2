#include <erebus/system/exception.hxx>
#include <erebus/system/format.hxx>

#include "protocol.hxx"

namespace Erp::Protocol
{

namespace
{


void assignPropertyEmpty(erebus::Property& out, const Er::Property& in)
{
    out.clear_value();
}

void assignPropertyBool(erebus::Property& out, const Er::Property& in)
{
    out.set_v_bool(in.getBool() == Er::True);
}

void assignPropertyInt32(erebus::Property& out, const Er::Property& in)
{
    out.set_v_int32(in.getInt32());
}

void assignPropertyUInt32(erebus::Property& out, const Er::Property& in)
{
    out.set_v_uint32(in.getUInt32());
}

void assignPropertyInt64(erebus::Property& out, const Er::Property& in)
{
    out.set_v_int64(in.getInt64());
}

void assignPropertyUInt64(erebus::Property& out, const Er::Property& in)
{
    out.set_v_uint64(in.getUInt64());
}

void assignPropertyDouble(erebus::Property& out, const Er::Property& in)
{
    out.set_v_double(in.getDouble());
}

void assignPropertyString(erebus::Property& out, const Er::Property& in)
{
    out.set_v_string(in.getString());
}

void assignPropertyBinary(erebus::Property& out, const Er::Property& in)
{
    out.set_v_binary(in.getBinary().bytes());
}

Er::Property getPropertyEmpty(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property();
}

Er::Property getPropertyBool(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property(in.v_bool() ? Er::True : Er::False, *pi);
}

Er::Property getPropertyInt32(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property(in.v_int32(), *pi);
}

Er::Property getPropertyUInt32(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property(in.v_uint32(), *pi);
}

Er::Property getPropertyInt64(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property(in.v_int64(), *pi);
}

Er::Property getPropertyUInt64(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property(in.v_uint64(), *pi);
}

Er::Property getPropertyDouble(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property(in.v_double(), *pi);
}

Er::Property getPropertyString(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property(in.v_string(), *pi);
}

Er::Property getPropertyBinary(const erebus::Property& in, const Er::PropertyInfo* pi)
{
    return Er::Property(Er::Binary(in.v_binary()), *pi);
}


} // namespace {}


void assignProperty(erebus::Property& out, const Er::Property& source)
{
    using AssignPropertyFn = void(*)(erebus::Property& out, const Er::Property& in);
    
    static AssignPropertyFn s_assignPropertyFns[] =
    {
        &assignPropertyEmpty,
        &assignPropertyBool,
        &assignPropertyInt32,
        &assignPropertyUInt32,
        &assignPropertyInt64,
        &assignPropertyUInt64,
        &assignPropertyDouble,
        &assignPropertyString,
        &assignPropertyBinary
    };

    static_assert(static_cast<std::size_t>(Er::PropertyType::Empty) == 0);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Bool) == 1);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Int32) == 2);
    static_assert(static_cast<std::size_t>(Er::PropertyType::UInt32) == 3);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Int64) == 4);
    static_assert(static_cast<std::size_t>(Er::PropertyType::UInt64) == 5);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Double) == 6);
    static_assert(static_cast<std::size_t>(Er::PropertyType::String) == 7);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Binary) == 8);

    out.set_id(source.unique());

    auto idx = static_cast<std::size_t>(source.type());
    ErAssert(idx < _countof(s_assignPropertyFns));
    std::invoke(s_assignPropertyFns[idx], out, source);
}

Er::Property getProperty(const erebus::Property& source, const PropertyMapping& mapping)
{
    auto id = source.id();
    if (id >= mapping.size())
        ErThrow(Er::format("Unknown property {}", id));


    using GetPropertyFn = Er::Property(*)(const erebus::Property&, const Er::PropertyInfo*);

    static GetPropertyFn s_getPropertyFns[] =
    {
        &getPropertyEmpty,
        &getPropertyBool,
        &getPropertyInt32,
        &getPropertyUInt32,
        &getPropertyInt64,
        &getPropertyUInt64,
        &getPropertyDouble,
        &getPropertyString,
        &getPropertyBinary
    };

    static_assert(erebus::Property::kVVoid == 2);
    static_assert(erebus::Property::kVBool == 3);
    static_assert(erebus::Property::kVInt32 == 4);
    static_assert(erebus::Property::kVUint32 == 5);
    static_assert(erebus::Property::kVInt64 == 6);
    static_assert(erebus::Property::kVUint64 == 7);
    static_assert(erebus::Property::kVDouble == 8);
    static_assert(erebus::Property::kVString == 9);
    static_assert(erebus::Property::kVBinary == 10);
    
    auto idx = static_cast<std::size_t>(source.value_case()) - 2;
    if (idx >= _countof(s_getPropertyFns))
        ErThrow(Er::format("Unsupported property type {}", idx));

    return std::invoke(s_getPropertyFns[idx], source, mapping[id]);
}


} // namespace Erp::Protocol {}