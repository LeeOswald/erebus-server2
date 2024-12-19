#include <erebus/system/logger/syslog_linux.hxx>

#include <syslog.h>

#include <boost/noncopyable.hpp>


namespace Er::Log
{

namespace
{

class SyslogSink
    : public ISink
    , public boost::noncopyable
{
public:
    ~SyslogSink()
    {
        ::closelog();
    }

    SyslogSink(const char* tag, IFormatter::Ptr formatter, IFilter::Ptr filter)
        : m_filter(filter)
        , m_formatter(formatter)
    {
        ::openlog(tag, LOG_CONS, LOG_DAEMON);
    }

    void write(Record::Ptr r) override
    {
        if (r->level() < level())
            return;

        if (m_filter && !m_filter->filter(r.get()))
            return;

        auto formatted = m_formatter->format(r.get());
        const auto available = formatted.length();
        if (!available)
            return;

        int priority;
        switch (r->level())
        {
        case Er::Log::Level::Debug: priority = LOG_DEBUG; break;
        case Er::Log::Level::Info: priority = LOG_INFO; break;
        case Er::Log::Level::Warning: priority = LOG_WARNING; break;
        case Er::Log::Level::Error: priority = LOG_ERR; break;
        default: priority = LOG_ERR; break;
        }

        ::syslog(priority, "%s", formatted.c_str());
    }

    void flush() override
    {
    }

private:
    const IFilter::Ptr m_filter;
    const IFormatter::Ptr m_formatter;
};


} // namespace {}


ER_SYSTEM_EXPORT ISink::Ptr makeSyslogSink(const char* tag, IFormatter::Ptr formatter, IFilter::Ptr filter)
{
    return std::make_shared<SyslogSink>(tag, formatter, filter);
}


} // namespace Er::Log {}