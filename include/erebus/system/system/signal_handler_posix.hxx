#pragma once

#include <erebus/system/erebus.hxx>

#include <csignal>
#include <future>

#include <boost/noncopyable.hpp>

namespace Er::System
{


class ER_SYSTEM_EXPORT SignalHandler final
    : public boost::noncopyable
{
public:
    ~SignalHandler() noexcept;
    explicit SignalHandler(const std::initializer_list<int>& signals) noexcept;

    int wait() const;
    int waitHandler(std::function<bool(int)> handler) const;

    auto asyncWaitHandler(std::function<bool(int)> handler) const
    {
        return std::async(
            std::launch::async,
            &SignalHandler::waitHandler,
            this,
            std::move(handler)
        );
    }

private:
    sigset_t m_signals;
};



} // namespace Er::System {}