#pragma once

#include <erebus/system/property_info.hxx>

#include <vector>

namespace Erp
{

template <class MutexT>
struct PropertyMapping
{
    MutexT lock;
    std::vector<Er::PropertyInfo const*> map;

    Er::PropertyInfo const* lookup(Er::PropertyInfo::Unique id) const noexcept
    {
        std::shared_lock l(lock);
        if (id >= map.size())
            return nullptr;

        return map[id];
    }

    void add(Er::PropertyInfo::Unique id, Er::PropertyInfo const* info)
    {
        std::unique_lock l(lock);
        if (id >= map.size())
            map.resize(id + 1);

        map[id] = info;
    }

    bool addIfMissing(Er::PropertyInfo::Unique id, Er::PropertyInfo const* info)
    {
        std::unique_lock l(lock);
        
        if (id >= map.size())
        {
            map.resize(id + 1);
            map[id] = info;
            return true;
        }

        if (!map[id])
        {
            map[id] = info;
            return true;
        }

        return false;
    }
};

} // namespace Erp {}