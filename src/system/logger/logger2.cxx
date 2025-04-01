#include <erebus/system/logger/null_logger2.hxx>



static Er::Log2::NullLogger s_null;
static Er::Log2::ILogger::Ptr s_global;

namespace Er::Log2
{

ER_SYSTEM_EXPORT ILogger* g_global = &s_null;
ER_SYSTEM_EXPORT bool g_verbose = false;


ER_SYSTEM_EXPORT ILogger::Ptr global() noexcept
{
    return s_global;
}

} // namespace Er::Log2 {}


namespace Erp::Log2
{

ER_SYSTEM_EXPORT void setGlobal(Er::Log2::ILogger::Ptr log) noexcept
{
    bool drainPending = (Er::Log2::g_global == &s_null) && !!log;

    s_global = log;
    Er::Log2::g_global = log ? log.get() : &s_null;

    if (drainPending)
    {
        auto r = s_null.pop();
        while (r)
        {
            Er::Log2::g_global->write(r);
            r = s_null.pop();
        }
    }
}

} // namespace Erp::Log2 {}