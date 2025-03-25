#include "common.hpp"

#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/ipc/grpc/grpc_server.hxx>

#include <erebus/system/waitable.hxx>


template <class Interface>
struct CompletionBase
    : public Interface
{
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

    void handlePropertyMappingExpired() override
    {
        m_propertyMappingExpired = true;
        m_complete.setAndNotifyAll(true);
    }

    void handleTransportError(Er::ResultCode result, std::string&& message) override
    {
        m_error = result;
        m_errorMessage = std::move(message);
        m_complete.setAndNotifyAll(true);
    }

protected:
    Er::Waitable<bool> m_complete;
    bool m_propertyMappingExpired = false;
    std::optional<Er::ResultCode> m_error;
    std::string m_errorMessage;
};


class TestClientServer
    : public testing::Test
{
public:
    ~TestClientServer()
    {
        stopClient();
        stopServer();
    }

    TestClientServer()
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

    void startClient()
    {
        Er::Ipc::Grpc::ChannelSettings args(Er::format("127.0.0.1:{}", m_port));

        auto channel = Er::Ipc::Grpc::createChannel(args);
        m_client = Er::Ipc::Grpc::createClient(args, channel, Er::Log2::get());
    }

    void stopClient()
    {
        m_client.reset();
    }

protected:
    std::uint16_t m_port;
    Er::Ipc::IServer::Ptr m_server;
    Er::Ipc::IClient::Ptr m_client;
};


struct PingCompletion
    : public CompletionBase<Er::Ipc::IClient::IPingCompletion>
{
    PingCompletion(long count)
        : remainingCount(count)
    {
    }

    void handleReply(std::size_t payloadSize, std::chrono::milliseconds rtt) override
    {
        Er::Log2::info(Er::Log2::get(), "Pinged peer with {} bytes of data in {} ms", payloadSize, rtt.count());

        --remainingCount;
        totalPayload += payloadSize;

        if (remainingCount <= 0)
            m_complete.setAndNotifyAll(true);
    }

    long remainingCount;
    long totalPayload = 0;
};


TEST_F(TestClientServer, FailToConnect)
{
    startClient();

    {
        auto completion = std::make_shared<PingCompletion>(1);

        m_client->ping(0, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_TRUE(completion->error());
        EXPECT_EQ(completion->remainingCount, 1);
        EXPECT_EQ(completion->totalPayload, 0);
    }
}

TEST_F(TestClientServer, Ping)
{
    startServer();
    startClient();

    // zero-length payload
    {
        const long pingCount = 10;
        auto completion = std::make_shared<PingCompletion>(pingCount);

        for (long i = 0; i < pingCount; ++i)
        {
            m_client->ping(0, completion);
        }

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_EQ(completion->remainingCount, 0);
        EXPECT_EQ(completion->totalPayload, 0);
    }

    // nonzero-length payload
    {
        const long pingCount = 10;
        long payloadSize = 0;
       
        auto completion = std::make_shared<PingCompletion>(pingCount);

        for (long i = 0; i < pingCount; ++i)
        {
            long size = (i + 1) * 1024;
            payloadSize += size;
            m_client->ping(size, completion);
        }

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_EQ(completion->remainingCount, 0);
        EXPECT_EQ(completion->totalPayload, payloadSize);
    }
}