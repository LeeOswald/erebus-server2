#include "common.hpp"

#include <erebus/system/luaxx/luaxx_state.hxx>

struct Qux 
{
    int baz() { return 4; }
    int qux = 3;
};

static Qux qux;

Qux* GetQuxPtr() { return &qux; }
Qux& GetQuxRef() { return qux; }

static const std::string test_metatable_script = R"(
function call_method()
   instance = get_instance()
   return instance:baz()
end

function access_member()
   instance = get_instance()
   return instance:qux()
end
)";

TEST(Lua, metatable_registry_ptr) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["get_instance"] = &GetQuxPtr;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz);
    state.loadString(test_metatable_script);
    EXPECT_EQ(state["call_method"](), 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, metatable_registry_ref) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["get_instance"] = &GetQuxRef;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz);
    state.loadString(test_metatable_script);
    EXPECT_EQ(state["call_method"](), 4);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, metatable_ptr_member) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["get_instance"] = &GetQuxPtr;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz, "qux", &Qux::qux);
    state.loadString(test_metatable_script);
    EXPECT_EQ(state["access_member"](), 3);
    EXPECT_EQ(state.size(), 0);
}

TEST(Lua, metatable_ref_member) 
{
    Er::Lua::State state(Er::Log2::get(), true);
    state["get_instance"] = &GetQuxRef;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz, "qux", &Qux::qux);
    state.loadString(test_metatable_script);
    EXPECT_EQ(state["access_member"](), 3);
    EXPECT_EQ(state.size(), 0);
}
