#pragma once

#include <erebus/system/erebus.hxx>

namespace Er::Util
{

ER_SYSTEM_EXPORT [[nodiscard]] std::string utf16To8bit(int cp, const wchar_t* s, std::optional<size_t> length = std::nullopt);
ER_SYSTEM_EXPORT [[nodiscard]] std::wstring utf8ToUtf16(const std::string& s);
ER_SYSTEM_EXPORT [[nodiscard]] std::wstring localToUtf16(const std::string& s);


    
} // namespace Er::Util {}