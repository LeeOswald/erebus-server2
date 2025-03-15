#include "common.hpp"

#include <erebus/system/luaxx.hxx>
#include <erebus/system/luaxx/luaxx_tuple.hxx>
#include <erebus/system/luaxx/luaxx_int64.hxx>

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


static const std::string test_get_property_id = R"(
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


TEST(Er_Lua, PropertyGetId)
{
    Er::LuaState state(Er::Log2::get());

    state.loadString(test_get_property_id, "test_get_property_id");

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

static const std::string test_get_property_type = R"(
function get_type(prop)
    local id = Er.Property.getType(prop)
    return id
end
)";

TEST(Er_Lua, PropertyGetType)
{
    Er::LuaState state(Er::Log2::get());

    state.loadString(test_get_property_type, "test_get_property_type");

    {
        Er::Property v;
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::Empty));
    }

    {
        Er::Property v(Er::True, Er::Unspecified::Bool);
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::Bool));
    }

    {
        Er::Property v(std::int32_t(1), Er::Unspecified::Int32);
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::Int32));
    }

    {
        Er::Property v(std::uint32_t(1), Er::Unspecified::UInt32);
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::UInt32));
    }

    {
        Er::Property v(std::int64_t(1), Er::Unspecified::Int64);
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::Int64));
    }

    {
        Er::Property v(std::uint64_t(1), Er::Unspecified::UInt64);
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::UInt64));
    }

    {
        Er::Property v(1.0, Er::Unspecified::Double);
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::Double));
    }

    {
        Er::Property v(std::string("1"), Er::Unspecified::String);
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::String));
    }

    {
        Er::Property v(Er::Binary(std::string("1")), Er::Unspecified::Binary);
        int res = state["get_type"](v);
        EXPECT_EQ(res, static_cast<int>(Er::PropertyType::Binary));
    }
}

static const std::string test_get_property_name = R"(
function get_name(prop)
    local name = Er.Property.getName(prop)
    return name
end
)";

TEST(Er_Lua, PropertyGetName)
{
    Er::LuaState state(Er::Log2::get());

    state.loadString(test_get_property_name, "test_get_property_name");

    {
        Er::Property v;
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.Empty");
    }

    {
        Er::Property v(Er::True, Er::Unspecified::Bool);
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.Bool");
    }

    {
        Er::Property v(std::int32_t(1), Er::Unspecified::Int32);
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.Int32");
    }

    {
        Er::Property v(std::uint32_t(1), Er::Unspecified::UInt32);
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.UInt32");
    }

    {
        Er::Property v(std::int64_t(1), Er::Unspecified::Int64);
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.Int64");
    }

    {
        Er::Property v(std::uint64_t(1), Er::Unspecified::UInt64);
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.UInt64");
    }

    {
        Er::Property v(1.0, Er::Unspecified::Double);
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.Double");
    }

    {
        Er::Property v(std::string("1"), Er::Unspecified::String);
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.String");
    }

    {
        Er::Property v(Er::Binary(std::string("1")), Er::Unspecified::Binary);
        std::string res = state["get_name"](v);
        EXPECT_STREQ(res.c_str(), "Er.Unspecified.Binary");
    }
}

static const std::string test_set_property = R"(
function set_bool(prop, v)
    local old = Er.Property.getBool(prop)
    Er.Property.setBool(prop, v)
    return old
end
function set_int32(prop, v)
    local old = Er.Property.getInt32(prop)
    Er.Property.setInt32(prop, v)
    return old
end
function set_uint32(prop, v)
    local old = Er.Property.getUInt32(prop)
    Er.Property.setUInt32(prop, v)
    return old
end
function set_int64(prop, v)
    local old = Er.Property.getInt64(prop)
    Er.Property.setInt64(prop, v)
    return old
end
function set_uint64(prop, v)
    local old = Er.Property.getUInt64(prop)
    Er.Property.setUInt64(prop, v)
    return old
end
function set_double(prop, v)
    local old = Er.Property.getDouble(prop)
    Er.Property.setDouble(prop, v)
    return old
end
function set_string(prop, v)
    local old = Er.Property.getString(prop)
    Er.Property.setString(prop, v)
    return old
end
function set_binary(prop, v)
    local old = Er.Property.getBinary(prop)
    Er.Property.setBinary(prop, v)
    return old
end
)";


TEST(Er_Lua, PropertySet)
{
    Er::LuaState state(Er::Log2::get());

    state.loadString(test_set_property, "test_set_property");

    {
        Er::Property prop(Er::True, Er::Unspecified::Bool);
        bool old = state["set_bool"](prop, false);
        EXPECT_EQ(old, true);
        old = state["set_bool"](prop, true);
        EXPECT_EQ(old, false);
    }

    {
        Er::Property prop(std::int32_t(1), Er::Unspecified::Int32);
        int32_t old = state["set_int32"](prop, -2);
        EXPECT_EQ(old, 1);
        old = state["set_int32"](prop, 3);
        EXPECT_EQ(old, -2);
    }

    {
        Er::Property prop(std::uint32_t(1), Er::Unspecified::UInt32);
        uint32_t old = state["set_uint32"](prop, 2);
        EXPECT_EQ(old, 1);
        old = state["set_uint32"](prop, 3);
        EXPECT_EQ(old, 2);
    }

    {
        Er::Property prop(std::int64_t(-9223372036854775807LL), Er::Unspecified::Int64);
        Er::Lua::Int64Wrapper old = state["set_int64"](prop, Er::Lua::Int64Wrapper(1152921504606846977LL));
        EXPECT_EQ(old.value, -9223372036854775807LL);
        old = state["set_int64"](prop, Er::Lua::Int64Wrapper(1));
        EXPECT_EQ(old.value, 1152921504606846977LL);
    }

    {
        Er::Property prop(std::uint64_t(0x8000000000000005ULL), Er::Unspecified::UInt64);
        Er::Lua::UInt64Wrapper old = state["set_uint64"](prop, Er::Lua::UInt64Wrapper(0x0000000000000006ULL));
        EXPECT_EQ(old.value, 0x8000000000000005ULL);
        old = state["set_uint64"](prop, Er::Lua::UInt64Wrapper(1));
        EXPECT_EQ(old.value, 0x0000000000000006ULL);
    }

    {
        Er::Property prop(double(-1.9), Er::Unspecified::Double);
        double old = state["set_double"](prop, 2.0);
        EXPECT_EQ(old, -1.9);
        old = state["set_double"](prop, 3.0);
        EXPECT_EQ(old, 2.0);
    }
    
    {
        Er::Property prop(std::string("aaa"), Er::Unspecified::String);
        std::string old = state["set_string"](prop, std::string("bbb"));
        EXPECT_STREQ(old.c_str(), "aaa");
        std::string old2 = state["set_string"](prop, std::string("ccc"));
        EXPECT_STREQ(old2.c_str(), "bbb");
    }

    {
        Er::Property prop(Er::Binary(std::string("aaa")), Er::Unspecified::Binary);
        std::string old = state["set_binary"](prop, std::string("bbb"));
        EXPECT_STREQ(old.c_str(), "aaa");
        std::string old2 = state["set_binary"](prop, std::string("ccc"));
        EXPECT_STREQ(old2.c_str(), "bbb");
    }
}