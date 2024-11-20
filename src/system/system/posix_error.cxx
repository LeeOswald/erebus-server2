#include <erebus/system/system/posix_error.hxx>

namespace Er::Util
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
    
    
} // namespace Er::Util {}
