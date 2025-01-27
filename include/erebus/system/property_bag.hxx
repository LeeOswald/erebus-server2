#pragma once

#include <erebus/system/property.hxx>

#include <vector>


namespace Er
{

using PropertyBag = std::vector<Property>;


template <typename T>
const T* get(const PropertyBag& bag, const PropertyInfo& info)
{
    for (auto& prop : bag)
    {
        if (prop.info() == &info)
            return &get<T>(prop);
    }

    return nullptr;
}

template <typename T>
bool update(PropertyBag& src, std::size_t index, Er::Property&& prop)
{
    if (index + 1 > src.size())
    {
        src.resize(index + 1);
        src[index] = std::move(prop);
        return true;
    }

    if (src[index].info() != prop.info())
    {
        src[index] = std::move(prop);
        return true;
    }

    if (prop != src[index])
    {
        src[index] = std::move(prop);
        return true;
    }

    return false;
}

} // namespace Er {}
