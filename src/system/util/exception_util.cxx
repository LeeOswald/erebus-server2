#include <erebus/system/util/exception_util.hxx>


namespace Er::Util
{

ResultCode ExceptionLogger::operator()(const Exception& e)
{
    auto result = e.find(ExceptionProps::Result);

    if (!e.message().empty())
    {
        Log::error(m_log, "{}", e.message());
    }
    else if (result)
    {
        auto code = result->getInt32();
        auto decoded = resultToString(static_cast<ResultCode>(code));
        if (!decoded.empty())
            Log::error(m_log, "{}: {}", code, decoded);
        else
            Log::error(m_log, "Unexpected error {}", code);
    }
    else
    {
        Log::error(m_log, "Unexpected exception");
    }

    if (e.location().function_name())
        Log::error(m_log, "in [{}]", e.location().function_name());

    if (e.location().file_name())
        Log::error(m_log, "at [{}]:{}", e.location().file_name(), e.location().line());

    for (auto& prop : e.properties())
    {
        Log::error(m_log, "{}: {}", prop.info()->readableName, prop.info()->format(prop));
    }

    return result ? static_cast<ResultCode>(result->getInt32()) : Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::bad_alloc& e)
{
    Log::error(m_log, "{}", e.what());
    return Result::OutOfMemory;
}

ResultCode ExceptionLogger::operator()(const std::bad_cast& e)
{
    Log::error(m_log, "{}", e.what());
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::length_error& e)
{
    Log::error(m_log, "{}", e.what());
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::out_of_range& e)
{
    Log::error(m_log, "{}", e.what());
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::invalid_argument& e)
{
    Log::error(m_log, "{}", e.what());
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::exception& e)
{
    Log::error(m_log, "{}", e.what());
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::exception_ptr& ep)
{
    Log::error(m_log, "Unexpected exception");
    return Result::Internal;
}

} // namespace Er::Util {}
