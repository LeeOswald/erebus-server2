#include <erebus/log.hxx>

namespace Erp
{

class NullLogger
    : public Log::ILog
    , public boost::noncopyable
{
public:
    ~DummyLogger() = default;
    DummyLogger() = default;

    void write(Log::Record::Ptr) override
    {
    }

    void flush() override
    {
    }

    void addSink(std::string_view, ISink::Ptr) override
    {
    }

    void removeSink(std::string_view) override
    {
    }

    void indent() override
    {
    }

    void unindent() override
    {
    }
};


} // namespace Erp {}
