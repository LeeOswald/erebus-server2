#pragma once

#include <erebus/system/property.hxx>

#include <algorithm>
#include <vector>


namespace Er
{


struct ER_SYSTEM_EXPORT PropertyBag final
{
public:
    using Container = std::vector<Property>;
    using Iterator = Container::iterator;
    using ConstIterator = Container::const_iterator;

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

    PropertyBag clone() const
    {
        return PropertyBag(*this);
    }

    void clear() noexcept
    {
        m_props.clear();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_props.empty();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_props.size();
    }

    void reserve(std::size_t size)
    {
        m_props.reserve(size);
    }

    void add(const Property& prop)
    {
        m_props.push_back(prop);
    }

    void add(Property&& prop)
    {
        m_props.push_back(std::move(prop));
    }

    void set(std::size_t index, const Property& prop)
    {
        if (m_props.size() < index)
            m_props.resize(index + 1);

        m_props[index] = prop;
    }

    void set(std::size_t index, Property&& prop)
    {
        if (m_props.size() < index)
            m_props.resize(index + 1);

        m_props[index] = std::move(prop);
    }

    [[nodiscard]] bool exists(const PropertyInfo* info) const noexcept
    {
        auto it = std::find_if(m_props.begin(), m_props.end(), [info](const Property& prop) { return prop.info() == info; });
        return it != m_props.end();
    }

    constexpr Property& operator[](std::size_t index) noexcept
    {
        ErAssert(m_props.size() > index);
        return m_props[index];
    }

    constexpr Property const& operator[](std::size_t index) const noexcept
    {
        ErAssert(m_props.size() > index);
        return m_props[index];
    }

    [[nodiscard]] Property* find(const PropertyInfo* info) noexcept
    {
        auto it = std::find_if(m_props.begin(), m_props.end(), [info](const Property& prop) { return prop.info() == info; });
        if (it == m_props.end())
            return nullptr;

        return &(*it);
    }

    [[nodiscard]] Property const* find(const PropertyInfo* info) const noexcept
    {
        auto it = std::find_if(m_props.begin(), m_props.end(), [info](const Property& prop) { return prop.info() == info; });
        if (it == m_props.end())
            return nullptr;

        return &(*it);
    }

    constexpr auto begin() noexcept
    {
        return m_props.begin();
    }

    constexpr auto begin() const noexcept
    {
        return m_props.begin();
    }

    constexpr auto end() noexcept
    {
        return m_props.end();
    }

    constexpr auto end() const noexcept
    {
        return m_props.end();
    }

    void erase(Iterator which) noexcept
    {
        m_props.erase(which);
    }

    void erase(ConstIterator which) noexcept
    {
        m_props.erase(which);
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

    Container m_props;
};


template <typename T>
T* get(PropertyBag& bag, const PropertyInfo* info) noexcept
{
    auto prop = bag.find(info);
    if (prop)
        return get<T>(prop);

    return nullptr;
}

template <typename T>
T const* get(PropertyBag const& bag, const PropertyInfo* info) noexcept
{
    auto prop = bag.find(info);
    if (prop)
        return get<T>(prop);

    return nullptr;
}

template <typename T>
bool update(PropertyBag& bag, std::size_t index, const PropertyInfo* info, T&& value)
{
    auto& existing = bag[index];
    ErAssert(existing.info() == info);
    auto& existingValue = get<T>(existing);
    if (existingValue != value)
    {
        bag[index] = Property(std::forward<T>(value), info);
        return true;
    }

    return false;
}





} // namespace Er {}