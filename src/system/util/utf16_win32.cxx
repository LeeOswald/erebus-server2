#include <erebus/system/util/utf16.hxx>

#include <windows.h>

namespace Er::Util
{
    
ER_SYSTEM_EXPORT std::string utf16To8bit(int cp, const wchar_t* s, std::optional<size_t> length)
{
    std::string out;

    if (!s)
        return out;

    if (!length)
        length = ::wcslen(s);

    if (!*length)
        return out;

    auto required = ::WideCharToMultiByte(cp, 0, s, static_cast<int>(*length), nullptr, 0, nullptr, nullptr);
    if (required > 0)
    {
        out.resize(required);
        ::WideCharToMultiByte(cp, 0, s, static_cast<int>(*length), out.data(), static_cast<int>(out.size()), nullptr, nullptr);
    }

    return out;
}

ER_SYSTEM_EXPORT std::wstring utf8ToUtf16(const std::string& s)
{
    std::wstring out;

    auto required = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.length()), nullptr, 0);
    if (required > 0)
    {
        out.resize(required);
        ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.length()), out.data(), static_cast<int>(out.size()));
    }

    return out;
}

ER_SYSTEM_EXPORT std::wstring localToUtf16(const std::string& s)
{
    std::wstring out;

    auto required = ::MultiByteToWideChar(CP_THREAD_ACP, 0, s.c_str(), static_cast<int>(s.length()), nullptr, 0);
    if (required > 0)
    {
        out.resize(required);
        ::MultiByteToWideChar(CP_THREAD_ACP, 0, s.c_str(), static_cast<int>(s.length()), out.data(), static_cast<int>(out.size()));
    }

    return out;
}
   
    
} // namespace Er::Util {}