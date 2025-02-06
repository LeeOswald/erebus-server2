#include "common.hpp"

#include <erebus/system/luaxx/luaxx_state.hxx>

#include <iostream>

int take_fun_arg(Er::Lua::function<int(int, int)> fun, int a, int b) 
{
    return fun(a, b);
}

struct Mutator 
{
    Mutator() 
    {}

    Mutator(Er::Lua::function<void(int)> fun) 
    {
        fun(-4);
    }

    Er::Lua::function<void()> Foobar(bool which,
                                 Er::Lua::function<void()> foo,
                                 Er::Lua::function<void()> bar) 
    {
        return which ? foo : bar;
    }
};

static const std::string test_ref_script = R"(
function add(a, b)
   return a+b
end

function subtract(a, b)
   return a-b
end

function pass_add(x, y)
   return take_fun_arg(add, x, y)
end

function pass_sub(x, y)
   return take_fun_arg(subtract, x, y)
end

a = 4

function mutate_a(new_a)
   a = new_a
end

test = ""

function foo()
   test = "foo"
end

function bar()
   test = "bar"
end

function return_two()
   return 1, 2
end

)";

TEST(Lua, function_reference) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["take_fun_arg"] = &take_fun_arg;
    state.loadString(test_ref_script);
    bool check1 = state["pass_add"](3, 5) == 8;
    bool check2 = state["pass_sub"](4, 2) == 2;
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_in_constructor) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Mutator"].SetClass<Mutator, Er::Lua::function<void(int)>>();
    state.loadString(test_ref_script);
    bool check1 = state["a"] == 4;
    state("mutator = Mutator.new(mutate_a)");
    bool check2 = state["a"] == -4;
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, pass_function_to_lua) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Mutator"].SetClass<Mutator>("foobar", &Mutator::Foobar);
    state.loadString(test_ref_script);
    state("mutator = Mutator.new()");
    state("mutator:foobar(true, foo, bar)()");
    bool check1 = state["test"] == "foo";
    state("mutator:foobar(false, foo, bar)()");
    bool check2 = state["test"] == "bar";
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_returned_lua_function) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_ref_script);
    Er::Lua::function<int(int, int)> lua_add = state["add"];
    EXPECT_EQ(lua_add(2, 4), 6);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_multivalue_lua_function) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_ref_script);
    Er::Lua::function<std::tuple<int, int>()> lua_add = state["return_two"];
    EXPECT_EQ(lua_add(), std::make_tuple(1, 2));
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_result_is_alive_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    Er::Lua::function<Er::Lua::Pointer<InstanceCounter>()> createObj = state["createObj"];
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    Er::Lua::Pointer<InstanceCounter> pointer = createObj();
    state.forceGC();

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_result_is_alive_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    Er::Lua::function<Er::Lua::Reference<InstanceCounter>()> createObj = state["createObj"];
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    Er::Lua::Reference<InstanceCounter> ref = createObj();
    state.forceGC();

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.size(), 0);
}

struct FunctionFoo 
{
    int x;

    FunctionFoo(int num) 
        : x(num) 
    {}
    
    int getX() 
    {
        return x;
    }
};

struct FunctionBar 
{
};

TEST(Lua, function_call_with_registered_class) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function getX(foo) return foo:get() end");
    Er::Lua::function<int(FunctionFoo &)> getX = state["getX"];
    FunctionFoo foo{4};
    EXPECT_EQ(getX(foo), 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_call_with_registered_class_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function getX(foo) return foo:get() end");
    Er::Lua::function<int(FunctionFoo *)> getX = state["getX"];
    FunctionFoo foo{4};
    EXPECT_EQ(getX(&foo), 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_call_with_registered_class_val) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function store(foo) globalFoo = foo end");
    state("function getX() return globalFoo:get() end");

    Er::Lua::function<void(FunctionFoo)> store = state["store"];
    Er::Lua::function<int()> getX = state["getX"];
    store(FunctionFoo{4});

    EXPECT_EQ(getX(), 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_call_with_registered_class_val_lifetime) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<InstanceCounter>();
    state("function store(foo) globalFoo = foo end");
    Er::Lua::function<void(InstanceCounter)> store = state["store"];

    int instanceCountBefore = InstanceCounter::instances;
    store(InstanceCounter{});

    EXPECT_EQ(InstanceCounter::instances, instanceCountBefore + 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_call_with_nullptr_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<FunctionFoo, int>();
    state("function makeNil() return nil end");
    Er::Lua::function<FunctionFoo &()> getFoo = state["makeNil"];
    bool error_encounted = false;

    try 
    {
        FunctionFoo & foo = getFoo();
    } 
    catch(Er::Lua::TypeError &) 
    {
        error_encounted = true;
    }

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_call_with_wrong_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<FunctionFoo, int>();
    state["Bar"].SetClass<FunctionBar>();
    state("function makeBar() return Bar.new() end");
    Er::Lua::function<FunctionFoo &()> getFoo = state["makeBar"];
    bool error_encounted = false;

    try 
    {
        FunctionFoo & foo = getFoo();
    } 
    catch(Er::Lua::TypeError &) 
    {
        error_encounted = true;
    }

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_call_with_wrong_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<FunctionFoo, int>();
    state["Bar"].SetClass<FunctionBar>();
    state("function makeBar() return Bar.new() end");
    Er::Lua::function<FunctionFoo *()> getFoo = state["makeBar"];
    EXPECT_FALSE(!!getFoo());
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, function_get_registered_class_by_value) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<FunctionFoo, int>();
    state("function getFoo() return Foo.new(4) end");
    Er::Lua::function<FunctionFoo()> getFoo = state["getFoo"];

    FunctionFoo foo = getFoo();

    EXPECT_EQ(foo.getX(), 4);
    EXPECT_EQ(state.size(), 0);
}
