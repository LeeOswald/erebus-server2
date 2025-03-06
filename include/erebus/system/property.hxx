 #pragma once

#include <erebus/system/property_info.hxx>

#include <atomic>
#include <unordered_map>
#include <variant>


namespace Er
{
    
/**
 * @brief An (almost) universal property
 *
 * Property that can be handled in a uniform way, accessed by scripts and marshaled through RPC
 */

struct ER_SYSTEM_EXPORT Property final
{
private:
    struct PropertyHash
    {
        std::size_t operator()(const Property &prop) const
        {
            return prop.hash();
        }
    };

public:
    using Map = std::unordered_map<Property, Property, PropertyHash>;

    ~Property()
    {
        if (_allocatesStorage(type()))
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
        ErAssert(info.type() == PropertyType::Bool);
        m_u.v_bool = v ? Er::True : Er::False;
    }

    Property(Bool v, const PropertyInfo& info) noexcept
        : m_u()
        , m_type(PropertyType::Bool, &info)
    {
        ErAssert(info.type() == PropertyType::Bool);
        m_u.v_bool = v;
    }

    Property(std::int32_t v, const PropertyInfo& info) noexcept
        : m_u()
        , m_type(PropertyType::Int32, &info)
    {
        ErAssert(info.type() == PropertyType::Int32);
        m_u.v_int32 = v;
    }

    Property(std::uint32_t v, const PropertyInfo& info) noexcept
        : m_u()
        , m_type(PropertyType::UInt32, &info)
    {
        ErAssert(info.type() == PropertyType::UInt32);
        m_u.v_uint32 = v;
    }

    Property(std::int64_t v, const PropertyInfo& info) noexcept
        : m_u(v)
        , m_type(PropertyType::Int64, &info)
    {
        ErAssert(info.type() == PropertyType::Int64);
    }

    Property(std::uint64_t v, const PropertyInfo& info) noexcept
        : m_u(v)
        , m_type(PropertyType::UInt64, &info)
    {
        ErAssert(info.type() == PropertyType::UInt64);
    }

    Property(double v, const PropertyInfo& info) noexcept
        : m_u(v)
        , m_type(PropertyType::Double, &info)
    {
        ErAssert(info.type() == PropertyType::Double);
    }

    Property(const char* v, const PropertyInfo& info)
        : m_u(std::make_unique<SharedData>(std::string(v)).release())
        , m_type(PropertyType::String, &info)
    {
        ErAssert(info.type() == PropertyType::String);
    }

    Property(const std::string& v, const PropertyInfo& info)
        : m_u(std::make_unique<SharedData>(v).release())
        , m_type(PropertyType::String, &info)
    {
        ErAssert(info.type() == PropertyType::String);
    }

    Property(std::string&& v, const PropertyInfo& info)
        : m_u(std::make_unique<SharedData>(std::move(v)).release())
        , m_type(PropertyType::String, &info)
    {
        ErAssert(info.type() == PropertyType::String);
    }

    Property(const Binary& v, const PropertyInfo& info)
        : m_u(std::make_unique<SharedData>(v).release())
        , m_type(PropertyType::Binary, &info)
    {
        ErAssert(info.type() == PropertyType::Binary);
    }

    Property(Binary&& v, const PropertyInfo& info)
        : m_u(std::make_unique<SharedData>(std::move(v)).release())
        , m_type(PropertyType::Binary, &info)
    {
        ErAssert(info.type() == PropertyType::Binary);
    }

    Property(const Map& v, const PropertyInfo& info)
        : m_u(std::make_unique<SharedData>(v).release())
        , m_type(PropertyType::Map, &info)
    {
        ErAssert(info.type() == PropertyType::Map);
    }

    Property(Map&& v, const PropertyInfo& info)
        : m_u(std::make_unique<SharedData>(std::move(v)).release())
        , m_type(PropertyType::Map, &info)
    {
        ErAssert(info.type() == PropertyType::Map);
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
        ErAssert(m_u._shared);
        
        return std::get<std::string>(m_u._shared->data);
    }

