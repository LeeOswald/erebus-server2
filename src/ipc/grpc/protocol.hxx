#pragma once

#include <grpcpp/grpcpp.h>
#include <erebus/erebus.pb.h>

#include <erebus/system/result.hxx>


namespace Erp::Protocol
{

void assignProperty(erebus::Property& out, const Er::Property& source);

Er::Property getProperty(const erebus::Property& source, const Er::PropertyInfo* info);

inline void assignPropertyInfo(erebus::PropertyInfo& out, const Er::PropertyInfo* info)
{
    out.set_id(info->unique());
    out.set_type(static_cast<std::uint32_t>(info->type()));
    out.set_name(info->name());
    out.set_readablename(info->readableName());
}


Er::ResultCode mapGrpcStatus(grpc::StatusCode status) noexcept;

std::string_view propTypeToString(const erebus::Property& source) noexcept;


} // namespace Erp::Protocol {}