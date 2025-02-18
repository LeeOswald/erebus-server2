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
        return Level::Debug;
    }

    Level setLevel(Level level) noexcept override
    {
        return Level::Debug;
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

    ISink::Ptr findSink(std::string_view name) override
    {
        return ISink::Ptr();
    }
};


} // namespace Er::Log2 {}
