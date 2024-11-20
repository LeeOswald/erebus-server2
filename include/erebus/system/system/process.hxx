#pragma once

#include <erebus/system/erebus.hxx>

#if ER_WINDOWS
    #include <windows.h>
#elif ER_POSIX
    #include <unistd.h>
#endif

namespace Er::System
{

using Pid = uintptr_t;


namespace CurrentProcess
{

ER_SYSTEM_EXPORT [[nodiscard]] Pid id() noexcept
{
#if ER_WINDOWS
    return static_cast<Pid>(::GetCurrentProcessId());
#elif ER_POSIX
    return static_cast<Pid>(::getpid());
#endif
}

ER_SYSTEM_EXPORT [[nodiscard]] std::string exe();

#if ER_POSIX
EREBUS_EXPORT void daemonize() noexcept;
#endif

} // namespace CurrentProcess {}

} // namespace Er::System {}