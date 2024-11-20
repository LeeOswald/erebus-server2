#include <erebus/system/system/packed_time.hxx>

#if ER_WINDOWS
    #include <windows.h>
#elif ER_POSIX
    #include <sys/time.h>
#endif

namespace Er::System
{

PackedTime::ValueType PackedTime::now() noexcept
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

std::tm PackedTime::toLocalTime() const noexcept
{
    auto seconds = toPosixTime();
    std::tm now_tm = {};
#if ER_WINDOWS
    ::localtime_s(&now_tm, &seconds);
#elif ER_POSIX
    ::localtime_r(&seconds, &now_tm);
#endif
    return now_tm;
}

std::tm PackedTime::toUtc() const noexcept
{
    auto seconds = toPosixTime();
    std::tm now_tm = {};
#if ER_WINDOWS
    ::gmtime_s(&now_tm, &seconds);
#elif ER_POSIX
    ::gmtime_r(&seconds, &now_tm);
#endif
    return now_tm;
}

} // namespace Er::System {}