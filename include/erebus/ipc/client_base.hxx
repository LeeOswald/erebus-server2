#pragma once

#include <erebus/ipc/client.hxx>

#include <boost/noncopyable.hpp>


namespace Er::Ipc
{


class ClientBase
    : public boost::noncopyable
{
public:
    ~ClientBase() = default;

    explicit ClientBase(IClient::Ptr&& client, Log2::ILogger::Ptr log)
        : m_client(std::move(client))
        , m_log(log)
    {
    }

private:
    IClient::Ptr m_client;
    Log2::ILogger::Ptr m_log;
};


} // namespace Er::Ipc {}