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

    const std::optional<Er::ResultCode>& transportError() const
    {
        return m_error;
    }

    const std::string& errorMessage() const
    {
        return m_errorMessage;
    }

    bool hasServerPropertyMappingExpired() const
    {
        return m_serverPropertyMappingExpired;
    }

    bool hasClientPropertyMappingExpired() const
    {
        return m_clientPropertyMappingExpired;
    }

    void handleServerPropertyMappingExpired() override
    {
        m_serverPropertyMappingExpired = true;
    }

    void handleClientPropertyMappingExpired() override
    {
        m_clientPropertyMappingExpired = true;
    }

    void handleTransportError(Er::ResultCode result, std::string&& message) override
    {
        if (m_serverPropertyMappingExpired || m_clientPropertyMappingExpired)
            return;

        m_error = result;
        m_errorMessage = std::move(message);
    }

    void done() override
    {
        m_complete.setAndNotifyAll(true);
    }

protected:
    Er::Waitable<bool> m_complete;
    bool m_serverPropertyMappingExpired = false;
    bool m_clientPropertyMappingExpired = false;
    std::optional<Er::ResultCode> m_error;
    std::string m_errorMessage;
};

using SimpleCompletion = CompletionBase<Er::Ipc::IClient::ICompletion>;


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
        , m_serverLog(makeLogger("server", Er::Log2::g_verbose ? Er::Log2::Level::Debug : Er::Log2::Level::Warning))
        , m_clientLog(makeLogger("client", Er::Log2::g_verbose ? Er::Log2::Level::Debug : Er::Log2::Level::Warning))
    {
        ErLogDebug("Server log {}", Er::Format::ptr(m_serverLog.get()));
        ErLogDebug("Client log {}", Er::Format::ptr(m_clientLog.get()));
    }

    Er::Log2::ILogger* serverLog() noexcept
    {
        return m_serverLog.get();
    }

    Er::Log2::ILogger* clientLog() noexcept
    {
        return m_clientLog.get();
    }

    void startServer()
    {
        Er::Ipc::Grpc::ServerArgs args(m_serverLog);
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
            m_clients.push_back(Er::Ipc::Grpc::createClient(args, channel, m_clientLog));
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

    Er::Log2::ILogger::Ptr makeLogger(std::string_view component, Er::Log2::Level level)
    {
        auto underlying = Er::Log2::global();
        ErAssert(underlying);
        auto log = Er::Log2::makeSyncLogger(component);
        log->addSink("global", std::static_pointer_cast<Er::Log2::ISink>(underlying));
        log->setLevel(level);
        return log;
    }

    bool putPropertyMapping(std::size_t client)
    {
        ErAssert(client < m_clients.size());

        auto completion = std::make_shared<SimpleCompletion>();

        m_clients[client]->putPropertyMapping(completion);
        return completion->wait();
    }

    bool getPropertyMapping(std::size_t client)
    {
        ErAssert(client < m_clients.size());

        auto completion = std::make_shared<SimpleCompletion>();

        m_clients[client]->getPropertyMapping(completion);
        return completion->wait();
    }

protected:
    Er::Log2::ILogger::Ptr m_serverLog;
    Er::Log2::ILogger::Ptr m_clientLog;

    std::uint16_t m_port;
    Er::Ipc::IServer::Ptr m_server;
    std::vector<Er::Ipc::IClient::Ptr> m_clients;
};


