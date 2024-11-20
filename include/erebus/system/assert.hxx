#pragma once

#include <erebus/system/debug.hxx>

#include <cstdlib>
#include <source_location>


#if !ER_ENABLE_ASSERT
    #if ER_DEBUG
        #define ER_ENABLE_ASSERT 1
    #endif
#endif


namespace Er
{

using PrintFailedAssertionFn = void(*)(std::string_view message);


ER_SYSTEM_EXPORT void setPrintFailedAssertionFn(PrintFailedAssertionFn f) noexcept;
ER_SYSTEM_EXPORT void printFailedAssertion(std::source_location location, const char* expression) noexcept;

} // namespace Er {}


#if ER_ENABLE_ASSERT

#define ErAssert(expr) \
    do \
    { \
        if (!bool(expr)) \
        { \
            ::Er::printFailedAssertion(std::source_location::current(), #expr); \
            if (::Er::isDebuggerPresent()) \
                _ER_TRAP(); \
            else \
                std::abort(); \
        } \
    } while (false)


#else

#define ErAssert(expr) \
    static_cast<void>(0)

#endif

