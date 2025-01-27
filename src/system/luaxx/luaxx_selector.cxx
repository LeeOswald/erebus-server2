#include <erebus/system/erebus.hxx>
#include <erebus/system/luaxx/luaxx_selector.hxx>


namespace Er::Lua
{

Selector::~Selector() noexcept(false)
{
    // If there is a functor is not empty, execute it and collect no args
    if (_functor_active)
    {
        ResetStackOnScopeExit save(_state);
        _traverse();
        _get();

        if (std::uncaught_exceptions())
        {
            try
            {
                _evaluate_function_call(0);
            }
            catch (...)
            {
                // We are already unwinding, ignore further exceptions.
            }
        }
        else
        {
            _evaluate_function_call(0);
        }
    }
}

void Selector::_check_create_table() const
{
    ResetStackOnScopeExit save(_state);
    _traverse();
    _get();

    if (lua_istable(_state, -1) == 0)
    {   // not table
        lua_pop(_state, 1); // flush the stack
        auto put = [this]()
        {
            lua_newtable(_state);
        };

        _traverse();
        _put(put);
    }
}

void Selector::_traverse() const
{
    lua_pushglobaltable(_state);
    for (auto& key : _traversal)
    {
        _get(key);
    }
}

void Selector::_evaluate_retrieve(int num_results) const
{
    _traverse();
    _get();
    _evaluate_function_call(num_results);
}

void Selector::_evaluate_function_call(int num_ret) const
{
    if (!_functor_active) return;
    _functor_active = false;

    // install handler, and swap(handler, function) on lua stack
    int handler_index = SetErrorHandler(_state);
    int func_index = handler_index - 1;
#if LUA_VERSION_NUM >= 502
    lua_pushvalue(_state, func_index);
    lua_copy(_state, handler_index, func_index);
    lua_replace(_state, handler_index);
#else
    lua_pushvalue(_state, func_index);
    lua_push_value(_state, handler_index);
    lua_replace(_state, func_index);
    lua_replace(_state, handler_index);
#endif
    // call lua function with error handler
    for (auto const& arg : _functor_arguments)
    {
        arg.Push(_state);
    }

    auto const statusCode = lua_pcall(_state, _functor_arguments.size(), num_ret, handler_index - 1);

    // remove error handler
    lua_remove(_state, handler_index - 1);

    if (statusCode != LUA_OK)
    {
        _exception_handler->Handle_top_of_stack(statusCode, _state);
    }
}


} // namespace Er::Lua {}