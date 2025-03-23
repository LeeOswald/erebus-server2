#include <erebus/system/system/posix_error.hxx>

#include <cstring>

namespace Er::System
{
    
ER_SYSTEM_EXPORT std::string posixErrorToString(int e)
{
    constexpr size_t required = 256;
    char result[required];
    result[0] = 0;
    
#if ER_LINUX
    auto s = ::strerror_r(e, result, required); // 's' may be or not be the same as 'result'
    return std::string(s);
#elif ER_WINDOWS
    if (::strerror_s(result, e) == 0)
        return std::string(result);
#endif

    return std::string();
}

ER_SYSTEM_EXPORT std::optional<ResultCode> resultFromPosixError(int code) noexcept
{
    switch (code)
    {
    case 0: return Result::Ok;
    case ENOMEM: return Result::OutOfMemory;
    case EPERM: return Result::AccessDenied;
    case EACCES: return Result::AccessDenied;
    case EEXIST: return Result::AlreadyExists;
    case EINVAL: return Result::InvalidArgument;
    case ENOENT: return Result::NotFound;
    case ENOSPC: return Result::InsufficientResources;
    case ENOTDIR: return Result::NotFound;
    }

    return std::nullopt;
}

ER_SYSTEM_EXPORT Exception makePosixException(std::source_location location, std::string&& message, int code)
{
    Exception x(location, std::move(message));

    auto result = resultFromPosixError(code);
    if (result)
        x.add(Property(std::int32_t(*result), ExceptionProps::Result));
        
    x.add(Property(std::int32_t(code), ExceptionProps::PosixError));

    auto decoded = posixErrorToString(code);
    if (!decoded.empty())
        x.add(Property(std::move(decoded), ExceptionProps::DecodedError));

    return x;
}

} // namespace Er::System {}


namespace Er::ExceptionProps
{

const PropertyInfo PosixError{ PropertyType::UInt32, "Er.Exception.posix_error", "POSIX error" };

} // namespace Er::ExceptionProps {}
