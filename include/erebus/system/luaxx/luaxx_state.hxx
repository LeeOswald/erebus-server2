#pragma once

#include <erebus/system/logger2.hxx>

#include <erebus/system/luaxx/luaxx_exception_handler.hxx>
#include <erebus/system/luaxx/luaxx_registry.hxx>
#include <erebus/system/luaxx/luaxx_selector.hxx>

#include <boost/noncopyable.hpp>


namespace Er::Lua 
{

class ER_SYSTEM_EXPORT State 
    : public boost::noncopyable
{
public:
    ~State();

    explicit State(Er::Log2::ILogger* log, bool openLibs);
    explicit State(Er::Log2::ILogger* log, lua_State* l);
    
    
    Selector operator[](const char* name) const;
    bool operator()(const char* code);

    void forceGC()
    {
        lua_gc(m_l, LUA_GCCOLLECT, 0);
    }

    uint64_t memUsed()
    {
        uint32_t kbytes = lua_gc(m_l, LUA_GCCOUNT, 0);
        uint32_t bytes = lua_gc(m_l, LUA_GCCOUNTB, 0);
        return uint64_t(kbytes) * 1024 + bytes;
    }

    int size() const
    {
        return lua_gettop(m_l);
    }

    void openLib(const std::string& name, lua_CFunction openf);
    bool loadString(std::string_view str, const char* name = nullptr);
    bool load(const std::string& fileName);

    void setExceptionHandler(ExceptionHandler::function handler)
    {
        *m_exceptionHandler = ExceptionHandler(std::move(handler));
    }

protected:
    void exceptionHandler(int luaStatusCode, std::string msg, std::exception_ptr exception);

    Er::Log2::ILogger* m_log;
    lua_State* m_l;
    bool m_owner;
    std::unique_ptr<Er::Lua::Registry> m_registry;
    std::unique_ptr<Er::Lua::ExceptionHandler> m_exceptionHandler;
};

} // namespace Er::Lua {}
