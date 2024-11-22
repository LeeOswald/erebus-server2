#include <erebus/system/log.hxx>

namespace Er::Log
{

namespace
{

class SimpleFormatterImpl
    : public IFormatter
    , public SimpleFormatter
{
public:
    ~SimpleFormatterImpl() = default;

    std::string format(const Record* r) const override
    {
        bool needPrefix = m_options.any();

        std::tm time{};
        std::uint32_t msec = 0;

        if (m_options[Option::DateTime] || m_options[Option::Time])
        {
            System::PackedTime pt(r->time());
            if (m_options[Option::TzUtc])
                time = pt.toUtc();
            else
                time = pt.toLocalTime();

            msec = pt.subSecond() / 1000;
        }

        std::ostringstream ss;
        bool prefixEmpty = true;

        if (needPrefix)
            ss << "[";

        if (m_options[Option::DateTime])
        {
            ss << Er::format("{:04}/{:02}/{:02} {:02}:{:02}:{:02}.{:03}", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, msec);
            prefixEmpty = false;
        }
        else if (m_options[Option::Time])
        {
            ss << Er::format("{:02}:{:02}:{:02}.{:03}", time.tm_hour, time.tm_min, time.tm_sec, msec);
            prefixEmpty = false;
        }

        if (m_options[Option::Level])
        {
            if (!prefixEmpty)
                ss << " ";

            switch (r->level())
            {
            case Log::Level::Debug: ss << "D"; break;
            case Log::Level::Info: ss << "I"; break;
            case Log::Level::Warning: ss << "W"; break;
            case Log::Level::Error: ss << "E"; break;
            case Log::Level::Fatal: ss << "!"; break;
            default: ss << "?"; break;
            }

            prefixEmpty = false;
        }

        if (m_options[Option::Tid])
        {
            if (!prefixEmpty)
                ss << " ";

            ss << "@" << r->tid();
            prefixEmpty = false;
        }

        if (needPrefix)
        {
            ss << "]";
            prefixEmpty = false;
        }

        if (!r->message().empty())
        {
            if (!prefixEmpty)
                ss << " ";

            for (unsigned i = 0; i < r->indent(); ++i)
                ss << m_indent;

            ss << r->message();
        }

        if (!m_options[Option::NoNewLine])
        {
            if (m_options[Option::TzCrLf])
                ss << "\r\n";
            else
                ss << "\n";
        }

        return ss.str();
    }
    
    SimpleFormatterImpl(Options options, unsigned indentSize)
        : m_options(options)
        , m_indent(std::clamp(indentSize, 1u, MaxIndent), ' ')
    {
    }

private:
    static constexpr unsigned MaxIndent = 64;

    const Options m_options;
    const std::string m_indent;
};


} // namespace {}

IFormatter::Ptr SimpleFormatter::make(SimpleFormatter::Options options, unsigned indentSize)
{
    return std::make_shared<SimpleFormatterImpl>(options, indentSize);
}

} // namespace Er::Log {}