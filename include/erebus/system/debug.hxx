#pragma once

#if ER_LINUX
    #if defined(__GNUC__) && (defined(__i386) || defined(__x86_64))
        #define _ER_TRAP() asm volatile ("int $3") /* NOLINT */
    #else
        #define _ER_TRAP() ::raise(SIGTRAP)
    #endif
#elif defined(_MSC_VER)
    #define _ER_TRAP() __debugbreak()
#elif defined(__MINGW32__)
    #define _ER_TRAP() ::DebugBreak()
#else
    #define _ER_TRAP() ((void)0)
#endif

#ifndef EREBUS_HXX_INCLUDED
    #include <erebus/system/erebus.hxx>
#endif

namespace Er
{

ER_SYSTEM_EXPORT bool isDebuggerPresent() noexcept;

} // namespace Er {}


