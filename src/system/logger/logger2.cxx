#include <erebus/system/logger/null_logger2.hxx>


namespace Er
{
    
namespace
{

Er::Log2::ILogger* nullLogger() noexcept
{
    static Er::Log2::NullLogger dummy;
    return &dummy;
}

thread_local Er::Log2::ILogger* g_logger = nullLogger();
    
} // namespace {}

namespace Log2
{

ER_SYSTEM_EXPORT Log2::ILogger* get() noexcept
{
    return g_logger;
}

ER_SYSTEM_EXPORT Log2::ILogger* set(Log2::ILogger* log) noexcept
{
    auto prev = g_logger;
    g_logger = log ? log : nullLogger();
    return prev;
}

} // namespace Log2 {}

} // namespace Er {}