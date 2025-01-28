 #pragma once

#include <erebus/system/property_info.hxx>


namespace Er
{
    
/**
 * @brief An (almost) universal property
 *
 * Property that can be handled in a uniform way, accessed by scripts and marshaled through RPC
 */

struct ER_SYSTEM_EXPORT Property final
{
    ~Property()
    {
        _free();
    }

    Property() noexcept
        : m_u()
        , m_type()
    {
    }

    Property(nullptr_t) = delete;
    
    Property(bool v, const PropertyInfo& info) noexcept
        : m_u()
        , m_type(PropertyType::Bool, &info)
    {
        ErAssert(info.type == PropertyType::Bool);
        m_u.v_bool = v ? Er::True : Er::False;
    }

    Property(Bool v, const PropertyInfo& info) noexcept
        : m_u()
        , m_type(PropertyType::Bool, &info)
    {
        ErAssert(info.type == PropertyType::Bool);
        m_u.v_bool = v;
    }

    Property(std::int32_t v, const PropertyInfo& info) noexcept
        : m_u()
        , m_type(PropertyType::Int32, &info)
    {
        ErAssert(info.type == PropertyType::Int32);
        m_u.v_int32 = v;
    }

    Property(std::uint32_t v, const PropertyInfo& info) noexcept
        : m_u()
        , m_type(PropertyType::UInt32, &info)
    {
        ErAssert(info.type == PropertyType::UInt32);
        m_u.v_uint32 = v;
    }

    Property(std::int64_t v, const PropertyInfo& info) noexcept
        : m_u(v)
        , m_type(PropertyType::Int64, &info)
    {
        ErAssert(info.type == PropertyType::Int64);
    }

    Property(std::uint64_t v, const PropertyInfo& info) noexcept
        : m_u(v)
        , m_type(PropertyType::UInt64, &info)
    {
        ErAssert(info.type == PropertyType::UInt64);
    }

    Property(double v, const PropertyInfo& info) noexcept
        : m_u(v)
        , m_type(PropertyType::Double, &info)
    {
        ErAssert(info.type == PropertyType::Double);
    }

    Property(const char* v, const PropertyInfo& info)
        : m_u(new std::string(v))
        , m_type(PropertyType::String, &info)
    {
        ErAssert(info.type == PropertyType::String);
    }

    Property(const std::string& v, const PropertyInfo& info)
        : m_u(new std::string(v))
        , m_type(PropertyType::String, &info)
    {
        ErAssert(info.type == PropertyType::String);
    }

    Property(std::string&& v, const PropertyInfo& info)
        : m_u(new std::string(std::move(v)))
        , m_type(PropertyType::String, &info)
    {
        ErAssert(info.type == PropertyType::String);
    }

    Property(const Binary& v, const PropertyInfo& info)
        : m_u(new Binary(v))
        , m_type(PropertyType::Binary, &info)
    {
        ErAssert(info.type == PropertyType::Binary);
    }

    Property(Binary&& v, const PropertyInfo& info)
        : m_u(new Binary(std::move(v)))
        , m_type(PropertyType::Binary, &info)
    {
        ErAssert(info.type == PropertyType::Binary);
    }

    Property(const BoolVector& v, const PropertyInfo& info)
        : m_u(new BoolVector(v))
        , m_type(PropertyType::Bools, &info)
    {
        ErAssert(info.type == PropertyType::Bools);
    }

    Property(BoolVector&& v, const PropertyInfo& info)
        : m_u(new BoolVector(std::move(v)))
        , m_type(PropertyType::Bools, &info)
    {
        ErAssert(info.type == PropertyType::Bools);
    }

    Property(const Int32Vector& v, const PropertyInfo& info)
        : m_u(new Int32Vector(v))
        , m_type(PropertyType::Int32s, &info)
    {
        ErAssert(info.type == PropertyType::Int32s);
    }

    Property(Int32Vector&& v, const PropertyInfo& info)
        : m_u(new Int32Vector(std::move(v)))
        , m_type(PropertyType::Int32s, &info)
    {
        ErAssert(info.type == PropertyType::Int32s);
    }

    Property(const UInt32Vector& v, const PropertyInfo& info)
        : m_u(new UInt32Vector(v))
        , m_type(PropertyType::UInt32s, &info)
    {
        ErAssert(info.type == PropertyType::UInt32s);
    }

    Property(UInt32Vector&& v, const PropertyInfo& info)
        : m_u(new UInt32Vector(std::move(v)))
        , m_type(PropertyType::UInt32s, &info)
    {
        ErAssert(info.type == PropertyType::UInt32s);
    }

    Property(const Int64Vector& v, const PropertyInfo& info)
        : m_u(new Int64Vector(v))
        , m_type(PropertyType::Int64s, &info)
    {
        ErAssert(info.type == PropertyType::Int64s);
    }

