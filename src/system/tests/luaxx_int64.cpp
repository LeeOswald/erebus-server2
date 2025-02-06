#include "common.hpp"

#include <erebus/system/luaxx.hxx>
#include <erebus/system/luaxx/luaxx_int64.hxx>

static const std::string test_int64 = R"(
function add(a, b)
    return a + b
end
function sub(a, b)
    return a - b
end
function mul(a, b)
    return a * b
end
function div(a, b)
    return a / b
end
function mod(a, b)
    return a % b
end
function neg(a)
    return -a
end
function band(a, b)
    return a & b
end
function bor(a, b)
    return a | b
end
function bxor(a, b)
    return a ~ b
end
function bnot(a)
    return ~a
end
function shl(a, n)
    return a << n
end
function shr(a, n)
    return a >> n
end
function eq(a, b)
    return a == b
end
function neq(a, b)
    return a ~= b
end
function lt(a, b)
    return a < b
end
function gt(a, b)
    return a > b
end
function le(a, b)
    return a <= b
end
function str(a)
    return tostring(a)
end
)";


TEST(Er_Lua, Int64)
{
    Er::LuaState state(Er::Log2::get());
    
    state.loadString(test_int64, "test_int64");

    // +
    {
        Er::Lua::Int64Wrapper a(0x4000000050000001LL);
        Er::Lua::Int64Wrapper b(0x0000000000000002LL);

        Er::Lua::Int64Wrapper res = state["add"](a, b);
        
        EXPECT_EQ(res.value, 0x4000000050000003LL);
    }

    // -
    {
        Er::Lua::Int64Wrapper a(0x4000000050000003LL);
        Er::Lua::Int64Wrapper b(0x0000000000000002LL);

        Er::Lua::Int64Wrapper res = state["sub"](a, b);

        EXPECT_EQ(res.value, 0x4000000050000001LL);
    }

    // *
    {
        Er::Lua::Int64Wrapper a(3);
        Er::Lua::Int64Wrapper b(-2);

        Er::Lua::Int64Wrapper res = state["mul"](a, b);

        EXPECT_EQ(res.value, -6);
    }

    // /
    {
        Er::Lua::Int64Wrapper a(6);
        Er::Lua::Int64Wrapper b(-2);

        Er::Lua::Int64Wrapper res = state["div"](a, b);

        EXPECT_EQ(res.value, -3);
    }

    // %
    {
        Er::Lua::Int64Wrapper a(7);
        Er::Lua::Int64Wrapper b(2);

        Er::Lua::Int64Wrapper res = state["mod"](a, b);

        EXPECT_EQ(res.value, 1);
    }

    // -x
    {
        Er::Lua::Int64Wrapper a(7);

        Er::Lua::Int64Wrapper res = state["neg"](a);

        EXPECT_EQ(res.value, -7);
    }

    // &
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        Er::Lua::Int64Wrapper res = state["band"](a, b);

        EXPECT_EQ(res.value, 0x0000000100100000LL);
    }

    // |
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        Er::Lua::Int64Wrapper res = state["bor"](a, b);

        EXPECT_EQ(res.value, 0x8000000100100001LL);
    }

    // ^
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        Er::Lua::Int64Wrapper res = state["bxor"](a, b);

        EXPECT_EQ(res.value, 0x8000000000000001LL);
    }

    // ~x
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);

        Er::Lua::Int64Wrapper res = state["bnot"](a);

        EXPECT_EQ(res.value, 0x7FFFFFFEFFEFFFFELL);
    }

    // <<
    {
        Er::Lua::Int64Wrapper a(0x0000000100100001LL);

        Er::Lua::Int64Wrapper res = state["shl"](a, 2);

        EXPECT_EQ(res.value, 0x400400004LL);
    }

    // >>
    {
        Er::Lua::Int64Wrapper a(0x0000000100100001LL);

        Er::Lua::Int64Wrapper res = state["shr"](a, 2);

        EXPECT_EQ(res.value, 0x40040000LL);
    }

    // ==
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        bool eq = state["eq"](a, b);
        EXPECT_FALSE(eq);

        eq = state["eq"](a, a);
        EXPECT_TRUE(eq);
    }

    // !=
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        bool neq = state["neq"](a, b);
        EXPECT_TRUE(neq);

        neq = state["neq"](a, a);;
        EXPECT_FALSE(neq);
    }

    // <
    {
        Er::Lua::Int64Wrapper a(8);
        Er::Lua::Int64Wrapper b(-10);

        bool lt = state["lt"](a, b);
        EXPECT_FALSE(lt);

        lt = state["lt"](b, a);;
        EXPECT_TRUE(lt);
    }

    // >
    {
        Er::Lua::Int64Wrapper a(8);
        Er::Lua::Int64Wrapper b(-10);

        bool gt = state["gt"](a, b);
        EXPECT_TRUE(gt);

        gt = state["gt"](b, a);
        EXPECT_FALSE(gt);
    }

    // <
    {
        Er::Lua::Int64Wrapper a(8);
        Er::Lua::Int64Wrapper b(-10);

        bool le = state["le"](a, b);
        EXPECT_FALSE(le);

        le = state["le"](b, a);;
        EXPECT_TRUE(le);

        le = state["le"](a, a);;
        EXPECT_TRUE(le);
    }

    // tostring
    {
        Er::Lua::Int64Wrapper a(-800);
        Er::Lua::Int64Wrapper b(801);
        
        std::string as = state["str"](a);
        EXPECT_STREQ(as.c_str(), "-800");

        std::string bs = state["str"](b);
        EXPECT_STREQ(bs.c_str(), "801");
    }
}

