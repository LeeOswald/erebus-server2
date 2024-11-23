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

    Result operator()(const Exception& e);
    Result operator()(const std::bad_alloc& e);
    Result operator()(const std::bad_cast& e);
    Result operator()(const std::length_error& e);
    Result operator()(const std::out_of_range& e);
    Result operator()(const std::invalid_argument& e);
    Result operator()(const std::exception& e);
    Result operator()(const std::exception_ptr& ep);

private:
    Log::ILog* m_log;
};

} // namespace Er::Util {}