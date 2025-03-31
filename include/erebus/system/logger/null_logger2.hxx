#include <erebus/system/logger2.hxx>

#include <mutex>
#include <queue>

namespace Er::Log2
{

struct NullLogger
    : public ILogger
    , public boost::noncopyable
{
    ~NullLogger() = default;
    NullLogger() = default;

    void indent() noexcept override
    {
    }

    void unindent() noexcept override
    {
    }

    void write(Record::Ptr r) override
    {
        std::lock_guard l(m_mutex);
        m_pending.push(r);
    }

    Record::Ptr pop()
    {
        std::lock_guard l(m_mutex);
        if (m_pending.empty())
            return {};

        auto r = m_pending.front();
        m_pending.pop();

        return r;
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

private:
    std::mutex m_mutex;
    std::queue<Record::Ptr> m_pending;
};


} // namespace Er::Log2 {}
