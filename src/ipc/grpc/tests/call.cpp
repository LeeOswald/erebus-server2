#include "common.hpp"

namespace
{


class TestService
    : public Er::Ipc::IService
    , public std::enable_shared_from_this<TestService>
{
public:
    void registerService(Er::Ipc::IServer* container) override
    {
        container->registerService("echo", shared_from_this());
        container->registerService("throws", shared_from_this());
    }

    void unregisterService(Er::Ipc::IServer* container) override
    {
        container->unregisterService(this);
    }

    Er::PropertyBag request(std::string_view request, std::uint32_t clientId, const Er::PropertyBag& args) override
    {
        if (request == "echo")
            return echo(clientId, args);
        else if (request == "throws")
            return throws(clientId, args);

        ErThrow(Er::format("Unsupported request {}", request));
    }

    StreamId beginStream(std::string_view request, std::uint32_t clientId, const Er::PropertyBag& args) override
    {
        ErThrow(Er::format("Unsupported request {}", request));
    }

    void endStream(StreamId id)
    {
    }

    Er::PropertyBag next(StreamId id)
    {
        return {};
    }

private:
    Er::PropertyBag echo(std::uint32_t clientId, const Er::PropertyBag& args)
    {
        return args;
    }

    Er::PropertyBag throws(std::uint32_t clientId, const Er::PropertyBag& args)
    {
        Er::Exception e(std::source_location::current(), "This is my exception");
        for (auto& prop : args)
        {
            e.add(prop);
        }

        throw e;
    }
};


class TestCall
    : public TestClientBase
{
public:
    TestCall() = default;

    void startServer()
    {
        TestClientBase::startServer();

        auto service = std::make_shared<TestService>();
        service->registerService(m_server.get());
    }
};


struct CallCompletion
    : public CompletionBase<Er::Ipc::IClient::ICallCompletion>
{
    CallCompletion()
    {
    }

    void handleReply(Er::PropertyBag&& reply) override
    {
        this->reply = std::move(reply);
    }

    void handleException(Er::Exception&& exception) override
    {
        this->exception = std::move(exception);
    }

    std::optional<Er::PropertyBag> reply;
    std::optional<Er::Exception> exception;
};

} // namespace {}


TEST_F(TestCall, NormalCall)
{
    startServer();
    startClient(1);

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_clients.front()->call("echo", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->transportError());
        EXPECT_TRUE(completion->hasServerPropertyMappingExpired());
    }

    ASSERT_TRUE(putPropertyMapping(0));

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_clients.front()->call("echo", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->transportError());
        EXPECT_FALSE(completion->hasServerPropertyMappingExpired());
        EXPECT_TRUE(completion->hasClientPropertyMappingExpired());
    }

    ASSERT_TRUE(getPropertyMapping(0));

    {
        auto completion = std::make_shared<CallCompletion>();

        Er::PropertyBag args;
        args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
        args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

        m_clients.front()->call("echo", args, completion);

        ASSERT_TRUE(completion->wait());

        EXPECT_FALSE(completion->transportError());
        EXPECT_FALSE(completion->hasServerPropertyMappingExpired());
        EXPECT_FALSE(completion->hasClientPropertyMappingExpired());

        ASSERT_TRUE(completion->reply);

        auto& reply = *completion->reply;
        ASSERT_EQ(reply.size(), 2);

        {
            auto v = Er::get<uint64_t>(reply, Er::Unspecified::UInt64);
            ASSERT_TRUE(v);

            EXPECT_EQ(*v, 12);
        }

        {
            auto v = Er::get<std::string>(reply, Er::Unspecified::String);
            ASSERT_TRUE(v);

            EXPECT_STREQ(v->c_str(), "Hello");
        }
    }
}

TEST_F(TestCall, NotImplemented)
{
    startServer();
    startClient(1);

    auto completion = std::make_shared<CallCompletion>();

    Er::PropertyBag args;
    args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
    args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

    m_clients.front()->call("bark", args, completion);

    ASSERT_TRUE(completion->wait());

    EXPECT_TRUE(completion->transportError());
    EXPECT_EQ(*completion->transportError(), Er::Result::Unimplemented);
    
}

