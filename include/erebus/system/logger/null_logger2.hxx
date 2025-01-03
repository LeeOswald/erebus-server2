#include <erebus/system/logger2.hxx>


namespace Er::Log2
{

class NullLogger
    : public ILogger
    , public boost::noncopyable
{
public:
    ~NullLogger() = default;
    NullLogger() = default;

    Level level() const noexcept override
    {
    }

    Level setLevel(Level level) noexcept override
    {
    }

    void indent() noexcept override
    {
    }

    void unindent() noexcept override
    {
    }

    void write(Record::Ptr r) override
    {
    }

    void flush() override
    {
    }

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
    }

    void removeSink(std::string_view name) override
    {
    }
};


} // namespace Er::Log2 {}
