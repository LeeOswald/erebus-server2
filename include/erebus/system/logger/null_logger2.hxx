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
        auto& s = state();

        std::lock_guard l(s.mutex);
        s.pending.push(r);
    }

    Record::Ptr pop()
    {
        auto& s = state();

        std::lock_guard l(s.mutex);
        if (s.pending.empty())
            return {};

        auto r = s.pending.front();
        s.pending.pop();

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
    struct State
    {
        std::mutex mutex;
        std::queue<Record::Ptr> pending;
    };

    static State& state() noexcept
    {
        static State s;
        return s;
    }
};


} // namespace Er::Log2 {}
