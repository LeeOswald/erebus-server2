#pragma once

#include <erebus/system/exception.hxx>
#include <erebus/system/logger2.hxx>
#include <erebus/system/result.hxx>


namespace Er::Util
{

class ER_SYSTEM_EXPORT ExceptionLogger
{
public:
    ExceptionLogger(Log2::ILogger* log)
        : m_log(log)
    {}

    ResultCode operator()(const Exception& e);
    ResultCode operator()(const std::bad_alloc& e);
    ResultCode operator()(const std::bad_cast& e);
    ResultCode operator()(const std::length_error& e);
    ResultCode operator()(const std::out_of_range& e);
    ResultCode operator()(const std::invalid_argument& e);
    ResultCode operator()(const std::exception& e);
    ResultCode operator()(const std::exception_ptr& ep);

    const std::string& lastError() const noexcept
    {
        return m_lastError;
    }

private:
    Log2::ILogger* m_log;
    std::string m_lastError;
};

} // namespace Er::Util {}