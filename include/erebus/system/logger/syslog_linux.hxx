#pragma once

#include <erebus/system/log.hxx>

namespace Er::Log
{

ER_SYSTEM_EXPORT ISink::Ptr makeSyslogSink(const char* tag, IFormatter::Ptr formatter, IFilter::Ptr filter);


} // namespace Er::Log {}