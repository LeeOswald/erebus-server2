#pragma once

#include <erebus/system/erebus.hxx>

#include <windows.h>

namespace Er::System
{
    
ER_SYSTEM_EXPORT [[nodiscard]] std::string win32ErrorToString(DWORD e, HMODULE module = 0);
    
    
} // namespace Er::System {}