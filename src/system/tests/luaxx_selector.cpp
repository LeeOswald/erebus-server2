#include "common.hpp"

#include <erebus/system/luaxx/luaxx_state.hxx>

static const std::string test_script = R"(
function foo()
end

function add(a, b)
   return a + b
end

function sum_and_difference(a, b)
   return (a+b), (a-b)
end

function bar()
   return 4, true, "hi"
end

function execute()
   return cadd(5, 6);
end

function doozy(a)
   x, y = doozy_c(a, 2 * a)
   return x * y
end

mytable = {}
function mytable.foo()
   return 4
end

function embedded_nulls()
   return "\0h\0i"
end

my_global = 4

my_table = {}
my_table[3] = "hi"
my_table["key"] = 6.4

nested_table = {}
nested_table[2] = -3;
nested_table["foo"] = "bar";

my_table["nested"] = nested_table

global1 = 5
global2 = 5

function resumable()
   coroutine.yield(1)
   coroutine.yield(2)
   return 3
end

co = coroutine.create(resumable)

function resume_co()
   ran, value = coroutine.resume(co)
   return value
end

function set_global()
   global1 = 8
end

should_be_one = 0

function should_run_once()
   should_be_one = should_be_one + 1
end
)";

TEST(Lua, select_global) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    int answer = state["my_global"];
    EXPECT_EQ(answer, 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, select_field) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    lua_Number answer = state["my_table"]["key"];
    EXPECT_EQ(answer, lua_Number(6.4));
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, select_index) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    std::string answer = state["my_table"][3];
    EXPECT_STREQ(answer.c_str(), "hi");
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, select_nested_field) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    std::string answer = state["my_table"]["nested"]["foo"];
    EXPECT_STREQ(answer.c_str(), "bar");
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, select_nested_index) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    int answer = state["my_table"]["nested"][2];
    EXPECT_EQ(answer, -3);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, select_equality) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    EXPECT_EQ(state["my_table"]["nested"][2], -3);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, select_cast) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    EXPECT_EQ(int(state["global1"]), state["global2"]);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, set_global) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    auto lua_dummy_global = state["dummy_global"];
    lua_dummy_global = 32;
    EXPECT_EQ(state["dummy_global"], 32);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, set_field) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["my_table"]["dummy_key"] = "testing";
    std::string s = state["my_table"]["dummy_key"];
    EXPECT_STREQ(s.c_str(), "testing");
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, set_index) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["my_table"][10] = 3;
    EXPECT_EQ(state["my_table"][10], 3);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, set_nested_field) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["my_table"]["nested"]["asdf"] = true;
    EXPECT_TRUE((bool)state["my_table"]["nested"]["asdf"]);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, set_nested_index) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["my_table"]["nested"][1] = 2;
    EXPECT_EQ(state["my_table"]["nested"][1], 2);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, create_table_field) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["new_table"]["test"] = 4;
    EXPECT_EQ(state["new_table"]["test"], 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, create_table_index) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["new_table"][3] = 4;
    EXPECT_EQ(state["new_table"][3], 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, cache_selector_field_assignment) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    Er::Lua::Selector s = state["new_table"][3];
    s = 4;
    EXPECT_EQ(state["new_table"][3], 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, cache_selector_field_access) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["new_table"][3] = 4;
    Er::Lua::Selector s = state["new_table"][3];
    EXPECT_EQ(s, 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, cache_selector_function) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    Er::Lua::Selector s = state["set_global"];
    s();
    EXPECT_EQ(state["global1"], 8);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_should_run_once) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    auto should_run_once = state["should_run_once"];
    should_run_once();
    EXPECT_EQ(state["should_be_one"], 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_result_is_alive_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    Er::Lua::Pointer<InstanceCounter> pointer = state["createObj"]();
    state.forceGC();

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_result_is_alive_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    Er::Lua::Reference<InstanceCounter> reference = state["createObj"]();
    state.forceGC();

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, get_and_set_Reference_keeps_identity) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Obj"].SetClass<InstanceCounter>();
    state("objA = Obj.new()");

    Er::Lua::Reference<InstanceCounter> objA_ref = state["objA"];
    state["objB"] = objA_ref;
    Er::Lua::Reference<InstanceCounter> objB_ref = state["objB"];

    state("function areVerySame() return objA == objB end");
    EXPECT_TRUE((bool)state["areVerySame"]());
    EXPECT_EQ(&objA_ref.get(), &objB_ref.get());
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, get_and_set_Pointer_keeps_identity) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Obj"].SetClass<InstanceCounter>();
    state("objA = Obj.new()");

    Er::Lua::Pointer<InstanceCounter> objA_ptr = state["objA"];
    state["objB"] = objA_ptr;
    Er::Lua::Pointer<InstanceCounter> objB_ptr = state["objB"];

    state("function areVerySame() return objA == objB end");
    EXPECT_TRUE((bool)state["areVerySame"]());
    EXPECT_EQ(objA_ptr, objB_ptr);
    EXPECT_EQ(state.size(), 0);
}

