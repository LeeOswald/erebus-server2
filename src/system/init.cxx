#include <erebus/system/logger/null_logger.hxx>

#include <atomic>


namespace Er
{
    
namespace
{
    
std::atomic<long> g_initialized = 0;

Er::Log::ILog* nullLogger() noexcept
{
    static Er::NullLogger dummy;
    return &dummy;
}

Er::Log::ILog* g_logger = nullLogger();
    
} // namespace {}

namespace Log
{

ER_SYSTEM_EXPORT Log::ILog* defaultLog() noexcept
{
    return g_logger;
}

} // namespace Log {}


ER_SYSTEM_EXPORT void initialize(Er::Log::ILog* log)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        g_logger = log;

        //Er::initializeTypeRegistry(log);
    }
}

ER_SYSTEM_EXPORT void finalize(Er::Log::ILog* log) noexcept
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        //Er::finalizeTypeRegistry();

        g_logger = nullLogger();
    }
}
    
} // namespace Er {}