    Property(Int64Vector&& v, const PropertyInfo& info)
        : m_u(new Int64Vector(std::move(v)))
        , m_type(PropertyType::Int64s, &info)
    {
        ErAssert(info.type == PropertyType::Int64s);
    }

    Property(const UInt64Vector& v, const PropertyInfo& info)
        : m_u(new UInt64Vector(v))
        , m_type(PropertyType::UInt64s, &info)
    {
        ErAssert(info.type == PropertyType::UInt64s);
    }

    Property(UInt64Vector&& v, const PropertyInfo& info)
        : m_u(new UInt64Vector(std::move(v)))
        , m_type(PropertyType::UInt64s, &info)
    {
        ErAssert(info.type == PropertyType::UInt64s);
    }

    Property(const DoubleVector& v, const PropertyInfo& info)
        : m_u(new DoubleVector(v))
        , m_type(PropertyType::Doubles, &info)
    {
        ErAssert(info.type == PropertyType::Doubles);
    }

    Property(DoubleVector&& v, const PropertyInfo& info)
        : m_u(new DoubleVector(std::move(v)))
        , m_type(PropertyType::Doubles, &info)
    {
        ErAssert(info.type == PropertyType::Doubles);
    }

    Property(const StringsVector& v, const PropertyInfo& info)
        : m_u(new StringsVector(v))
        , m_type(PropertyType::Strings, &info)
    {
        ErAssert(info.type == PropertyType::Strings);
    }

    Property(StringsVector&& v, const PropertyInfo& info)
        : m_u(new StringsVector(std::move(v)))
        , m_type(PropertyType::Strings, &info)
    {
        ErAssert(info.type == PropertyType::Strings);
    }

    Property(const BinariesVector& v, const PropertyInfo& info)
        : m_u(new BinariesVector(v))
        , m_type(PropertyType::Binaries, &info)
    {
        ErAssert(info.type == PropertyType::Binaries);
    }

    Property(BinariesVector&& v, const PropertyInfo& info)
        : m_u(new BinariesVector(std::move(v)))
        , m_type(PropertyType::Binaries, &info)
    {
        ErAssert(info.type == PropertyType::Binaries);
    }

    friend constexpr void swap(Property& a, Property& b) noexcept
    {
        std::swap(a.m_type, b.m_type);
        std::swap(a.m_u._largest, b.m_u._largest);
    }

    Property(const Property& other)
        : m_u(DontInit{})
        , m_type(DontInit{})
    {
        _clone(other);
    }

    Property& operator=(const Property& other)
    {
        Property tmp(other);
        swap(tmp, *this);
        return *this;
    }

    Property(Property&& other) noexcept
        : Property()
    {
        swap(other, *this);
    }

    Property& operator=(Property&& other) noexcept
    {
        Property tmp(std::move(other));
        swap(tmp, *this);
        return *this;
    }

    [[nodiscard]] constexpr PropertyType type() const noexcept
    {
        return m_type.type();
    }

    [[nodiscard]] const PropertyInfo* info() const noexcept
    {
        return m_type.info();
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_type.empty();
    }

    [[nodiscard]] constexpr const Bool& getBool() const noexcept
    {
        ErAssert(type() == PropertyType::Bool);
        return m_u.v_bool;
    }

    [[nodiscard]] constexpr const int32_t& getInt32() const noexcept
    {
        ErAssert(type() == PropertyType::Int32);
        return m_u.v_int32;
    }

    [[nodiscard]] constexpr const uint32_t& getUInt32() const noexcept
    {
        ErAssert(type() == PropertyType::UInt32);
        return m_u.v_uint32;
    }

    [[nodiscard]] constexpr const int64_t& getInt64() const noexcept
    {
        ErAssert(type() == PropertyType::Int64);
        return m_u.v_int64;
    }

    [[nodiscard]] constexpr const uint64_t& getUInt64() const noexcept
    {
        ErAssert(type() == PropertyType::UInt64);
        return m_u.v_uint64;
    }

    [[nodiscard]] constexpr const double& getDouble() const noexcept
    {
        ErAssert(type() == PropertyType::Double);
        return m_u.v_double;
    }

    [[nodiscard]] constexpr const std::string& getString() const noexcept
    {
        ErAssert(type() == PropertyType::String);
        ErAssert(m_u.v_string);
        return *m_u.v_string;
    }

    [[nodiscard]] constexpr const Binary& getBinary() const noexcept
    {
        ErAssert(type() == PropertyType::Binary);
        ErAssert(m_u.v_binary);
        return *m_u.v_binary;
    }

