#pragma once

#include <erebus/system/logger2.hxx>


namespace Er::Log2
{

ER_SYSTEM_EXPORT ISink::Ptr makeDebuggerSink(IFormatter::Ptr&& formatter, Filter&& filter = Filter{});


} // namespace Er::Log2 {}