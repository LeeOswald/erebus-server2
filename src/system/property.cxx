#include <erebus/system/property.hxx>


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
    m_u.a_string = new StringVector(*other.m_u.a_string);
}

void Property::_cloneBinaryV(const Property& other)
{
    ErAssert(other.type() == PropertyType::Binaries);
    ErAssert(other.m_u.a_binary);
    m_u.a_binary = new BinaryVector(*other.m_u.a_binary);
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


} // namespace Er {}