TEST(Er_Lua, UInt64)
{
    Er::LuaState state(Er::Log2::get());

    state.loadString(test_int64, "test_int64");

    // +
    {
        Er::Lua::UInt64Wrapper a(0x4000000050000001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000000000002ULL);

        Er::Lua::UInt64Wrapper res = state["add"](a, b);

        EXPECT_EQ(res.value, 0x4000000050000003ULL);
    }

    // -
    {
        Er::Lua::UInt64Wrapper a(0x4000000050000003ULL);
        Er::Lua::UInt64Wrapper b(0x0000000000000002ULL);

        Er::Lua::UInt64Wrapper res = state["sub"](a, b);

        EXPECT_EQ(res.value, 0x4000000050000001ULL);
    }

    // *
    {
        Er::Lua::UInt64Wrapper a(3);
        Er::Lua::UInt64Wrapper b(2);

        Er::Lua::UInt64Wrapper res = state["mul"](a, b);

        EXPECT_EQ(res.value, 6);
    }

    // /
    {
        Er::Lua::UInt64Wrapper a(6);
        Er::Lua::UInt64Wrapper b(2);

        Er::Lua::UInt64Wrapper res = state["div"](a, b);

        EXPECT_EQ(res.value, 3);
    }

    // %
    {
        Er::Lua::UInt64Wrapper a(7);
        Er::Lua::UInt64Wrapper b(2);

        Er::Lua::UInt64Wrapper res = state["mod"](a, b);

        EXPECT_EQ(res.value, 1);
    }

    // -x
    {
        Er::Lua::UInt64Wrapper a(7);

        Er::Lua::UInt64Wrapper res = state["neg"](a);

        EXPECT_EQ(res.value, uint64_t(-7));
    }

    // &
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        Er::Lua::UInt64Wrapper res = state["band"](a, b);

        EXPECT_EQ(res.value, 0x0000000100100000ULL);
    }

    // |
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        Er::Lua::UInt64Wrapper res = state["bor"](a, b);

        EXPECT_EQ(res.value, 0x8000000100100001ULL);
    }

    // ^
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        Er::Lua::UInt64Wrapper res = state["bxor"](a, b);

        EXPECT_EQ(res.value, 0x8000000000000001ULL);
    }

    // ~x
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);

        Er::Lua::UInt64Wrapper res = state["bnot"](a);

        EXPECT_EQ(res.value, 0x7FFFFFFEFFEFFFFEULL);
    }

    // <<
    {
        Er::Lua::UInt64Wrapper a(0x0000000100100001ULL);

        Er::Lua::UInt64Wrapper res = state["shl"](a, 2);

        EXPECT_EQ(res.value, 0x400400004ULL);
    }

    // >>
    {
        Er::Lua::UInt64Wrapper a(0x0000000100100001ULL);

        Er::Lua::UInt64Wrapper res = state["shr"](a, 2);

        EXPECT_EQ(res.value, 0x40040000ULL);
    }

    // ==
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        bool eq = state["eq"](a, b);
        EXPECT_FALSE(eq);

        eq = state["eq"](a, a);
        EXPECT_TRUE(eq);
    }

    // !=
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        bool neq = state["neq"](a, b);
        EXPECT_TRUE(neq);

        neq = state["neq"](a, a);
        EXPECT_FALSE(neq);
    }

    // <
    {
        Er::Lua::UInt64Wrapper a(8);
        Er::Lua::UInt64Wrapper b(5);

        bool lt = state["lt"](a, b);
        EXPECT_FALSE(lt);

        lt = state["lt"](b, a);
        EXPECT_TRUE(lt);
    }

    // >
    {
        Er::Lua::UInt64Wrapper a(8);
        Er::Lua::UInt64Wrapper b(2);

        bool gt = state["gt"](a, b);
        EXPECT_TRUE(gt);

        gt = state["gt"](b, a);
        EXPECT_FALSE(gt);
    }

    // <
    {
        Er::Lua::UInt64Wrapper a(8);
        Er::Lua::UInt64Wrapper b(4);

        bool le = state["le"](a, b);
        EXPECT_FALSE(le);

        le = state["le"](b, a);
        EXPECT_TRUE(le);

        le = state["le"](a, a);
        EXPECT_TRUE(le);
    }

    // tostring
    {
        Er::Lua::UInt64Wrapper a(1650);
        
        std::string as = state["str"](a);
        EXPECT_STREQ(as.c_str(), "1650");
    }
}