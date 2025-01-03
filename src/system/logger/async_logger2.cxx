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


class AsyncLogger
    : public ILogger
    , public boost::noncopyable
{
public:
    ~AsyncLogger()
    {
    }

    AsyncLogger(std::chrono::milliseconds threshold)
        : m_threshold(threshold)
        , m_level(Level::Debug)
        , m_tee(makeTee(ThreadSafe::No)) // until we have only one worker thread
        , m_worker([this](std::stop_token stop) { run(stop); })
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

        bool thresholdReached = false;
        {
            std::unique_lock l(m_mutexQueue);
            m_queue.push(r);

            if (m_queue.size() == 1)
            {
                // remember the oldest record's timestamp
                m_last = std::chrono::steady_clock::now(); 
            }

            // don't send out records too often
            if (m_threshold != std::chrono::milliseconds{})
            {
                auto now = std::chrono::steady_clock::now();
                auto delta = now - m_last;
                if (std::chrono::duration_cast<std::chrono::milliseconds>(delta) >= m_threshold)
                    thresholdReached = true;
            }
        }

        if (thresholdReached)
            m_queueNotEmpty.notify_one();
    }
    
    void flush() override
    {
        {
            std::unique_lock l(m_mutexQueue);

            // issue an empty record to force flushing all sinks
            m_queue.push(Record::Ptr{});
        }

        m_queueNotEmpty.notify_one();

        // wait until really flushed
        {
            auto stop = m_worker.get_stop_token();
            
            std::unique_lock l(m_mutexQueue);
            m_queueEmpty.wait(l, stop, [this]() { return m_queue.empty(); });
        }
    }
    
    void addSink(std::string_view name, ISink::Ptr sink) override
    {
        std::unique_lock l(m_mutexTee);
        m_tee->addSink(name, sink);
    }
    
    void removeSink(std::string_view name) override
    {
        std::unique_lock l(m_mutexTee);
        m_tee->removeSink(name);
    }
    
private:
    struct PerThread
    {
        unsigned indent = 0;
    };

    using ThreadDataHolder = ThreadData<PerThread>;

    void run(std::stop_token stop)
    {
        System::CurrentThread::setName("Logger");

        do
        {
            std::queue<Record::Ptr> q;

            {
                std::unique_lock lw(m_mutexQueue);
                
                if (m_queue.empty())
                    m_queueEmpty.notify_all();

                m_queueNotEmpty.wait(lw, stop, [this]() { return !m_queue.empty(); });

                q.swap(m_queue);

                // clean the oldest record's timestamp
                m_last = std::chrono::time_point<std::chrono::steady_clock>::min();
            }

            // queue is now unlocked
            if (!q.empty())
            {
                sendToSinks(q, stop);
            }

        } while (!stop.stop_requested());
    }

    void sendToSinks(std::queue<Record::Ptr>& records, std::stop_token stop)
    {
        while (!records.empty())
        {
            auto record = records.front();
            records.pop();

            if (!record)
                m_tee->flush(); // empty record means forced flush
            else
                m_tee->write(record);

            if (stop.stop_requested())
                break;
        }
    }

    std::chrono::milliseconds m_threshold;
    Level m_level;
    std::mutex m_mutexTee;
    ITee::Ptr m_tee;
    ThreadDataHolder m_threadData;
    std::mutex m_mutexQueue;
    std::condition_variable_any m_queueNotEmpty;
    std::condition_variable_any m_queueEmpty;
    std::queue<Record::Ptr> m_queue;
    std::chrono::time_point<std::chrono::steady_clock> m_last = std::chrono::time_point<std::chrono::steady_clock>::min();
    std::jthread m_worker;
};


} // namespace {}


ER_SYSTEM_EXPORT ILogger::Ptr makeLogger(std::chrono::milliseconds threshold)
{
    return std::make_shared<AsyncLogger>(threshold);
}

} // namespace Er::Log2 {}