    [[nodiscard]] constexpr const BoolVector& getBools() const noexcept
    {
        ErAssert(type() == PropertyType::Bools);
        ErAssert(m_u.a_bool);
        return *m_u.a_bool;
    }

    [[nodiscard]] constexpr const Int32Vector& getInt32s() const noexcept
    {
        ErAssert(type() == PropertyType::Int32s);
        ErAssert(m_u.a_int32);
        return *m_u.a_int32;
    }

    [[nodiscard]] constexpr const UInt32Vector& getUInt32s() const noexcept
    {
        ErAssert(type() == PropertyType::UInt32s);
        ErAssert(m_u.a_uint32);
        return *m_u.a_uint32;
    }

    [[nodiscard]] constexpr const Int64Vector& getInt64s() const noexcept
    {
        ErAssert(type() == PropertyType::Int64s);
        ErAssert(m_u.a_int64);
        return *m_u.a_int64;
    }

    [[nodiscard]] constexpr const UInt64Vector& getUInt64s() const noexcept
    {
        ErAssert(type() == PropertyType::UInt64s);
        ErAssert(m_u.a_uint64);
        return *m_u.a_uint64;
    }

    [[nodiscard]] constexpr const DoubleVector& getDoubles() const noexcept
    {
        ErAssert(type() == PropertyType::Doubles);
        ErAssert(m_u.a_double);
        return *m_u.a_double;
    }

    [[nodiscard]] constexpr const StringsVector& getStrings() const noexcept
    {
        ErAssert(type() == PropertyType::Strings);
        ErAssert(m_u.a_string);
        return *m_u.a_string;
    }

    [[nodiscard]] constexpr const BinariesVector& getBinaries() const noexcept
    {
        ErAssert(type() == PropertyType::Binaries);
        ErAssert(m_u.a_binary);
        return *m_u.a_binary;
    }

    [[nodiscard]] bool operator==(const Property& other) const noexcept
    {
        return _eq(other);
    }

    [[nodiscard]] std::string str() const
    {
        return _str();
    }

    [[nodiscard]] auto name() const
    {
        auto inf = info();
        return inf ? inf->name : "/?/?/?";
    }

private:
    static bool _allocatesStorage(PropertyType type) noexcept
    {
        return (type >= PropertyType::String);
    }

    void _free() noexcept;
    void _freeString() noexcept;
    void _freeBinary() noexcept;
    void _freeBoolV() noexcept;
    void _freeInt32V() noexcept;
    void _freeUInt32V() noexcept;
    void _freeInt64V() noexcept;
    void _freeUInt64V() noexcept;
    void _freeDoubleV() noexcept;
    void _freeStringV() noexcept;
    void _freeBinaryV() noexcept;
    void _clone(const Property& other);
    void _cloneString(const Property& other);
    void _cloneBinary(const Property& other);
    void _cloneBoolV(const Property& other);
    void _cloneInt32V(const Property& other);
    void _cloneUInt32V(const Property& other);
    void _cloneInt64V(const Property& other);
    void _cloneUInt64V(const Property& other);
    void _cloneDoubleV(const Property& other);
    void _cloneStringV(const Property& other);
    void _cloneBinaryV(const Property& other);
    bool _eq(const Property& other) const noexcept;
    bool _eqString(const Property& other) const noexcept;
    bool _eqBinary(const Property& other) const noexcept;
    bool _eqBoolV(const Property& other) const noexcept;
    bool _eqInt32V(const Property& other) const noexcept;
    bool _eqUInt32V(const Property& other) const noexcept;
    bool _eqInt64V(const Property& other) const noexcept;
    bool _eqUInt64V(const Property& other) const noexcept;
    bool _eqDoubleV(const Property& other) const noexcept;
    bool _eqStringV(const Property& other) const noexcept;
    bool _eqBinaryV(const Property& other) const noexcept;
    std::string _str() const;
    std::string _strEmpty() const;
    std::string _strBool() const;
    std::string _strInt32() const;
    std::string _strUInt32() const;
    std::string _strInt64() const;
    std::string _strUInt64() const;
    std::string _strDouble() const;
    std::string _strString() const;
    std::string _strBinary() const;
    std::string _strBoolV() const;
    std::string _strInt32V() const;
    std::string _strUInt32V() const;
    std::string _strInt64V() const;
    std::string _strUInt64V() const;
    std::string _strDoubleV() const;
    std::string _strStringV() const;
    std::string _strBinaryV() const;

    struct DontInit
    {
    };

    union Storage
    {
        Bool v_bool;
        int32_t v_int32;
        uint32_t v_uint32;
        int64_t v_int64;
        uint64_t v_uint64;
        double v_double;
        void* _ptr;
        std::string* v_string;
        Binary* v_binary;
        BoolVector* a_bool;
        Int32Vector* a_int32;
        UInt32Vector* a_uint32;
        Int64Vector* a_int64;
        UInt64Vector* a_uint64;
        DoubleVector* a_double;
        StringsVector* a_string;
        BinariesVector* a_binary;
        uint64_t _largest; // must be the largest type

