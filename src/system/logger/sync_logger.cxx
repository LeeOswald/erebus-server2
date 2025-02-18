#include <erebus/system/logger2.hxx>
#include <erebus/system/system/thread.hxx>
#include <erebus/system/util/thread_data.hxx>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>


namespace Er::Log2
{

namespace
{


class SyncLogger
    : public ILogger
    , public boost::noncopyable
{
public:
    ~SyncLogger()
    {
    }

    SyncLogger()
        : m_level(Level::Debug)
        , m_tee(makeTee(ThreadSafe::Yes))
    {
    }

    Level level() const noexcept override
    {
        return m_level;
    }

    Level setLevel(Level level) noexcept override
    {
        auto prev = m_level;
        m_level = level;
        return prev;
    }

    void indent() noexcept override
    {
        auto& td = m_threadData.data();
        ++td.indent;
    }

    void unindent() noexcept override
    {
        auto& td = m_threadData.data();
        ErAssert(td.indent > 0);
        --td.indent;
    }

    void write(Record::Ptr r) override
    {
        if (!r) [[unlikely]]
            return;

        if (r->level() < m_level) [[unlikely]]
            return;

        auto indent = m_threadData.data().indent;
        if (indent > 0)
            r->setIndent(indent);

        m_tee->write(r);
    }

    void flush() override
    {
    }

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
        m_tee->addSink(name, sink);
    }

    void removeSink(std::string_view name) override
    {
        m_tee->removeSink(name);
    }

    ISink::Ptr findSink(std::string_view name) override
    {
        return m_tee->findSink(name);
    }

private:
    struct PerThread
    {
        unsigned indent = 0;
    };

    using ThreadDataHolder = ThreadData<PerThread>;
    Level m_level;
    ITee::Ptr m_tee;
    ThreadDataHolder m_threadData;
};


} // namespace {}


ER_SYSTEM_EXPORT ILogger::Ptr makeSyncLogger()
{
    return std::make_shared<SyncLogger>();
}

} // namespace Er::Log2 {}