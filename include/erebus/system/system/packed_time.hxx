#pragma once

#include <erebus/system/erebus.hxx>

#include <ctime>
#if ER_WINDOWS
    #include <erebus/system/system/unwindows.h>
#elif ER_POSIX
    #include <sys/time.h>
#endif


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

    [[nodiscard]] static ValueType now() noexcept
    {
#if ER_WINDOWS
        FILETIME ft;
        ::GetSystemTimePreciseAsFileTime(&ft);
        // shift is difference between 1970-Jan-01 & 1601-Jan-01
        // in 100-nanosecond units
        const std::uint64_t shift = 116444736000000000ULL; // (27111902 << 32) + 3577643008
        // 100-nanos since 1601-Jan-01
        std::uint64_t packed = (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32) | static_cast<std::uint64_t>(ft.dwLowDateTime);
        packed -= shift; // filetime is now 100-nanos since 1970-Jan-01
        return (packed / 10U); // truncate to microseconds
#elif ER_POSIX
        timeval tv;
        ::gettimeofday(&tv, nullptr);

        return  ValueType(tv.tv_sec) * 1000000UL + ValueType(tv.tv_usec);
#endif
    }

    [[nodiscard]] std::tm toLocalTime() const noexcept;
    [[nodiscard]] std::tm toUtc() const noexcept;

    [[nodiscard]] constexpr std::uint64_t microseconds() const noexcept
    {
        return value;
    }

    [[nodiscard]] constexpr std::uint64_t milliseconds() const noexcept
    {
        return value / 1000UL;
    }

    [[nodiscard]] constexpr std::uint64_t seconds() const noexcept
    {
        return value / 1000000UL;
    }

    [[nodiscard]] constexpr std::uint32_t subSecond() const noexcept
    {
        return static_cast<std::uint32_t>(value % 1000000UL);
    }
};

} // namespace Er::System {}
