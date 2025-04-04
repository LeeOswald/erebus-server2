#include <erebus/system/exception.hxx>
#include <erebus/system/format.hxx>

#include "protocol.hxx"

namespace Erp::Protocol
{

namespace
{

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
        &assignPropertyBool,
        &assignPropertyInt32,
        &assignPropertyUInt32,
        &assignPropertyInt64,
        &assignPropertyUInt64,
        &assignPropertyDouble,
        &assignPropertyString,
        &assignPropertyBinary
    };

    static_assert(static_cast<std::size_t>(Er::PropertyType::Bool) == 1);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Int32) == 2);
    static_assert(static_cast<std::size_t>(Er::PropertyType::UInt32) == 3);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Int64) == 4);
    static_assert(static_cast<std::size_t>(Er::PropertyType::UInt64) == 5);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Double) == 6);
    static_assert(static_cast<std::size_t>(Er::PropertyType::String) == 7);
    static_assert(static_cast<std::size_t>(Er::PropertyType::Binary) == 8);

    out.set_id(source.unique());

    auto idx = static_cast<std::size_t>(source.type()) - 1;
    ErAssert(idx < _countof(s_assignPropertyFns));
    std::invoke(s_assignPropertyFns[idx], out, source);
}

Er::Property getProperty(const erebus::Property& source, const Er::PropertyInfo* info)
{
    using GetPropertyFn = Er::Property(*)(const erebus::Property&, const Er::PropertyInfo*);

    static GetPropertyFn s_getPropertyFns[] =
    {
        &getPropertyBool,
        &getPropertyInt32,
        &getPropertyUInt32,
        &getPropertyInt64,
        &getPropertyUInt64,
        &getPropertyDouble,
        &getPropertyString,
        &getPropertyBinary
    };

    static_assert(erebus::Property::kVBool == 2);
    static_assert(erebus::Property::kVInt32 == 3);
    static_assert(erebus::Property::kVUint32 == 4);
    static_assert(erebus::Property::kVInt64 == 5);
    static_assert(erebus::Property::kVUint64 == 6);
    static_assert(erebus::Property::kVDouble == 7);
    static_assert(erebus::Property::kVString == 8);
    static_assert(erebus::Property::kVBinary == 9);
    
    auto idx = static_cast<std::size_t>(source.value_case()) - 2;
    if (idx >= _countof(s_getPropertyFns))
        ErThrow(Er::format("Unsupported property type {}", idx));

    return std::invoke(s_getPropertyFns[idx], source, info);
}


Er::ResultCode mapGrpcStatus(grpc::StatusCode status) noexcept
{
    switch (status)
    {
    case grpc::OK: return Er::Result::Ok;
    case grpc::CANCELLED: return Er::Result::Canceled;
    case grpc::UNKNOWN: return Er::Result::Failure;
    case grpc::INVALID_ARGUMENT: return Er::Result::InvalidArgument;
    case grpc::DEADLINE_EXCEEDED: return Er::Result::Timeout;
    case grpc::NOT_FOUND: return Er::Result::NotFound;
    case grpc::ALREADY_EXISTS: return Er::Result::AlreadyExists;
    case grpc::PERMISSION_DENIED: return Er::Result::AccessDenied;
    case grpc::UNAUTHENTICATED: return Er::Result::Unauthenticated;
    case grpc::RESOURCE_EXHAUSTED: return Er::Result::ResourceExhausted;
    case grpc::FAILED_PRECONDITION: return Er::Result::FailedPrecondition;
    case grpc::ABORTED: return Er::Result::Aborted;
    case grpc::OUT_OF_RANGE: return Er::Result::OutOfRange;
    case grpc::UNIMPLEMENTED: return Er::Result::Unimplemented;
    case grpc::INTERNAL: return Er::Result::Internal;
    case grpc::UNAVAILABLE: return Er::Result::Unavailable;
    case grpc::DATA_LOSS: return Er::Result::DataLoss;
    default: return Er::Result::Failure;
    }
}

std::string_view propTypeToString(const erebus::Property& source) noexcept
{
    auto ty = source.value_case();
    switch (ty)
    {
    case erebus::Property::kVBool: return "Bool";
    case erebus::Property::kVInt32: return "Int32";
    case erebus::Property::kVUint32: return "UInt32";
    case erebus::Property::kVInt64: return "Int64";
    case erebus::Property::kVUint64: return "UInt64";
    case erebus::Property::kVDouble: return "Double";
    case erebus::Property::kVString: return "String";
    case erebus::Property::kVBinary: return "Binary";
    }
    return "<\?\?\?>";
}

} // namespace Erp::Protocol {}