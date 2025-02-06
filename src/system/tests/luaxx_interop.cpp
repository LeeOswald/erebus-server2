#include "common.hpp"

#include <erebus/system/luaxx/luaxx_state.hxx>
#include <erebus/system/luaxx/luaxx_tuple.hxx>

#include <memory>
#include <string>


int my_add(int a, int b) 
{
    return a + b;
}

void no_return() 
{
}

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

TEST(Lua, function_no_args) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["foo"]();
    EXPECT_TRUE(true);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, add) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    int v = state["add"](5, 2);
    EXPECT_EQ(v, 7);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, multi_return) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    int sum, difference;
    Er::Lua::tie(sum, difference) = state["sum_and_difference"](3, 1);
    EXPECT_EQ(sum, 4);
    EXPECT_EQ(difference, 2);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, multi_return_invoked_once) {
    Er::Lua::State state(Er::Log2::get(), true);
    int invocation_count = 0;
    state["two_ints"] = [&invocation_count]()
    {
        ++invocation_count;
        return std::tuple<int, int>{1, 2};
    };
    int res_a = 0, res_b = 0;
    Er::Lua::tie(res_a, res_b) = state["two_ints"]();
    EXPECT_EQ(invocation_count, 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, heterogeneous_return) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    int x;
    bool y;
    std::string z;
    Er::Lua::tie(x, y, z) = state["bar"]();
    EXPECT_EQ(x, 4);
    EXPECT_TRUE(y);
    EXPECT_STREQ(z.c_str(), "hi");
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_field) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    int answer = state["mytable"]["foo"]();
    EXPECT_EQ(answer, 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_c_function) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["cadd"] = std::function<int(int, int)>(my_add);
    int answer = state["cadd"](3, 6);
    EXPECT_EQ(answer, 9);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_c_fun_from_lua) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["cadd"] = std::function<int(int, int)>(my_add);
    int answer = state["execute"]();
    EXPECT_EQ(answer, 11);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, no_return) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["no_return"] = &no_return;
    state["no_return"]();
    EXPECT_TRUE(true);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_std_fun) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    std::function<int(int, int)> mult = [](int x, int y){ return x * y; };
    state["cmultiply"] = mult;
    int answer = state["cmultiply"](5, 6);
    EXPECT_EQ(answer, 30);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_lambda) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["cmultiply"] = [](int x, int y){ return x * y; };
    int answer = state["cmultiply"](5, 6);
    EXPECT_EQ(answer, 30);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_normal_c_fun) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["cadd"] = &my_add;
    const int answer = state["cadd"](4, 20);
    EXPECT_EQ(answer, 24);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_normal_c_fun_many_times) 
{
    // Ensures there isn't any strange overflow problem or lingering
    // state
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["cadd"] = &my_add;
    bool result = true;
    for (int i = 0; i < 25; ++i) 
    {
        const int answer = state["cadd"](4, 20);
        result = result && (answer == 24);
    }
    EXPECT_TRUE(result);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_functor) 
{
    struct the_answer 
    {
        int answer = 42;
        int operator()() 
        {
            return answer;
        }
    };
    the_answer functor;
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["c_the_answer"] = std::function<int()>(functor);
    int answer = state["c_the_answer"]();
    EXPECT_EQ(answer, 42);
    EXPECT_EQ(state.size(), 0);

}

std::tuple<int, int> my_sum_and_difference(int x, int y) 
{
    return std::make_tuple(x+y, x-y);
}

TEST(Lua, multivalue_c_fun_return) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["test_fun"] = &my_sum_and_difference;
    int sum, difference;
    Er::Lua::tie(sum, difference) = state["test_fun"](-2, 2);
    EXPECT_EQ(sum, 0);
    EXPECT_EQ(difference, -4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, multivalue_c_fun_from_lua) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    state["doozy_c"] = &my_sum_and_difference;
    int answer = state["doozy"](5);
    EXPECT_EQ(answer, -75);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, embedded_nulls) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    const std::string result = state["embedded_nulls"]();
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, coroutine) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    bool check1 = state["resume_co"]() == 1;
    bool check2 = state["resume_co"]() == 2;
    bool check3 = state["resume_co"]() == 3;
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);
    EXPECT_TRUE(check3);
    EXPECT_EQ(state.size(), 0);
}

