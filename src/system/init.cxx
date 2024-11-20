#include <erebus/private/null_logger.hxx>

#include <atomic>


namespace Er
{
    
namespace
{
    
std::atomic<long> g_initialized = 0;

ILog* nullLogger() noexcept
{
    static Erp::NullLogger dummy;
    return &dummy;
}

ILog* g_logger = nullLogger();
    
} // namespace {}

namespace Log
{

EREBUS_EXPORT Log::ILog* defaultLog() noexcept
{
    return g_logger;
}

} // namespace Log {}


ER_SYSTEM_EXPORT void initialize(Er::Log::ILog* log)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        g_logger = log;

        //Er::Private::initializeKnownProps();

        //Er::ExceptionProps::Private::registerAll(log);

        //Er::initializeTypeRegistry(log);
    }
}

ER_SYSTEM_EXPORT void finalize(Er::Log::ILog* log) noexcept
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        //Er::finalizeTypeRegistry();

        //Er::ExceptionProps::Private::unregisterAll(log);
        
        //Er::Private::finalizeKnownProps();

        g_logger = nullLogger();
    }
}
    
} // namespace Er {}