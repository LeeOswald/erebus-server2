#include <erebus/system/luaxx/luaxx_int64.hxx>
#include <erebus/system/luaxx/luaxx_state.hxx>

namespace Er::Lua
{

ER_SYSTEM_EXPORT void registerInt64(State& state)
{
    Selector s = state["Er"]["Int64"];
    s.SetClass<Int64Wrapper, int32_t, uint32_t>();

    s["__add"] = &Int64Wrapper::__add;
    s["__sub"] = &Int64Wrapper::__sub;
    s["__mul"] = &Int64Wrapper::__mul;
    s["__div"] = &Int64Wrapper::__div;
    s["__mod"] = &Int64Wrapper::__mod;
    s["__unm"] = &Int64Wrapper::__unm;
    s["__band"] = &Int64Wrapper::__band;
    s["__bor"] = &Int64Wrapper::__bor;
    s["__bxor"] = &Int64Wrapper::__bxor;
    s["__bnot"] = &Int64Wrapper::__bnot;
    s["__shl"] = &Int64Wrapper::__shl;
    s["__shr"] = &Int64Wrapper::__shr;
    s["__eq"] = &Int64Wrapper::__eq;
    s["__lt"] = &Int64Wrapper::__lt;
    s["__gt"] = &Int64Wrapper::__gt;
    s["__le"] = &Int64Wrapper::__le;
    s["__tostring"] = &Int64Wrapper::__tostring;
}

ER_SYSTEM_EXPORT void registerUInt64(State& state)
{
    Selector s = state["Er"]["UInt64"];
    s.SetClass<UInt64Wrapper, uint32_t, uint32_t>();

    s["__add"] = &UInt64Wrapper::__add;
    s["__sub"] = &UInt64Wrapper::__sub;
    s["__mul"] = &UInt64Wrapper::__mul;
    s["__div"] = &UInt64Wrapper::__div;
    s["__mod"] = &UInt64Wrapper::__mod;
    s["__unm"] = &UInt64Wrapper::__unm;
    s["__band"] = &UInt64Wrapper::__band;
    s["__bor"] = &UInt64Wrapper::__bor;
    s["__bxor"] = &UInt64Wrapper::__bxor;
    s["__bnot"] = &UInt64Wrapper::__bnot;
    s["__shl"] = &UInt64Wrapper::__shl;
    s["__shr"] = &UInt64Wrapper::__shr;
    s["__eq"] = &UInt64Wrapper::__eq;
    s["__lt"] = &UInt64Wrapper::__lt;
    s["__gt"] = &UInt64Wrapper::__gt;
    s["__le"] = &UInt64Wrapper::__le;
    s["__tostring"] = &UInt64Wrapper::__tostring;
}



} // namespace Er::Lua {}