#include <erebus/system/property_info.hxx>

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>


namespace Er
{

namespace
{

struct Registration
{
    const PropertyInfo* info;
    std::uint32_t id;
    long refcount;

    constexpr Registration(const PropertyInfo* info, std::uint32_t id) noexcept
        : info(info)
        , id(id)
        , refcount(1)
    {}
};

struct Registry
{
    std::shared_mutex mutex;
    std::unordered_map<std::string, Registration> properties; // name -> info
};

//
// we want the registry be available very early (while global/static PropertyInfo instances are being constructed)
// and also very lately (until global/static PropertyInfo instances are all gone)
// hence, and because of the initialization order indeterminacy, we cannot rely on, e.g., the C++ local static 
// initialization related guarantees to create our Registry instance.
// we use 'create on demand' approach instead
//

// std::atomic<T*> constructor is trivial
std::atomic<Registry*> g_registry = nullptr;

Registry& registry()
{
    auto existing = g_registry.load(std::memory_order_acquire);
    if (!existing)
    {
        std::unique_ptr<Registry> r(new Registry{});
        Registry* expected = nullptr;
        if (g_registry.compare_exchange_strong(expected, r.get(), std::memory_order_acq_rel))
        {
            // we have successfully set our Registry instance
            existing = r.release();
        }
        else
        {
            // someone has already set the Registry instance
            // our instance will be destroyed
            existing = g_registry.load(std::memory_order_relaxed);
        }
    }

    ErAssert(existing);
    return *existing;
}


} // namespace {}



std::uint32_t PropertyInfo::registerProperty(const PropertyInfo* info)
{
    auto& r = registry();

    std::unique_lock l(r.mutex);
    auto nextId = static_cast<std::uint32_t>(r.properties.size());
    auto result = r.properties.insert({ info->name, Registration(info, nextId) });
    if (result.second)
    {
        // new info added
        return nextId;
    }

    // this info already exists
    result.first->second.refcount += 1;
    return result.first->second.id;
}

void PropertyInfo::unregisterProperty(const PropertyInfo* info) noexcept
{
    auto& r = registry();

    std::unique_lock l(r.mutex);
    auto it = r.properties.find(info->name);
    if (it != r.properties.end())
    {
        if (--it->second.refcount == 0)
        {
            r.properties.erase(it);
        }
    }
}


namespace Unspecified
{

ER_SYSTEM_EXPORT const PropertyInfo Empty{ PropertyType::Empty, "unspecified/empty", "Empty" };


} // namespace Unspecified {}

} // namespace Er {}