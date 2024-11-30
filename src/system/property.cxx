#include <erebus/system/property.hxx>

#include <sstream>

namespace Er
{

bool Property::_allocatesStorage(PropertyType type) noexcept
{
    return (type >= PropertyType::String);
}

void Property::_free() noexcept
{
    using FreeFn = void (Property::*)() noexcept;

    static FreeFn s_freeFns[] =
    {
        &Property::_freeString,
        &Property::_freeBinary,
        &Property::_freeBoolV,
        &Property::_freeInt32V,
        &Property::_freeUInt32V,
        &Property::_freeInt64V,
        &Property::_freeUInt64V,
        &Property::_freeDoubleV,
        &Property::_freeStringV,
        &Property::_freeBinaryV
    };

    auto ty = type();

    if (!_allocatesStorage(ty))
    {
        m_type = InfoAndType();
        return;
    }

    auto idx = static_cast<std::size_t>(ty) - static_cast<std::size_t>(PropertyType::String);
    ErAssert(idx < _countof(s_freeFns));
    std::invoke(s_freeFns[idx], *this);
}

void Property::_freeString() noexcept
{
    ErAssert(type() == PropertyType::String);
    delete m_u.v_string;
    m_u.v_string = nullptr;
    m_type = InfoAndType();
}

void Property::_freeBinary() noexcept
{
    ErAssert(type() == PropertyType::Binary);
    delete m_u.v_binary;
    m_u.v_binary = nullptr;
    m_type = InfoAndType();
}

void Property::_freeBoolV() noexcept
{
    ErAssert(type() == PropertyType::Bools);
    delete m_u.a_bool;
    m_u.a_bool = nullptr;
    m_type = InfoAndType();
}

void Property::_freeInt32V() noexcept
{
    ErAssert(type() == PropertyType::Int32s);
    delete m_u.a_int32;
    m_u.a_int32 = nullptr;
    m_type = InfoAndType();
}

void Property::_freeUInt32V() noexcept
{
    ErAssert(type() == PropertyType::UInt32s);
    delete m_u.a_uint32;
    m_u.a_uint32 = nullptr;
    m_type = InfoAndType();
}

void Property::_freeInt64V() noexcept
{
    ErAssert(type() == PropertyType::Int64s);
    delete m_u.a_int64;
    m_u.a_int64 = nullptr;
    m_type = InfoAndType();
}

void Property::_freeUInt64V() noexcept
{
    ErAssert(type() == PropertyType::UInt64s);
    delete m_u.a_uint64;
    m_u.a_uint64 = nullptr;
    m_type = InfoAndType();
}

void Property::_freeDoubleV() noexcept
{
    ErAssert(type() == PropertyType::Doubles);
    delete m_u.a_double;
    m_u.a_double = nullptr;
    m_type = InfoAndType();
}

void Property::_freeStringV() noexcept
{
    ErAssert(type() == PropertyType::Strings);
    delete m_u.a_string;
    m_u.a_string = nullptr;
    m_type = InfoAndType();
}

void Property::_freeBinaryV() noexcept
{
    ErAssert(type() == PropertyType::Binaries);
    delete m_u.a_binary;
    m_u.a_binary = nullptr;
    m_type = InfoAndType();
}

void Property::_clone(const Property& other)
{
    using CloneFn = void (Property::*)(const Property&);

    static CloneFn s_cloneFns[] =
    {
        &Property::_cloneString,
        &Property::_cloneBinary,
        &Property::_cloneBoolV,
        &Property::_cloneInt32V,
        &Property::_cloneUInt32V,
        &Property::_cloneInt64V,
        &Property::_cloneUInt64V,
        &Property::_cloneDoubleV,
        &Property::_cloneStringV,
        &Property::_cloneBinaryV
    };

    auto ty = other.type();
    if (!_allocatesStorage(ty))
    {
        m_u._largest = other.m_u._largest;
        m_type = other.m_type;
        return;
    }

    auto idx = static_cast<std::size_t>(ty) - static_cast<std::size_t>(PropertyType::String);
    ErAssert(idx < _countof(s_cloneFns));
    std::invoke(s_cloneFns[idx], *this, other);
    m_type = other.m_type;
}

void Property::_cloneString(const Property& other)
{
    ErAssert(other.type() == PropertyType::String);
    ErAssert(other.m_u.v_string);
    m_u.v_string = new std::string(*other.m_u.v_string);
}

void Property::_cloneBinary(const Property& other)
{
    ErAssert(other.type() == PropertyType::Binary);
    ErAssert(other.m_u.v_binary);
    m_u.v_binary = new Binary(*other.m_u.v_binary);
}

void Property::_cloneBoolV(const Property& other)
{
    ErAssert(other.type() == PropertyType::Bools);
    ErAssert(other.m_u.a_bool);
    m_u.a_bool = new BoolVector(*other.m_u.a_bool);
}

void Property::_cloneInt32V(const Property& other)
{
    ErAssert(other.type() == PropertyType::Int32s);
    ErAssert(other.m_u.a_int32);
    m_u.a_int32 = new Int32Vector(*other.m_u.a_int32);
}

void Property::_cloneUInt32V(const Property& other)
{
    ErAssert(other.type() == PropertyType::UInt32s);
    ErAssert(other.m_u.a_uint32);
    m_u.a_uint32 = new UInt32Vector(*other.m_u.a_uint32);
}

void Property::_cloneInt64V(const Property& other)
{
    ErAssert(other.type() == PropertyType::Int64s);
    ErAssert(other.m_u.a_int64);
    m_u.a_int64 = new Int64Vector(*other.m_u.a_int64);
}

void Property::_cloneUInt64V(const Property& other)
{
    ErAssert(other.type() == PropertyType::UInt64s);
    ErAssert(other.m_u.a_uint64);
    m_u.a_uint64 = new UInt64Vector(*other.m_u.a_uint64);
}

void Property::_cloneDoubleV(const Property& other)
{
    ErAssert(other.type() == PropertyType::Doubles);
    ErAssert(other.m_u.a_double);
    m_u.a_double = new DoubleVector(*other.m_u.a_double);
}

void Property::_cloneStringV(const Property& other)
{
    ErAssert(other.type() == PropertyType::Strings);
    ErAssert(other.m_u.a_string);
    m_u.a_string = new StringsVector(*other.m_u.a_string);
}

void Property::_cloneBinaryV(const Property& other)
{
    ErAssert(other.type() == PropertyType::Binaries);
    ErAssert(other.m_u.a_binary);
    m_u.a_binary = new BinariesVector(*other.m_u.a_binary);
}

bool Property::_eq(const Property& other) const noexcept
{
    using EqFn = bool (Property::*)(const Property&) const noexcept;

    static EqFn s_eqFns[] =
    {
        &Property::_eqString,
        &Property::_eqBinary,
        &Property::_eqBoolV,
        &Property::_eqInt32V,
        &Property::_eqUInt32V,
        &Property::_eqInt64V,
        &Property::_eqUInt64V,
        &Property::_eqDoubleV,
        &Property::_eqStringV,
        &Property::_eqBinaryV
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

bool Property::_eqBoolV(const Property& other) const noexcept
{
    auto& v1 = getBools();
    auto& v2 = other.getBools();
    return v1 == v2;
}

bool Property::_eqInt32V(const Property& other) const noexcept
{
    auto& v1 = getInt32s();
    auto& v2 = other.getInt32s();
    return v1 == v2;
}

bool Property::_eqUInt32V(const Property& other) const noexcept
{
    auto& v1 = getUInt32s();
    auto& v2 = other.getUInt32s();
    return v1 == v2;
}

bool Property::_eqInt64V(const Property& other) const noexcept
{
    auto& v1 = getInt64s();
    auto& v2 = other.getInt64s();
    return v1 == v2;
}

bool Property::_eqUInt64V(const Property& other) const noexcept
{
    auto& v1 = getUInt64s();
    auto& v2 = other.getUInt64s();
    return v1 == v2;
}

bool Property::_eqDoubleV(const Property& other) const noexcept
{
    auto& v1 = getDoubles();
    auto& v2 = other.getDoubles();
    return v1 == v2;
}

bool Property::_eqStringV(const Property& other) const noexcept
{
    auto& v1 = getStrings();
    auto& v2 = other.getStrings();
    return v1 == v2;
}

bool Property::_eqBinaryV(const Property& other) const noexcept
{
    auto& v1 = getBinaries();
    auto& v2 = other.getBinaries();
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
        &Property::_strBoolV,
        &Property::_strInt32V,
        &Property::_strUInt32V,
        &Property::_strInt64V,
        &Property::_strUInt64V,
        &Property::_strDoubleV,
        &Property::_strStringV,
        &Property::_strBinaryV
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
    auto& v = getDouble();
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

std::string Property::_strBoolV() const
{
    auto& a = getBools();
    if (a.empty())
        return "[]";

    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto v : a)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        ss << (v ? "True" : "False");
    }
    ss << "]";
    return ss.str();
}

std::string Property::_strInt32V() const
{
    auto& a = getInt32s();
    if (a.empty())
        return "[]";

    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto v : a)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        ss << v;
    }
    ss << "]";
    return ss.str();
}

