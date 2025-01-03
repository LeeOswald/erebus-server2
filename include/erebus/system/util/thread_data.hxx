#pragma once

#include <erebus/system/erebus.hxx>

#include <boost/noncopyable.hpp>

#include <mutex>
#include <vector>

namespace Er
{

template <class DataT>
class ThreadData
    : public boost::noncopyable
{
public:
    using Data = DataT;

    ~ThreadData()
    {
        releaseInstanceId(m_instanceId);
    }

    ThreadData()
        : m_instanceId(allocateInstanceId())
    {
    }

    [[nodiscard]] Data& data()
    {
        static thread_local std::vector<Data> td;
        
        if (td.size() < m_instanceId + 1)
            td.resize(m_instanceId + 1);

        return td[m_instanceId];
    }

private:
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

    const std::size_t m_instanceId;
};


} // namespace Er {}