        constexpr Storage(DontInit) noexcept
        {
        }

        constexpr Storage() noexcept
            : _largest(0)
        {
        }

        constexpr Storage(double v) noexcept
            : v_double(v)
        {
        }

        constexpr Storage(int64_t v) noexcept
            : v_int64(v)
        {
        }

        constexpr Storage(uint64_t v) noexcept
            : v_uint64(v)
        {
        }

        constexpr Storage(void* v) noexcept
            : _ptr(v)
        {
        }

        // we don't have constructors for smaller types like int32 here
        // because we have to zero-initialize higher bits of _largest in the default constructor anyway
    } ;

    static_assert(sizeof(Storage) == sizeof(Storage::_largest));

    struct InfoAndType
    {
        std::uintptr_t ty;
        static constexpr std::uintptr_t TypeMask = 0x1FULL; // 5 lower pointer bits

        constexpr InfoAndType(DontInit) noexcept
        {
        }

        InfoAndType(PropertyType type, const PropertyInfo* info) noexcept
            : ty(reinterpret_cast<std::uintptr_t>(info) | static_cast<std::uintptr_t>(type))
        {
            ErAssert((reinterpret_cast<std::uintptr_t>(info) & TypeMask) == 0); // misaligned PropertyInfo
        }

        InfoAndType() noexcept
            : InfoAndType(PropertyType::Empty, &Unspecified::Empty)
        {}

        constexpr PropertyType type() const noexcept
        {
            return static_cast<PropertyType>(ty & TypeMask);
        }

        const PropertyInfo* info() const noexcept
        {
            return reinterpret_cast<const PropertyInfo*>(ty & ~TypeMask);
        }

        constexpr bool empty() const noexcept
        {
            return type() == PropertyType::Empty;
        }
    };
    
    Storage m_u;
    InfoAndType m_type;
};


template <typename T>
const T& get(const Property& v) noexcept;

template <>
[[nodiscard]] inline const Bool& get<>(const Property& v) noexcept
{
    return v.getBool();
}

template <>
[[nodiscard]] inline const int32_t& get<>(const Property& v) noexcept
{
    return v.getInt32();
}

template <>
[[nodiscard]] inline const uint32_t& get<>(const Property& v) noexcept
{
    return v.getUInt32();
}

template <>
[[nodiscard]] inline const int64_t& get<>(const Property& v) noexcept
{
    return v.getInt64();
}

template <>
[[nodiscard]] inline const uint64_t& get<>(const Property& v) noexcept
{
    return v.getUInt64();
}

template <>
[[nodiscard]] inline const double& get<>(const Property& v) noexcept
{
    return v.getDouble();
}

template <>
[[nodiscard]] inline const std::string& get<>(const Property& v) noexcept
{
    return v.getString();
}

template <>
[[nodiscard]] inline const Binary& get<>(const Property& v) noexcept
{
    return v.getBinary();
}

template <>
[[nodiscard]] inline const BoolVector& get<>(const Property& v) noexcept
{
    return v.getBools();
}

template <>
[[nodiscard]] inline const Int32Vector& get<>(const Property& v) noexcept
{
    return v.getInt32s();
}

template <>
[[nodiscard]] inline const UInt32Vector& get<>(const Property& v) noexcept
{
    return v.getUInt32s();
}

template <>
[[nodiscard]] inline const Int64Vector& get<>(const Property& v) noexcept
{
    return v.getInt64s();
}

template <>
[[nodiscard]] inline const UInt64Vector& get<>(const Property& v) noexcept
{
    return v.getUInt64s();
}

template <>
[[nodiscard]] inline const DoubleVector& get<>(const Property& v) noexcept
{
    return v.getDoubles();
}

template <>
[[nodiscard]] inline const StringsVector& get<>(const Property& v) noexcept
{
    return v.getStrings();
}

template <>
[[nodiscard]] inline const BinariesVector& get<>(const Property& v) noexcept
{
    return v.getBinaries();
}


[[nodiscard]] std::string_view propertyTypeToString(PropertyType type)
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
    case PropertyType::Bools: return "Bool[]";
    case PropertyType::Int32s: return "Int32[]";
    case PropertyType::UInt32s: return "UInt32[]";
    case PropertyType::Int64s: return "Int64[]";
    case PropertyType::UInt64s: return "UInt64[]";
    case PropertyType::Doubles: return "Double[]";
    case PropertyType::Strings: return "String[]";
    case PropertyType::Binaries: return "Binary[]";
    }

    return "\?\?\?";
}

} // namespace Er {}
