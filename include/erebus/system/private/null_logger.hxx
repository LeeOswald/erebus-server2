#include <erebus/system/log.hxx>

#include <boost/noncopyable.hpp>


namespace Erp
{

class NullLogger
    : public Er::Log::ILog
    , public boost::noncopyable
{
public:
    ~NullLogger() = default;
    NullLogger() = default;

    void write(Er::Log::Record::Ptr) override
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
