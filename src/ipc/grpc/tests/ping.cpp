#include "common.hpp"


namespace
{


struct PingCompletion
    : public CompletionBase<Er::Ipc::IClient::IPingCompletion>
{
    PingCompletion()
    {
    }

    void handleReply(std::size_t payloadSize, std::chrono::milliseconds rtt) override
    {
        ErLogDebug("Pinged peer with {} bytes of data in {} ms", payloadSize, rtt.count());

        totalPayload += payloadSize;
    }

    long totalPayload = 0;
};


using TestPing = TestClientBase;

} // namespace {}


TEST_F(TestPing, FailToConnect)
{
    startClient(1);

    {
        auto completion = std::make_shared<PingCompletion>();

        m_clients.front()->ping(0, completion, g_callTimeout);

        ASSERT_TRUE(completion->wait(g_callTimeout));

        EXPECT_TRUE(completion->transportError());
        EXPECT_EQ(completion->totalPayload, 0);
    }
}

static std::vector<std::shared_ptr<PingCompletion>> makeCompletions(long count)
{
    std::vector<std::shared_ptr<PingCompletion>> v;
    v.reserve(count);
    while (count--)
    {
        v.push_back(std::make_shared<PingCompletion>());
    }

    return v;
}

TEST_F(TestPing, Ping)
{
    startServer();
    startClient(1);

    // zero-length payload
    {
        const long pingCount = 10;
        auto completions = makeCompletions(pingCount);

        for (long i = 0; i < pingCount; ++i)
        {
            m_clients.front()->ping(0, completions[i], g_callTimeout);
        }

        for (long i = 0; i < pingCount; ++i)
        {
            ASSERT_TRUE(completions[i]->wait(g_callTimeout));
        }

        for (long i = 0; i < pingCount; ++i)
        {
            EXPECT_FALSE(completions[i]->transportError());
            EXPECT_EQ(completions[i]->totalPayload, 0);
        }
    }

    // nonzero-length payload
    {
        const long pingCount = 10;
       
        auto completions = makeCompletions(pingCount);

        for (long i = 0; i < pingCount; ++i)
        {
            long size = (i + 1) * 1024;
            m_clients.front()->ping(size, completions[i], g_callTimeout);
        }

        for (long i = 0; i < pingCount; ++i)
        {
            ASSERT_TRUE(completions[i]->wait(g_callTimeout));
        }

        for (long i = 0; i < pingCount; ++i)
        {
            long size = (i + 1) * 1024;
            EXPECT_FALSE(completions[i]->transportError());
            EXPECT_EQ(completions[i]->totalPayload, size);
        }
    }
}

TEST_F(TestPing, ConcurrentPing)
{
    struct ClientWorker
    {
        Er::Ipc::IClient* client;
        std::vector<std::shared_ptr<PingCompletion>> completions;
        const long pingCount;
        const long payloadSize;
        std::jthread worker;

        ClientWorker(Er::Ipc::IClient* client, long pingCount, long payloadSize)
            : client(client)
            , completions(makeCompletions(pingCount))
            , pingCount(pingCount)
            , payloadSize(payloadSize)
            , worker([this]() { run();  })
        {
        }

        bool wait()
        {
            bool ok = true;
            for (long i = 0; i < pingCount; ++i)
            {
                auto r = completions[i]->wait(g_callTimeout);
                if (!r)
                    ErLogError("Ping #{} did not complete", i);

                ok = ok && r;
            }

            return ok;
        }

        bool check()
        {
            for (long i = 0; i < pingCount; ++i)
            {
                if (completions[i]->transportError())
                {
                    ErLogError("Ping #{} returned an error {}", i, completions[i]->errorMessage());
                    return false;
                }

                if (completions[i]->totalPayload != payloadSize)
                {
                    ErLogError("Ping #{} returned {} bytes payload out of {}", i, completions[i]->totalPayload, payloadSize);
                    return false;
                }
            }

            return true;
        }

    private:
        void run()
        {
            for (long i = 0; i < pingCount; ++i)
            {
                client->ping(payloadSize, completions[i], g_callTimeout);
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
        ASSERT_TRUE(cli->wait());
    }

    for (auto& cli : clients)
    {
        EXPECT_TRUE(cli->check());
    }
}