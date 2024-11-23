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

ER_SYSTEM_EXPORT void set(ILog* log) noexcept
{
    g_logger = log ? log : nullLogger();
}

} // namespace Log {}

} // namespace Er {}