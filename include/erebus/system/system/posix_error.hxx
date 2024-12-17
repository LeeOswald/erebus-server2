#pragma once

#include <erebus/system/result.hxx>
#include <erebus/system/exception.hxx>
#include <erebus/system/property_info.hxx>

namespace Er::System
{
    
ER_SYSTEM_EXPORT [[nodiscard]] std::string posixErrorToString(int code);

ER_SYSTEM_EXPORT [[nodiscard]] std::optional<ResultCode> resultFromPosixError(int code) noexcept;

ER_SYSTEM_EXPORT [[nodiscard]] Er::Exception makePosixException(std::source_location location, std::string&& message, int code);

} // namespace Er::System {}


namespace Er::ExceptionProps
{

extern ER_SYSTEM_EXPORT const Er::PropertyInfo PosixError;

} // namespace Er::ExceptionProps {}


#define ErThrowPosixError(message, code) throw Er::System::makePosixException(std::source_location::current(), message, code)