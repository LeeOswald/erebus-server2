#include <erebus/system/log.hxx>
#include <erebus/system/system/thread.hxx>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>

namespace Er::Log
{

namespace
{

class AsyncLogger
    : public ILog
    , public boost::noncopyable
{
public:
    ~AsyncLogger()
    {
        releaseInstanceId(m_instanceId);
    }

    AsyncLogger()
        : m_instanceId(allocateInstanceId())
        , m_queue(MaxQueueSize)
        , m_worker([this](std::stop_token stop) { run(stop); })
    {
    }

    void write(Record::Ptr r) override
    {
        if (!r) [[unlikely]]
            return;

        if (r->level() < level())
            return;

        auto indent = threadData().indent;
        if (indent > 0)
            r->setIndent(indent);

        {
            std::unique_lock l(m_mutexQueue);
            m_queue.push_back(r);
        }

        m_queueNotEmpty.notify_one();
    }

    void flush() override
    {
        {
            std::unique_lock l(m_mutexQueue);

            // issue an empty record to force flushing all sinks
            m_queue.push_back(Record::Ptr{});
        }

        m_queueNotEmpty.notify_one();

        {
            auto stop = m_worker.get_stop_token();
            
            std::unique_lock l(m_mutexQueue);
            m_queueEmpty.wait(l, stop, [this]() { return m_queue.empty(); });
        }
    }

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
        std::unique_lock l(m_mutexSinks);
        m_sinks.push_back({ name, sink });
        m_sinkCount++;
    }

    void removeSink(std::string_view name) override
    {
        std::unique_lock l(m_mutexSinks);
        auto it = std::find_if(m_sinks.begin(), m_sinks.end(), [name](const SinkData& d) { return d.name == name; });
        if (it != m_sinks.end())
        {
            m_sinks.erase(it);
            m_sinkCount--;
        }
    }

    void indent() override
    {
        auto& td = threadData();
        ++td.indent;
    }

    void unindent() override
    {
        auto& td = threadData();
        ErAssert(td.indent > 0);
        --td.indent;
    }

private:
    using RecordQueue = boost::circular_buffer<Record::Ptr>;

    void run(std::stop_token stop)
    {
        System::CurrentThread::setName("Logger");

        do
        {
            RecordQueue q;

            {
                std::unique_lock lw(m_mutexQueue);
                
                if (m_queue.empty())
                    m_queueEmpty.notify_all();

                m_queueNotEmpty.wait(lw, stop, [this]() { return !m_queue.empty(); });

                if (m_sinkCount.load(std::memory_order_relaxed) < 1)
                {
                    // looks like we have no sinks but we don't want to lose messages
                    // maybe sinks will be added later
                    m_mutexQueue.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                q.swap(m_queue);
            }

            // queue is now unlocked
            if (!q.empty())
            {
                sendToSinks(q, stop);
            }

        } while (!stop.stop_requested());
    }

    void sendToSinks(const RecordQueue& records, std::stop_token stop)
    {
        std::unique_lock l(m_mutexSinks);

        for (auto record : records)
        {
            for (auto& sink : m_sinks)
            {
                if (sink.healthy)
                {
                    try
                    {
                        if (!record)
                        {
                            sink.sink->flush();
                        }
                        else
                        {
                            sink.sink->write(record);
                        }
                    }
                    catch (...)
                    {
                        sink.healthy = false;
                    }
                }
            }

            if (stop.stop_requested())
                break;
        }
    }

    struct SinkData
    {
        std::string name;
        ISink::Ptr sink;
        bool healthy = true;

        SinkData(std::string_view name, ISink::Ptr sink)
            : name(name)
            , sink(sink)
        {
        }
    };

    struct GlobalData
        : public boost::noncopyable
    {
        struct InstanceData
        {
            static const std::size_t InvalidInstanceId = std::numeric_limits<std::size_t>::max();

            std::size_t instanceId = InvalidInstanceId;

            constexpr InstanceData() noexcept = default;
        };

        std::mutex mutex;
        std::vector<InstanceData> instanceData;

        GlobalData() noexcept = default;
    };

    [[nodiscard]] static GlobalData& globalData() noexcept
    {
        static GlobalData gd;
        return gd;
    }

    [[nodiscard]] static std::size_t allocateInstanceId()
    {
        auto& g = globalData();
        std::unique_lock l(g.mutex);

        std::size_t id = 0;
        for (;;)
        {
            if (g.instanceData.size() < id + 1)
            {
                g.instanceData.resize(id + 1);
                ErAssert(g.instanceData[id].instanceId == GlobalData::InstanceData::InvalidInstanceId);
                g.instanceData[id].instanceId = id;
                return id;
            }

            if (g.instanceData[id].instanceId == GlobalData::InstanceData::InvalidInstanceId)
            {
                g.instanceData[id].instanceId = id;
                return id;
            }

            ++id;
        }
    }

    static void releaseInstanceId(std::size_t id) noexcept
    {
        auto& g = globalData();
        std::unique_lock l(g.mutex);
        g.instanceData[id].instanceId = GlobalData::InstanceData::InvalidInstanceId;
    }

    struct ThreadData
    {
        unsigned indent = 0;

        ThreadData() noexcept = default;
    };

    [[nodiscard]] ThreadData& threadData()
    {
        static thread_local std::vector<ThreadData> td;
        
        if (td.size() < m_instanceId + 1)
            td.resize(m_instanceId + 1);

        return td[m_instanceId];
    }

    static const std::size_t MaxQueueSize = 1024;
    
    const std::size_t m_instanceId;
    std::mutex m_mutexQueue;
    std::condition_variable_any m_queueNotEmpty;
    std::condition_variable_any m_queueEmpty;
    RecordQueue m_queue;
    std::atomic<long> m_sinkCount;
    std::mutex m_mutexSinks;
    std::vector<SinkData> m_sinks;
    std::jthread m_worker;
};

} // namespace {}


ER_SYSTEM_EXPORT ILog::Ptr makeAsyncLogger()
{
    return std::make_shared<AsyncLogger>();
}

} // namespace Er::Log {}