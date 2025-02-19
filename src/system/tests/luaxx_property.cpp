#include "common.hpp"

#include <erebus/system/luaxx.hxx>
#include <erebus/system/luaxx/luaxx_tuple.hxx>

TEST(Er_Lua, PropertyTypes)
{
    Er::LuaState state(Er::Log2::get());

    uint32_t type = state["Er"]["PropertyType"]["Empty"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Empty));

    type = state["Er"]["PropertyType"]["Bool"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Bool));

    type = state["Er"]["PropertyType"]["Int32"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Int32));

    type = state["Er"]["PropertyType"]["UInt32"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::UInt32));

    type = state["Er"]["PropertyType"]["Int64"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Int64));

    type = state["Er"]["PropertyType"]["UInt64"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::UInt64));

    type = state["Er"]["PropertyType"]["Double"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Double));

    type = state["Er"]["PropertyType"]["String"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::String));

    type = state["Er"]["PropertyType"]["Binary"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Binary));
}



