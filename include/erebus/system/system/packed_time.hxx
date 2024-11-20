#pragma once

#include <erebus/system/erebus.hxx>

#include <ctime>


namespace Er::System
{

//
// microseconds since 1970-Jan-01
//

struct ER_SYSTEM_EXPORT PackedTime
{
    using ValueType = std::uint64_t;
    
    ValueType value;

    constexpr PackedTime(ValueType value  = {}) noexcept
        : value(value)
    {
    }

    [[nodiscard]] constexpr std::time_t toPosixTime() const noexcept
    {
        return value / 1000000UL;
    }

    static [[nodiscard]] ValueType now() noexcept;

    [[nodiscard]] std::tm toLocalTime() const noexcept;
    [[nodiscard]] std::tm toUtc() const noexcept;

    constexpr [[nodiscard]] std::uint32_t subSecond() const noexcept
    {
        return static_cast<std::uint32_t>(value % 1000000UL);
    }
};

} // namespace Er::System {}
