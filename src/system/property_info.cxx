#include <erebus/system/property.hxx>

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


Registry& registry()
{
    static Registry* r = new Registry();
    return *r;
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

std::string PropertyInfo::format(const Property& prop) const
{
    if (!formatter)
        return prop.str();

    return formatter(prop);
}

const PropertyInfo* PropertyInfo::lookup(const std::string& name) noexcept
{
    auto& r = registry();

    std::shared_lock l(r.mutex);
    auto it = r.properties.find(name);
    if (it != r.properties.end())
        return it->second.info;

    return nullptr;
}

namespace Unspecified
{

ER_SYSTEM_EXPORT const PropertyInfo Empty{ PropertyType::Empty, "unspecified/empty", "Empty" };


} // namespace Unspecified {}

} // namespace Er {}