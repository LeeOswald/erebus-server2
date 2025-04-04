#pragma once

#include <erebus/system/erebus.hxx>

namespace Er::Ipc
{

using ClientId = std::uint32_t;

constexpr ClientId InvalidClientId = ClientId(-1);

} // namespace Er::Ipc {}