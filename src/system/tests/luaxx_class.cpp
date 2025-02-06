#include "common.hpp"

#include <erebus/system/luaxx/luaxx_state.hxx>

struct Bar 
{
    int x;

    Bar(int num) 
    { 
        x = num; 
    }

    std::string Print(int y) 
    {
        return std::to_string(x) + "+" + std::to_string(y);
    }

    void SetX(int x2) 
    {
        x = x2;
    }

    int GetX() 
    {
        return x;
    }
};

struct Zoo 
{
    int x;

    Zoo(Bar *bar) 
    {
        x = bar->x;
    }

    int GetX()
    {
        return x;
    }

    void ChangeBar(Bar& bar)
    {
        bar.x = x * 2;
    }
};

struct BarHolder 
{
    Bar bar;

    BarHolder(int num) 
        : bar(num) 
    {}

    Bar& getRef() 
    {
        return bar;
    }

    Bar* getPtr() 
    {
        return &bar;
    }

    Bar getValue() 
    {
        return bar;
    }
};

struct ZooAcceptor 
{
    ZooAcceptor(Zoo*) 
    {}

    void acceptZoo(Zoo*) 
    {}
};

static int gc_counter;

struct GCTest 
{
    GCTest() 
    {
        ++gc_counter;
    }

    GCTest(const GCTest& other) 
    {
        ++gc_counter;
    }

    ~GCTest() 
    {
        --gc_counter;
    }
};

std::string ShowBarRef(Bar& bar) 
{
    return std::to_string(bar.x);
}

std::string ShowBarPtr(Bar* bar) 
{
    return std::to_string(bar->x);
}

static const std::string test_class_script = R"(
bar = Bar.new(8)
barx = bar:get_x()
barp = bar : print(2)
)";

TEST(Lua, register_class)
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>("print", &Bar::Print, "get_x", &Bar::GetX);
    state.loadString(test_class_script);
    int result1 = state["barx"];
    std::string result2 = state["barp"];
    EXPECT_EQ(result1, 8);
    EXPECT_STREQ(result2.c_str(), "8+2");

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, get_member_variable)
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>("x", &Bar::x);
    state("bar = Bar.new(-2)");
    state("barx = bar:x()");
    state("tmp = bar.x ~= nil");
    EXPECT_EQ(state["barx"], -2);
    bool b = state["tmp"];
    EXPECT_TRUE(b);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, set_member_variable) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>("x", &Bar::x);
    state("bar = Bar.new(-2)");
    state("bar:set_x(-4)");
    state("barx = bar:x()");
    EXPECT_EQ(state["barx"], -4);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, class_field_set) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>("set", &Bar::SetX, "get", &Bar::GetX);
    state("bar = Bar.new(4)");
    state("x = bar:get()");
    const bool check1 = state["x"] == 4;
    state("bar:set(6)");
    state("x = bar:get()");
    const bool check2 = state["x"] == 6;
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);

    EXPECT_EQ(state.size(), 0);
}

static const std::string test_gc_script = R"(
objs = {}
function make_ten()
   for i = 1, 10 do
      objs[i] = GCTest.new()
   end
end

function destroy_ten()
   for i = 1, 10 do
      objs[i] = nil
   end
end
)";

