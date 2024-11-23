#pragma once

#include <erebus/system/result.hxx>

namespace Er::System
{
    
struct ER_SYSTEM_EXPORT PosixErrorProvider
{
    using ErrorCode = int;

    std::string posixErrorToString(int e);
};

} // namespace Er::System {}

namespace Er
{

template <>
class ErrorProvider<System::PosixErrorProvider>
{
    using Provider = System::PosixErrorProvider;
    using ErrorCode = typename Provider::ErrorCode;

    template <typename... Args>
    [[nodiscard]] auto format(Args... args)
    {
        return Provider::posixErrorToString(std::forward<Args>(args)...);
    }
};


} // namespace Er {}