struct Special 
{
    int foo = 3;
};

static Special special;

Special* return_special_pointer() { return &special; }

TEST(Lua, pointer_return) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["return_special_pointer"] = &return_special_pointer;
    Special* p = state["return_special_pointer"]();
    EXPECT_EQ(p, &special);
    EXPECT_EQ(state.size(), 0);
}

Special& return_special_reference() { return special; }

TEST(Lua, reference_return) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["return_special_reference"] = &return_special_reference;
    Special &ref = state["return_special_reference"]();
    EXPECT_EQ(&ref, &special);
    EXPECT_EQ(state.size(), 0);
}

InstanceCounter return_value() { return {}; }

TEST(Lua, return_value) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["MyClass"].SetClass<InstanceCounter>();
    state["return_value"] = &return_value;
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    state("globalValue = return_value()");

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, return_unregistered_type) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["return_value"] = &return_value;
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    bool error_encounted = false;
    state.setExceptionHandler([&error_encounted](int, std::string msg, std::exception_ptr) 
    {
        error_encounted = true;
    });

    state("globalValue = return_value()");

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, value_parameter) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["MyClass"].SetClass<InstanceCounter>();
    state("function acceptValue(value) valCopy = value end");
    int const instanceCountBefore = InstanceCounter::instances;

    state["acceptValue"](InstanceCounter{});

    EXPECT_EQ(InstanceCounter::instances, instanceCountBefore + 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, wrong_value_parameter) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["MyClass"].SetClass<InstanceCounter>();
    state("function acceptValue(value) valCopy = value end");
    int const instanceCountBefore = InstanceCounter::instances;

    bool expected = false;
    try 
    {
        state["acceptValue"](Special{});
    } 
    catch(Er::Lua::CopyUnregisteredType& e)
    {
        expected = true;
        EXPECT_EQ(e.type().get(), typeid(Special));
    }

    EXPECT_TRUE(expected);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, value_parameter_keeps_type_info) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["MyClass"].SetClass<Special>();
    state("function acceptValue(value) valCopy = value end");
    state["acceptValue"](Special{});

    Special* foo = state["valCopy"];

    EXPECT_TRUE(!!foo);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, callback_with_value) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["MyClass"].SetClass<InstanceCounter>();
    state("val = MyClass.new()");

    std::unique_ptr<InstanceCounter> copy;
    state["accept"] = [&copy](InstanceCounter counter) 
    {
        copy.reset(new InstanceCounter(std::move(counter)));
    };

    int const instanceCountBeforeCall = InstanceCounter::instances;
    state("accept(val)");

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCall + 1);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, nullptr_to_nil) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["getNullptr"] = []() -> void* 
    {
        return nullptr;
    };
    state("x = getNullptr()");
    state("result = x == nil");
    EXPECT_TRUE(static_cast<bool>(state["result"]));
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, get_primitive_by_value) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    EXPECT_EQ(static_cast<int>(state["global1"]), 5);
}

TEST(Lua, get_primitive_by_const_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    EXPECT_EQ(static_cast<const int &>(state["global1"]), 5);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, get_primitive_by_rvalue_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state.loadString(test_script);
    EXPECT_EQ(static_cast<int &&>(state["global1"]), 5);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_with_primitive_by_value) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    bool success = false;
    auto const accept_int_by_value = [&success](int x) {success = x == 5;};
    state["test"] = accept_int_by_value;
    state["test"](5);
    EXPECT_TRUE(success);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_with_primitive_by_const_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    bool success = false;
    auto const accept_int_by_const_ref =
        [&success](const int & x) {success = x == 5;};
    state["test"] = accept_int_by_const_ref;
    state["test"](5);
    EXPECT_TRUE(success);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, call_with_primitive_by_rvalue_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    bool success = false;
    auto const accept_int_by_rvalue_ref =
        [&success](int && x) {success = x == 5;};
    state["test"] = accept_int_by_rvalue_ref;
    state["test"](5);
    EXPECT_TRUE(success);
    EXPECT_EQ(state.size(), 0);
}
