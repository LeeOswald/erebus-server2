#include <erebus/system/system/thread.hxx>


namespace Er::System::CurrentThread
{

#if ER_WINDOWS

#pragma pack(push,8)
typedef struct
{
    DWORD dwType;      // Must be 0x1000.
    LPCSTR szName;     // Pointer to name (in user addr space).
    DWORD dwThreadID;  // Thread ID (-1=caller thread).
    DWORD dwFlags;     // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

const DWORD VCThreadNameMagic = 0x406D1388;

ER_SYSTEM_EXPORT void setName(const char* name) noexcept
{
    if (::IsDebuggerPresent())
    {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = DWORD(-1);
        info.dwFlags = 0;

        __try
        {
            ::RaiseException(VCThreadNameMagic, 0, sizeof(info) / sizeof(DWORD), reinterpret_cast<DWORD_PTR*>(&info));
        }
        __except (EXCEPTION_CONTINUE_EXECUTION)
        {
        }
    }
}

#elif ER_LINUX

ER_SYSTEM_EXPORT void setName(const char* name) noexcept
{
    auto self = ::pthread_self();
    ::pthread_setname_np(self, name);
}

#endif


} // namespace Er::System::CurrentThread {}