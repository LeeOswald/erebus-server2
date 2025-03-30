#include <erebus/system/program.hxx>
#include <erebus/system/logger/null_logger2.hxx>



namespace Er::Log2
{
    

ILogger* nullLogger() noexcept
{
    static NullLogger dummy;
    return &dummy;
}

Er::Log2::ILogger* g_logger = nullLogger();


ER_SYSTEM_EXPORT Log2::ILogger* get() noexcept
{
    return g_logger;
}

ER_SYSTEM_EXPORT ILogger::Ptr strongRef() noexcept
{
    auto program = Er::Program::instance();
    if (!program)
        return {};

    return program->log();
}

} // namespace Er::Log2 {}


namespace Erp::Log2
{

ER_SYSTEM_EXPORT Er::Log2::ILogger* set(Er::Log2::ILogger* log) noexcept
{
    auto prev = Er::Log2::g_logger;
    Er::Log2::g_logger = log ? log : Er::Log2::nullLogger();
    return prev;
}

} // namespace Erp::Log2 {}