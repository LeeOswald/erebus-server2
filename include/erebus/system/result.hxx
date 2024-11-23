#pragma once

#include <erebus/system/erebus.hxx>

namespace Er
{
    
template <typename Provider>
class ErrorProvider;


enum class Result : std::int32_t
{
    Ok = 0,
    Failure = -1,
    OutOfMemory = -2,
    Internal = -3,
};



struct ER_SYSTEM_EXPORT ResultProvider
{
    using ErrorCode = Result;

    [[nodiscard]] std::string resultToString(Result r);
};

template <>
class ErrorProvider<ResultProvider>
{
    using Provider = ResultProvider;
    using ErrorCode = typename Provider::ErrorCode;

    template <typename... Args>
    [[nodiscard]] auto format(Args... args)
    {
        return Provider::resultToString(std::forward<Args>(args)...);
    }
};


} // namespace Er {}