TEST(Lua, class_gc) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    gc_counter = 0;
    state["GCTest"].SetClass<GCTest>();
    state.loadString(test_gc_script);
    state["make_ten"]();
    const bool check1 = gc_counter == 10;
    state["destroy_ten"]();
    state.forceGC();
    const bool check2 = gc_counter == 0;
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, ctor_wrong_type) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>();
    state["Zoo"].SetClass<Zoo, Bar*>();
    state["ZooAcceptor"].SetClass<ZooAcceptor, Zoo*>();
    state("bar = Bar.new(4)");

    bool error_encounted = false;
    state.setExceptionHandler([&error_encounted](int, std::string, std::exception_ptr) {
        error_encounted = true;
    });

    state("zooAcceptor = ZooAcceptor.new(bar)");
    EXPECT_TRUE(error_encounted);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, pass_wrong_type) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>();
    state["Zoo"].SetClass<Zoo, Bar*>();
    state["ZooAcceptor"].SetClass<ZooAcceptor, Zoo*>("acceptZoo", &ZooAcceptor::acceptZoo);
    state("bar = Bar.new(4)");
    state("zoo = Zoo.new(bar)");
    state("zooAcceptor = ZooAcceptor.new(zoo)");

    bool error_encounted = false;
    state.setExceptionHandler([&error_encounted](int, std::string, std::exception_ptr) {
        error_encounted = true;
    });

    state("zooAcceptor:acceptZoo(bar)");
    EXPECT_TRUE(error_encounted);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, pass_pointer) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>();
    state["Zoo"].SetClass<Zoo, Bar*>("get", &Zoo::GetX);
    state("bar = Bar.new(4)");
    state("zoo = Zoo.new(bar)");
    state("zoox = zoo:get()");
    EXPECT_EQ(state["zoox"], 4);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, pass_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>("get", &Bar::GetX);
    state["Zoo"].SetClass<Zoo, Bar*>("change_bar", &Zoo::ChangeBar);
    state("bar = Bar.new(4)");
    state("zoo = Zoo.new(bar)");
    state("zoo:change_bar(bar)");
    state("barx = bar:get()");
    EXPECT_EQ(state["barx"], 8);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, return_pointer) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>("get", &Bar::GetX);
    state["BarHolder"].SetClass<BarHolder, int>("get", &BarHolder::getPtr);
    state("bh = BarHolder.new(4)");
    state("bar = bh:get()");
    state("barx = bar:get()");
    EXPECT_EQ(state["barx"], 4);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, return_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>("get", &Bar::GetX);
    state["BarHolder"].SetClass<BarHolder, int>("get", &BarHolder::getRef);
    state("bh = BarHolder.new(4)");
    state("bar = bh:get()");
    state("barx = bar:get()");
    EXPECT_EQ(state["barx"], 4);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, return_val) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>("get", &Bar::GetX);
    state["BarHolder"].SetClass<BarHolder, int>("get", &BarHolder::getValue);
    state("bh = BarHolder.new(4)");
    state("bar = bh:get()");
    state("barx = bar:get()");
    EXPECT_EQ(state["barx"], 4);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, freestanding_fun_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>();
    state("bar = Bar.new(4)");
    state["print_bar"] = &ShowBarRef;
    state("barstring = print_bar(bar)");
    std::string s = state["barstring"];
    EXPECT_STREQ(s.c_str(), "4");

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, freestanding_fun_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["Bar"].SetClass<Bar, int>();
    state("bar = Bar.new(4)");
    state["print_bar"] = &ShowBarPtr;
    state("barstring = print_bar(bar)");
    std::string s = state["barstring"];
    EXPECT_STREQ(s.c_str(), "4");

    EXPECT_EQ(state.size(), 0);
}

struct ConstMemberTest 
{
    const bool foo = true;

    bool get_bool() const 
    {
        return true;
    }
};

TEST(Lua, const_member_function) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["ConstMemberTest"].SetClass<ConstMemberTest>(
        "get_bool", &ConstMemberTest::get_bool);
    state("tmp = ConstMemberTest.new()");
    bool b = state["tmp"];
    EXPECT_TRUE(b);

    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, const_member_variable) 
{
    Er::Lua::State state(Er::Log2::get(), true);

    state["ConstMemberTest"].SetClass<ConstMemberTest>(
        "foo", &ConstMemberTest::foo);
    state("tmp1 = ConstMemberTest.new().foo ~= nil");
    state("tmp2 = ConstMemberTest.new().set_foo == nil");
    bool b1 = state["tmp1"];
    EXPECT_TRUE(b1);
    bool b2 = state["tmp2"];
    EXPECT_TRUE(b2);

    EXPECT_EQ(state.size(), 0);
}
