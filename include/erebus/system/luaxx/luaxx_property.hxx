#pragma once

#include <erebus/system/erebus.hxx>


namespace Er::Lua
{

class State;

ER_SYSTEM_EXPORT void registerPropertyTypes(State& state);


} // namespace Er::Lua {}