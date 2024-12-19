#include <erebus/system/logger/null_logger.hxx>


namespace Er
{
    
namespace
{

Er::Log::ILog* nullLogger() noexcept
{
    static Er::NullLogger dummy;
    return &dummy;
}

thread_local Er::Log::ILog* g_logger = nullLogger();
    
} // namespace {}

namespace Log
{

ER_SYSTEM_EXPORT Log::ILog* get() noexcept
{
    return g_logger;
}

ER_SYSTEM_EXPORT ILog* set(ILog* log) noexcept
{
    auto prev = g_logger;
    g_logger = log ? log : nullLogger();
    return prev;
}

} // namespace Log {}

} // namespace Er {}