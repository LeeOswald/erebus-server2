#include <erebus/system/property.hxx>

#include <sstream>

namespace Er
{

void Property::_free() noexcept
{
    auto ty = type();

    if (!_allocatesStorage(ty))
    {
        m_type = InfoAndType();
        return;
    }

    if (m_u._shared)
        m_u._shared->release();
}

void Property::_clone(const Property& other)
{
    auto ty = other.type();
    m_type = other.m_type;
    m_u._largest = other.m_u._largest;

    if (_allocatesStorage(ty))
    {
        m_u._shared->addRef();
    }
}

bool Property::_eq(const Property& other) const noexcept
{
    using EqFn = bool (Property::*)(const Property&) const noexcept;

    static EqFn s_eqFns[] =
    {
        &Property::_eqString,
        &Property::_eqBinary,
        &Property::_eqMap
    };

    auto ty = type();
    if (ty != other.type())
        return false;

    if (!_allocatesStorage(ty))
    {
        return m_u._largest == other.m_u._largest;
    }

    auto idx = static_cast<std::size_t>(ty) - static_cast<std::size_t>(PropertyType::String);
    ErAssert(idx < _countof(s_eqFns));
    return std::invoke(s_eqFns[idx], *this, other);
}

bool Property::_eqString(const Property& other) const noexcept
{
    auto& v1 = getString();
    auto& v2 = other.getString();
    return v1 == v2;
}

bool Property::_eqBinary(const Property& other) const noexcept
{
    auto& v1 = getBinary();
    auto& v2 = other.getBinary();
    return v1 == v2;
}

bool Property::_eqMap(const Property& other) const noexcept
{
    auto& v1 = getMap();
    auto& v2 = other.getMap();
    return v1 == v2;
}

std::string Property::_str() const
{
    using StrFn = std::string (Property::*)() const;

    static StrFn s_strFns[] =
    {
        &Property::_strEmpty,
        &Property::_strBool,
        &Property::_strInt32,
        &Property::_strUInt32,
        &Property::_strInt64,
        &Property::_strUInt64,
        &Property::_strDouble,
        &Property::_strString,
        &Property::_strBinary,
        &Property::_strMap
    };

    auto ty = type();
    auto idx = static_cast<std::size_t>(ty);
    ErAssert(idx < _countof(s_strFns));
    return std::invoke(s_strFns[idx], *this);
}

std::string Property::_strEmpty() const
{
    return "[empty]";
}

std::string Property::_strBool() const
{
    auto v = getBool();
    return v ? "True" : "False";
}

std::string Property::_strInt32() const
{
    auto v = getInt32();
    return std::to_string(v);
}

std::string Property::_strUInt32() const
{
    auto v = getUInt32();
    return std::to_string(v);
}

std::string Property::_strInt64() const
{
    auto v = getInt64();
    return std::to_string(v);
}

std::string Property::_strUInt64() const
{
    auto v = getUInt64();
    return std::to_string(v);
}

std::string Property::_strDouble() const
{
    auto v = getDouble();
    return std::to_string(v);
}

std::string Property::_strString() const
{
    return getString();
}

std::string Property::_strBinary() const
{
    auto& v = getBinary();
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

std::string Property::_strMap() const
{
    auto& v = getMap();
    std::ostringstream ss;
    ss << "{";
    bool first = true;
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        if (first)
        {
            ss << " ";
            first = false;
        }
        else
        {
            ss << ", ";
        }

        auto& key = it->first;
        auto infoKey = key.info();
        ErAssert(infoKey);
        ss << "\'" << infoKey->format(key) << "\'";

        auto& value = it->second;
        auto infoValue = value.info();
        ErAssert(infoValue);
        ss << ": \'" << infoValue->format(value) << "\'";
    }

    if (!v.empty())
        ss << " ";

    ss << "}";

    return ss.str();
}

std::size_t Property::_hash() const noexcept
{
    using HashFn = std::size_t(Property::*)() const noexcept;

    static HashFn s_hashFns[] =
    {
        &Property::_hashEmpty,
        &Property::_hashBool,
        &Property::_hashInt32,
        &Property::_hashUInt32,
        &Property::_hashInt64,
        &Property::_hashUInt64,
        &Property::_hashDouble,
        &Property::_hashString,
        &Property::_hashBinary,
        &Property::_hashMap
    };

    auto ty = type();
    auto idx = static_cast<std::size_t>(ty);
    ErAssert(idx < _countof(s_hashFns));
    return std::invoke(s_hashFns[idx], *this);
}

std::size_t Property::_hashEmpty() const noexcept
{
    return 0;
}

std::size_t Property::_hashBool() const noexcept
{
    auto v = getBool();
    std::hash<bool> h;
    return h(v);
}

std::size_t Property::_hashInt32() const noexcept
{
    auto v = getInt32();
    std::hash<std::int32_t> h;
    return h(v);
}

std::size_t Property::_hashUInt32() const noexcept
{
    auto v = getUInt32();
    std::hash<std::uint32_t> h;
    return h(v);
}

std::size_t Property::_hashInt64() const noexcept
{
    auto v = getInt64();
    std::hash<std::int64_t> h;
    return h(v);
}

std::size_t Property::_hashUInt64() const noexcept
{
    auto v = getUInt64();
    std::hash<std::uint64_t> h;
    return h(v);
}

std::size_t Property::_hashDouble() const noexcept
{
    auto v = getDouble();
    std::hash<double> h;
    return h(v);
}

std::size_t Property::_hashString() const noexcept
{
    auto& v = getString();
    std::hash<std::string> h;
    return h(v);
}

std::size_t Property::_hashBinary() const noexcept
{
    auto& v = getBinary();
    return v.hash();
}

std::size_t Property::_hashMap() const noexcept
{
    auto& v = getMap();
    std::size_t hash = 0x9e3779b9;
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        auto& key = it->first;
        auto keyHash = key.hash();

        auto& value = it->second;
        auto valueHash = value.hash();

        // TODO: find a nicer hash function
        hash ^= (keyHash << 6) + (keyHash >> 1) + (valueHash << 6) + (valueHash >> 1);
    }

    return hash;
}

std::string_view propertyTypeToString(PropertyType type)
{
    switch (type)
    {
    case PropertyType::Empty: return "Empty";
    case PropertyType::Bool: return "Bool";
    case PropertyType::Int32: return "Int32";
    case PropertyType::UInt32: return "UInt32";
    case PropertyType::Int64: return "Int64";
    case PropertyType::UInt64: return "UInt64";
    case PropertyType::Double: return "Double";
    case PropertyType::String: return "String";
    case PropertyType::Binary: return "Binary";
    case PropertyType::Map: return "Map";
    }

    return "\?\?\?";
}

} // namespace Er {}