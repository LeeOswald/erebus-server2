#include <erebus/system/util/autoptr.hxx>
#include <erebus/system/system/win32_error.hxx>
#include <erebus/system/util/utf16.hxx>



namespace Er::System
{
    
std::string Win32ErrorProvider::win32ErrorToString(DWORD r, HMODULE module)
{
    if (r == 0)
        return std::string();

    Util::AutoPtr<wchar_t, decltype([](void* ptr) { ::HeapFree(::GetProcessHeap(), 0, ptr); })> buffer;
    auto cch = ::FormatMessageW(
        (module ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM) | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        module,
        r,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        reinterpret_cast<wchar_t*>(buffer.writeable()),
        0,
        nullptr
    );

    std::wstring s;

    if (cch > 0)
    {
        s = std::wstring(buffer.get(), cch);

        // Windows appends \r\n to error messages for some reason
        while (s.size() && (s[s.size() - 1] == L'\n' || s[s.size() - 1] == L'\r'))
        {
            s.erase(s.size() - 1);
        }
    }

    return Util::utf16To8bit(CP_UTF8, s.data(), s.length());
}

    
    
} // namespace Er::System {}