TEST_F(TestCall, Exception)
{
    startServer();
    startClient(1);
    ASSERT_TRUE(putPropertyMapping(0));
    ASSERT_TRUE(getPropertyMapping(0));

    auto completion = std::make_shared<CallCompletion>();

    Er::PropertyBag args;
    args.push_back(Er::Property(uint64_t(12), Er::Unspecified::UInt64));
    args.push_back(Er::Property(std::string("Hello"), Er::Unspecified::String));

    m_clients.front()->call("throws", args, completion);

    ASSERT_TRUE(completion->wait());

    EXPECT_FALSE(completion->transportError());
    EXPECT_TRUE(completion->exception);

    auto& e = *completion->exception;
    auto& props = e.properties();

    ASSERT_EQ(props.size(), 2);

    {
        auto v = Er::get<uint64_t>(props, Er::Unspecified::UInt64);
        ASSERT_TRUE(v);

        EXPECT_EQ(*v, 12);
    }

    {
        auto v = Er::get<std::string>(props, Er::Unspecified::String);
        ASSERT_TRUE(v);

        EXPECT_STREQ(v->c_str(), "Hello");
    }
}

TEST_F(TestCall, ConcurrentCall)
{
    struct ClientWorker
    {
        Er::Ipc::IClient* client;
        long id;
        long callCount;
        std::vector<std::string> requests;
        std::vector<std::shared_ptr<CallCompletion>> completions;
        bool prepared;
        std::jthread worker;
        
        ~ClientWorker()
        {
            ErLogDebug("~ClientWorker({})", id);
        }

        ClientWorker(Er::Ipc::IClient* client, long id, long callCount)
            : client(client)
            , id(id)
            , callCount(callCount)
            , prepared(prepare())
            , worker([this]() { run();  })
        {
            ErLogDebug("ClientWorker({})", id);
        }

        bool wait()
        {
            long succeeded = 0;
            for (long i = 0; i < callCount; ++i)
            {
                if (completions[i]->wait())
                {
                    ++succeeded;
                }
                else
                {
                    ErLogError("Call[{}][{}] did not complete", id, i);
                }
            }

            return succeeded == callCount;
        }

        bool check()
        {
            long succeeded = 0;
            for (long i = 0; i < callCount; ++i)
            {
                if (!completions[i]->transportError())
                {
                    if (completions[i]->reply)
                    {
                        auto& reply = *completions[i]->reply;

                        auto v = Er::get<std::string>(reply, Er::Unspecified::String);
                        if (v)
                        {
                            auto s = Er::format("Call[{}][{}]", id, i);
                            if (*v == s)
                            {
                                ++succeeded;
                            }
                            else
                            {
                                ErLogError("Call[{}][{}] reply was [{}] instead of [{}]", *v, s);
                            }
                        }
                        else
                        {
                            ErLogError("Call[{}][{}] did not yield a reply string");
                        }
                    }
                    else
                    {
                        ErLogError("Call[{}][{}] did not yield a reply");
                    }
                }
                else
                {
                    ErLogError("Call[{}][{}] failed: {} ({})", id, i, *completions[i]->transportError(), completions[i]->errorMessage());
                }
            }

            return succeeded == callCount;
        }

    private:
        bool prepare()
        {
            for (long i = 0; i < callCount; ++i)
            {
                auto c = std::make_shared<CallCompletion>();

                auto s = Er::format("Call[{}][{}]", id, i);

                requests.push_back(std::move(s));
                completions.push_back(c);
            }

            return true;
        }

        void run()
        {
            for (long i = 0; i < callCount; ++i)
            {
                Er::PropertyBag args;
                args.push_back(Er::Property(requests[i], Er::Unspecified::String));

                client->call("echo", args, completions[i]);
            }
        }
    };

    const long clientCount = 5;
    const long callCount = 10;

    startServer();
    startClient(clientCount);

    for (long i = 0; i < clientCount; ++i)
    {
        ASSERT_TRUE(putPropertyMapping(i));
        ASSERT_TRUE(getPropertyMapping(i));
    }

    std::vector<std::unique_ptr<ClientWorker>> clients;
    clients.reserve(clientCount);
    for (long i = 0; i < clientCount; ++i)
    {
        clients.push_back(std::make_unique<ClientWorker>(m_clients[i].get(), i, callCount));
    }

    for (long i = 0; i < clientCount; ++i)
    {
        ASSERT_TRUE(clients[i]->wait());
    }

    for (long i = 0; i < clientCount; ++i)
    {
        ASSERT_TRUE(clients[i]->check());
    }
}