#pragma once

#include <gtest/gtest.h>

#include <erebus/system/exception.hxx>
#include <erebus/system/logger2.hxx>
#include <erebus/system/result.hxx>
#include <erebus/system/waitable.hxx>
#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/ipc/grpc/grpc_server.hxx>


#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

extern std::uint16_t g_serverPort;
extern std::chrono::milliseconds g_operationTimeout;



template <class Interface>
struct CompletionBase
    : public Interface
{
    CompletionBase() = default;

    bool wait(std::chrono::milliseconds timeout = g_operationTimeout)
    {
        return m_complete.waitValueFor(true, timeout);
    }

    const std::optional<Er::ResultCode>& error() const
    {
        return m_error;
    }

    const std::string& errorMessage() const
    {
        return m_errorMessage;
    }

    bool serverPropertyMappingExpired() const
    {
        return m_serverPropertyMappingExpired;
    }

    bool clientPropertyMappingExpired() const
    {
        return m_clientPropertyMappingExpired;
    }

    bool success() const
    {
        return m_success;
    }

    void handleServerPropertyMappingExpired() override
    {
        m_serverPropertyMappingExpired = true;
        m_complete.setAndNotifyAll(true);
    }

    void handleClientPropertyMappingExpired() override
    {
        m_clientPropertyMappingExpired = true;
        m_complete.setAndNotifyAll(true);
    }

    void handleTransportError(Er::ResultCode result, std::string&& message) override
    {
        m_error = result;
        m_errorMessage = std::move(message);
        m_complete.setAndNotifyAll(true);
    }

    void handleSuccess() override
    {
        m_success = true;
        m_complete.setAndNotifyAll(true);
    }

protected:
    Er::Waitable<bool> m_complete;
    bool m_success = false;
    bool m_serverPropertyMappingExpired = false;
    bool m_clientPropertyMappingExpired = false;
    std::optional<Er::ResultCode> m_error;
    std::string m_errorMessage;
};


class TestClientBase
    : public testing::Test
{
public:
    ~TestClientBase()
    {
        stopClient();
        stopServer();
    }

    TestClientBase()
        : m_port(g_serverPort)
    {
    }

    void startServer()
    {
        Er::Ipc::Grpc::ServerArgs args(Er::Log2::get());
        args.endpoints.push_back(Er::Ipc::Grpc::ServerArgs::Endpoint(Er::format("127.0.0.1:{}", m_port)));

        m_server = Er::Ipc::Grpc::create(args);
    }

    void stopServer()
    {
        m_server.reset();
    }

    void startClient(std::size_t count)
    {
        ErAssert(count);

        Er::Ipc::Grpc::ChannelSettings args(Er::format("127.0.0.1:{}", m_port));

        auto channel = Er::Ipc::Grpc::createChannel(args);

        m_clients.clear();
        m_clients.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            m_clients.push_back(Er::Ipc::Grpc::createClient(args, channel, Er::Log2::get()));
        }
    }

    void stopClient()
    {
        m_clients.clear();
    }

    std::size_t clientCount() const
    {
        return m_clients.size();
    }

protected:
    std::uint16_t m_port;
    Er::Ipc::IServer::Ptr m_server;
    std::vector<Er::Ipc::IClient::Ptr> m_clients;
};
