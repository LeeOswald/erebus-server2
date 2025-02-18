#include "common.hpp"

#include <erebus/system/logger2.hxx>
#include <erebus/system/luaxx/luaxx_state.hxx>

#include <string>
#include <sstream>


auto getCapture()
{
    auto tee = static_cast<Er::Log2::ITee*>(Er::Log2::get()->findSink("tee").get());
    auto capturePtr = static_cast<CapturedStderr*>(tee->findSink("capture").get());
    ErAssert(capturePtr);
    return capturePtr;
}


TEST(Lua, load_error) 
{
    auto capturePtr = getCapture();

    Er::Lua::State state(Er::Log2::get(), true);

    const char* expected = "cannot open";
    EXPECT_FALSE(state.load("../test/non_exist.lua"));
    Er::Log2::get()->flush();
    EXPECT_NE(capturePtr->content().find(expected), std::string::npos);

    EXPECT_EQ(state.size(), 0);
}


static const std::string test_syntax_errror_script = R"(
function syntax_error()
    1 2 3 4
end
)";

TEST(Lua, load_syntax_error) 
{
    auto capturePtr = getCapture();

    Er::Lua::State state(Er::Log2::get(), true);

    const char* expected = "unexpected symbol";

    EXPECT_FALSE(state.loadString(test_syntax_errror_script));
    Er::Log2::get()->flush();
    EXPECT_NE(capturePtr->content().find(expected), std::string::npos);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, do_syntax_error) 
{
    auto capturePtr = getCapture();

    Er::Lua::State state(Er::Log2::get(), true);

    const char* expected = "unexpected symbol";

    bool b = state("function syntax_error() 1 2 3 4 end");
    EXPECT_FALSE(b);
    Er::Log2::get()->flush();
    EXPECT_NE(capturePtr->content().find(expected), std::string::npos);

    EXPECT_EQ(state.size(), 0);
}


static const std::string test_test_errror_script = R"(
function err_func1(x, y)
	err_func2(x + y)
end

function divide_by_zero()
	return 1 / 0
end

function _overflow(n)
	return _overflow(n + 1) + 1
end

function do_overflow()
	_overflow(1)
end

)";

TEST(Lua, call_undefined_function) 
{
    auto capturePtr = getCapture();

    Er::Lua::State state(Er::Log2::get(), true);

    state.loadString(test_test_errror_script);
    const char* expected = "attempt to call a nil value";

    state["undefined_function"]();
    Er::Log2::get()->flush();
    EXPECT_NE(capturePtr->content().find(expected), std::string::npos);
    
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_undefined_function2) 
{
    auto capturePtr = getCapture();

    Er::Lua::State state(Er::Log2::get(), true);

    state.loadString(test_test_errror_script);
#if LUA_VERSION_NUM < 503
    const char* expected = "attempt to call global 'err_func2'";
#else
    const char* expected = "attempt to call a nil value (global 'err_func2')";
#endif

    state["err_func1"](1, 2);

    EXPECT_NE(capturePtr->content().find(expected), std::string::npos);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_stackoverflow) 
{
    auto capturePtr = getCapture();

    Er::Lua::State state(Er::Log2::get(), true);

    state.loadString(test_test_errror_script);
    const char* expected = "stack overflow";

    state["do_overflow"]();

    EXPECT_NE(capturePtr->content().find(expected), std::string::npos);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, parameter_conversion_error) 
{
    auto capturePtr = getCapture();

    Er::Lua::State state(Er::Log2::get(), true);

    const char * expected =
        "bad argument #2 to 'accept_string_int_string' (number expected, got string)";
    std::string largeStringToPreventSSO(50, 'x');
    state["accept_string_int_string"] = [](std::string, int, std::string){};


    state["accept_string_int_string"](
        largeStringToPreventSSO,
        "not a number",
        largeStringToPreventSSO);

    EXPECT_NE(capturePtr->content().find(expected), std::string::npos);

    EXPECT_EQ(state.size(), 0);
}
