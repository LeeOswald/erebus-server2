#include <erebus/system/logger/null_logger2.hxx>



namespace Er::Log2
{

ILogger* nullLogger() noexcept
{
    static NullLogger dummy;
    return &dummy;
}

ER_SYSTEM_EXPORT ILogger* g_global = nullLogger();
ER_SYSTEM_EXPORT bool g_verbose = false;

static ILogger::Ptr s_global;


ER_SYSTEM_EXPORT ILogger::Ptr global() noexcept
{
    return s_global;
}

} // namespace Er::Log2 {}


namespace Erp::Log2
{

ER_SYSTEM_EXPORT void setGlobal(Er::Log2::ILogger::Ptr log) noexcept
{
    Er::Log2::s_global = log;
    Er::Log2::g_global = log ? log.get() : Er::Log2::nullLogger();
}

} // namespace Erp::Log2 {}