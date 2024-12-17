#pragma once

#include <erebus/system/erebus.hxx>

#if ER_WINDOWS
    #include <erebus/system/system/unwindows.h>
#elif ER_POSIX
    #include <pthread.h>
#endif


namespace Er::System
{

using Tid = uintptr_t;


namespace CurrentThread
{

[[nodiscard]] inline Tid id() noexcept
{
#if ER_POSIX
    return ::gettid();
#elif ER_WINDOWS
    return ::GetCurrentThreadId();
#endif
}

ER_SYSTEM_EXPORT void setName(const char* name) noexcept;


} // namespace CurrentThread {}

} // namespace Er::System {}