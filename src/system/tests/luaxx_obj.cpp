#include "common.hpp"

#include <erebus/system/luaxx/luaxx_state.hxx>

#include <vector>

struct Foo 
{
    int x;
    const int y;

    Foo(int x_) 
        : x(x_)
        , y(3) 
    {}

    int GetX() 
    { 
        return x; 
    }
    
    int DoubleAdd(int y) 
    {
        return 2 * (x + y);
    }

    void SetX(int x_) 
    {
        x = x_;
    }
};

TEST(Lua, register_obj) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance, "double_add", &Foo::DoubleAdd);
    const int answer = state["foo_instance"]["double_add"](3);
    EXPECT_EQ(answer, 8);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, register_obj_member_variable) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance, "x", &Foo::x);
    state["foo_instance"]["set_x"](3);
    const int answer = state["foo_instance"]["x"]();
    EXPECT_EQ(answer, 3);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, register_obj_to_table)
{
    Er::Lua::State state(Er::Log2::get(), true);
    Foo foo1(1);
    Foo foo2(2);
    Foo foo3(3);
    auto foos = state["foos"];
    foos[1].SetObj(foo1, "get_x", &Foo::GetX);
    foos[2].SetObj(foo2, "get_x", &Foo::GetX);
    foos[3].SetObj(foo3, "get_x", &Foo::GetX);
    const int answer = int(foos[1]["get_x"]()) +
        int(foos[2]["get_x"]()) +
        int(foos[3]["get_x"]());
    EXPECT_EQ(answer, 6);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, mutate_instance) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance, "set_x", &Foo::SetX);
    state["foo_instance"]["set_x"](4);
    EXPECT_EQ(foo_instance.x, 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, multiple_methods) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance,
                                 "double_add", &Foo::DoubleAdd,
                                 "set_x", &Foo::SetX);
    state["foo_instance"]["set_x"](4);
    const int answer = state["foo_instance"]["double_add"](3);
    EXPECT_EQ(answer, 14);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, register_obj_const_member_variable) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance, "y", &Foo::y);
    const int answer = state["foo_instance"]["y"]();
    state("tmp = foo_instance.set_y == nil");
    EXPECT_EQ(answer, 3);
    bool b = state["tmp"];
    EXPECT_TRUE(b);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, bind_vector_push_back) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    std::vector<int> test_vector;
    state["vec"].SetObj(test_vector, "push_back",
                        static_cast<void(std::vector<int>::*)(int&&)>(&std::vector<int>::push_back));
    state["vec"]["push_back"](4);
    EXPECT_EQ(test_vector[0], 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, bind_vector_push_back_string) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    std::vector<std::string> test_vector;
    state["vec"].SetObj(test_vector, "push_back",
                        static_cast<void(std::vector<std::string>::*)(std::string&&)>(&std::vector<std::string>::push_back));
    state["vec"]["push_back"]("hi");
    std::string s = test_vector[0];
    EXPECT_STREQ(s.c_str(), "hi");
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, bind_vector_push_back_foos) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    std::vector<Foo> test_vector;
    state["Foo"].SetClass<Foo, int>();
    state["vec"].SetObj(test_vector, "push_back",
                        static_cast<void(std::vector<Foo>::*)(Foo&&)>(&std::vector<Foo>::push_back));
    state["vec"]["push_back"](Foo{1});
    state["vec"]["push_back"](Foo{2});
    EXPECT_EQ(test_vector.size(), 2);
    EXPECT_EQ(state.size(), 0);
}

struct FooHolder 
{
    Foo foo;
    
    FooHolder(int num) 
        : foo(num) 
    {}

    Foo& getRef() 
    {
        return foo;
    }

    Foo* getPtr() 
    {
        return &foo;
    }

    Foo getValue() 
    {
        return foo;
    }

    void acceptFoo(Foo *) 
    {};
};

struct ObjBar
{
};

TEST(Lua, obj_member_return_pointer) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<Foo, int>("get", &Foo::GetX);
    FooHolder fh{4};
    state["fh"].SetObj(fh, "get", &FooHolder::getPtr);
    state("foo = fh:get()");
    state("foox = foo:get()");
    EXPECT_EQ(state["foox"], 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, obj_member_return_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<Foo, int>("get", &Foo::GetX);
    FooHolder fh{4};
    state["fh"].SetObj(fh, "get", &FooHolder::getRef);
    state("foo = fh:get()");
    state("foox = foo:get()");
    EXPECT_EQ(state["foox"], 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, obj_member_return_val) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<Foo, int>("get", &Foo::GetX);
    FooHolder fh{4};
    state["fh"].SetObj(fh, "get", &FooHolder::getValue);
    state("foo = fh:get()");
    state("foox = foo:get()");
    EXPECT_EQ(state["foox"], 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, obj_member_wrong_type) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["Foo"].SetClass<Foo, int>();
    state["Bar"].SetClass<ObjBar>();
    FooHolder fh{5};
    state["fh"].SetObj(fh, "acceptFoo", &FooHolder::acceptFoo);
    state("bar = Bar.new()");

    bool error_encounted = false;
    state.setExceptionHandler([&error_encounted](int, std::string, std::exception_ptr) 
    {
        error_encounted = true;
    });

    state("fh.acceptFoo(bar)");
    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.size(), 0);
}
