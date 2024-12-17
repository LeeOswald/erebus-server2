#pragma once

#include <erebus/system/property.hxx>

#include <vector>


namespace Er
{


struct ER_SYSTEM_EXPORT PropertyBag final
{
public:
    ~PropertyBag() = default;

    explicit PropertyBag(std::size_t reserve = 0)
    {
        if (reserve)
            m_props.reserve(reserve);
    }

    friend void swap(PropertyBag& a, PropertyBag& b) noexcept
    {
        a.m_props.swap(b.m_props);
    }

    PropertyBag(PropertyBag&& o) noexcept
        : PropertyBag()
    {
        swap(*this, o);
    }

    PropertyBag& operator=(PropertyBag&& o) noexcept
    {
        PropertyBag tmp(std::move(o));
        swap(*this, tmp);
        return *this;
    }

private:
    PropertyBag(const PropertyBag& o)
        : m_props(o.m_props)
    {
    }

    PropertyBag& operator=(const PropertyBag& o)
    {
        m_props = o.m_props;
        return *this;
    }

    std::vector<Property> m_props;
};


} // namespace Er {}