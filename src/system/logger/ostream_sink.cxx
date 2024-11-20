#include <erebus/system/log.hxx>

#include <boost/noncopyable.hpp>


namespace Er::Log
{

namespace
{

class OStreamSink
    : public ISink
    , public boost::noncopyable
{
public:
    OStreamSink(std::ostream& stream, IFormatter::Ptr formatter, IFilter::Ptr filter)
        : m_stream(stream)
        , m_filter(filter)
        , m_formatter(formatter)
    {
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

        const auto data = formatted.data();

        m_stream.write(data, available);
    }

    void flush() override
    {
        m_stream.flush();
    }

private:
    std::ostream& m_stream;
    const IFilter::Ptr m_filter;
    const IFormatter::Ptr m_formatter;
};

} // namespace {}


ER_SYSTEM_EXPORT ISink::Ptr makeOStreamSink(std::ostream& stream, IFormatter::Ptr formatter, IFilter::Ptr filter)
{
    return std::make_shared<OStreamSink>(stream, formatter, filter);
}

} // namespace Er::Log {}