struct SelectorBar 
{
};

struct SelectorFoo 
{
    int x;

    SelectorFoo(int num) 
        : x(num) 
    {}

    int getX() 
    {
        return x;
    }
};

TEST(Lua, selector_call_with_registered_class) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<SelectorFoo, int>("get", &SelectorFoo::getX);
    state("function getXFromFoo(foo) return foo:get() end");
    SelectorFoo foo{4};
    EXPECT_EQ(state["getXFromFoo"](foo), 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_call_with_registered_class_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<SelectorFoo, int>("get", &SelectorFoo::getX);
    state("function getXFromFoo(foo) return foo:get() end");
    SelectorFoo foo{4};
    EXPECT_EQ(state["getXFromFoo"](&foo), 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_call_with_wrong_type_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    auto acceptFoo = [] (SelectorFoo *) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state["acceptFoo"] = acceptFoo;
    state("bar = Bar.new()");

    bool error_encounted = false;
    state.setExceptionHandler([&error_encounted](int, std::string, std::exception_ptr) 
    {
        error_encounted = true;
    });
    state("acceptFoo(bar)");

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_call_with_wrong_type_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    auto acceptFoo = [] (SelectorFoo &) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state["acceptFoo"] = acceptFoo;
    state("bar = Bar.new()");

    bool error_encounted = false;
    state.setExceptionHandler([&error_encounted](int, std::string, std::exception_ptr) 
    {
        error_encounted = true;
    });
    state("acceptFoo(bar)");

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_call_with_nullptr_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    auto acceptFoo = [] (SelectorFoo &) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["acceptFoo"] = acceptFoo;

    bool error_encounted = false;
    state.setExceptionHandler([&error_encounted](int, std::string, std::exception_ptr) 
    {
        error_encounted = true;
    });
    state("acceptFoo(nil)");

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_get_nullptr_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = nil");
    bool error_encounted = false;

    try
    {
        SelectorFoo & foo = state["bar"];
    } 
    catch(Er::Lua::TypeError &) 
    {
        error_encounted = true;
    }

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_get_wrong_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state("bar = Bar.new()");
    bool error_encounted = false;

    try
    {
        SelectorFoo & foo = state["bar"];
    } 
    catch(Er::Lua::TypeError &) 
    {
        error_encounted = true;
    }

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_get_wrong_ref_to_string) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = \"Not a Foo\"");
    bool expected_message = false;

    try
    {
        SelectorFoo & foo = state["bar"];
    } 
    catch(Er::Lua::TypeError & e) 
    {
        expected_message = std::string(e.what()).find("got string") != std::string::npos;
    }

    EXPECT_TRUE(expected_message);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_get_wrong_ref_to_table) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = {}");
    bool expected_message = false;

    try
    {
        SelectorFoo & foo = state["bar"];
    } 
    catch(Er::Lua::TypeError & e) 
    {
        expected_message = std::string(e.what()).find("got table") != std::string::npos;
    }

    EXPECT_TRUE(expected_message);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_get_wrong_ref_to_unregistered) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state("foo = Foo.new(4)");
    bool expected_message = false;

    try
    {
        SelectorBar& bar = state["foo"];
    } 
    catch(Er::Lua::TypeError & e) 
    {
        expected_message = std::string(e.what()).find("unregistered type expected") != std::string::npos;
    }

    EXPECT_TRUE(expected_message);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, selector_get_wrong_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state("bar = Bar.new()");
    SelectorFoo * foo = state["bar"];
    EXPECT_FALSE(!!foo);
    EXPECT_EQ(state.size(), 0);
}
