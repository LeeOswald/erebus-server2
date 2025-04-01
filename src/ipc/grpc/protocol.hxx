#pragma once

#include <erebus/erebus.pb.h>
#include <erebus/system/property_info.hxx>


namespace Erp::Protocol
{

void assignProperty(erebus::Property& out, const Er::Property& source);

Er::Property getProperty(const erebus::Property& source, Er::IPropertyMapping* mapping, std::uint32_t context);

} // namespace Erp::Protocol {}