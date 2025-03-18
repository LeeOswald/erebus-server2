#pragma once

#include <erebus/system/property_info.hxx>
#include <erebus/erebus.pb.h>

#include <vector>

namespace Erp::Protocol
{

void assignProperty(erebus::Property& out, const Er::Property& source);


using PropertyMapping = std::vector<const Er::PropertyInfo*>;

Er::Property getProperty(const erebus::Property& source, const PropertyMapping& mapping);

} // namespace Erp::Protocol {}