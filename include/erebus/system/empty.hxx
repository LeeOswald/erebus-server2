#pragma once

#include <erebus/system/erebus.hxx>


namespace Er
{

/**
 * @brief Just an empty class or placeholder
 *  
 */
 
struct Empty
{
    constexpr Empty() noexcept = default;

    friend constexpr bool operator==(const Empty& a, const Empty& b) noexcept
    {
        return true;
    }

    friend constexpr auto operator<=>(const Empty& a, const Empty& b) noexcept
    {
        return std::strong_ordering::equal;
    }
};


} // namespace Er {}