    [[nodiscard]] constexpr const Binary& getBinary() const noexcept
    {
        ErAssert(type() == PropertyType::Binary);
        ErAssert(m_u._shared);
        
        return std::get<Binary>(m_u._shared->data);
    }

    [[nodiscard]] constexpr const Map& getMap() const noexcept
    {
        ErAssert(type() == PropertyType::Map);
        ErAssert(m_u._shared);
        
        return std::get<Map>(m_u._shared->data);
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
        return inf ? inf->name() : "/?/?/?";
    }

    [[nodiscard]] std::size_t hash() const noexcept
    {
        return _hash();
    }

private:
    inline static bool _allocatesStorage(PropertyType type) noexcept
    {
        return (type >= PropertyType::String);
    }
    
    void _free() noexcept;
    void _clone(const Property& other);
    bool _eq(const Property& other) const noexcept;
    bool _eqString(const Property& other) const noexcept;
    bool _eqBinary(const Property& other) const noexcept;
    bool _eqMap(const Property& other) const noexcept;
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
    std::string _strMap() const;
    std::size_t _hash() const noexcept;
    std::size_t _hashEmpty() const noexcept;
    std::size_t _hashBool() const noexcept;
    std::size_t _hashInt32() const noexcept;
    std::size_t _hashUInt32() const noexcept;
    std::size_t _hashInt64() const noexcept;
    std::size_t _hashUInt64() const noexcept;
    std::size_t _hashDouble() const noexcept;
    std::size_t _hashString() const noexcept;
    std::size_t _hashBinary() const noexcept;
    std::size_t _hashMap() const noexcept;

    struct DontInit
    {
    };

    // binary-large types like strings or maps are shared
    struct SharedData
    {
        enum class Type
        {
            String,
            Binary,
            Map
        };

        Type type;
        std::atomic<std::size_t> refs;
        std::variant<std::string, Binary, Map> data;
        
        ~SharedData() = default;

        SharedData(const std::string& v)
            : type(Type::String)
            , refs(1)
            , data(v)
        {}

        SharedData(std::string&& v)
            : type(Type::String)
            , refs(1)
            , data(std::move(v))
        {}

        SharedData(const Binary& v)
            : type(Type::Binary)
            , refs(1)
            , data(v)
        {}

        SharedData(Binary&& v)
            : type(Type::Binary)
            , refs(1)
            , data(std::move(v))
        {}

        SharedData(const Map& v)
            : type(Type::Map)
            , refs(1)
            , data(v)
        {}

        SharedData(Map&& v)
            : type(Type::Map)
            , refs(1)
            , data(std::move(v))
        {}

        std::size_t addRef() noexcept
        {
            auto prev = refs.fetch_add(1, std::memory_order_acq_rel);
            return prev + 1;
        }

        std::size_t release() noexcept
        {
            auto prev = refs.fetch_sub(1, std::memory_order_acq_rel);
            if (prev == 1)
                delete this;

            return prev - 1;
        }
    };

    union Storage
    {
        Bool v_bool;
        int32_t v_int32;
        uint32_t v_uint32;
        int64_t v_int64;
        uint64_t v_uint64;
        double v_double;
        SharedData* _shared;
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

        constexpr Storage(SharedData* v) noexcept
            : _shared(v)
        {
        }

        // we don't have constructors for smaller types like int32 here
        // because we have to zero-initialize higher bits of _largest in the default constructor anyway
    } ;

    static_assert(sizeof(Storage) == sizeof(Storage::_largest));

    struct InfoAndType
    {
        std::uintptr_t ty;
        static constexpr std::uintptr_t TypeMask = 0x0FULL; // 4 lower pointer bits

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



[[nodiscard]] std::string_view propertyTypeToString(PropertyType type);

} // namespace Er {}
