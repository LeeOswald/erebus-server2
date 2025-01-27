#pragma once

#include <erebus/system/logger2.hxx>

namespace Er
{

struct RegisteredType;

ER_SYSTEM_EXPORT void initializeTypeRegistry(Er::Log2::ILogger* log);
ER_SYSTEM_EXPORT void finalizeTypeRegistry() noexcept;

ER_SYSTEM_EXPORT [[nodiscard]] RegisteredType* lookupType(std::string_view name);


namespace detail 
{


template <typename Type>
[[nodiscard]] constexpr auto strippedTypeName() noexcept 
{
    std::string_view prettyFunction{ static_cast<const char*>(ER_PRETTY_FUNCTION) };
    auto first = prettyFunction.find_first_not_of(' ', prettyFunction.find_first_of(ER_PRETTY_FUNCTION_PREFIX) + 1);
    auto value = prettyFunction.substr(first, prettyFunction.find_last_of(ER_PRETTY_FUNCTION_SUFFIX) - first);
    return value;
}

template <typename Type, auto = strippedTypeName<Type>().find_first_of('.')>
[[nodiscard]] constexpr std::string_view typeName(int) noexcept 
{
    constexpr auto value = strippedTypeName<Type>();
    return value;
}

template <typename Type>
[[nodiscard]] std::string_view typeName(char) noexcept 
{
    static const auto value = strippedTypeName<Type>();
    return value;
}


} // namespace detail {}


template <typename Type>
struct TypeName final 
{
    [[nodiscard]] static constexpr std::string_view value() noexcept 
    {
        return detail::typeName<Type>(0);
    }
};


using TypeIndex = std::uintptr_t;


struct RegisteredType
{
    std::string_view name;
    TypeIndex index;

    constexpr RegisteredType(std::string_view name, TypeIndex index) noexcept
        : name(name)
        , index(index)
    {}
};


struct TypeInfo final 
{
    template <typename Type>
    TypeInfo(std::in_place_type_t<Type>)
        : m_name(TypeName<std::remove_cv_t<std::remove_reference_t<Type>>>::value()) 
    {
        static auto index = lookupType(m_name)->index;
        m_index = index;
    }

    constexpr TypeIndex index() const noexcept 
    {
        return m_index;
    }
    
    constexpr std::string_view name() const noexcept 
    {
        return m_name;
    }

private:
    std::string_view m_name;
    TypeIndex m_index;
};


template <typename Type>
[[nodiscard]] const TypeInfo& typeId() noexcept 
{
    if constexpr (std::is_same_v<Type, std::remove_cv_t<std::remove_reference_t<Type>>>) 
    {
        static TypeInfo instance{ std::in_place_type<Type> };
        return instance;
    }
    else 
    {
        return typeId<std::remove_cv_t<std::remove_reference_t<Type>>>();
    }
}

template <typename Type>
[[nodiscard]] const TypeInfo& typeId(Type&&) noexcept 
{
    return typeId<std::remove_cv_t<std::remove_reference_t<Type>>>();
}


} // namespace Er {}