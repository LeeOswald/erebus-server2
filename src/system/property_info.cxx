#include <erebus/system/exception.hxx>
#include <erebus/system/format.hxx>
#include <erebus/system/logger2.hxx>
#include <erebus/system/property.hxx>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace
{

struct Registration
{
    using Ptr = std::unique_ptr<Registration>;

    const Er::PropertyInfo* info;

    constexpr Registration(const Er::PropertyInfo* info) noexcept
        : info(info)
    {}
};

struct Registry
{
    std::shared_mutex mutex;
    std::unordered_map<std::string, const Er::PropertyInfo*> persistentProps;
    std::unordered_map<std::string, std::unique_ptr<Er::PropertyInfo>> transientProps;
    std::uint32_t unique = 0;
};


Registry& registry()
{
    static auto r = new Registry();
    return *r;
}


} // namespace {}


namespace Erp
{


ER_SYSTEM_EXPORT std::uint32_t registerPersistentProperty(const Er::PropertyInfo* info)
{
    auto& r = registry();

    std::unique_lock l(r.mutex);

    // maybe already there
    auto it = r.persistentProps.find(info->name());
    if (it != r.persistentProps.end())
    {
        if (it->second->type() != info->type())
        {
            ErThrow(Er::format("Property [{}] of type {} already registered but with different type {}", info->name(), 
                static_cast<unsigned>(info->type()), static_cast<unsigned>(it->second->type())));
        }

        return it->second->unique();
    }

    auto id = r.unique++;
    auto result = r.persistentProps.insert({ info->name(), info });
    ErAssert(result.second);
    
    return id;
}

ER_SYSTEM_EXPORT std::string formatProperty(const Er::PropertyInfo* info, const Er::Property& prop)
{
    auto& f = info->formatter();
    if (!f)
        return prop.str();

    return f(prop);
}

ER_SYSTEM_EXPORT std::uint32_t propertyMappingVersion() noexcept
{
    auto& r = registry();

    std::shared_lock l(r.mutex);

    return r.unique;
}

ER_SYSTEM_EXPORT const Er::PropertyInfo* allocateTransientProperty(Er::PropertyType type, const std::string& name, const std::string& readableName)
{
    auto& r = registry();

    std::unique_lock l(r.mutex);

    // maybe already there
    auto it = r.persistentProps.find(name);
    if (it != r.persistentProps.end())
    {
        if (it->second->type() != type)
        {
            ErThrow(Er::format("Property [{}] of type {} already registered but with different type {}", name,
                static_cast<unsigned>(type), static_cast<unsigned>(it->second->type())));
        }

        Er::Log2::debug(Er::Log2::get(), "Transient property mapped to persistent property {} [{}] of type {}", name, readableName, static_cast<std::uint32_t>(type));
        return it->second;
    }

    auto it2 = r.transientProps.find(name);
    if (it2 != r.transientProps.end())
    {
        if (it2->second->type() != type)
        {
            ErThrow(Er::format("Property [{}] of type {} already registered but with different type {}", name,
                static_cast<unsigned>(type), static_cast<unsigned>(it2->second->type())));
        }

        Er::Log2::debug(Er::Log2::get(), "Transient property found: {} [{}] of type {}", name, readableName, static_cast<std::uint32_t>(type));
        return it2->second.get();
    }

    // allocate a new transient property
    auto id = r.unique++;
    auto prop = std::make_unique<Er::PropertyInfo>(Er::PropertyInfo::Transient{}, id, type, name, readableName);
    auto pi = prop.get();

    r.transientProps.insert({ name, std::move(prop) });

    Er::Log2::debug(Er::Log2::get(), "Transient property registered: {} [{}] of type {}", name, readableName, static_cast<std::uint32_t>(type));

    return pi;
}

} // namespace Erp {}

namespace Er
{

ER_SYSTEM_EXPORT const PropertyInfo* lookupProperty(const std::string& name) noexcept
{
    auto& r = registry();

    std::shared_lock l(r.mutex);

    auto it = r.persistentProps.find(name);
    if (it != r.persistentProps.end())
        return it->second;

    auto it2 = r.transientProps.find(name);
    if (it2 != r.transientProps.end())
        return it2->second.get();

    return nullptr;
}

ER_SYSTEM_EXPORT std::uint32_t enumerateProperties(std::function<bool(const PropertyInfo*)> cb) noexcept
{
    auto& r = registry();

    std::shared_lock l(r.mutex);

    for (auto& pi : r.persistentProps)
    {
        if (!cb(pi.second))
            break;
    }

    for (auto& pi : r.transientProps)
    {
        if (!cb(pi.second.get()))
            break;
    }

    return r.unique;
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