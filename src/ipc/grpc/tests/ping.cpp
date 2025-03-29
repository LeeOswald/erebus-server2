#include "common.hpp"


namespace
{


struct PingCompletion
    : public CompletionBase<Er::Ipc::IClient::IPingCompletion>
{
    PingCompletion(long count)
        : remainingCount(count)
    {
    }

    void handleReply(std::size_t payloadSize, std::chrono::milliseconds rtt) override
    {
        Er::Log2::debug(Er::Log2::get(), "Pinged peer with {} bytes of data in {} ms", payloadSize, rtt.count());

        --remainingCount;
        totalPayload += payloadSize;

        if (remainingCount <= 0)
            m_complete.setAndNotifyAll(true);
    }

    long remainingCount;
    long totalPayload = 0;
};


using TestPing = TestClientBase;

} // namespace {}


TEST_F(TestPing, FailToConnect)
{
    startClient(1);

    {
        auto completion = std::make_shared<PingCompletion>(1);

        m_clients.front()->ping(0, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_TRUE(completion->error());
        EXPECT_EQ(completion->remainingCount, 1);
        EXPECT_EQ(completion->totalPayload, 0);
    }
}

TEST_F(TestPing, Ping)
{
    startServer();
    startClient(1);

    // zero-length payload
    {
        const long pingCount = 10;
        auto completion = std::make_shared<PingCompletion>(pingCount);

        for (long i = 0; i < pingCount; ++i)
        {
            m_clients.front()->ping(0, completion);
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
            m_clients.front()->ping(size, completion);
        }

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->error());
        EXPECT_EQ(completion->remainingCount, 0);
        EXPECT_EQ(completion->totalPayload, payloadSize);
    }
}

TEST_F(TestPing, ConcurrentPing)
{
    struct ClientWorker
    {
        Er::Ipc::IClient* client;
        std::shared_ptr<PingCompletion> completion;
        const long pingCount;
        const long payloadSize;
        std::jthread worker;

        ClientWorker(Er::Ipc::IClient* client, long pingCount, long payloadSize)
            : client(client)
            , completion(std::make_shared<PingCompletion>(pingCount))
            , pingCount(pingCount)
            , payloadSize(payloadSize)
            , worker([this]() { run();  })
        {
        }

        void run()
        {
            for (long i = 0; i < pingCount; ++i)
            {
                client->ping(payloadSize, completion);
            }
        }
    };

    const long clientCount = 10;

    startServer();
    startClient(clientCount);

    const long pingCount = 100;

    std::vector<std::unique_ptr<ClientWorker>> clients;
    clients.reserve(clientCount);
    for (long i = 0; i < clientCount; ++i)
    {
        clients.push_back(std::make_unique<ClientWorker>(m_clients[i].get(), pingCount, (i + 1) * 32));
    }

    for (auto& cli : clients)
    {
        ASSERT_TRUE(cli->completion->wait());
    }

    long i = 0;
    for (auto& cli : clients)
    {
        EXPECT_FALSE(cli->completion->error());
        EXPECT_EQ(cli->completion->remainingCount, 0);
        EXPECT_EQ(cli->completion->totalPayload, clients[i]->payloadSize * pingCount);

        ++i;
    }
}