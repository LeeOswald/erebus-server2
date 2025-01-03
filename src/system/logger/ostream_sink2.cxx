#include <erebus/system/logger/ostream_sink2.hxx>


namespace Er::Log2
{

namespace
{

class OStreamSink
    : public SinkBase
{
public:
    OStreamSink(std::ostream& stream, IFormatter::Ptr formatter, Filter&& filter)
        : SinkBase(formatter, std::move(filter))
        , m_stream(stream)
    {
    }

    void write(Record::Ptr r) override
    {
        if (!filter(r.get()))
            return;

        auto formatted = format(r.get());
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
};

} // namespace {}


ER_SYSTEM_EXPORT ISink::Ptr makeOStreamSink(std::ostream& stream, IFormatter::Ptr formatter, Filter&& filter)
{
    return std::make_shared<OStreamSink>(stream, formatter, std::move(filter));
}

} // namespace Er::Log2 {}