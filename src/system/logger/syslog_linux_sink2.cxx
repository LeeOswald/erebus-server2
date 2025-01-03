#include <erebus/system/logger/syslog_linux_sink2.hxx>

#include <syslog.h>



namespace Er::Log2
{

namespace
{

class SyslogSink
    : public SinkBase
{
public:
    ~SyslogSink()
    {
        ::closelog();
    }

    SyslogSink(const char* tag, IFormatter::Ptr formatter, Filter&& filter)
        : SinkBase(formatter, std::move(filter))
    {
        ::openlog(tag, LOG_CONS, LOG_DAEMON);
    }

    void write(Record::Ptr r) override
    {
        if (!filter(r.get()))
            return;

        auto formatted = format(r.get());
        const auto available = formatted.length();
        if (!available)
            return;

        int priority;
        switch (r->level())
        {
        case Er::Log2::Level::Debug: priority = LOG_DEBUG; break;
        case Er::Log2::Level::Info: priority = LOG_INFO; break;
        case Er::Log2::Level::Warning: priority = LOG_WARNING; break;
        case Er::Log2::Level::Error: priority = LOG_ERR; break;
        default: priority = LOG_ERR; break;
        }

        ::syslog(priority, "%s", formatted.c_str());
    }

    void flush() override
    {
    }
};


} // namespace {}


ER_SYSTEM_EXPORT ISink::Ptr makeSyslogSink(const char* tag, IFormatter::Ptr formatter, Filter&& filter)
{
    return std::make_shared<SyslogSink>(tag, formatter, std::move(filter));
}


} // namespace Er::Log2 {}