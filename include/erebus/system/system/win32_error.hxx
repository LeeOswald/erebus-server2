#pragma once

#include <erebus/system/result.hxx>

#include <erebus/system/system/unwindows.h>

namespace Er::System
{

struct ER_SYSTEM_EXPORT Win32ErrorProvider
{
    using ErrorCode = DWORD;

    [[nodiscard]] std::string win32ErrorToString(DWORD e, HMODULE module = 0);
};


} // namespace Er::System {}

namespace Er
{

template <>
class ErrorProvider<System::Win32ErrorProvider>
{
    using Provider = System::Win32ErrorProvider;
    using ErrorCode = typename Provider::ErrorCode;

    template <typename... Args>
    [[nodiscard]] auto format(Args... args)
    {
        return Provider::win32ErrorToString(std::forward<Args>(args)...);
    }
};


} // namespace Er {}