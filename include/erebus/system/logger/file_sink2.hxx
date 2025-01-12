#pragma once

#include <erebus/system/logger2.hxx>

namespace Er::Log2
{

ER_SYSTEM_EXPORT ISink::Ptr makeFileSink(
    ThreadSafe mode, 
    std::string_view fileName,
    IFormatter::Ptr&& formatter,
    unsigned logsToKeep, 
    std::uint64_t maxFileSize,
    Filter&& filter = Filter{}
);

} // namespace Er::Log2 {}