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
    using Ptr = std::shared_ptr<Registration>;

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
};


Registry& registry()
{
    static Registry r;
    return r;
}


} // namespace {}



void PropertyInfo::registerProperty(const PropertyInfo* info)
{
    auto& r = registry();

    std::unique_lock l(r.mutex);
    auto what = std::make_shared<Registration>(info);
    auto result = r.properties.insert({ info->name(), what });
    if (!result.second)
    {
        // this info already exists
        result.first->second->refcount += 1;
    }
}

void PropertyInfo::unregisterProperty(const PropertyInfo* info) noexcept
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

std::string PropertyInfo::format(const Property& prop) const
{
    if (!m_formatter)
        return prop.str();

    return m_formatter(prop);
}

const PropertyInfo* PropertyInfo::lookup(const std::string& name) noexcept
{
    auto& r = registry();

    std::shared_lock l(r.mutex);
    auto it = r.properties.find(name);
    if (it != r.properties.end())
        return it->second->info;

    return nullptr;
}

namespace Unspecified
{

const PropertyInfo Empty{ PropertyType::Empty, "unspecified/empty", "Empty" };
const PropertyInfo Bool{ PropertyType::Bool, "unspecified/bool", "Bool" };
const PropertyInfo Int32{ PropertyType::Int32, "unspecified/int32", "Int32" };
const PropertyInfo UInt32{ PropertyType::UInt32, "unspecified/uint32", "UInt32" };
const PropertyInfo Int64{ PropertyType::Int64, "unspecified/int64", "Int64" };
const PropertyInfo UInt64{ PropertyType::UInt64, "unspecified/uint64", "UInt64" };
const PropertyInfo Double{ PropertyType::Double, "unspecified/double", "Double" };
const PropertyInfo String{ PropertyType::String, "unspecified/string", "String" };
const PropertyInfo Binary{ PropertyType::Binary, "unspecified/binary", "Binary" };
const PropertyInfo Bools{ PropertyType::Bools, "unspecified/bool[]", "Bool[]" };
const PropertyInfo Int32s{ PropertyType::Int32s, "unspecified/int32[]", "Int32[]" };
const PropertyInfo UInt32s{ PropertyType::UInt32s, "unspecified/uint32[]", "UInt32[]" };
const PropertyInfo Int64s{ PropertyType::Int64s, "unspecified/int64[]", "Int64[]" };
const PropertyInfo UInt64s{ PropertyType::UInt64s, "unspecified/uint64[]", "UInt64[]" };
const PropertyInfo Doubles{ PropertyType::Doubles, "unspecified/double[]", "Double[]" };
const PropertyInfo Strings{ PropertyType::Strings, "unspecified/string[]", "String[]" };
const PropertyInfo Binaries{ PropertyType::Binaries, "unspecified/binary[]", "Binary[]" };

} // namespace Unspecified {}

} // namespace Er {}