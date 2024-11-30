#pragma once

#include <erebus/system/exception.hxx>
#include <erebus/system/log.hxx>
#include <erebus/system/result.hxx>


namespace Er::Util
{

class ER_SYSTEM_EXPORT ExceptionLogger
{
public:
    ExceptionLogger(Log::ILog* log)
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

private:
    Log::ILog* m_log;
};

} // namespace Er::Util {}