std::string Property::_strUInt32V() const
{
    auto& a = getUInt32s();
    if (a.empty())
        return "[]";

    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto v : a)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        ss << v;
    }
    ss << "]";
    return ss.str();
}

std::string Property::_strInt64V() const
{
    auto& a = getInt64s();
    if (a.empty())
        return "[]";

    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto v : a)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        ss << v;
    }
    ss << "]";
    return ss.str();
}

std::string Property::_strUInt64V() const
{
    auto& a = getUInt64s();
    if (a.empty())
        return "[]";

    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto v : a)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        ss << v;
    }
    ss << "]";
    return ss.str();
}

std::string Property::_strDoubleV() const
{
    auto& a = getDoubles();
    if (a.empty())
        return "[]";

    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto v : a)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        ss << v;
    }
    ss << "]";
    return ss.str();
}

std::string Property::_strStringV() const
{
    auto& a = getStrings();
    if (a.empty())
        return "[]";

    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto& v : a)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        ss << "\"" << v << "\"";
    }
    ss << "]";
    return ss.str();
}

std::string Property::_strBinaryV() const
{
    auto& a = getBinaries();
    if (a.empty())
        return "[]";

    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (auto& v : a)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        ss << "\"" << v << "\"";
    }
    ss << "]";
    return ss.str();
}

} // namespace Er {}