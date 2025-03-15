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


static const std::string test_property_info = R"(
function get_id(prop)
    local id = Er.Property.getId(prop)
    if id == Er.Unspecified.Empty.id() then
        return 0
    elseif id == Er.Unspecified.Bool.id() then
        return 1
    elseif id == Er.Unspecified.Int32.id() then
        return 2
    elseif id == Er.Unspecified.UInt32.id() then
        return 3
    elseif id == Er.Unspecified.Int64.id() then
        return 4
    elseif id == Er.Unspecified.UInt64.id() then
        return 5
    elseif id == Er.Unspecified.Double.id() then
        return 6
    elseif id == Er.Unspecified.String.id() then
        return 7
    elseif id == Er.Unspecified.Binary.id() then
        return 8
    end

    return -1
end
)";


TEST(Er_Lua, PropertyInfo)
{
    Er::LuaState state(Er::Log2::get());

    state.loadString(test_property_info, "test_property_info");

    {
        Er::Property v;
        int res = state["get_id"](v);
        EXPECT_EQ(res, 0);
    }

    {
        Er::Property v(Er::True, Er::Unspecified::Bool);
        int res = state["get_id"](v);
        EXPECT_EQ(res, 1);
    }

    {
        Er::Property v(std::int32_t(1), Er::Unspecified::Int32);
        int res = state["get_id"](v);
        EXPECT_EQ(res, 2);
    }

    {
        Er::Property v(std::uint32_t(1), Er::Unspecified::UInt32);
        int res = state["get_id"](v);
        EXPECT_EQ(res, 3);
    }

    {
        Er::Property v(std::int64_t(1), Er::Unspecified::Int64);
        int res = state["get_id"](v);
        EXPECT_EQ(res, 4);
    }

    {
        Er::Property v(std::uint64_t(1), Er::Unspecified::UInt64);
        int res = state["get_id"](v);
        EXPECT_EQ(res, 5);
    }

    {
        Er::Property v(1.0, Er::Unspecified::Double);
        int res = state["get_id"](v);
        EXPECT_EQ(res, 6);
    }

    {
        Er::Property v(std::string("1"), Er::Unspecified::String);
        int res = state["get_id"](v);
        EXPECT_EQ(res, 7);
    }

    {
        Er::Property v(Er::Binary(std::string("1")), Er::Unspecified::Binary);
        int res = state["get_id"](v);
        EXPECT_EQ(res, 8);
    }
}