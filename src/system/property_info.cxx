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
    using Ptr = std::unique_ptr<Registration>;

    const PropertyInfo* info;
    long refcount;

    constexpr Registration(const PropertyInfo* info) noexcept
        : info(info)
        , refcount(1)
    {}
};

struct Registry
{
    std::shared_mutex mutex;
    std::unordered_map<std::string, Registration::Ptr> properties; // name -> info
    std::uint32_t unique = 0;
};


Registry& registry()
{
    static auto r = new Registry();
    return *r;
}


} // namespace {}



ER_SYSTEM_EXPORT std::uint32_t registerProperty(const PropertyInfo* info)
{
    auto& r = registry();

    std::unique_lock l(r.mutex);
    auto what = std::make_unique<Registration>(info);
    auto result = r.properties.insert({ info->name(), std::move(what) });
    if (!result.second)
    {
        // this info already exists
        result.first->second->refcount += 1;
    }

    return r.unique++;
}

ER_SYSTEM_EXPORT void unregisterProperty(const PropertyInfo* info) noexcept
{
    auto& r = registry();

    std::unique_lock l(r.mutex);
    auto it = r.properties.find(info->name());
    if (it != r.properties.end())
    {
        if (--it->second->refcount == 0)
        {
            r.properties.erase(it);
        }
    }
}

ER_SYSTEM_EXPORT const PropertyInfo* lookupProperty(const std::string& name) noexcept
{
    auto& r = registry();

    std::shared_lock l(r.mutex);
    auto it = r.properties.find(name);
    if (it != r.properties.end())
        return it->second->info;

    return nullptr;
}

ER_SYSTEM_EXPORT void enumerateProperties(std::function<bool(const PropertyInfo*)> cb) noexcept
{
    auto& r = registry();

    std::shared_lock l(r.mutex);
    for (auto& pi : r.properties)
    {
        if (!cb(pi.second->info))
            break;
    }
}

ER_SYSTEM_EXPORT std::string formatProperty(const PropertyInfo* info, const Property& prop)
{
    auto& f = info->formatter();
    if (!f)
        return prop.str();

    return f(prop);
}

namespace Unspecified
{

const PropertyInfo Empty{ PropertyType::Empty, "Er.Unspecified.Empty", "Empty" };
const PropertyInfo Bool{ PropertyType::Bool, "Er.Unspecified.Bool", "Bool" };
const PropertyInfo Int32{ PropertyType::Int32, "Er.Unspecified.Int32", "Int32" };
const PropertyInfo UInt32{ PropertyType::UInt32, "Er.Unspecified.UInt32", "UInt32" };
const PropertyInfo Int64{ PropertyType::Int64, "Er.Unspecified.Int64", "Int64" };
const PropertyInfo UInt64{ PropertyType::UInt64, "Er.Unspecified.UInt64", "UInt64" };
const PropertyInfo Double{ PropertyType::Double, "Er.Unspecified.Double", "Double" };
const PropertyInfo String{ PropertyType::String, "Er.Unspecified.String", "String" };
const PropertyInfo Binary{ PropertyType::Binary, "Er.Unspecified.Binary", "Binary" };


} // namespace Unspecified {}

} // namespace Er {}