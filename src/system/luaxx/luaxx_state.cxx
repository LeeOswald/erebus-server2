#include <erebus/system/exception.hxx>
#include <erebus/system/luaxx/luaxx_state.hxx>

namespace Er::Lua
{

State::~State()
{
    if (m_l && m_owner)
    {
        lua_gc(m_l, LUA_GCCOLLECT, 0);
        lua_close(m_l);
    }
}

State::State(Er::Log2::ILogger* log, bool openLibs)
    : m_log(log)
    , m_l(luaL_newstate())
    , m_owner(true)
{
    if (!m_l)
        throw Er::Exception(std::source_location::current(), "Failed to allocate a Lua state");

    if (openLibs)
        luaL_openlibs(m_l);

    m_registry.reset(new Er::Lua::Registry(m_l));
    m_exceptionHandler.reset(new Er::Lua::ExceptionHandler(
        [this](int luaStatusCode, std::string msg, std::exception_ptr exception)
        {
            exceptionHandler(luaStatusCode, msg, exception);
        }
    ));
}

State::State(Er::Log2::ILogger* log, lua_State* l)
    : m_log(log)
    , m_l(l)
    , m_owner(false)
{
    m_registry.reset(new Er::Lua::Registry(m_l));
    m_exceptionHandler.reset(new Er::Lua::ExceptionHandler(
        [this](int luaStatusCode, std::string msg, std::exception_ptr exception)
        {
            exceptionHandler(luaStatusCode, msg, exception);
        }
    ));
}

bool State::load(const std::string& fileName)
{
    ResetStackOnScopeExit savedStack(m_l);
    int status = luaL_loadfile(m_l, fileName.c_str());
#if LUA_VERSION_NUM >= 502
    auto const lua_ok = LUA_OK;
#else
    auto const lua_ok = 0;
#endif
    if (status != lua_ok)
    {
        if (status == LUA_ERRSYNTAX)
        {
            const char* msg = lua_tostring(m_l, -1);
            m_exceptionHandler->Handle(status, msg ? msg : std::string("Failed to load ").append(fileName).append(": syntax error"));
        }
        else if (status == LUA_ERRFILE)
        {
            const char* msg = lua_tostring(m_l, -1);
            m_exceptionHandler->Handle(status, msg ? msg : std::string("Failed to load ").append(fileName).append(": file error"));
        }

        return false;
    }

    status = lua_pcall(m_l, 0, LUA_MULTRET, 0);
    if (status == lua_ok)
    {
        return true;
    }

    const char* msg = lua_tostring(m_l, -1);
    m_exceptionHandler->Handle(status, msg ? msg : std::string("Failed to parse ").append(fileName));
    return false;
}

bool State::loadString(std::string_view str, const char* name)
{
    Er::Lua::ResetStackOnScopeExit savedStack(m_l);
    int status = luaL_loadbuffer(m_l, str.data(), str.length(), name);
#if LUA_VERSION_NUM >= 502
    auto const lua_ok = LUA_OK;
#else
    auto const lua_ok = 0;
#endif
    if (status != lua_ok)
    {
        if (status == LUA_ERRSYNTAX)
        {
            const char* msg = lua_tostring(m_l, -1);
            m_exceptionHandler->Handle(status, msg ? msg : "Failed to load script");
        }

        return false;
    }

    status = lua_pcall(m_l, 0, LUA_MULTRET, 0);
    if (status == lua_ok)
    {
        return true;
    }

    const char* msg = lua_tostring(m_l, -1);
    m_exceptionHandler->Handle(status, msg ? msg : "Failed to parse script");

    return false;
}

void State::exceptionHandler(int luaStatusCode, std::string msg, std::exception_ptr exception)
{
    m_log->write(Er::Log2::Record::make(
        "lua",
        Er::Log2::Level::Error,
        Er::System::PackedTime::now(),
        Er::System::CurrentThread::id(),
        std::move(msg)
    ));
}

void State::openLib(const std::string& name, lua_CFunction openf)
{
    ResetStackOnScopeExit savedStack(m_l);
#if LUA_VERSION_NUM >= 502
    luaL_requiref(m_l, name.c_str(), openf, 1);
#else
    lua_pushcfunction(m_l, openf);
    lua_pushstring(m_l, name.c_str());
    lua_call(m_l, 1, 0);
#endif
}

Selector State::operator[](const char* name) const
{
    return Selector(m_l, *m_registry, *m_exceptionHandler, name);
}

bool State::operator()(const char* code)
{
    ResetStackOnScopeExit savedStack(m_l);
    int status = luaL_dostring(m_l, code);
    if (status)
    {
        m_exceptionHandler->Handle_top_of_stack(status, m_l);
        return false;
    }

    return true;
}

} // namespace Er::Lua {}